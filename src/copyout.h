/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * The functions here allow copying from HFS+ to unix files.
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
 * $Id: copyout.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

extern const char *copyout_error;

typedef int (*cpofunc)(record *, const char *);

extern int copyout_macb(record *, const char *);
extern int copyout_binh(record *, const char *);
extern int copyout_text(record *, const char *);
extern int copyout_raw (record *, const char *);
