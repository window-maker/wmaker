/* getstyle.c - outputs style related options from WindowMaker to stdout
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

/* table of style related options */
static char *options[] = {
    "TitleJustify",	
    "WindowTitleFont",	
    "MenuTitleFont",	
    "MenuTextFont",	
    "IconTitleFont",	
    "ClipTitleFont",
    "DisplayFont",	
    "HighlightColor",	
    "HighlightTextColor",
    "ClipTitleColor",
    "CClipTitleColor",
    "FTitleColor",	
    "PTitleColor",	
    "UTitleColor",	
    "FTitleBack",	
    "PTitleBack",	
    "UTitleBack",	
    "MenuTitleColor",	
    "MenuTextColor",	
    "MenuDisabledColor", 
    "MenuTitleBack",	
    "MenuTextBack",
    "IconBack",	
    NULL
};


/* table of theme related options */
static char *theme_options[] = {
    "WorkspaceBack",
    NULL
};




char *ProgName;


void
print_help()
{
    printf("usage: %s [-options] [<style file>]\n", ProgName);
    puts("options:");
    puts(" -h	print help");
    puts(" -t	get theme options too");
}


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
    proplist_t prop, style, key, val;
    char *path;
    int i, theme_too=0;
    char *style_file = NULL;


    ProgName = argv[0];

    if (argc>1) {
	for (i=1; i<argc; i++) {
            if (strcmp(argv[i], "-t")==0) {
                theme_too++;;
            } else if (argv[i][0] != '-') {
                style_file = argv[i];
            } else {
                print_help();
                exit(1);
            }
        }
    }

    path = defaultsPathForDomain("WindowMaker");

    prop = PLGetProplistWithPath(path);
    if (!prop) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    style = PLMakeDictionaryFromEntries(NULL, NULL, NULL);


    for (i=0; options[i]!=NULL; i++) {
        key = PLMakeString(options[i]);

        val = PLGetDictionaryEntry(prop, key);
        if (val)
            PLInsertDictionaryEntry(style, key, val);
    }

    if (theme_too) {
        for (i=0; theme_options[i]!=NULL; i++) {
            key = PLMakeString(theme_options[i]);

            val = PLGetDictionaryEntry(prop, key);
            if (val)
                PLInsertDictionaryEntry(style, key, val);
        }
    }

    if (style_file) {
	val = PLMakeString(style_file);
	PLSetFilename(style, val);
	PLSave(style, NO);
    } else {
	puts(PLGetDescriptionIndent(style, 0));
    }
    exit(0);
}


