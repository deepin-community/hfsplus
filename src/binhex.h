/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * Utilties to write and read binhex (.hqx) formats.  
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
 * $Id: binhex.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */
 
extern const char *binhex_error;

/* BinHex Encoding ==================================== */
 
  /* begin BinHex encoding using the given file descriptor */
int binhex_start(int fd);

  /* encode bytes of data, buffering lines and flushing */
int binhex_insert(const void *buf, register int len);

  /* insert a two-byte CRC checksum */
int binhex_insertcrc(void);

  /* finish BinHex encoding */
int binhex_end(void);

/* BinHex Decoding =================================== */
 
  /* begin BinHex decoding usinf give file descriptor as input */
int binhex_open(int fd);

  /* decode and return bytes from the hqx stream */
int binhex_read(void *buf, register int maxlen);

  /* read and compare CRC bytes */
int binhex_readcrc(void);

  /* finish BinHex decoding */
int binhex_close(void);

