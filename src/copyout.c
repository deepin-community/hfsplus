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
 * $Id: copyout.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# else
int open(const char *, int, ...);
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# else
int dup(int);
# endif

# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <limits.h>
# include <sys/stat.h>

# include "libhfsp.h"
# include "volume.h"
# include "record.h"
# include "blockiter.h"
# include "unicode.h"
# include "swab.h" 

# include "copyout.h"
# include "charset.h"
# include "binhex.h"
# include "crc.h"

const char *copyout_error = "no error";

extern int errno;

# define ERROR(code, str)	(copyout_error = (str), errno = (code))

# define MACB_BLOCKSZ	128

/* Copy Routines =========================================================== */

/*
 * NAME:	fork->macb()
 * DESCRIPTION:	copy a single fork for MacBinary II
 */
static
int fork_macb(volume* vol, hfsp_fork_raw *fork, UInt8 forktype,
	     UInt32 cnid, int ofile)
{
    UInt64	total    = fork->total_size;
    UInt32	blksize  = vol -> blksize;
    size_t	size, bytes;
    blockiter   iter;
    char	buf[blksize]; /* this is a gcc feature */
    
    blockiter_init(&iter, vol, fork, forktype, cnid);
	
    /* This loop is optimized for a pipelined processor */
    while (total > 0)
    {
        UInt32  block   = blockiter_curr(&iter);
        int     result  = volume_readinbuf(vol, buf, block);
        size    = total > blksize ? blksize : total;
        if (result)
	{
	    ERROR(errno, hfsp_error);
            return result;
        }

	bytes = write(ofile, buf, size);
	total -= size;
	if (bytes != size)
	{
	    if (bytes == -1)
		ERROR(errno, "error writing data");
	    else
		ERROR(EIO, "wrote incomplete chunk");
	    return -1;
       	}
        if (blockiter_next(&iter))
	    break;
    }

    // fill up to MACB_BLOCKSZ
    size = (UInt32) fork->total_size & (MACB_BLOCKSZ -1);
    if (size > 0)		//   % MACB_BLCOKSZ
    {
	char buf[MACB_BLOCKSZ] = { 0 };
	bytes = write(ofile, buf, MACB_BLOCKSZ - size);
	if (bytes != (MACB_BLOCKSZ - size))
	{
	    if (bytes == -1)
		ERROR(errno, "error writing data");
	    else
		ERROR(EIO, "wrote incomplete chunk");
	    return -1;
       	}
    }

    return 0;
}

/*
 * NAME:	do_macb()
 * DESCRIPTION:	perform copy using MacBinary II translation
 */
static
int do_macb(record *ifile, int ofile)
{
    unsigned char   buf[MACB_BLOCKSZ] = { 0 };
    hfsp_cat_file   *file = &ifile->record.u.file;
    UInt64	    hsize;  // huge , HFS+ size
    UInt32	    bsize;  // size supported by Macbinary
    size_t	    bytes;
    void	    *p;

	/* Mhh, this limits filenames to 63 characters */
    buf[1] = unicode_uni2asc(&buf[2], &ifile->key.name, 63);

    p = &buf[65];
    bstoreU32_inc(p, file->user_info.fdType);	    // 65
    bstoreU32_inc(p, file->user_info.fdCreator);    // 69

    bstoreU8_inc (p, file->user_info.fdFlags >> 8); // 73

    p = &buf[83];
    // Umh , MacBinay does not support Uint64 sizes
    hsize = file->data_fork.total_size;
    if (hsize > UINT_MAX)
    {
	ERROR(errno, "MacBinary does not support files longer than 2^32 bytes");
	return -1;
    }
    bsize = hsize;
    bstoreU32_inc(p, bsize);			    // 83
    
    hsize = file->res_fork.total_size;
    if (hsize > UINT_MAX)
    {	/* I wonder if any MacOS resource manager is able to cope with that anyway :) */
	ERROR(errno, "MacBinary does not support files longer than 2^32 bytes");
	return -1;
    }
    bsize = hsize;
    bstoreU32_inc(p, bsize);			    // 87
    
    bstoreU32_inc(p, file->create_date);	/* 91 */
    bstoreU32_inc(p, file->content_mod_date);	/* 95 */

    p = &buf[101];
    bstoreU8_inc (p, file->user_info.fdFlags & 0xFF); /* 101 */
    p = &buf[122];
    bstoreU8_inc (p, 129);
    bstoreU8_inc (p, 129);

    bstoreU16_inc (p, crc_macb(buf, 124, 0x0000));

    bytes = write(ofile, buf, MACB_BLOCKSZ);
    if (bytes == -1)
    {
	ERROR(errno, "error writing data");
	return -1;
    }
    else if (bytes != MACB_BLOCKSZ)
    {
	ERROR(EIO, "wrote incomplete chunk");
	return -1;
    }

    if (fork_macb(ifile->tree->vol, &file->data_fork, HFSP_EXTENT_DATA,
		file->id, ofile) == -1)
	return -1;

    if (fork_macb(ifile->tree->vol, &file->res_fork, HFSP_EXTENT_RSRC,
		file->id, ofile) == -1)
	return -1;

    return 0;
}

/*
 * NAME:	fork->binh()
 * DESCRIPTION:	copy a single fork for BinHex
 * (The ofile is iplicitly given by binhex_start
 */
static
int fork_binh(volume* vol, hfsp_fork_raw* fork, UInt8 forktype,
	     UInt32 cnid)
{
    UInt64	total    = fork->total_size;
    UInt32	blksize  = vol -> blksize;
    size_t	size;
    blockiter   iter;
    char	buf[blksize]; /* this is a gcc feature */
    
    blockiter_init(&iter, vol, fork, forktype, cnid);

    /* This loop is optimized for a pipelined processor */
    while (total > 0)
    {
        UInt32  block   = blockiter_curr(&iter);
        int     result  = volume_readinbuf(vol, buf, block);
        size    = total > blksize ? blksize : total;
        if (result)
	{
	    ERROR(errno, hfsp_error);
            return result;
        }

	total -= size;
	
	if (binhex_insert(buf, size) == -1)
	{
	    ERROR(errno, binhex_error);
	    return -1;
	}

        if (blockiter_next(&iter))
	    break;
    }

    if (binhex_insertcrc() == -1)
    {
	ERROR(errno, binhex_error);
	return -1;
    }

    return 0;
}

/*
 * NAME:	binhx()
 * DESCRIPTION:	auxiliary BinHex routine
 */
static
int binhx(record *ifile)
{
    hfsp_cat_file   *file = &ifile->record.u.file;
    unsigned char   byte;
    unsigned char   name[255];
    void	    *p = name;
    UInt64	    hsize;  // huge , HFS+ size
    UInt32	    bsize;  // size supported by Macbinary

    byte = unicode_uni2asc(name, &ifile->key.name, 255);
    if (binhex_insert(&byte, 1) == -1 ||
        binhex_insert(name, byte + 1) == -1)
    {
	ERROR(errno, binhex_error);
	return -1;
    }
    /* name is recycled as buffer now */

    bstoreU32_inc(p, file->user_info.fdType);
    bstoreU32_inc(p, file->user_info.fdCreator);
    bstoreU16_inc(p, file->user_info.fdFlags);
    
    hsize = file->data_fork.total_size;
    if (hsize > UINT_MAX)
    {
	ERROR(errno, "Binhex does not support files longer than 2^32 bytes");
	return -1;
    }
    bsize = hsize;
    bstoreU32_inc(p, bsize);

    hsize = file->res_fork.total_size;
    if (hsize > UINT_MAX)
    {
	ERROR(errno, "Binhex does not support files longer than 2^32 bytes");
	return -1;
    }
    bsize = hsize;
    bstoreU32_inc(p, bsize);

    if (binhex_insert(name, 18) == -1 ||
	binhex_insertcrc() == -1)
    {
	ERROR(errno, binhex_error);
	return -1;
    }

    if (fork_binh(ifile->tree->vol, &file->data_fork, HFSP_EXTENT_DATA,
		    file->id) == -1)
	return -1;

    if (fork_binh(ifile->tree->vol, &file->res_fork, HFSP_EXTENT_RSRC,
		    file->id) == -1)
	return -1;

    return 0;
}

/*
 * NAME:	do_binh()
 * DESCRIPTION:	perform copy using BinHex translation
 */
static
int do_binh(record *ifile, int ofile)
{
    int result;

    if (binhex_start(ofile) == -1)
    {
	ERROR(errno, binhex_error);
	return -1;
    }

    result = binhx(ifile);

    if (binhex_end() == -1 && result == 0)
    {
	ERROR(errno, binhex_error);
	result = -1;
    }

    return result;
}

/*
 * NAME:	do_text()
 * DESCRIPTION:	perform copy using text translation
 */
static
int do_text(volume* vol, hfsp_fork_raw *fork, UInt8 forktype, 
	    UInt32 cnid, int ofile)
{
    UInt64	total    = fork->total_size;
    UInt32	blksize  = vol -> blksize;
    blockiter	iter;
    char	buf[blksize]; /* this is a gcc feature */

    blockiter_init(&iter, vol, fork, forktype, cnid);

    /* This loop is optimized for a pipelined processor */
    while (total > 0)
    {
        UInt32  block   = blockiter_curr(&iter);
        int     result  = volume_readinbuf(vol, buf, block);
        size_t	size    = total > blksize ? blksize : total;
	size_t	bytes;
	char	*latin1, *ptr, *ptrend;
        if (result)
	{
	    ERROR(errno, hfsp_error);
            return result;
        }

	latin1 = ptr = cs_latin1(buf, &size);
	if (ptr == 0)
	{
	    ERROR(ENOMEM, 0);
	    return -1;
	}
	ptrend = ptr + size;
	total -= size;
	while (ptr < ptrend)
	{
	    if (*ptr == '\r')
		*ptr = '\n';
	    ptr ++;
	}

	bytes = write(ofile, latin1, size);
	free(latin1);
        if (bytes != size)
	{
	    if (bytes == -1)
		ERROR(errno, "error writing data");
            else
                ERROR(EIO, "wrote incomplete chunk");
            return -1;
        }
	if (blockiter_next(&iter))
	    break;
    }

    return 0;
}

/*
 * NAME:	do_raw()
 * DESCRIPTION:	perform copy using no translation
 */
static
int do_raw(volume* vol, hfsp_fork_raw *fork, UInt8 forktype, 
	    UInt32 cnid, int ofile)
{
    UInt64	total    = fork->total_size;
    UInt32	blksize  = vol -> blksize;
    size_t	size, bytes;
    blockiter	iter;
    char	buf[blksize]; /* this is a gcc feature */

    blockiter_init(&iter, vol, fork, forktype, cnid);

    /* This loop is optimized for a pipelined processor */
    while (total > 0)
    {
	UInt32	block	= blockiter_curr(&iter);
	int	result  = volume_readinbuf(vol, buf, block);
	void*	p	= buf;
	size	= total > blksize ? blksize : total;
	if (result)
	{
	    ERROR(errno, hfsp_error);
	    return result;
	}

	bytes = write(ofile, p, size);
	
	total -= size;
	
	if (bytes != size)
	{
	    if (bytes == -1)
		ERROR(errno, "error writing data");
	    else
		ERROR(EIO, "wrote incomplete chunk");
	    return -1;
	}
	if (blockiter_next(&iter))
	    break;
    }

    return 0;
}

/* Utility Routines ======================================================== */

/*
 * NAME:	opensrc()
 * DESCRIPTION:	open the source file; set hint for destination filename
 */
static
int opensrc(record *src, const char **dsthint, const char *ext)
{
    static char name[255 + 4 + 1]; // Not multihtreaded, well
    char *ptr;

    if (src->record.type != HFSP_FILE)
    {
	ERROR(-1, "source is not a file");
	return -1;
    }
    unicode_uni2asc(name, &src->key.name, 255);

    for (ptr = name; *ptr; ++ptr)
    {
	if (*ptr == '/')
	    *ptr = '-';
    }

    if (ext)
	strcat(name, ext);

    *dsthint = name;
    return 0;
}

/*
 * NAME:	opendst()
 * DESCRIPTION:	open the destination file
 */
static
int opendst(const char *dstname, const char *hint)
{
    int fd;

    if (strcmp(dstname, "-") == 0)
	fd = dup(STDOUT_FILENO);
    else
    {
	struct stat sbuf;
	char *path = NULL;

	if (stat(dstname, &sbuf) != -1 &&
	    S_ISDIR(sbuf.st_mode))
	{
	    path = malloc(strlen(dstname) + 1 + strlen(hint) + 1);
	    if (!path)
	    {
		ERROR(ENOMEM, 0);
		return -1;
	    }

	    strcpy(path, dstname);
	    strcat(path, "/");
	    strcat(path, hint);

	    dstname = path;
	}

	// Mhh, better use umask here ?
	fd = open(dstname, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (path)
	    free(path);
    }

    if (fd == -1)
    {
	ERROR(errno, "error opening destination file");
	return -1;
    }

    return fd;
}

/*
 * NAME:	closefiles()
 * DESCRIPTION:	close source and destination files
 */
static
int closefiles(record *src, int ofile)
{
    int result = 0;
    if (close(ofile) == -1)
    {
	ERROR(errno, "error closing destination file");
	result = -1;
    }
    return result;
}

/*
 * NAME:	openfiles()
 * DESCRIPTION:	open source and destination files
 */
static
int openfiles(record *src, const char *dstname,
	      const char *ext, int *ofile)
{
    const char *dsthint;
    
    if (opensrc(src, &dsthint, ext))
	return -1;
    *ofile = opendst(dstname, dsthint);
    if (*ofile == -1)
	return -1;

    return 0;
}

/* Interface Routines ====================================================== */

/*
 * NAME:	cpo->macb()
 * DESCRIPTION:	copy an HFS file to a UNIX file using MacBinary II translation
 */
int copyout_macb(record *src, const char *dstname)
{
    int ofile, result = 0;

    if (openfiles(src, dstname, ".bin", &ofile))
	return -1;

    result = do_macb(src, ofile);

    result = closefiles(src, ofile) || result;

    return result;
}

/*
 * NAME:	cpo->binh()
 * DESCRIPTION:	copy an HFS file to a UNIX file using BinHex translation
 */
int copyout_binh(record *src, const char *dstname)
{
    int ofile, result;

    if (openfiles(src, dstname, ".hqx", &ofile) == -1)
	return -1;

    result = do_binh(src, ofile);

    return closefiles(src, ofile) || result;
}

/*
 * NAME:	cpo->text()
 * DESCRIPTION:	copy an HFS file to a UNIX file using text translation
 */
int copyout_text(record* src, const char *dstname)
{
    const char	    *ext = ".txt";
    int		    ofile, result = 0;
    hfsp_cat_file   *file = &src->record.u.file;

    if (openfiles(src, dstname, ext, &ofile) == -1)
	return -1;

    result = do_text(src->tree->vol, &file->data_fork, 
	    HFSP_EXTENT_DATA, file->id,ofile);

    return closefiles(src, ofile) || result;
}

/*
 * NAME:	cpo->raw()
 * DESCRIPTION:	copy the data fork of an HFS file to a UNIX file
 */
int copyout_raw(record *src, const char *dstname)
{
    int ofile, result = 0;
    hfsp_cat_file   *file = &src->record.u.file;

    if (openfiles(src, dstname, NULL, &ofile) == -1)
	return -1;

    result = do_raw(src->tree->vol, &file->data_fork,
		     HFSP_EXTENT_DATA, file->id, ofile);

    return closefiles(src, ofile) || result;
}
