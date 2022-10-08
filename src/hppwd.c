/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * This programm is ananlog to pwd for HFS+ volumes.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: hppwd.c,v 1.1.1.1 2002/03/05 19:50:29 klaus Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdio.h>

# include "libhfsp.h"
# include "record.h"
# include "unicode.h"

# include "hfsputil.h"
# include "hpcache.h"
# include "dstring.h"

static int hppwd_ascend(dstring* append, record *r)
{
    record  parent = *r;
    int	    result = 0;
    int	    len;
    UInt16  type;
    char    buf[255];

    if (!record_up(&parent))
	result = hppwd_ascend(append, &parent);
    type = r->record.type;
    if (dstring_append(append , "/", 1) || result)
	return -1;
    if (type == HFSP_FOLDER)
	len = unicode_uni2asc(buf,&r->key.name, 255);
    else if (type == HFSP_FOLDER_THREAD)
	len = unicode_uni2asc(buf,&r->record.u.thread.nodeName, 255);
    else
	return -1; // not a folder ?
    if (dstring_append(append , buf, len))
	return -1;
    return 0;
}

/*
 * NAME:	hppwd->main()
 * DESCRIPTION:	implement hppwd command
 */
int main(int argc, char *argv[])
{
    volume  vol;
    record  rec;
    dstring append; // path is accumulated here

    argv0 = argv[0];
    
    if (argc != 1)
    {
	fprintf(stderr, "Usage: %s\n", argv0);
	return 1;
    }
    dstring_init(&append);

    if (hpcache_reopen(&vol,&rec,HFSP_MODE_RDONLY))
        goto fail;

    if (hppwd_ascend(&append, &rec))
	goto fail;

    printf("%s\n", dstring_string(&append));
    dstring_free(&append);

    return 0;
  fail:
    hfsputil_perror(argv0);
    dstring_free(&append);
    return 1;
}
