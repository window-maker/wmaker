/* geticonset.c - outputs icon configuration from WindowMaker to stdout
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


int 
main(int argc, char **argv)
{
    proplist_t window_name, icon_key, window_attrs, icon_value;
    proplist_t all_windows, iconset;
    proplist_t keylist;
    char *path;
    int i;

    
    ProgName = argv[0];
    
    if (argc>2 || (argc==2 && strcmp(argv[1], "-h")==0)) {
	printf("Syntax:\n%s [<iconset file>]\n", argv[0]);
	exit(1);
    }
    
    path = defaultsPathForDomain("WMWindowAttributes");
    
    all_windows = PLGetProplistWithPath(path);
    if (!all_windows) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    iconset = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    keylist = PLGetAllDictionaryKeys(all_windows);
    icon_key = PLMakeString("Icon");
    
    for (i=0; i<PLGetNumberOfElements(keylist); i++) {
	proplist_t icondic;
	
	window_name = PLGetArrayElement(keylist, i);
	if (!PLIsString(window_name))
	    continue;
		
	window_attrs = PLGetDictionaryEntry(all_windows, window_name);
	if (window_attrs && PLIsDictionary(window_attrs)) {
	    icon_value = PLGetDictionaryEntry(window_attrs, icon_key);
	    if (icon_value) {
		
		icondic = PLMakeDictionaryFromEntries(icon_key, icon_value, 
						      NULL);

		PLInsertDictionaryEntry(iconset, window_name, icondic);
	    }
	}
    }
    

    if (argc==2) {
	proplist_t val;
	val = PLMakeString(argv[1]);
	PLSetFilename(iconset, val);
	PLSave(iconset, NO);
    } else {
	puts(PLGetDescriptionIndent(iconset, 0));
    }
    exit(0);
}


