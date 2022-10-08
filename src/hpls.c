/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * implement a ls simliar to standard linux ls for HFS+
 *
 * Copyright (C) 2000 Klaus Halfmann <khalfmann@libra.de>
 * Original 1996-1998 Robert Leslie <rob@mars.org>
 * Additional work by  Brad Boyer (flar@pants.nu)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: hpls.c,v 1.2 2002/03/17 17:20:39 klaus Exp $
 */
  
# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_TERMIOS_H
#  include <termios.h>
# endif

# ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
# else
int ioctl(int, int, ...);
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <ctype.h>
# include <errno.h>

# include "libhfsp.h"
# include "record.h"
# include "hfstime.h"
# include "volume.h"
# include "unicode.h"

#include "darray.h"
#include "dlist.h"
#include "dstring.h"
#include "hpcache.h"
#include "hfsputil.h"

# define HPLS_ALL_FILES		0x0001
# define HPLS_ESCAPE		0x0002
# define HPLS_QUOTE		0x0004
# define HPLS_QMARK_CTRL	0x0008
# define HPLS_IMMEDIATE_DIRS	0x0010
# define HPLS_CATIDS		0x0020
# define HPLS_REVERSE		0x0040
# define HPLS_SIZE		0x0080
# define HPLS_INDICATOR		0x0100
# define HPLS_RECURSIVE		0x0200
# define HPLS_NAME		0x0400
# define HPLS_SPACE		0x0800

# define F_MASK			0x0007
# define F_LONG			0x0000
# define F_ONE			0x0001
# define F_MANY			0x0002
# define F_HORIZ		0x0003
# define F_COMMAS		0x0004

# define T_MASK			0x0008
# define T_MOD			0x0000
# define T_CREATE		0x0008

# define S_MASK			0x0030
# define S_NAME			0x0000
# define S_TIME			0x0010
# define S_SIZE			0x0020

typedef struct _queueent_
{
    char	*path;
    record	dirent;
    void	(*free)(struct _queueent_ *);
} queueent;

extern char *optarg;
extern int optind;

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage()
{
  fprintf(stderr, "Usage: %s [options] [hfs-path ...]\n", argv0);

  return 1;
}

/*
 * NAME:	dpfree()
 * DESCRIPTION:	free a queue entry containing dynamically-allocated data
 */
static inline
void dpfree(queueent *ent)
{
    free(ent->path);
}

/*
 * NAME:	qnew()
 * DESCRIPTION:	create a new queue array
 */
inline static
int qnew(darray* da)
{
  return darray_new(da, sizeof(queueent));
}

/*
 * NAME:	qfree()
 * DESCRIPTION:	free a queue array
 */
static
void qfree(darray *array)
{
    int i, sz;
    queueent *ent;

    sz  =	      darray_size(array);
    ent = (queueent*) darray_array(array);

    for (i = 0; i < sz; ++i,ent++)
    {
	if (ent->free)
	    ent->free((void*) ent);
    }

    darray_free(array);
}

static
int reverse;

/*
 * NAME:	compare_names()
 * DESCRIPTION:	lexicographically compare two filenames
 */
static
int compare_names(const queueent *ent1, const queueent *ent2)
{
  return reverse ^ strcasecmp(ent1->path, ent2->path);
}

/*
 * NAME:	compare_mtimes()
 * DESCRIPTION:	chronologically compare two modification dates
 */
static
int compare_mtimes(const queueent *ent1, const queueent *ent2)
{
  return reverse ^ (ent2->dirent.record.u.file.content_mod_date 
		  - ent1->dirent.record.u.file.content_mod_date);
}

/*
 * NAME:	compare_ctimes()
 * DESCRIPTION:	chronologically compare two creation dates
 */
static
int compare_ctimes(const queueent *ent1, const queueent *ent2)
{
  return reverse ^ (ent2->dirent.record.u.file.create_date 
		  - ent1->dirent.record.u.file.create_date);
}

/*
 * NAME:	compare_sizes()
 * DESCRIPTION:	compare two file sizes
 */
static
int compare_sizes(const queueent *ent1, const queueent *ent2)
{
    UInt64 size1 = 0, size2 = 0;
    const hfsp_cat_file *f1, *f2;
    if (ent1->dirent.record.type == HFSP_FILE)
    {
	f1 = &ent1->dirent.record.u.file;
	size1 = f1->data_fork.total_size + f1->res_fork.total_size;
    }
    if (ent2->dirent.record.type == HFSP_FILE)
    {
	f2= &ent1->dirent.record.u.file;
	size2 = f2->data_fork.total_size + f2->res_fork.total_size;
    }
    // Cant use size2-size1 since sizeof(UInt64) > sizeof(int) (usually :)
    if (size1 > size2)
	return reverse;
    else if (size2 < size1)
	return -reverse;

    return 0;
}

/*
 * NAME:	sortfiles()
 * DESCRIPTION:	arrange files in order according to sort selection
 */
static
void sortfiles(darray *files, int flags, int options)
{
  int (*compare)(const queueent *, const queueent *);

  switch (options & S_MASK)
    {
    case S_NAME:
      compare = compare_names;
      break;

    case S_TIME:
      switch (options & T_MASK)
	{
	case T_MOD:
	  compare = compare_mtimes;
	  break;

	case T_CREATE:
	  compare = compare_ctimes;
	  break;

	default:
	  abort();
	}
      break;

    case S_SIZE:
      compare = compare_sizes;
      break;

    default:
      return;
    }

  reverse = (flags & HPLS_REVERSE) ? -1 : 1;

  darray_sort(files, (int (*)(const void *, const void *)) compare);
}

/* Escape a character when needed */
static char* escape_char(unsigned char c, char buf[])
{
    char *add = buf;
    buf[0] = '\0';  // just in case ...
    switch (c)
    {
      case '\\':
	add = "\\\\";
	break;

      case '\n':
	add = "\\n";
        break;

      case '\b':
	add = "\\b";
	break;

      case '\r':
        add = "\\r";
        break;

      case '\t':
        add = "\\t";
        break;

      case '\f':
        add = "\\f";
        break;

      case ' ':
        add = "\\ ";
        break;

      case '\"':
	add = "\\\"";
	break;

      default:
        if (isgraph(c))
	  *add = c;
        else
	  sprintf(buf, "\\%03o", c);
    }
    return add;
}

/*
 * NAME:	outpath()
 * DESCRIPTION:	modulate an output string given current flags
 */
static
int outpath(dstring *str, 
	queueent *ent, 
	int flags)
{
    const char *path;

    path = ent->path;

    dstring_shrink(str, 0);

    if ((flags & HPLS_QUOTE) &&
	dstring_append(str, "\"", 1) == -1)
	return -1;

    if (flags & (HPLS_ESCAPE | HPLS_QUOTE | HPLS_QMARK_CTRL))
    {
	const unsigned char *ptr;

	for (ptr = path; *ptr; ++ptr)
	{
	    char buf[5];
	    const char *add = buf;

	    if (flags & HPLS_ESCAPE)
		add = escape_char(*ptr, buf);
	    else  /* ! (flags & HPLS_ESCAPE) */
	    {
		if (isprint(*ptr) || ! (flags & HPLS_QMARK_CTRL))
		{
		    sprintf(buf, "%c", *ptr);
		    add = buf;
		}
		else
		{
		  sprintf(buf, "?");
		  add = buf;
		}
	    }

	  if (dstring_append(str, add, -1) == -1)
	    return -1;
	}
    }
    else // ! (HPLS_ESCAPE | HPLS_QUOTE | HPLS_QMARK_CTRL)
    {
	if (dstring_append(str, path, -1) == -1)
	    return -1;
    }

    if ((flags & HPLS_QUOTE) &&
	dstring_append(str, "\"", 1) == -1)
    return -1;

    if (flags & HPLS_INDICATOR)
    {
	char c = 0;

	if (ent->dirent.record.type == HFSP_FOLDER)
	    c = '/';
	// Mmhh, actually there are others like "appe" "appl" "sysz" ...
        else if (!strncmp((char*)&ent->dirent.record.u.file.user_info.fdType, 
		    "APPL",4))
	    c = '*';

	if (c && dstring_append(str, &c, 1) == -1)
	    return -1;
    }

    return 0;
}

/*
 * NAME:	misclen()
 * DESCRIPTION:	string length of miscellaneous section
 */
static
int misclen(int flags)
{
  return ((flags & HPLS_CATIDS) ? 8 : 0) +
         ((flags & HPLS_SIZE)   ? 5 : 0);
}

/*
 * NAME:	showmisc()
 * DESCRIPTION:	output miscellaneous numbers
 */
static
void showmisc(record *ent, int flags)
{
    UInt64  size;

    if (flags & HPLS_CATIDS)
    {
        if (ent->record.type < HFSP_FOLDER_THREAD)
	    printf("%7lu ", ent->record.u.file.id);
	else
	    printf("%7lu ", ent->record.u.thread.parentID);
    }
    if (flags & HPLS_SIZE && 
	ent->record.type == HFSP_FILE)
    {
	size = ent->record.u.file.data_fork.total_size + 
	       ent->record.u.file.res_fork.total_size;
	printf("%4Lu ", size / 1024 + (size % 1024 != 0));
    }
}

/*
 * NAME:	show_long()
 * DESCRIPTION:	output a list of files in long format
 */
static
void show_long(int sz, queueent *ents, char **strs,
	       int flags, int options, int width)
{
    int	    i;
    time_t  now;

    now = time(0);

    for (i = 0; i < sz; ++i)
    {
	record	*ent = &ents[i].dirent;
	time_t	when;
	char	timebuf[26];
	int	isThread;

        showmisc(ent, flags);

	isThread = ent->record.type >= HFSP_FOLDER_THREAD;
        timebuf[0] = 0;

	// Only files / folders have time fields
        if (!isThread)
	{
	    switch (options & T_MASK)
	    {
	      case T_MOD:
		when = ent->record.u.file.content_mod_date;
		break;

	      case T_CREATE:
		when = ent->record.u.file.create_date;
		break;

	      default:
		abort();
	    }
	    when -= HFSPTIMEDIFF;

	    strcpy(timebuf, ctime(&when));

	    if (now > when + 6L * 30L * 24L * 60L * 60L ||
		now < when - 60L * 60L)
	    strcpy(timebuf + 11, timebuf + 19);

	    timebuf[16] = 0;
	}

        if (ent->record.type == HFSP_FOLDER)
	{
	    hfsp_cat_folder* f = &ent->record.u.folder;
	    printf("d%c %9lu item%c               %s %s\n",
	       f->user_info.frFlags & HFS_FNDR_ISINVISIBLE ? 'i' : ' ',
	       f->valence, f->valence == 1 ? ' ' : 's',
	       timebuf + 4, strs[i]);
	}
	else if (ent->record.type == HFSP_FILE)
	{
	    hfsp_cat_file* f = &ent->record.u.file;
	    printf("%c%c %4.4s/%4.4s %9Lu %9Lu %s %s\n",
	       f->flags & HFSP_FILE_LOCKED ? 'F' : 'f',
	       f->user_info.fdFlags & HFS_FNDR_ISINVISIBLE ? 'i' : ' ',
	       (char*) &f->user_info.fdType, (char*) &f->user_info.fdCreator,
	       f->data_fork.total_size, f->res_fork.total_size,
	       timebuf + 4, strs[i]);
	} 
	else // Thread
	{
	    printf("   Thread                                     %s\n",
		    strs[i]);
	}

    }
}

/*
 * NAME:	show_one()
 * DESCRIPTION:	output a list of files in single-column format
 */
static
void show_one(int sz, queueent *ents, char **strs,
	      int flags, int options, int width)
{
  int i;

  for (i = 0; i < sz; ++i)
    {
      showmisc(&ents[i].dirent, flags);
      printf("%s\n", strs[i]);
    }
}

/*
 * NAME:	show_many()
 * DESCRIPTION:	output a list of files in vertical-column format
 */
static
void show_many(int sz, queueent *ents, char **strs,
	       int flags, int options, int width)
{
  int i, len, misc, maxlen = 0, rows, cols, row;

  misc = misclen(flags);

  for (i = 0; i < sz; ++i)
    {
      len = strlen(strs[i]) + misc;
      if (len > maxlen)
	maxlen = len;
    }

  maxlen += 2;

  cols = width / maxlen;
  if (cols == 0)
    cols = 1;

  rows = sz / cols + (sz % cols != 0);

  for (row = 0; row < rows; ++row)
    {
      i = row;

      while (1)
	{
	  showmisc(&ents[i].dirent, flags);
	  printf("%s", strs[i]);

	  i += rows;
	  if (i >= sz)
	    break;

	  for (len = strlen(strs[i - rows]) + misc;
	       len < maxlen; ++len)
	    putchar(' ');
	}

      putchar('\n');
    }
}

/*
 * NAME:	show_horiz()
 * DESCRIPTION:	output a list of files in horizontal-column format
 */
static
void show_horiz(int sz, queueent *ents, char **strs,
		int flags, int options, int width)
{
  int i, len, misc, maxlen = 0, cols;

  misc = misclen(flags);

  for (i = 0; i < sz; ++i)
    {
      len = strlen(strs[i]) + misc;
      if (len > maxlen)
	maxlen = len;
    }

  maxlen += 2;

  cols = width / maxlen;
  if (cols == 0)
    cols = 1;

  for (i = 0; i < sz; ++i)
    {
      if (i)
	{
	  if (i % cols == 0)
	    putchar('\n');
	  else
	    {
	      for (len = strlen(strs[i - 1]) + misc;
		   len < maxlen; ++len)
		putchar(' ');
	    }
	}

      showmisc(&ents[i].dirent, flags);
      printf("%s", strs[i]);
    }

  if (i)
    putchar('\n');
}

/*
 * NAME:	show_commas()
 * DESCRIPTION:	output a list of files in comma-delimited format
 */
static
void show_commas(int sz, queueent *ents, char **strs,
		 int flags, int options, int width)
{
    int i, pos = 0;

    for (i = 0; i < sz; ++i)
    {
	record *ent = &ents[i].dirent;
	int len	    = strlen(strs[i]) + misclen(flags) + ((i < sz - 1) ? 2 : 0);

	if (pos && pos + len >= width)
	{
	    putchar('\n');
	    pos = 0;
	}

	showmisc(ent, flags);
	printf("%s", strs[i]);

	if (i < sz - 1)
	{
	    putchar(',');
	    putchar(' ');
	}

	pos += len;
    }

    if (pos)
	putchar('\n');
}

/*
 * NAME:	showfiles()
 * DESCRIPTION:	display a set of files
 */
static
int showfiles(darray *files, int flags, int options, int width)
{
    dlist list;
    int i, sz, result = 0;
    queueent *ents;
    dstring str;
    char **strs;
    void (*show)(int, queueent *, char **, int, int, int);

    if (dlist_init(&list) == -1)
    {
	fprintf(stderr, "%s: not enough memory\n", argv0);
	return -1;
    }

    sz   =		darray_size(files);
    ents = (queueent*)	darray_array(files);

    dstring_init(&str);

    for (i = 0; i < sz; ++i)
    {
	if (outpath(&str, &ents[i], flags) == -1 ||
	    dlist_append(&list, dstring_string(&str)) == -1)
	{
	    result = -1;
	    break;
	}
    }

    dstring_free(&str);

    strs = (char**) dlist_array(&list);

  switch (options & F_MASK)
    {
    case F_LONG:
      show = show_long;
      break;

    case F_ONE:
      show = show_one;
      break;

    case F_MANY:
      show = show_many;
      break;

    case F_HORIZ:
      show = show_horiz;
      break;

    case F_COMMAS:
      show = show_commas;
      break;

    default:
      abort();
    }

  show(sz, ents, strs, flags, options, width);

  dlist_free(&list);

  return result;
}

/*
 * NAME:	process()
 * DESCRIPTION:	sort and display results
 */
static
int process(darray *dirs, darray *files,
	    int flags, int options, int width)
{
    int		i;
    queueent	*ents;
    int		result = 0;

    int fsz = darray_size(files);
    int dsz = darray_size(dirs);

    if (fsz)
    {
	sortfiles(files, flags, options);
	if (showfiles(files, flags, options, width) == -1)
	    result = -1;

	flags |= HPLS_NAME | HPLS_SPACE;
    }
    else if (dsz > 1)
	flags |= HPLS_NAME;

    ents = (queueent*) darray_array(dirs);

    for (i = 0; i < dsz; ++i)	// for all directories
    {
	record	    *path;
	queueent    *ent, newent;
	record	    *dir;
	UInt16	    type;

	ent	= &ents[i];
	dir	= &newent.dirent;
	path	= &ent->dirent;
	type	= path->record.type;
	
	/* Supress '.' or invisible files  */
	if ( (type == HFSP_FILE || type == HFSP_FOLDER) &&
		(path->record.u.folder.user_info.frFlags & HFS_FNDR_ISINVISIBLE) &&
	    ! (flags & HPLS_ALL_FILES))
	    continue;	// glob does not remove all invisible files/folders
	darray_shrink(files, 0); // reset file list

	if (record_init_parent(dir, path))
	{
	    hfsputil_perrorp(ent->path);
	    result = -1;
	    continue;
	}

	do // for all files in directory
	{
	    dstring	name; // String to create filenames in
	    char	buf[255];
	    int		len;

	    type = dir->record.type;
	    len = 0; // just in case
	    if (type == HFSP_FILE_THREAD)
		continue;   // dont know how to work with that by now
	    if (type == HFSP_FOLDER_THREAD)
	    {	
		if (!(flags & HPLS_ALL_FILES))
		    continue;	
		// emulate '.'
		buf [0] = '.';	buf[1] = '\0'; len = 1;
	    }
	    else // FILE or Folder
	    {
		if ((dir->record.u.folder.user_info.frFlags & HFS_FNDR_ISINVISIBLE) &&
		  ! (flags & HPLS_ALL_FILES))
		    continue;
		len = unicode_uni2asc(buf, &dir->key.name, 255);
	    }
	    
	    newent.free = NULL;

	    // insert to list of folders ..
	    if ((type == HFSP_FOLDER) && (flags & HPLS_RECURSIVE))
	    {
		dstring	spath; // String to append path to
		dstring_init(&spath);
		if (dstring_append(&spath, ent->path, -1))
		    result = -1;

		if (dstring_append(&spath, "/", 1))
		    result = -1;
		
		if (dstring_append(&spath, buf, -1) == -1)
		    result = -1;

		newent.path = strdup(dstring_string(&spath));
		if (newent.path)
		    newent.free = dpfree;
		else
		    result = -1;

		dstring_free(&spath);

		if (!darray_append(dirs, &newent))
		{
		    result = -1;
		    if (newent.path)
			free(newent.path);
		}

		if (result)
		{
		    fprintf(stderr, "%s: not enough memory\n", argv0);
		    break;
		}

		dsz  = darray_size(dirs);
		ents = (queueent*) darray_array(dirs);
		ent  = &ents[i];
	    } 
	    
	    dstring_init(&name);
	    if (dstring_append(&name, buf, -1) == -1)
	        result = -1;
	    newent.path = strdup(dstring_string(&name));
	    if (newent.path)
		newent.free = dpfree;
	    else
		result = -1;
		
	    if (!darray_append(files, &newent))
	    {
		fprintf(stderr, "%s: not enough memory\n", argv0);
		result = -1;
		break;
	    }

	} while (!result && !record_next(dir)); // for all files

	if (result)
	    break;

	if (flags & HPLS_SPACE)
	    printf("\n");
	if (flags & HPLS_NAME)
	    printf("%s:\n", ent->path);

	sortfiles(files, flags, options);
	if (showfiles(files, flags, options, width))
	    result = -1;

	flags |= HPLS_NAME | HPLS_SPACE;
    } // all directories

    return result;
}

/*
 * NAME:	queuepath()
 * DESCRIPTION:	append a file or directory to the list to process
 * 
 * if name is NULL append the whole contents of the (directory/thread) root.
 */
static
int queuepath(record* root, char *name, darray *dirs, darray *files, int flags)
{
    queueent ent;
    darray   *array;
    UInt16   type; /* Folder or File (Thread) */

    if (name)
    {
	if (record_init_string_parent(&ent.dirent, root, name))
	{
	    hfsputil_perrorp(name);
	    return (errno == ENOENT) ? 0 : -1;
	}
    }
    else
	ent.dirent = *root;
    

    type     = ent.dirent.record.type;
    ent.path = name ? name : ".";
    ent.free = 0;

    array = ((type == HFSP_FOLDER || type == HFSP_FOLDER_THREAD) &&
    	   ! (flags & HPLS_IMMEDIATE_DIRS)) ? dirs : files;

    if (darray_append(array, &ent) == 0)
    {
	fprintf(stderr, "%s: not enough memory\n", argv0);
	return -1;
    }

    return 0;
}

/*
 * NAME:	hpls->main()
 * DESCRIPTION:	implement hpls command
 */
int main(int argc, char *argv[])
{
    volume vol;
    record rec;
  
    int fargc, i;
    char **fargv    = NULL;
    int result	    = 0;
    int flags, options, width;
    char *ptr;
    darray dirs, files;

    argv0 = argv[0];
    options = T_MOD | S_NAME;

    if (isatty(STDOUT_FILENO))
    {
	options |= F_MANY;
	flags    = HPLS_QMARK_CTRL;
    }
    else
    {
	options |= F_ONE;
	flags    = 0;
    }

    ptr   = getenv("COLUMNS");
    width = ptr ? atoi(ptr) : 80;

# ifdef TIOCGWINSZ // Special handling for windows ?
    {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 &&
	    ws.ws_col != 0)
	width = ws.ws_col;
    }
# endif

    while (1) // scan all the arguments 
    {
      int opt = getopt(argc, argv, "1abcdfilmqrstxw:CFNQRSU");

      if (opt == EOF)
	break;

      switch (opt)
	{
	case '?':
	  return usage(argv[0]);

	case '1':
	  options = (options & ~F_MASK) | F_ONE;
	  break;

	case 'a':
	  flags |= HPLS_ALL_FILES;
	  break;

	case 'b':
	  flags |= HPLS_ESCAPE;
	  flags &= ~HPLS_QMARK_CTRL;
	  break;

	case 'c':
	  options = (options & ~(T_MASK | S_MASK)) | T_CREATE | S_TIME;
	  break;

	case 'd':
	  flags |= HPLS_IMMEDIATE_DIRS;
	  break;

	case 'f':
	  flags |= HPLS_ALL_FILES;
	  flags &= ~HPLS_SIZE;
	  options &= ~S_MASK;
	  if ((options & F_MASK) == F_LONG)
	    options = (options & ~F_MASK) |
	      (isatty(STDOUT_FILENO) ? F_MANY : F_ONE);
	  break;

	case 'i':
	  flags |= HPLS_CATIDS;
	  break;

	case 'l':
	  options = (options & ~F_MASK) | F_LONG;
	  break;

	case 'm':
	  options = (options & ~F_MASK) | F_COMMAS;
	  break;

	case 'q':
	  flags |= HPLS_QMARK_CTRL;
	  flags &= ~HPLS_ESCAPE;
	  break;

	case 'r':
	  flags |= HPLS_REVERSE;
	  break;

	case 's':
	  flags |= HPLS_SIZE;
	  break;

	case 't':
	  options = (options & ~S_MASK) | S_TIME;
	  break;

	case 'x':
	  options = (options & ~F_MASK) | F_HORIZ;
	  break;

	case 'w':
	  width = atoi(optarg);
	  break;

	case 'C':
	  options = (options & ~F_MASK) | F_MANY;
	  break;

	case 'F':
	  flags |= HPLS_INDICATOR;
	  break;

	case 'N':
	  flags &= ~(HPLS_ESCAPE | HPLS_QMARK_CTRL);
	  break;

	case 'Q':
	  flags |= HPLS_QUOTE | HPLS_ESCAPE;
	  flags &= ~HPLS_QMARK_CTRL;
	  break;

	case 'R':
	  flags |= HPLS_RECURSIVE;
	  break;

	case 'S':
	  options = (options & ~S_MASK) | S_SIZE;
	  break;

	case 'U':
	  options &= ~S_MASK;
	  break;
	}
    } // scan all arguments
    if (hpcache_reopen(&vol,&rec,HFSP_MODE_RDONLY))
	goto fail;

    fargv = (char**) hfsputil_glob(&rec, argc - optind, &argv[optind], &fargc, &result);

    result = qnew(&dirs)  || result;
    result = qnew(&files) || result;

    if (result)
    {
	fprintf(stderr, "%s: not enough memory\n", argv0);
	result = 1;
    }

    if (result == 0)
    {
	if (fargc == 0) // show curent dir
	{
	    if (queuepath(&rec, NULL, &dirs, &files, flags))
		result = 1;
	}
	else // show files / dirs found by globbing
	{
	    for (i = 0; i < fargc; ++i)
	    {
		if (queuepath(&rec, fargv[i], &dirs, &files, flags))
		{
		    result = 1;
		    break;
		}
	    }
	}
    }

    if (result == 0 && process(&dirs, &files, flags, options, width) == -1)
	result = 1;

    qfree(&files);
    qfree(&dirs);

    if (fargv)
	free(fargv);

    result = volume_close(&vol) || result;

    return result;
  fail:
    hfsputil_perror(argv[0]);	
    return result;
}
