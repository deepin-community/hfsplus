/*
 * hfsputils - tools for reading and writing Macintosh HFS+ volumes
 *
 * This program allows to change the "current" directory remembered
 * by the set of hp... tools.
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
 * $Id: hpcd.c,v 1.2 2002/03/23 11:47:25 klaus Exp $
 */
   
# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "libhfsp.h"
# include "record.h"
# include "volume.h"

# include "hpcache.h" 
# include "hfsputil.h"  

/*
 * NAME:	hpcd->main()
 * DESCRIPTION:	implement hcd command
 */
int main(int argc, char *argv[])
{
    volume vol;
    record old,new;
   
    char    *path = ".";
    int	    fargc;
    char    **fargv = 0;
    int	    result = 0;

    argv0 = argv[0];
    if (argc > 2)
    {
	fprintf(stderr, "Usage: %s [hfs-path]\n", argv0);
	    return 1;
    }

    if (hpcache_reopen(&vol,&old,HFSP_MODE_RDONLY))
        goto fail;

    if (argc == 2)
    {
	fargv = hfsputil_glob(&old, 1, &argv[1], &fargc, &result);
	if (result == 0)
	{
	    if (fargc != 1)
	    {
		fprintf(stderr, "%s: %s: ambiguous path\n", argv0, argv[1]);
		result = 1;
	    }
	    else
		path = fargv[0];
	}
    }

    /* todo: Schleife einbauen, die durch die Pfadtrenner iteriert */
    if (result == 0)
    {
	if (path[0] == '.' && path[1] == '.')	// == '..'
	{
	    new = old;
	    result = record_up(&new); 
	    if (result)
		hfsputil_perror(argv0);
	}
	else
	{
	    result = record_init_string_parent(&new, &old, path);
	    if (result)
		hfsputil_perror(argv0);
	    else if (new.record.type != HFSP_FOLDER)
	    {
		fprintf(stderr, "%s: %s is not a directory\n", argv0, path);
		result = 1;
	    }
	}
    }
    if (result == 0)
    {
	if (new.record.type == HFSP_FOLDER)
	    hpcache_cwd(new.record.u.folder.id);
	else if (new.record.type == HFSP_FOLDER_THREAD)
	    hpcache_cwd(new.key.parent_cnid);
	else
	{
	    fprintf(stderr, "%s: %s is not a directory\n", argv0, path);
	    result = 1;
	}
    }
    if (fargv)
	free(fargv);

    result = volume_close(&vol) || result;                                      
    return result;
  fail:
    hfsputil_perror(argv[0]);
    return result; 
}
