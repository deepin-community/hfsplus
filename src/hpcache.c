/*
 * hfsputils - tools for reading and writing Macintosh HFS volumes
 *
 * The structure defined here is saved in the users home directory
 * to cache the status of the HFS+ access.
 *
 * Copyright (C) 2000 Klaus Halfmann <khalfmann@libra.de>
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
 * $Id: hpcache.c,v 1.3 2002/03/16 08:16:14 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>

#include "libhfsp.h"
#include "volume.h"
#include "btree.h"
#include "record.h"

#include "hpcache.h"

#define CACHE_NAME ".hfsplusvolume"

typedef struct
{
    char    vname[255]; /* name of the original opened device. */
    UInt32  cnid;	/* node od of the current directory.   */
    int	    partition;  /* partition actually mounted 
			   (0 -> dont check for partitions)    */
} hpcache;

static hpcache    volume_cache;

/* return the filename of the file in the users home directory.
 * 
 * result must be freed();
 */
static char* hpcache_filename()
{
    char    *home, *path;

    home = getenv("HOME");
    if (!home)
	home = "";

    path = malloc(strlen(home) + 1 + sizeof(CACHE_NAME));
    if (path)
    {
	strcpy(path, home);
	strcat(path, "/" CACHE_NAME);
    }
    return path;
}

/* open the file in the user home directory */
static FILE* hpcache_openfile(char* mode)
{
    char* path = hpcache_filename();
    FILE* fd   = NULL;
    
    if (path)
    {
	fd = fopen(path, mode);
	free(path); 
    }

    return fd;
}

/* read the cached value and put it into the static variable */
static int hpcache_read()
{
    FILE*   fd	    = hpcache_openfile("r");
    int	    params; /* Number of parameters read by fscanf */
    
    if (!fd)
    {
	hfsp_error = "Unable to read file for cached Volume information.";
	return -1;
    }

    params = fscanf(fd, "%255[^:]:%lu:%u",
		volume_cache.vname, &volume_cache.cnid, &volume_cache.partition);
    if (params < 2)
    {
	hfsp_error = "Failure while reading " CACHE_NAME;
	return -1;
    }
    else if (params == 2)	     /* old format */
	volume_cache.partition = 0;  /* ignore partitions in this case */

    fclose(fd);

    return 0;
}

/* write the cached value to the users home directory. */
static int hpcache_write()
{
    FILE* fd = hpcache_openfile("w+");

    if (!fd)
    {
	hfsp_error = "Unable to create file for cached Volume information.";
	return -1;
    }

    fprintf(fd, "%s:%lu:%u", 
	    volume_cache.vname, volume_cache.cnid, volume_cache.partition);

    return 0;
}

/* call volume_open etc. and set the record to the volumes record 
 *
 * partition open given partition number (if possible)
 *	      0 - do not look for partition(s) at all.
 *	     -1 - take whatever partition is found
 *	     >0 - take given partion
 * openMode, one of HFSP_MODE_RDONLY, HFSP_MODE_RDWR
 */
int hpcache_open(volume* vol, record* rec, char* devicename,
		int partition, int openMode)
{
    if (volume_open(vol, devicename, partition, openMode))
        return -1;
    if (record_init_root(rec, &vol->catalog))  
	return -1;
    strncpy(volume_cache.vname, devicename, 255);
	// This is ok because this is the folder thread ...
    volume_cache.cnid	   = HFSP_ROOT_CNID;
    volume_cache.partition = partition;
    return hpcache_write();
}

/* call volume_open etc. and set the record to the last used folder 
 *
 * openMode, one of HFSP_MODE_RDONLY, HFSP_MODE_RDWR
 */
int hpcache_reopen(volume* vol, record* rec, int openMode)
{
    if (hpcache_read())
        return -1;
    if (volume_open(vol, volume_cache.vname, volume_cache.partition, openMode))
        return -1;
    if (record_init_cnid(rec, &vol->catalog, volume_cache.cnid))  
	return -1;
    return 0;
}

/* Set the current folder to the given CNID */
int hpcache_cwd(UInt32 cnid)
{
    volume_cache.cnid = cnid;
    return hpcache_write();
}
  
/* destroy the file identifying the cache, 
   You still must close volume_close yourself. */
int hpcache_destroy()
{
    char*   path = hpcache_filename();
    int	    result = unlink(path);
    if (result)
	hfsp_error = "Error while destroying " CACHE_NAME;
    free(path);
    return result;
}

