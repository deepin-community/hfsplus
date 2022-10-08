/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 *
 * Copyright (C) 2000 Klaus Halfmann <klaus.halfmann@feri.de>
 * Original work 1996-1998 Robert Leslie <rob@mars.org>
 *
 * This file defines some byte swapping function. I did not find this
 * in any standard or linux way.
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
 * $Id: swab.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

#include <endian.h>
#include <byteswap.h> 

 /* basic fuction:
    value = swab_inc(ptr);
	ptr is afterwards incremented by sizeof(value)
 */

#if BYTE_ORDER == LITTLE_ENDIAN

#define bswabU16(val) bswap_16(val)

#define bswabU16_inc(ptr) bswap_16(*((UInt16*) (ptr))++)
#define bswabU32_inc(ptr) bswap_32(*((UInt32*) (ptr))++)
#define bswabU64_inc(ptr) bswap_64(*((UInt64*) (ptr))++)

#define bstoreU16_inc(ptr, val) (*((UInt16*) (ptr))++) = bswap_16(val)
#define bstoreU32_inc(ptr, val) (*((UInt32*) (ptr))++) = bswap_32(val)
#define bstoreU64_inc(ptr, val) (*((UInt64*) (ptr))++) = bswap_64(val)

#else // BYTE_ORDER == BIG_ENDIAN

#define bswabU16(val) val

#define bswabU16_inc(ptr) (*((UInt16*) (ptr))++)
#define bswabU32_inc(ptr) (*((UInt32*) (ptr))++)
#define bswabU64_inc(ptr) (*((UInt64*) (ptr))++)

#define bstoreU16_inc(ptr, val) (*((UInt16*) (ptr))++) = val
#define bstoreU32_inc(ptr, val) (*((UInt32*) (ptr))++) = val
#define bstoreU64_inc(ptr, val) (*((UInt64*) (ptr))++) = val

#endif

/* for the sake of compleetness and readability */
#define bswabU8_inc(ptr)	(*((UInt8*) (ptr))++)
#define bstoreU8_inc(ptr,val)	(*((UInt8*) (ptr))++) = val
