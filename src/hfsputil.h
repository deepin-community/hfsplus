/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 *
 * Utility functions for HFS+ and linux files.
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
 * $Id: hfsputil.h,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

extern int errno;

extern const char *argv0, *bargv0;

 /* output an HFS error */
void hfsputil_perror(const char *);

 /* output an HFS error for a pathname */
void hfsputil_perrorp(const char *);

// hfsvol *hfsputil_remount(mountent *, int);
// void hfsputil_unmount(volume *, int *);

// void hfsputil_pinfo(hfspvolent *);
char **hfsputil_glob(record *root, int, char *[], int *, int *);
// char *hfsputil_getcwd(volume *);

/* return 1 iff paths refer to same object */
int   hfsputil_samepath(const char *, const char *);

/*  make given UNIX path absolute (must be free()'d) */
char* hfsputil_abspath (const char *);

