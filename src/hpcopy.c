/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * This program allows to copy files from a HFS+ volume.
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
 * $Id: hpcopy.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <sys/stat.h>

# include "libhfsp.h"
# include "volume.h"
# include "record.h"

# include "hfsputil.h"
# include "hpcache.h"

// # include "copyin.h" Sorry, not yet
# include "copyout.h"

extern int optind;

/*
 * NAME:	automode_unix()
 * DESCRIPTION:	automatically choose copyin transfer mode for UNIX path
 */
/* not yet
static
cpifunc automode_unix(const char *path)
{
    int i;
    struct {
	const char *ext;
	cpifunc func;
    } exts[] = {
	{ ".bin",  cpi_macb },
	{ ".hqx",  cpi_binh },

	{ ".txt",  cpi_text },
	{ ".c",    cpi_text },
	{ ".h",    cpi_text },
	{ ".html", cpi_text },
	{ ".htm",  cpi_text },
	{ ".rtf",  cpi_text },

	{ 0,       0        }
    };

    path += strlen(path);

    for (i = 0; exts[i].ext; ++i)
    {
	if (strcasecmp(path - strlen(exts[i].ext), exts[i].ext) == 0)
	    return exts[i].func;
    }

    return cpi_raw;
} */

/*
 * NAME:	do_copyin()
 * DESCRIPTION:	copy files from UNIX to HFS
 */
/* Sorry not in use by now ...
static
int do_copyin(record *rec, int argc, char *argv[], const char *dest, int mode)
{
    hfsdirent ent;
    struct stat sbuf;
    cpifunc copyfile;
    int i, result = 0;

    if (argc > 1 && (hfs_stat(vol, dest, &ent) == -1 ||
		   ! (ent.flags & HFS_ISDIR)))
    {
	HFSP_ERROR(ENOTDIR, 0);
	hfsutil_perrorp(dest);

	return 1;
    }

    switch (mode)
    {
      case 'm':
	copyfile = cpi_macb;
	break;

      case 'b':
	copyfile = cpi_binh;
	break;

      case 't':
	copyfile = cpi_text;
	break;

      case 'r':
	copyfile = cpi_raw;
	break;
    }

    for (i = 0; i < argc; ++i)
    {
	if (stat(argv[i], &sbuf) != -1 &&
	    S_ISDIR(sbuf.st_mode))
	{
	    HFSP_ERROR(EISDIR, 0);
	    hfsutil_perrorp(argv[i]);

	    result = 1;
	}
	else
	{
	    if (mode == 'a')
		copyfile = automode_unix(argv[i]);

	    if (copyfile(argv[i], vol, dest) == -1)
	    {
		HFSP_ERROR(errno, cpi_error);
		hfsutil_perrorp(argv[i]);

		result = 1;
	    }
	}
    }

    return result;
}
*/

/*
 * NAME:	automode_hfs()
 * DESCRIPTION:	automatically choose copyout transfer mode for HFS path
 */
static
cpofunc automode_hfs(record *rec)
{
    if (rec->record.type == HFSP_FILE)
    {
	hfsp_cat_file* file = &rec->record.u.file;
	char *type = (char*)&file->user_info.fdType;
	if (!strcmp(type, "TEXT")  ||
	    !strcmp(type, "ttro"))
		return copyout_text;
	else if (file->res_fork.total_size == 0) // no resourcefork, well
	    return copyout_raw;
    }

    return copyout_macb;
}

/*
 * NAME:	do_copyout()
 * DESCRIPTION:	copy files from HFS+ to UNIX
 */
static
int do_copyout(record *rec, int argc, char *argv[], const char *dest, int mode)
{
    struct stat sbuf;
    cpofunc copyfile;
    int i, result = 0;

    if (argc > 1 && (stat(dest, &sbuf) == -1 ||
		   ! S_ISDIR(sbuf.st_mode)))
	HFSP_ERROR(ENOTDIR, 0);

    switch (mode)
    {
      case 'm':
	copyfile = copyout_macb;
	break;

      case 'b':
	copyfile = copyout_binh;
	break;

      case 't':
	copyfile = copyout_text;
	break;

      case 'r':
	copyfile = copyout_raw;
	break;
      
      case 'a':	// auto copyfile will be assigen per file
	copyfile = NULL;
	break;

      default:
	copyfile = NULL;
	HFSP_ERROR(-1, "Unexpected case");
    }

    for (i = 0; i < argc; ++i)
    {
	record	file;
	char*	argi = argv[i];
	if (	
	    record_init_string_parent( &file, rec, argi) || 
	    (file.record.type != HFSP_FILE))
	{
	    hfsputil_perrorp(argi);
	    HFSP_ERROR(EISDIR, 0);
	}
	else
	{
	    if (mode == 'a')
		copyfile = automode_hfs(&file);

	    if (copyfile(&file, dest))
	    {
		hfsputil_perrorp(argi);
		HFSP_ERROR(errno, copyout_error);

		result = 1;
	    }
	}
    }

    return result;
  fail:
    return -1;
}

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage(void)
{
    printf("Usage: %s [-m|-b|-t|-r|-a] source-path [...] target-path\n",
	  argv0);
    printf( " -m MacBinary II: A popular format for binary file transfer.\n"
	    "    Both forks of the Macintosh file are preserved. This is the\n"
	    "    recommended mode for transferring arbitrary Macintosh files.\n"
	    "\n"
	    " -b BinHex: An alternative format for ASCII file transfer. \n"
	    "    Both forks of the Macintosh file are preserved.\n"
	    "\n"
	    " -t Text: Performs end-of-line translation.\n"
	    "    Only the data fork of the Macintosh file is copied.\n"
	    "\n"
	    " -r Raw Data: Performs no translation.\n"
	    "    Only the data fork of the Macintosh file is copied.\n"
	    "\n"
	    " -a Automatic: A mode will be chosen automatically for each file\n"
	    "    based on a set of predefined heuristics.\n");

    return 1;
}

/*
 * NAME:	hcopy->main()
 * DESCRIPTION:	implement hcopy command
 */
int main(int argc, char *argv[])
{
    volume	vol;
    record	rec;

    int		nargs, mode = 'a', result = 0;
    const char	*target;
    int		fargc;
    char	**fargv;
    int (*copy)(record*, int, char *[], const char *, int);

    argv0 = argv[0];
    while (1)
    {
	int opt;

	opt = getopt(argc, argv, "mbtra");
	if (opt == EOF)
	    break;

	switch (opt)
	{
	  case '?':
	    return usage();

	  default:
	    mode = opt;
	}
    }

    nargs = argc - optind;

    if (nargs < 2)
	return usage();

    target = argv[argc - 1];

    if (hpcache_reopen(&vol,&rec,HFSP_MODE_RDONLY))
        goto fail;

    copy  = do_copyout;
    fargv = hfsputil_glob(&rec, nargs - 1, &argv[optind], &fargc, &result);

    if (!result)
	result = copy(&rec, fargc, fargv, target, mode);

    result = volume_close(&vol) || result;

    if (fargv && fargv != &argv[optind])
	free(fargv);

    if (result)
	hfsputil_perrorp(argv0);
    return result;
  fail:
    hfsputil_perrorp(argv0);
    return -1;
}
