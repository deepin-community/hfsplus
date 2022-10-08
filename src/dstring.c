/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * A dynamic string implementation needed for glob.
 *
 * String a either kept in the given (50 bytes) buffer or
 * preallocated using two times the intial size.
 *
 * Copyright (C) 2000 Klaus Halfmann <khalfmann@libra.de>
 * Original 1996-1998 Robert Leslie <rob@mars.org>
 * Additional work by  Brad Boyer (flar@pants.nu)
 * *
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
 * $Id: dstring.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdlib.h>
# include <string.h>

# include "dstring.h"

/*
 * NAME:	dstring->init()
 * DESCRIPTION:	initialize a new dynamic string
 */
void dstring_init(dstring *string)
{
  string->str     = string->sbuf;
  string->len     = 0;
  string->space   = sizeof(string->sbuf);
  string->sbuf[0] = 0;
}

/*
 * NAME:	dstring->copy()
 * DESCRIPTION: coyp from an existing dstring
 */
int dstring_copy(dstring *string, dstring *from)
{
    size_t  len	    = from->len;
    size_t  space   = sizeof(string->sbuf);
    char    *sbuf;  
    
    string->len	    = len;

    if (len < space)	// fits into normal buffer
    {
	sbuf	= string->sbuf;
	strncpy(sbuf, from->str, space);
    }
    else    // must allocate new space
    {
	space = from->space;	// use given preallocation
	sbuf = malloc(space);
	if (!sbuf)
	    return -1;
	strncpy(sbuf, from->str, space);
    }
    string -> str   = sbuf;
    string->space   = space;

    return 0;
}

/*
 * NAME:	dstring->append()
 * DESCRIPTION:	append to a dynamic string
 */
int dstring_append(dstring *string, const char *str, size_t len)
{
    size_t newlen;

    if (len == (size_t) -1)
	len = strlen(str);

    newlen = string->len + len;

    /* make sure there is room */

    if (newlen >= string->space)
    {
	char *new;

	newlen *= 2;

	new = malloc(newlen);
	if (!new)
	    return -1;

	string->space = newlen;

	memcpy(new, string->str, string->len);

	if (string->str != string->sbuf)
	    free(string->str);

	string->str = new;
    }

    /* append the string */

    memcpy(string->str + string->len, str, len);

    string->len += len;
    string->str[string->len] = '\0';

    return 0;
}

/*
 * NAME:	dstring->string()
 * DESCRIPTION:	return a pointer to a dynamic string's content
 */
char *dstring_string(dstring *string)
{
  return string->str;
}

/*
 * NAME:	dstring->length()
 * DESCRIPTION:	return the length of a dynamic string
 */
int dstring_length(dstring *string)
{
  return string->len;
}


/*
 * NAME:	dstring->shrink()
 * DESCRIPTION:	truncate a dynamic string to a shorter length
 */
void dstring_shrink(dstring *string, size_t len)
{
  if (len < string->len)
    string->len = len;
}

/*
 * NAME:	dstring->free()
 * DESCRIPTION:	free a dynamic string
 */
void dstring_free(dstring *string)
{
    if (string->str != string->sbuf)
	free(string->str);
}
