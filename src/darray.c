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
 * $Id: darray.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdlib.h>
# include <string.h>

# include "darray.h"

#define DARRAY_RESERVE	8   // intial reserved space (elemsz)

/*
 * NAME:	darray->new()
 * DESCRIPTION:	allocate and return a new dynamic array
 */
int darray_new(darray* array,size_t elemsz)
{
    array->memsz = DARRAY_RESERVE * elemsz;
    array->mem   = malloc(array->memsz);
    if (array->mem == 0)
	return -1;

    array->eltend = array->mem;
    array->elemsz = elemsz;

    return 0;
}

/*
 * NAME:	darray->free()
 * DESCRIPTION:	dispose of a dynamic array
 */
void darray_free(darray *array)
{
  free(array->mem);
}

/*
 * NAME:	darray->size()
 * DESCRIPTION:	return the number of elements in a dynamic array
 */
unsigned int darray_size(darray *array)
{
  return (array->eltend - array->mem) / array->elemsz;
}

/*
 * NAME:	darray->array()
 * DESCRIPTION:	return the array as an indexable block
 */
inline
void *darray_array(darray *array)
{
  return (void *) array->mem;
}


/*
 * NAME:	darray->append()
 * DESCRIPTION:	add an element to the end of a dynamic array
 */
void *darray_append(darray *array, void *elem)
{
    char    *eltend = array->eltend;
    size_t  elemsz  = array->elemsz;

    if ((size_t) (eltend - array->mem) == array->memsz)
    {
	char *newmem;
	size_t newsz;

	newsz  = array->memsz * 2;
	newmem = realloc(array->mem, newsz);
	if (newmem == 0)
	    return 0;

	eltend = newmem + array->memsz;

	array->mem   = newmem;
	array->memsz = newsz;
    }

    memcpy(eltend, elem, elemsz);
    array->eltend = eltend + elemsz;

    return eltend;
}

/*
 * NAME:	darray->shrink()
 * DESCRIPTION:	truncate elements from the end of a dynamic array
 */
void darray_shrink(darray *array, unsigned int nelts)
{
  if (nelts < (array->eltend - array->mem) / array->elemsz)
    array->eltend = array->mem + nelts * array->elemsz;
}

/*
 * NAME:	darray->sort()
 * DESCRIPTION:	arrange items according to sorting criteria
 */
void darray_sort(darray *array, int (*compare)(const void *, const void *))
{
  qsort(array->mem, (array->eltend - array->mem) / array->elemsz,
	array->elemsz, compare);
}
