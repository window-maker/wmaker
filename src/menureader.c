/* menureader.c- root menu definition readers
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 2000-2002 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "WindowMaker.h"

#include "misc.h"

#include "rootmenu.h"


typedef struct PLMenuReaderData {
    WRootMenuReader *reader;

    WMPropList *pl;
    int curIndex;
    
    WMPropList **submenu;
    int *curSubIndex;
    int submenuDepth;
    
} PLMenuReaderData;


typedef struct TextMenuReaderData {
    WRootMenuReader *reader;

    FILE *file;
    
} TextMenuReaderData;


typedef struct PipeMenuReaderData {
    WRootMenuReader *reader;
    
} PipeMenuReaderData;


typedef struct DirMenuReaderData {
    WRootMenuReader *reader;

    char **dirList;
    int dirCount;

} DirMenuReaderData;

/*
typedef struct GNOMEMenuReaderData {
} GNOMEMenuReaderData;
*/





static WRootMenuData *pl_openMenu(WMPropList *pl);
static Bool pl_hasMoreData(WRootMenuData *data);
static Bool pl_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut);
static void pl_closeMenuFile(WRootMenuData *data);



static WRootMenuData *text_openMenuFile(char *path);
static Bool text_hasMoreData(WRootMenuData *data);
static Bool text_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut);
static void text_closeMenuFile(WRootMenuData *data);


static WRootMenuData *dir_openMenuFile(char *path);
static Bool dir_hasMoreData(WRootMenuData *data);
static Bool dir_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut);
static void dir_closeMenuFile(WRootMenuData *data);






static WRootMenuReader PLMenuReader = {
	pl_hasMoreData,
	pl_nextCommand,
	pl_closeMenuFile
};

static WRootMenuReader TextMenuReader = {
	text_hasMoreData,
	text_nextCommand,
	text_closeMenuFile
};

static WRootMenuReader DirMenuReaderData = {
	dir_hasMoreData,
	dir_nextCommand,
	dir_closeMenuFile
};

/*
WRootMenuReader GNOMEMenuReaderData = {
};
*/




#define LINESIZE 1024

static char linebuf[LINESIZE];



/* ---------- proplist ---------- */


static WRootMenuData *pl_openMenuFile(WMPropList *pl)
{
    PLRootMenuData *data = wmalloc(sizeof(PLRootMenuData));

    data->reader = PLMenuReader;

    data->pl = pl;
    data->curIndex = 0;

    data->submenu = NULL;
    data->curSubIndex = NULL;
    data->submenuDepth = 0;

    return data;
}


static Bool pl_hasMoreData(WRootMenuData *data)
{
}


static Bool pl_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut)
{
}


static void pl_closeMenuFile(WRootMenuData *data)
{
    if (data->submenu)
	wfree(data->submenu);
    if (data->curSubIndex)
	wfree(data->curSubIndex);

    WMReleasePropList(data->pl);
    
    wfree(data);
}


/* ---------- text ---------- */


static WRootMenuData *text_openMenuFile(char *path)
{
    TextMenuReaderData *data;
    
    data = wmalloc(sizeof(TextMenuReaderData));
    data->reader = TextMenuReader;
    
    data->file = fopen(path, "r");
    if (!data->file) {

	return NULL;
    }
}


static Bool text_hasMoreData(WRootMenuData *data)
{
}


static Bool text_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut)
{
}


static void text_closeMenuFile(WRootMenuData *data)
{
}


/* ---------- directory ---------- */


static WRootMenuData *dir_openMenuFile(char *paths, time_t *timestamp)
{
    DirMenuReaderData *data;
    char **dirs;
    int dirN;
    time_t checksum = 0;
    int i, c;

    /* timestamp for directory is a "checksum" of the directory times */

    wtokensplit(paths, &dirs, &dirN);

    if (dirN == 0) {
	return NULL;
    }

    for (c = 0, i = 0; i < dirN; i++) {
	char *tmp;

	if (strcmp(dirs[i], "-noext")==0) {
	    i++;
	    continue;
	}

	tmp = wexpandpath(dirs[i]);
	wfree(dirs[i]);
	dirs[i] = tmp;

	if (stat(dirs[i], &stat_buf)<0) {
	    wsyserror(_("%s:could not stat menu"), dirs[i]);
	    wfree(dirs[i]);
	    dirs[i] = NULL;
	} else {
	    c++;
	    checksum += stat_buf.st_mtime;
	}
    }

    if (*timestamp == checksum && *timestamp != 0) {
	return NULL;
    }

    if (c == 0) {
	for (i = 0; i < dirN; i++) {
	    if (dirs[i])
		wfree(dirs[i]);
	}
	wfree(dirs);
	
	return NULL;
    }
    
    data = wmalloc(sizeof(DirMenuReaderData));
    data->reader = DirMenuReader;
    
    
    
}


static Bool dir_hasMoreData(WRootMenuData *data)
{
}

static Bool dir_nextCommand(WRootMenuData *data,
			char **title,
			char **command,
			char **parameter,
			char **shortcut)
{
}

static void dir_closeMenuFile(WRootMenuData *data)
{
}





WRootMenuData *OpenMenu(char *path, time_t *menuTime)
{
    proplist pl;
    struct stat stat_buf;
    WRootMenuData *data;
    
    /* check whether it's a piped menu */
    if (*path == '|') {
	/* piped menus have the following semantics for menuTime:
	 * if it's 0, then it wasnt loaded yet
	 * if it's 1, it was already loaded, so do not reload 
	 *	(would be too slow)
	 * now, menuTime will only be set to 1 if the pipe command is
	 *	specified as ||command instead of |command
	 * in other words ||command means force the submenu to always refresh
	 */
	if (*menuTime == 0) {
	    data = pipe_openMenu(path);
	} 
	if (path[1] != '|') {
	    *menuTime = 1;
	}
	return data;
    }

    if (stat(path, &stat_buf) < 0) {
	wsyserror(_("could not stat() menu file '%s'"));
	return NULL;
    }
    
    /* check whether it's a directory */
    if (S_ISDIR(stat_buf.st_mode)) {
	return dir_openMenuFile(path, menuTime);
    }


    if (*menuTime >= stat_buf.st_mtime && *menuTime != 0) {
	/* no changes in the menu file */
	return NULL;
    }

    /* then check whether it's a proplist menu */
    pl = WMReadPropListFromFile(path);
    if (pl && WMIsPLArray(pl)) {
	*menuTime = stat_buf.st_mtime;
	return pl_openMenu(pl);
    }

    *menuTime = stat_buf.st_mtime;
    /* assume its a plain text menu */
    return text_openMenuFile(path);
}



WRootMenuData *ReopenRootMenu(time_t *checkTime, 
			      char **menuPath,
			      time_t *menuTimestamp)
{
    proplist pl;
    struct stat stat_buf;
    char *path;
    

    if (stat(path, &stat_buf) < 0) {
	wsyserror(_("could not stat() menu file '%s'"));
	return NULL;
    }

    if (*menuTime >= stat_buf.st_mtime && *checkTime != 0) {
	/* no changes in WMRootMenu, see if the contents changed */
	if (*menuPath != NULL) {
	    return OpenMenu(*menuPath, menuTimestamp);
	} else {
	    return NULL;
	}
    }

    *checkTime = stat_buf.st_mtime;

    pl = WMReadPropListFromFile(path);
    if (!pl) {
	wwarning(_("could not load domain %s from user defaults database"),
		 "WMRootMenu");
	return NULL;
    }

    if (WMIsPLString(pl)) {
	char *tmp;
	char *path;
	Bool menu_is_default = False;

	tmp = wexpandpath(WMGetFromPLString(pl));

	path = getLocalizedMenuFile(tmp);

	if (!path) {
	    path = wfindfile(DEF_CONFIG_PATHS, tmp);
	}

	if (!path) {
	    wwarning(_("could not find menu file '%s' referenced in WMRootMenu"),
		     tmp);
	    path = wfindfile(DEF_CONFIG_PATHS, DEF_MENU_FILE);
	    menu_is_default = True;
	}

	if (!path) {
	    wwarning(_("could not find any usable menu files. Please check '%s'"),
		      tmp);
	    wfree(tmp);
	    return NULL;
	}
	
	wfree(tmp);
	
	if (*menuPath) {
	    if (strcmp(*menuPath, path) != 0) {
		*menuTimestamp = 0;
		wfree(*menuPath);
		*menuPath = path;
		
		if (menu_is_default) {
		    wwarning(_("using default menu file \"%s\" as the menu referenced in WMRootMenu could not be found "),
			     path);
		}
	    } else {
		/* the menu path didn't change, but the 
		 * pointed file might have changed, so we don't return 
		 */
	    }
	} else {
	    *menuPath = path;
	}

	return OpenMenu(*menuPath, menuTimestamp);
    } else if (WMIsPLArray(pl)) {

	*menuTimestamp = stat_buf.st_mtime;

	return pl_openMenu(pl);
    } else {
	wwarning(_("invalid content in menu file '%s'.\nIt should either be a property list menu or the path to the file, enclosed in \"."), 
		 path);
	return NULL;
    }
}



