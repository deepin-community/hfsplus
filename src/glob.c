/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
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
 * $Id: glob.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdlib.h>
# include <string.h>

# include "libhfsp.h"
# include "record.h"
# include "unicode.h"

# include "dlist.h"
# include "dstring.h"
# include "glob.h"

/*
 * NAME:	strmatch()
 * DESCRIPTION:	return 1 iff a string matches a given (glob) pattern
 */
static
int strmatch(const char *str, const char *pat)
{
    while (1)
    {
	if (!*str && *pat && *pat != '*')
	    return 0; // no more string but still pattern

	switch (*pat)
	{
	    case NULL:	// pattern at end
		return (!*str); // String at end ?

	    case '*':  // match all
		if (*++pat == 0)
		    return 1;

		while (1)
		{
		    if (strmatch(str, pat))
			return 1;

		    if (*str++ == 0)
			return 0;
		}

	    case '?':
		break;

	    case '[':
	    {
		++pat;

		while (1)
		{
		    unsigned char p0, p1, s;

		    p0 = *pat;

		    if (p0 == ']' || p0 == 0)
			return 0;

		    s = *str;

		    if (p0 == s)
			break;

		    if (pat[1] == '-')
		    {
			p1 = pat[2];

			if (p1 == 0)
			    return 0;

			if ((p0 <= s && p1 >= s) ||
			    (p0 >= s && p1 <= s))
			    break;

			pat += 2;
		    }

		    ++pat;
		}

		while (*pat != ']')
		{
		    if (*pat == 0)
		    {
			--pat;
			break;
		    }

		    ++pat;
		}
	    } // case '[':
	    break;

	case '\\':
	  if (*++pat == 0)
	    return 0;

	  /* fall through */

	default: // simple equality ...
	  if ( *pat != *str)
	    return 0;
	}

	++pat, ++str;
    }
}

/* need foreward declaration for recursion */
static
int doglob(record *rec, dlist *list, const char *dir, const char *rem);

/*
 * NAME:	glob_braces()
 * DESCRIPTION:	check for patterns like {a,b,c}
 *
 * obrace   pointer to '{'
 * cbrace   pointer to '}'
 * new	    String matched so far
 */
static int glob_braces(record* rec,dlist* list,const char * dir,
	    const char *rem, const char* obrace, const char* cbrace,
	    dstring* new)
{
    const char	*elt;
    const char	*ptr;
    int		len;

    if (!cbrace || // no closing '}' ?
	dstring_append(new, rem, obrace - rem) == -1)
    {
	dstring_free(new);
	return -1;
    }
    len = dstring_length(new); /* save current length for shrink */

    for (ptr = obrace; *ptr != '}'; )
    {
	ptr = elt = ptr + 1;

	while (*ptr != '}' && *ptr != ',') // find next element
	    ++ptr;

	if (dstring_append(new, elt, ptr - elt)  ||
	    dstring_append(new, cbrace + 1, -1)  ||
	    doglob(rec, list, dir, dstring_string(new)))
	{
	    dstring_free(new);
	    return -1;
	}

	dstring_shrink(new, len);
    }

    dstring_free(new);
    return 0;
}

/*
 * NAME:	glob_special()
 * DESCRIPTION:	check for patterns containing wildcards '*[?'
 *
 * ptr points to end of pattern in rem.
 */
static int glob_special(record* rec,dlist* list,const char * dir,
	    const char *rem, dstring* new, const char* ptr)
{
    int	    result = 0;
    int	    rec_err;    // error return by a record_ function
    record  d;
    dstring pat;
    int	    found = 0;  // marker when anything was found
    int	    len = dstring_length(new); /* remeber length for shrink */

    dstring_init(&pat);
    if (dstring_append(&pat, rem, ptr - rem)) 
    {	    /* text matched upto wildcard */
	dstring_free(&pat);
	dstring_free(new);
	return -1;
    }

    if (!*dir && strstr(rem, "..")) // correct ?
    {
	d = *rec;
	rec_err = record_up(&d); // glob in parent directory ?
    }
    else
	rec_err = record_init_parent(&d, rec);
				// glob in current directory
    if (rec_err)
    {
	dstring_free(&pat);
	dstring_free(new);
	return -1;
    }

    // Iterate directory and match pattern(s)
    while (!record_next(&d))
    {
	char buf[255];
	// invisible files will still be considered here
	unicode_uni2asc(buf, &d.key.name, 255);

	if (strmatch(buf, dstring_string(&pat)))
	{
	    dstring_shrink(new, len);
	    if (dstring_append(new, buf, -1) == -1)
	    {
		result = -1;
		break;
	    }

	    if (!*ptr)
	    {
		result = dlist_append(list, dstring_string(new));
		found  = 1;

		if (result == -1)
		    break;
	    }
	    else if (d.record.type == HFSP_FOLDER) /* match '...*<slash>...' */
	    {
		if (dstring_append(new, "/", 1) == -1)
		    result = -1;
		else
		{
		    found  = 1;
		    result = doglob(rec, list, dstring_string(new), ptr + 1);
		}

		if (result == -1)
		    break;
	    }
	}
    }

    /* What happens here ? */
    if (result == 0 && ! found)
    {
	dstring_shrink(new, len);
	if (dstring_append(new, rem, -1) == -1)
	    result = -1;
	else
	{
	    char *ptr, *rem;

	    for (rem = dstring_string(new) + len, ptr = rem; *rem;
	       ++rem, ++ptr)
	    {
		if (*rem == '\\')
		    ++rem;

		*ptr = *rem;
	    }
	    *ptr = 0;

	    result = dlist_append(list, dstring_string(new));
	}
    }

    dstring_free(&pat);
    dstring_free(new);

    return result;
}

/*
 * NAME:	doglob()
 * DESCRIPTION:	perform recursive depth-first traversal of path to be globbed
 * 
 * rec	    current directory
 * dlist    list of dynamic strings found so far
 * dir	    name of current directory
 * rem	    (wildcarded) parameter to glob
 */
static
int doglob(record *rec, dlist *list, const char *dir, const char *rem)
{
    dstring	new;
    int		special;    /* set when a wildcard is found */
    int		result = 0;
    const char	*obrace, *cbrace, *ptr;

    dstring_init(&new);

    special = 0;
    obrace = cbrace = NULL;

    // Check for {....}, \ *[?
    for (ptr = rem; *ptr && (obrace || *ptr != '/'); ++ptr)
    {
	switch (*ptr)
	{
	    case '{':
	    if (!obrace)
		obrace = ptr;
	    break;

	    case '}':
	    if (obrace && !cbrace)
		cbrace = ptr;
	    break;

	    case '\\':
	    if (*++ptr == 0)
		--ptr;

	    case '*':
	    case '[':
	    case '?':
		special = 1;
	    break;
	}
    } // for 

    if (obrace)
	return glob_braces(rec, list, dir, rem, obrace, cbrace, &new);

    if (dstring_append(&new, dir, -1)) /* start with current directory */
    {
	dstring_free(&new);
	return -1;
    }

    if (special)
	return glob_special(rec, list, dir, rem, &new, ptr);

    /* ptr now points to '/' glob_subdir */
    if (dstring_append(&new, rem, ptr - rem) == -1)
	result = -1;
    else
    {
	if (*ptr)
	{
	    if (dstring_append(&new, "/", 1) == -1)
		result = -1;
	    else
		result = doglob(rec, list, dstring_string(&new), ptr + 1);

	    dstring_free(&new);
	    return result;
	}

	result = dlist_append(list, dstring_string(&new));
    }

    dstring_free(&new);
    return result;
}

/*
 * NAME:	hfs->glob()
 * DESCRIPTION:	perform glob pattern matching
 *
 * argc number of patterns to match.
 * argv the patterns.
 * return array of strings containing the matches, must be free()d.
 * *netls contains number of elements in returned array.
 */
char **hfsp_glob(record *rec, int argc, char *argv[], int *nelts)
{
    dlist   list; // accumulate the matches here
    int	    i;

    if (dlist_init(&list) == -1)
	return 0;

    for (i = 0; i < argc; ++i)
    {
	if (doglob(rec, &list, "", argv[i]) == -1)
	{
	    dlist_free(&list);
	    return 0;
	}
    }

    *nelts = dlist_size(&list);

    return dlist_array(&list);
}
