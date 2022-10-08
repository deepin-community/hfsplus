/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * create a new folder in the current (HFS+) working directory
 *
 * Copyright (C) 2000 Klaus Halfmann <klaus.halfmann@feri.de>
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
 * $Id: hpmkdir.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */
  
# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
/*

# ifdef HAVE_TERMIOS_H
#  include <termios.h>
# endif

# ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
# else
int ioctl(int, int, ...);
# endif

# include <string.h>
# include <time.h>
# include <ctype.h>
# include <errno.h>
*/
# include <stdio.h>
# include <stdlib.h>

# include "libhfsp.h"
# include "record.h"
// # include "hfstime.h"
# include "volume.h"
# include "unicode.h"

#include "darray.h"
#include "dlist.h"
#include "dstring.h"
#include "hpcache.h"
#include "hfsputil.h"

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage()
{
  fprintf(stderr, "Usage: %s hfs-path ...\n", argv0);

  return 1;
}

/* create all the folders specified in the argvs.
 *
 * rec, base directory where files are found.
 *
 */
static
int do_mkdir(record *rec, int argc, char *argv[], int flags)
{
    int i;
    int result = 0;

    for (i = 0; i < argc; ++i)
    {
	record	folder; 
	char*	argi = argv[i];
	if (record_init_string(&folder, HFSP_FOLDER, argi, rec))
	{
	    hfsputil_perrorp(argi);
	    HFSP_ERROR(-1, 0);
	}
	else if (record_insert(&folder))
	{
	    hfsputil_perrorp(argi);
	    HFSP_ERROR(errno, 0);

	    result = 1;
	}
    }

    return result;
  fail:
    return -1;
}

/*
 * NAME:	hpmkdir->main()
 * DESCRIPTION:	implement hpmkdir command
 */
int main(int argc, char *argv[])
{
    volume	vol;
    record	rec;

    int		nargs, result = 0;
    int		fargc, flags  = 0;
    char	**fargv;

    argv0 = argv[0];
    while (1) // Our only option is -?/-h
    {
	int opt;

	opt = getopt(argc, argv, "h");
	if (opt == EOF)
	    break;

	switch (opt)
	{
	  case '?':
	  case 'h':
	    return usage();
	}
    }

    nargs = argc - optind;

    if (nargs < 1)
	return usage();

    if (hpcache_reopen(&vol,&rec,HFSP_MODE_RDWR))
        goto fail;

    fargv = hfsputil_glob(&rec, nargs, &argv[optind], &fargc, &result);

    if (!result)
	result = do_mkdir(&rec, fargc, fargv, flags);

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
