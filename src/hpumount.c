/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * This program allows to "umount" a device. In fact it does so
 * by throwing away some infos stroed in the users directory.
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
 * $Id: hpumount.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdio.h>
# include <stdlib.h>

# include "libhfsp.h"
# include "hpcache.h"
# include "hfsputil.h"

/*
 * NAME:	hpmount->main()
 * DESCRIPTION:	implement hpmount command
 */
int main(int argc, char *argv[])
{
    argv0 = argv[0];
    if (argc != 1)
    {
	fprintf(stderr, "Usage: %s \n", argv0);
	goto fail;
    }

    if (hpcache_destroy())
	hfsputil_perror("destroy");
    
    return 0;
  fail:
    return 1;
}
