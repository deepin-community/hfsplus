/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
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
 * $Id: os.h,v 1.2 2002/03/25 15:48:37 klaus Exp $
 */

/*
 * NAME:	os->open()
 * DESCRIPTION:	open and lock a new descriptor from the given path and mode
 */
int os_open(void **priv, const char *path, int mode);

/*
 * NAME:	os->close()
 * DESCRIPTION:	close an open descriptor
 */
int os_close(void **priv);

/*
 * NAME:	os->same()
 * DESCRIPTION:	return 1 iff path is same as the open descriptor
 */
int os_same(void **priv, const char *path);


/** Offset that is silently used by os_seek.
 *  Needed to transparaently support things like partitions */
extern UInt64 os_offset;

/*
 * NAME:	os->seek()
 * DESCRIPTION:	set a descriptor's seek pointer (offset in blocks)
 */
unsigned long os_seek(void **priv, unsigned long offset, int blksize_bits);

/*
 * NAME:	os->read()
 * DESCRIPTION:	read blocks from an open descriptor
 */
unsigned long os_read(void **priv, void *buf, unsigned long len, int blksize_bits);

/*
 * NAME:	os->write()
 * DESCRIPTION:	write blocks to an open descriptor
 */
unsigned long os_write(void **priv, const void *buf, unsigned long len, int blksize_bits);
