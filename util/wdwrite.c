/* wdwrite.c - write key/value to defaults database
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


/*
 * WindowMaker defaults DB writer
 */


#include "../src/wconfig.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <proplist.h>
#include <pwd.h>


char *ProgName;

char*
gethomedir()
{
    char *home = getenv("HOME");
    struct passwd *user;

    if (home)
      return home;
    
    user = getpwuid(getuid());
    if (!user) {
	perror(ProgName);
        return "/";
    }
    if (!user->pw_dir) {
        return "/";
    } else {
        return user->pw_dir;
    }
}



void wAbort()
{
    exit(0);
}

void help()
{
    printf("Syntax:\n%s <domain> <option> <value>\n", ProgName);
    exit(1);
}


int main(int argc, char **argv)
{
    char path[256];
    proplist_t dom, key, value, dict;
    char *gsdir;
    
    ProgName = argv[0];

    if (argc<4) {
	help();
    }
    
    dom = PLMakeString(argv[1]);
    key = PLMakeString(argv[2]);
    value = PLGetProplistWithDescription(argv[3]);
    if (!value) {
	printf("%s:syntax error in value \"%s\"", ProgName, argv[3]);
	exit(1);
    }
    gsdir = getenv("GNUSTEP_USER_ROOT");
    if (gsdir) {
	strcpy(path, gsdir);
    } else {
	strcpy(path, gethomedir());
	strcat(path, "/GNUstep");
    }
    strcat(path, "/");
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, argv[1]);

    dict = PLGetProplistWithPath(path);
    if (!dict) {
	dict = PLMakeDictionaryFromEntries(key, value, NULL);
	PLSetFilename(dict, PLMakeString(path));
    } else {
	PLRemoveDictionaryEntry(dict, key);
	PLInsertDictionaryEntry(dict, key, value);
    }
    
    PLSave(dict, YES);
	
    return 0;
}


