/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * characterset conversions for filecopying.
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
 * $Id: charset.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */
    /* a two byte unicode character, gnus wchar_t is 4 bytes in linux */
typedef unsigned short UCS2;

    /* return a unicode String for MacOS Standard Roman.
     * lenptr may be null in wich case the given strs len is used.
     * must be free()d */
UCS2 *cs_unicode(char *str, int *lenptr);

    /* return a Latin-1 (ISO 8859-1) string for MacOS Standard Roman.
     * lenptr may be null in wich case the given strs len is used.
     * on return *lenptr is set to the returned strings len.
     * must be free()d */
char *cs_latin1(char *, int *);

    /* return a MacOS Standard Roman string for Latin-1 (ISO 8859-1).
     * lenptr may be null in wich case the given strs len is used.
     * on return *lenptr is set to the returned strings len.
     * must be free()d */
char *cs_macroman(char *, int *);
