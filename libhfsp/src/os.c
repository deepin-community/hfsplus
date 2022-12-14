/*
 * libhfsp - library for reading and writing Macintosh HFS+ volumes
 *
 * Copyright (C) 2000-2001 Klaus Halfmann <klaus.halfmann@t-online.de>
 * Original Copyright (C) 1996-1998 Robert Leslie
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
 * $Id: os.c,v 1.2 2002/03/25 15:48:37 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#define _FILE_OFFSET_BITS 64

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# else
int open(const char *, int, ...);
int fcntl(int, int, ...);
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# else
int close(int);
off_t lseek(int, off_t, int);
size_t read(int, void *, size_t);
size_t write(int, const char *, size_t);
int stat(const char *, struct stat *);
int fstat(int, struct stat *);
# endif

# include <stdio.h>


# include <errno.h>
# include <sys/stat.h>
    // need BLKGETSIZE ioctl call
// # include <linux/fs.h>

# include "libhfsp.h"
# include "os.h"

/** Offset that is silently used by os_seek.
 *  Nedded to transparaently support things like partitions */
UInt64 os_offset;

/*
 * NAME:	os->open()
 * DESCRIPTION:	open and lock a new descriptor from the given path and mode
 */
int os_open(void **priv, const char *path, int mode)
{
  int fd;
  struct flock lock;
  int	 c;

  switch (mode)
    {
    case HFSP_MODE_RDONLY:
      mode = O_RDONLY;
      break;

    case HFSP_MODE_RDWR:
    default:
      fprintf(stderr,"*** Warning: You are about to open '%s' "
	     "for writing ***\n", path);
      fprintf(stderr,"*** Do you really want to do that ? (y/n) ***\n");
      do 
	 c = getchar();
      while (c != 'y' && c != 'n');
      if (c != 'y')
	  exit(1);
      mode = O_RDWR;
      break;
    }


  fd = open(path, mode);
  if (fd == -1)
    HFSP_ERROR(errno, "error opening medium");

  /* lock descriptor against concurrent access */

  lock.l_type   = (mode == O_RDONLY) ? F_RDLCK : F_WRLCK;
  lock.l_start  = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len    = 0;

  if (fcntl(fd, F_SETLK, &lock) == -1 &&
      (errno == EACCES || errno == EAGAIN))
    HFSP_ERROR(EAGAIN, "unable to obtain lock for medium");

  *priv = (void *) fd;

  return 0;

fail:
  if (fd != -1)
    close(fd);

  return -1;
}

/*
 * NAME:	os->close()
 * DESCRIPTION:	close an open descriptor
 */
int os_close(void **priv)
{
  int fd = (int) *priv;

  *priv = (void *) -1;

  if (close(fd) == -1)
    HFSP_ERROR(errno, "error closing medium");

  return 0;

fail:
  return -1;
}

/*
 * NAME:	os->same()
 * DESCRIPTION:	return 1 iff path is same as the open descriptor
 */
int os_same(void **priv, const char *path)
{
  int fd = (int) *priv;
  struct stat fdev, dev;

  if (fstat(fd, &fdev) == -1 ||
      stat(path, &dev) == -1)
    HFSP_ERROR(errno, "can't get path information");

  return fdev.st_dev == dev.st_dev &&
         fdev.st_ino == dev.st_ino;

fail:
  return -1;
}

/*
 * NAME:	os->seek()
 * DESCRIPTION:	set a descriptor's seek pointer (offset in blocks) and returns
 * 		this offset if everything went well
 */
unsigned long os_seek(void **priv, unsigned long offset, int blksize_bits)
{
    int	    fd = (int) *priv;
    off_t   result, where = offset;

    where  = os_offset + (where << blksize_bits);
    result = lseek(fd, where, SEEK_SET) - os_offset;
    
    result = result >> blksize_bits; 
    return (unsigned long) result;
}

/*
 * NAME:	os->read()
 * DESCRIPTION:	read blocks from an open descriptor
 *
 * @param **priv	is a pointer to the file descriptor
 * @param *buf		buffer to put the content in
 * @param len		number of buffers to read
 * @param blksize_bits	blocksize as a power of 2 (e.g. 9 for 512)
 *
 * @return number of read blocks or -1 on error
 */
unsigned long os_read(void **priv, void *buf, unsigned long len, int blksize_bits)
{
    int fd = (int) *priv;
    size_t result= 0;
    int num= len << blksize_bits;

    while( result< num) {
	size_t size = read(fd, &((char *)buf)[ result], num- result);
	if (size <= 0) /* EOF == 0 should not happen, too */
	    HFSP_ERROR(errno, "error reading from medium");
	result += size;
    }
    return (unsigned long) result >> blksize_bits;

fail:
    return -1;
}

/*
 * NAME:	os->write()
 * DESCRIPTION:	write blocks to an open descriptor
 */
unsigned long os_write(void **priv, const void *buf, unsigned long len, int blksize_bits)
{
  int fd = (int) *priv;
  size_t result;

  result = write(fd, buf, len << blksize_bits);

  if (result == -1)
    HFSP_ERROR(errno, "error writing to medium");

  return (unsigned long) result >> blksize_bits;

fail:
  return -1;
}
