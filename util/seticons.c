/* seticons.c - sets icon configuration in WindowMaker
 *
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

#define PROG_VERSION "seticons (Window Maker) 0.1"


#include <stdlib.h>
#include <stdio.h>
#include <proplist.h>

#include <string.h>

#include "../src/wconfig.h"

char *ProgName;

char*
defaultsPathForDomain(char *domain)
{
    char path[1024];
    char *gspath, *tmp;

    gspath = getenv("GNUSTEP_USER_ROOT");
    if (gspath) {
	strcpy(path, gspath);
	strcat(path, "/");
    } else {
	char *home;
	
	home = getenv("HOME");
	if (!home) {
	    printf("%s:could not get HOME environment variable!\n", ProgName);
	    exit(0);
	}

	strcpy(path, home);
	strcat(path, "/GNUstep/");
    }
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    tmp = malloc(strlen(path)+2);
    strcpy(tmp, path);
    
    return tmp;
}


void
print_help()
{
    printf("Usage: %s [OPTIONS] FILE\n", ProgName);
    puts("Reads icon configuration from FILE and updates Window Maker.");
    puts("");
    puts("  --help	display this help and exit");
    puts("  --version	output version information and exit");
}


int 
main(int argc, char **argv)
{
    proplist_t window_name, icon_key, window_attrs, icon_value;
    proplist_t all_windows, iconset;
    proplist_t keylist;
    int i;
    char *path = NULL;

    ProgName = argv[0];


    if (argc < 2) {
	printf("%s: missing argument\n", ProgName);
	printf("Try '%s --help' for more information\n", ProgName);
    }

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-h")==0
	    || strcmp(argv[i], "--help")==0) {
	    print_help();
	    exit(0);
	} else if (strcmp(argv[i], "--version")==0) {
	    puts(PROG_VERSION);
	    exit(0);
	} else {
	    if (path) {
		printf("%s: invalid argument '%s'\n", ProgName, argv[i]);
		printf("Try '%s --help' for more information\n", ProgName);
		exit(1);
	    }
	    path = argv[i];
	}
    }
    
    path = defaultsPathForDomain("WMWindowAttributes");
    
    all_windows = PLGetProplistWithPath(path);
    if (!all_windows) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    iconset = PLGetProplistWithPath(argv[1]);
    if (!iconset) {
	printf("%s:could not load icon set file \"%s\".\n", ProgName, argv[1]);
	exit(1);
    }

    
    keylist = PLGetAllDictionaryKeys(iconset);
    icon_key = PLMakeString("Icon");
    
    for (i=0; i<PLGetNumberOfElements(keylist); i++) {	
	window_name = PLGetArrayElement(keylist, i);
	if (!PLIsString(window_name))
	    continue;

	icon_value = PLGetDictionaryEntry(iconset, window_name);
	if (!icon_value || !PLIsDictionary(icon_value))
	    continue;

	window_attrs = PLGetDictionaryEntry(all_windows, window_name);
	if (window_attrs) {
	    if (PLIsDictionary(window_attrs)) {
		PLMergeDictionaries(window_attrs, icon_value);
	    }
	} else {
	    PLInsertDictionaryEntry(all_windows, window_name, icon_value);
	}
    }
    
    PLSave(all_windows, YES);

    exit(0);
}


