/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 *
 * Utility functions to dynamically manage a list of strings.
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
 * $Id: dlist.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdlib.h>
# include <string.h>

# include "dlist.h"

/*
 * NAME:	dlist->init()
 * DESCRIPTION:	initialize a new dynamic list
 */
int dlist_init(dlist *list)
{
    list->memsz = 100;
    list->mem   = malloc(list->memsz);
    if (list->mem == 0)
	return -1;

    list->eltend = (char **) list->mem;
    list->strs   = list->mem + list->memsz;

    return 0;
}

/*
 * NAME:	dlist->free()
 * DESCRIPTION:	dispose of a dynamic list
 */
void dlist_free(dlist *list)
{
    free(list->mem);
}

/*
 * NAME:	dlist->array()
 * DESCRIPTION:	return the array of strings in a list; can dispose with free()
 */
char **dlist_array(dlist *list)
{
    return (char **) list->mem;
} 

/*
 * NAME:	dlist->size()
 * DESCRIPTION:	return the number of strings in a list
 */
int dlist_size(dlist *list)
{
  return list->eltend - (char **) list->mem;
}

/*
 * NAME:	dlist->append()
 * DESCRIPTION:	insert a string to the end of a list
 */
int dlist_append(dlist *list, const char *str)
{
    size_t len = strlen(str) + 1;

    /* make sure there is room */

    if (sizeof(char *) + len > (size_t) (list->strs - (char *) list->eltend))
    {
	dlist	newlist;
	size_t	strsz = (list->mem + list->memsz) - list->strs;
	char	**elt;

	newlist.memsz = list->memsz * 2 + sizeof(char *) + len;
	newlist.mem   = malloc(newlist.memsz);
	if (!newlist.mem)
	    return -1;

	newlist.eltend = (char **) newlist.mem;
	newlist.strs   = newlist.mem + newlist.memsz - strsz;

	memcpy(newlist.strs, list->strs, strsz);

	for (elt = (char **) list->mem; elt < list->eltend; ++elt)
	    *newlist.eltend++ = newlist.strs + (*elt - list->strs);

	free(list->mem);

	*list = newlist;
    }

    list->strs -= len;
    strcpy(list->strs, str);

    *list->eltend++ = list->strs;

    return 0;
}
