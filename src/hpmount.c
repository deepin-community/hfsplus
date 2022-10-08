/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * This program allows to "mount" a device. In fact it does so
 * by creating some addtional info in the users home directory
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
 * $Id: hpmount.c,v 1.5 2002/03/26 18:00:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# include <stdio.h>
# include <stdlib.h>

# include "libhfsp.h"
# include "volume.h"

# include "hpcache.h"
# include "hfsputil.h"

/*
 *   NAME:        usage()
 *   DESCRIPTION: display usage message
 *   RETURN:	  always 1, to be used as exit status
 *    */
static
int usage()
{
    fprintf(stderr, "Usage:   %s <options> source-path	  \n", argv0);
    fprintf(stderr, "Options: -r  mount volume readonly   \n");
    fprintf(stderr, "         -p[n] mount partition n > 0 \n");
    fprintf(stderr, "         in case n is omitted mount first reasonable partition\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(Note: -p should be used only in case your kernel/OS \n");
    fprintf(stderr, " does not support Macintosh partition maps)\n");
    return 1;
}

/** Parse the partition number from the given argument.
 *  Will exit() on error.
 */
int parse_partition(char *arg)
{
    char* endchar;
    int   result;

    if (arg == NULL)	/* no option at all */
	return -1;

    result = strtol(arg, &endchar, 0);
    if (('\0' != *endchar) || ( result < 0))
    {
	fprintf(stderr, "Invalid partition '%s'\n", arg);
	exit(-1);
    }
    return result;
}


/*
 * NAME:	hpmount->main()
 * DESCRIPTION:	implement hpmount command
 */
int main(int argc, char *argv[])
{
    char*   path = NULL;
    volume  vol;
    record  rec;
    int	    nargs, result = 0;
    /* mike: we should probably set this to 1 to mount the first found partition as default.
       this doesn't mean anything for an unpartitioned device */
    int     partition = 0;	/* do not care for partitions */
    int     mode   = HFSP_MODE_RDWR;
	    argv0  = argv[0];

    while (1)
    {
	int opt;

	opt = getopt(argc, argv, "hrp::");
	if (opt == EOF)
	    break;

	switch (opt)
	{
	  case 'r':
	    mode = HFSP_MODE_RDONLY;
	    break;
	  case 'p':
	    partition = parse_partition(optarg);
	    break;
	  case '?':
	  case 'h':
	  default :
	    return usage();
	}
    }

    nargs = argc - optind;
    if (nargs < 1)
	return usage();

    path = hfsputil_abspath(argv[optind]);
    if (!path)
    {
	fprintf(stderr, "%s: not enough memory\n", argv0);
	goto fail;
    }

    // suid_enable();
    result = hpcache_open(&vol, &rec, path, partition, mode);
    // suid_disable();

    if (result)
	hfsputil_perror(path);

    result = volume_close(&vol) || result;

    free(path);

    return result;

    fail:
    if (path)
        free(path);

    return 1;
}
