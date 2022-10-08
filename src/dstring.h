/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * A dynamic String implementation need for globbbing.
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
 * $Id: dstring.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

typedef struct {
    char    *str;
    size_t  len;
    size_t  space;
    char    sbuf[50];
} dstring;

/* intialize to epty String */
extern void	dstring_init(dstring *);

/* intialize by copy */
extern int	dstring_copy(dstring *to, dstring* from);

/* append to dstring */
extern int	dstring_append(dstring *, const char *, size_t);
extern void	dstring_shrink(dstring *, size_t);
extern void	dstring_free(dstring *);

extern inline char	*dstring_string(dstring *string)
{
    return string->str;
}

extern inline int	dstring_length(dstring *string)
{
    return string->len;
}  

