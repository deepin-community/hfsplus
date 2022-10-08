/*
 * libhfsp - library for reading and writing Macintosh HFS+ volumes
 *
 * This is a test file used for development. I hope it will be
 * gone when this ever becomes final.
 * 
 * Copyright (C) 2000 Klaus Halfmann <klaus.halfmann@feri.de>
 * Original code 1996-1998 by Robert Leslie <rob@mars.rog>
 * other work 2000 from Brad Boyer (flar@pants.nu) 
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: test.c,v 1.2 2002/03/16 08:17:39 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif                                                                         

#include <stdio.h>

#include "libhfsp.h"
#include "volume.h"
#include "btree.h"
#include "record.h"

int main(int argc, char* argv[])
{
    volume  v;
    btree   b;
    record  root, ls;
    UInt16  i;
    if (volume_open(&v, argv[1], 1, HFSP_MODE_RDONLY))
	perror(hfsp_error);

    if (volume_get_extends_tree(&v))
	perror(hfsp_error);

    /*
    UInt32  j;
    for (j=0xFFA0; j < 0xFFB0 ; j++)
	printf("allocated %lX %d\n", j,		volume_allocated(&v, j));
    */
    
    if (volume_close(&v))		    
	perror(hfsp_error);
    return 0;
}
