/*
 * hfsputils - tools for reading and writing Macintosh HFS volumes
 *
 * The structure defined here is saved in the users home directory
 * to cache the status of the HFS+ access.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: hpcache.h,v 1.2 2002/03/10 18:08:55 klaus Exp $
 */

/* call volume_open etc. and set the record to the volumes record 
 *
 * partition open given partition number (if possible)
 *	      0 - do not look for partition(s) at all.
 *	     -1 - take whatever partition is found
 *	     >0 - take given partion
 * openMode, one of HFSP_MODE_RDONLY, HFSP_MODE_RDWR
 */
extern int hpcache_open(volume* vol, record* rev, char* devicename, 
			int partition, int openMode);

/* call volume_open etc. and set the record to the last used folder
 *
 * openMode, one of HFSP_MODE_RDONLY, HFSP_MODE_RDWR
 */
extern int hpcache_reopen(volume* vol, record* rev, int openMode);

/* Set the current folder to the given CNID */
extern int hpcache_cwd(UInt32 cnid);

/* Delete the information collected by _open */
extern int hpcache_destroy();
