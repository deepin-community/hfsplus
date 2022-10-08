/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * User command for cehcking filesystems, lik fsck
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
 * $Id: hpfsck.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

#include  <stdio.h>

# include "libhfsp.h"
# include "fscheck.h"

# include "hfsputil.h"

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage(void)
{
    printf("Usage: %s [-v] [-i] device\n", argv0);
    printf("\t-v Be more verbose\n");
    printf("\t-i Ignore errors (default stop after first error)\n");

    return 1;
}


extern int optind;

/*
 * NAME:	hpfsck->main()
 * DESCRIPTION:	implement hpfsck command
 */
int main(int argc, char *argv[])
{
    int	    mode = HFSPCHECK_NORMAL;
    char    *device;
    int	    nargs, result;

    argv0 = argv[0];
    while (1)
    {
	int opt;

	opt = getopt(argc, argv, "vi");
	if (opt == EOF)
	    break;

	switch (opt)
	{
	  case '?':
	    return usage();
	  case 'v':
	    mode |= HFSPCHECK_VERBOSE;
	    break;
	  case 'i':
	    mode |= HFSPCHECK_IGNOREERR;
	    break;
	}
    }

    nargs = argc - optind;

    if (nargs != 1)
	return usage();

    device = argv[argc - 1];

    result = maximum_check(device, mode);

    if (result != FSCK_NOERR)
	hfsputil_perror(argv[0]);

    return result;
}
