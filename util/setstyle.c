/* setstyle.c - loads style related options to wmaker
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


#define PROG_VERSION "setstyle (Window Maker) 0.2"

#include <stdlib.h>
#include <stdio.h>
#include <proplist.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>

#include "../src/wconfig.h"


char *FontOptions[] = {
    "IconTitleFont",
	"ClipTitleFont",
	"DisplayFont",
	"MenuTextFont",
	"MenuTitleFont",
	"WindowTitleFont",
	NULL
};



char *ProgName;
int ignoreFonts = 0;


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
hackPaths(proplist_t style, char *prefix)
{
    proplist_t keys;
    proplist_t key;
    proplist_t value;
    int i;


    keys = PLGetAllDictionaryKeys(style);

    for (i = 0; i < PLGetNumberOfElements(keys); i++) {
	key = PLGetArrayElement(keys, i);

	value = PLGetDictionaryEntry(style, key);
	if (value && PLIsArray(value) && PLGetNumberOfElements(value) > 2) {
	    proplist_t type;
	    char *t;
	    
	    type = PLGetArrayElement(value, 0);
	    t = PLGetString(type);
	    if (t && (strcasecmp(t, "tpixmap")==0
		      || strcasecmp(t, "spixmap")==0
		      || strcasecmp(t, "mpixmap")==0
		      || strcasecmp(t, "cpixmap")==0
		      || strcasecmp(t, "tvgradient")==0
		      || strcasecmp(t, "thgradient")==0		
		      || strcasecmp(t, "tdgradient")==0)) {
		proplist_t file;
		char buffer[4018];

		file = PLGetArrayElement(value, 1);
		sprintf(buffer, "%s/%s", prefix, PLGetString(file));
		PLRemoveArrayElement(value, 1);
		PLInsertArrayElement(value, PLMakeString(buffer), 1);
	    }
	}
    }
    
}



/*
 * since some of the options introduce incompatibilities, we will need
 * to do a kluge here or the themes ppl will get real annoying.
 * So, treat for the absence of the following options:
 * IconTitleColor
 * IconTitleBack
 */
void
hackStyle(proplist_t style)
{
    proplist_t keys;
    proplist_t tmp;
    int i;
    int foundIconTitle = 0;

    keys = PLGetAllDictionaryKeys(style);

    for (i = 0; i < PLGetNumberOfElements(keys); i++) {
	char *str;

	tmp = PLGetArrayElement(keys, i);
	str = PLGetString(tmp);
	if (str) {
	    int j, found;

	    if (ignoreFonts) {
		for (j = 0, found = 0; FontOptions[j]!=NULL; j++) {
		    if (strcasecmp(str, FontOptions[j])==0) {
			PLRemoveDictionaryEntry(style, tmp);
			found = 1;
			break;
		    }
		} 
		if (found)
		    continue;
	    }

	    if (strcasecmp(str, "IconTitleColor")==0
		|| strcasecmp(str, "IconTitleBack")==0) {
		foundIconTitle = 1;
	    }
	}
    }

    if (!foundIconTitle) {
	/* set the default values */
	tmp = PLGetDictionaryEntry(style, PLMakeString("FTitleColor"));
	if (tmp) {
	    PLInsertDictionaryEntry(style, PLMakeString("IconTitleColor"),
				    tmp);
	}

	tmp = PLGetDictionaryEntry(style, PLMakeString("FTitleBack"));
	if (tmp) {
	    proplist_t type;
	    proplist_t value;
	    char *str;
	    
	    type = PLGetArrayElement(tmp, 0);
	    if (!type)
		return;

	    value = NULL;
	    
	    str = PLGetString(type);
	    if (strcasecmp(str, "solid")==0) {
		value = PLGetArrayElement(tmp, 1);
	    } else if (strcasecmp(str, "dgradient")==0
		       || strcasecmp(str, "hgradient")==0
		       || strcasecmp(str, "vgradient")==0) {
		proplist_t c1, c2;
		int r1, g1, b1, r2, g2, b2;
		char buffer[32];

		c1 = PLGetArrayElement(tmp, 1);
		c2 = PLGetArrayElement(tmp, 2);
		if (sscanf(PLGetString(c1), "#%2x%2x%2x", &r1, &g1, &b1)==3
		    && sscanf(PLGetString(c2), "#%2x%2x%2x", &r2, &g2, &b2)==3) {
		    sprintf(buffer, "#%2x%2x%2x", (r1+r2)/2, (g1+g2)/2,
			    (b1+b2)/2);
		    value = PLMakeString(buffer);
		} else {
		    value = c1;
		}
	    } else if (strcasecmp(str, "mdgradient")==0
		       || strcasecmp(str, "mhgradient")==0
		       || strcasecmp(str, "mvgradient")==0) {

		value = PLGetArrayElement(tmp, 1);

	    } else if (strcasecmp(str, "tpixmap")==0
		       || strcasecmp(str, "cpixmap")==0
		       || strcasecmp(str, "spixmap")==0) {

		value = PLGetArrayElement(tmp, 2);
	    }

	    if (value) {
		PLInsertDictionaryEntry(style, PLMakeString("IconTitleBack"),
					value);
	    }
	}
    }
}


BOOL
StringCompareHook(proplist_t pl1, proplist_t pl2)
{
    char *str1, *str2;

    str1 = PLGetString(pl1);
    str2 = PLGetString(pl2);

    if (strcasecmp(str1, str2)==0)
      return YES;
    else
      return NO;
}



void
print_help()
{
    printf("Usage: %s [OPTIONS] FILE\n", ProgName);
    puts("Reads style/theme configuration from FILE and updates Window Maker.");
    puts("");
    puts("  --no-fonts	ignore font related options");
    puts("  --help	display this help and exit");
    puts("  --version	output version information and exit");
}



int 
main(int argc, char **argv)
{
    proplist_t prop, style;
    char *path;
    char *file = NULL;
    struct stat statbuf;
    int i;

    ProgName = argv[0];
    
    if (argc<2) {
	printf("%s: missing argument\n", ProgName);
	printf("Try '%s --help' for more information\n", ProgName);
	exit(1);
    }

    for (i = 1; i < argc; i++) {
	if (strcmp("--no-fonts", argv[i])==0) {
	    ignoreFonts = 1;
	} else if (strcmp("--version", argv[i])==0) {
	    puts(PROG_VERSION);
	    exit(0);
	} else if (strcmp("--help", argv[i])==0) {
	    print_help();
	    exit(0);
	} else {
	    if (file) {
		printf("%s: invalid argument '%s'\n", ProgName, argv[i]);
		printf("Try '%s --help' for more information\n", ProgName);
		exit(1);
	    }
	    file = argv[i];
	}
    }

    PLSetStringCmpHook(StringCompareHook);

    path = defaultsPathForDomain("WindowMaker");

    prop = PLGetProplistWithPath(path);
    if (!prop) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    if (stat(file, &statbuf) < 0) {
	perror(file);
	exit(1);
    }

    if (S_ISDIR(statbuf.st_mode)) {
	char buffer[4018];
	char *prefix;

	if (*argv[argc-1] != '/') {
	    if (!getcwd(buffer, 4000)) {
		printf("%s: complete path for %s is too long\n", ProgName,
		       file);
		exit(1);
	    }
	    if (strlen(buffer) + strlen(file) > 4000) {
		printf("%s: complete path for %s is too long\n", ProgName,
		       file);
		exit(1);
	    }
	    strcat(buffer, "/");
	} else {
	    buffer[0] = 0;
	}
	strcat(buffer, file);

	prefix = malloc(strlen(buffer)+10);
	if (!prefix) {
	    printf("%s: out of memory\n", ProgName);
	    exit(1);
	}
	strcpy(prefix, buffer);

	strcat(buffer, "/style");
	
	style = PLGetProplistWithPath(buffer);
	if (!style) {
	    printf("%s:could not load style file \"%s\".\n", ProgName, 
		   buffer);
	    exit(1);
	}

	hackPaths(style, prefix);
	free(prefix);
    } else {
	style = PLGetProplistWithPath(file);
	if (!style) {
	    printf("%s:could not load style file \"%s\".\n", ProgName, file);
	    exit(1);
	}
    }

    hackStyle(style);

    PLMergeDictionaries(prop, style);
    
    PLSave(prop, YES);

    exit(0);
}


