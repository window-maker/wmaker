/* setstyle.c - loads style related options to wmaker
 *
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997~2000 Alfredo K. Kojima
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


#define PROG_VERSION "setstyle (Window Maker) 0.6"

#include <stdlib.h>
#include <stdio.h>
#include <proplist.h>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include <string.h>

#include "../src/wconfig.h"

#define MAX_OPTIONS 128

char *FontOptions[] = {
    "IconTitleFont",
	"ClipTitleFont",
	"DisplayFont",
	"LargeDisplayFont",
	"MenuTextFont",
	"MenuTitleFont",
	"WindowTitleFont",
	NULL
};

char *CursorOptions[] = {
    "NormalCursor"
    ,"ArrowCursor"
    ,"MoveCursor"
    ,"ResizeCursor"
    ,"TopLeftResizeCursor"
    ,"TopRightResizeCursor"
    ,"BottomLeftResizeCursor"
    ,"BottomRightResizeCursor"
    ,"VerticalResizeCursor"
    ,"HorizontalResizeCursor"
    ,"WaitCursor"
    ,"QuestionCursor"
    ,"TextCursor"
    ,"SelectCursor"
    ,NULL
};



char *ProgName;
int ignoreFonts = 0;
int ignoreCursors = 0;

Display *dpy;



proplist_t readBlackBoxStyle(char *path);



char*
defaultsPathForDomain(char *domain)
{
    static char path[1024];
    char *gspath;

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

    return path;
}



void
hackPathInTexture(proplist_t texture, char *prefix)
{
    proplist_t type;
    char *t;

    /* get texture type */
    type = PLGetArrayElement(texture, 0);
    t = PLGetString(type);
    if (t == NULL)
	return;
    if (strcasecmp(t, "tpixmap")==0
	|| strcasecmp(t, "spixmap")==0
	|| strcasecmp(t, "mpixmap")==0
	|| strcasecmp(t, "cpixmap")==0
	|| strcasecmp(t, "tvgradient")==0
	|| strcasecmp(t, "thgradient")==0		
	|| strcasecmp(t, "tdgradient")==0) {
	proplist_t file;
	char buffer[4018];

	/* get pixmap file path */
	file = PLGetArrayElement(texture, 1);
	sprintf(buffer, "%s/%s", prefix, PLGetString(file));
	/* replace path with full path */
	PLRemoveArrayElement(texture, 1);
	PLInsertArrayElement(texture, PLMakeString(buffer), 1);
    } else if (strcasecmp(t, "bitmap") == 0) {
	proplist_t file;
	char buffer[4018];

	/* get bitmap file path */
	file = PLGetArrayElement(texture, 1);
	sprintf(buffer, "%s/%s", prefix, PLGetString(file));
	/* replace path with full path */
	PLRemoveArrayElement(texture, 1);
	PLInsertArrayElement(texture, PLMakeString(buffer), 1);
	
	/* get mask file path */
	file = PLGetArrayElement(texture, 2);
	sprintf(buffer, "%s/%s", prefix, PLGetString(file));
	/* replace path with full path */
	PLRemoveArrayElement(texture, 2);
	PLInsertArrayElement(texture, PLMakeString(buffer), 2);
    }
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
	if (!value)
	    continue;
	
	if (strcasecmp(PLGetString(key), "WorkspaceSpecificBack")==0) {
	    if (PLIsArray(value)) {
		int j;
		proplist_t texture;
		
		for (j = 0; j < PLGetNumberOfElements(value); j++) {
		    texture = PLGetArrayElement(value, j);

		    if (texture && PLIsArray(texture) 
			&& PLGetNumberOfElements(texture) > 2) {

			hackPathInTexture(texture, prefix);
		    }
		}
	    }
	} else {
	    
	    if (PLIsArray(value) && PLGetNumberOfElements(value) > 2) {

		hackPathInTexture(value, prefix);
	    }
	}
    }
    
}


static proplist_t
getColor(proplist_t texture)
{
    proplist_t value, type;
    char *str;

    type = PLGetArrayElement(texture, 0);
    if (!type)
	return NULL;

    value = NULL;

    str = PLGetString(type);
    if (strcasecmp(str, "solid")==0) {
	value = PLGetArrayElement(texture, 1);
    } else if (strcasecmp(str, "dgradient")==0
	       || strcasecmp(str, "hgradient")==0
	       || strcasecmp(str, "vgradient")==0) {
	proplist_t c1, c2;
	int r1, g1, b1, r2, g2, b2;
	char buffer[32];

	c1 = PLGetArrayElement(texture, 1);
	c2 = PLGetArrayElement(texture, 2);
	if (!dpy) {
	    if (sscanf(PLGetString(c1), "#%2x%2x%2x", &r1, &g1, &b1)==3
		&& sscanf(PLGetString(c2), "#%2x%2x%2x", &r2, &g2, &b2)==3) {
		sprintf(buffer, "#%02x%02x%02x", (r1+r2)/2, (g1+g2)/2,
			(b1+b2)/2);
		value = PLMakeString(buffer);
	    } else {
		value = c1;
	    }
	} else {
	    XColor color1;
	    XColor color2;
	    
	    XParseColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), 
			PLGetString(c1), &color1);
	    XParseColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
			PLGetString(c2), &color2);

	    sprintf(buffer, "#%02x%02x%02x", 
		    (color1.red+color2.red)>>9,
		    (color1.green+color2.green)>>9,
		    (color1.blue+color2.blue)>>9);
	    value = PLMakeString(buffer);	    
	}
    } else if (strcasecmp(str, "mdgradient")==0
	       || strcasecmp(str, "mhgradient")==0
	       || strcasecmp(str, "mvgradient")==0) {
	
	value = PLGetArrayElement(texture, 1);

    } else if (strcasecmp(str, "tpixmap")==0
	       || strcasecmp(str, "cpixmap")==0
	       || strcasecmp(str, "spixmap")==0) {
	
	value = PLGetArrayElement(texture, 2);
    }

    return value;
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
    int foundResizebarBack = 0;

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
	    if (ignoreCursors) {
	        for (j = 0, found = 0; CursorOptions[j] != NULL; j++) {
		    if (strcasecmp(str, CursorOptions[j]) == 0) {
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
	    } else if (strcasecmp(str, "ResizebarBack")==0) {
		foundResizebarBack = 1;
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
	    proplist_t value;

	    value = getColor(tmp);

	    if (value) {
		PLInsertDictionaryEntry(style, PLMakeString("IconTitleBack"),
					value);
	    }
	}
    }

    if (!foundResizebarBack) {
	/* set the default values */
	tmp = PLGetDictionaryEntry(style, PLMakeString("UTitleBack"));
	if (tmp) {
	    proplist_t value;

	    value = getColor(tmp);

	    if (value) {
		proplist_t t;
		
		t = PLMakeArrayFromElements(PLMakeString("solid"), value, 
					    NULL);
		PLInsertDictionaryEntry(style, PLMakeString("ResizebarBack"),
					t);
	    }
	}
    }


    if (!PLGetDictionaryEntry(style, PLMakeString("MenuStyle"))) {
	PLInsertDictionaryEntry(style, PLMakeString("MenuStyle"),
				PLMakeString("normal"));
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
    puts("  --no-fonts		ignore font related options");
    /* Why these stupid tabs?  They're misleading to the programmer,
     * and they don't do any better than aligning via spaces:  If you
     * have a proportional font, all bets are off anyway.  Sheesh.
     */
    puts("  --no-cursors		ignore cursor related options");
    puts("  --ignore <option>	ignore changes in the specified option");
    puts("  --help		display this help and exit");
    /*
    puts("  --format <format>	specifies the format of the theme to be converted");
     */
    puts("  --version		output version information and exit");
    puts("");
    puts("Supported formats: blackbox");
}


#define F_BLACKBOX	1

int 
main(int argc, char **argv)
{
    proplist_t prop, style;
    char *path;
    char *file = NULL;
    struct stat statbuf;
    int i;
    int ignoreCount = 0;
    char *ignoreList[MAX_OPTIONS];

    dpy = XOpenDisplay("");

    ProgName = argv[0];
    
    if (argc<2) {
	printf("%s: missing argument\n", ProgName);
	printf("Try '%s --help' for more information\n", ProgName);
	exit(1);
    }

    for (i = 1; i < argc; i++) {
	if (strcmp("--ignore", argv[i])==0) {
	    i++;
	    if (i == argc) {
		printf("%s: missing argument for option --ignore\n", ProgName);
		exit(1);
	    }
	    ignoreList[ignoreCount++] = argv[i];

	} else if (strcmp("--no-fonts", argv[i])==0) {
	    ignoreFonts = 1;
	} else if (strcmp("--no-cursors", argv[i])==0) {
	    ignoreCursors = 1;
	} else if (strcmp("--version", argv[i])==0) {
	    puts(PROG_VERSION);
	    exit(0);
	} else if (strcmp("--help", argv[i])==0) {
	    print_help();
	    exit(0);
#if 0
	} else if (strcmp("--format", argv[i])==0) {
	    i++;
	    if (i == argc) {
		printf("%s: missing argument for option --format\n", ProgName);
		exit(1);
	    }
	    if (strcasecmp(argv[i], "blackbox")==0) {
		format = F_BLACKBOX;
	    } else {
		printf("%s: unknown theme format '%s'\n", ProgName, argv[i]);
		exit(1);
	    }
#endif
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
	perror(path);
	printf("%s:could not load WindowMaker configuration file.\n",
	       ProgName);
	exit(1);
    }

    if (stat(file, &statbuf) < 0) {
	perror(file);
	exit(1);
    }
#if 0
    if (format == F_BLACKBOX) {
	style = readBlackBoxStyle(file);
	if (!style) {
	    printf("%s: could not open style file\n", ProgName);
	    exit(1);
	}
    } else
#endif
 {
	if (S_ISDIR(statbuf.st_mode)) {
	    char buffer[4018];
	    char *prefix;
	    /* theme pack */

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
		perror(buffer);
		printf("%s:could not load style file.\n", ProgName);
		exit(1);
	    }

	    hackPaths(style, prefix);
	    free(prefix);
	} else {
	    /* normal style file */
	    
	    style = PLGetProplistWithPath(file);
	    if (!style) {
		perror(file);
		printf("%s:could not load style file.\n", ProgName);
		exit(1);
	    }
	}
    }
    
    if (!PLIsDictionary(style)) {
	printf("%s: '%s' is not a style file/theme\n", ProgName, file);
	exit(1);
    }

    hackStyle(style);

    if (ignoreCount > 0) {
	for (i = 0; i < ignoreCount; i++) {
	    PLRemoveDictionaryEntry(style, PLMakeString(ignoreList[i]));
	}
    }

    PLMergeDictionaries(prop, style);

    PLSave(prop, YES);
    {
	XEvent ev;
	
	if (dpy) {
	    int i;
	    char *msg = "Reconfigure";

	    memset(&ev, 0, sizeof(XEvent));
	    
	    ev.xclient.type = ClientMessage;
	    ev.xclient.message_type = XInternAtom(dpy, "_WINDOWMAKER_COMMAND",
						  False);
	    ev.xclient.window = DefaultRootWindow(dpy);
	    ev.xclient.format = 8;

	    for (i = 0; i <= strlen(msg); i++) {
		ev.xclient.data.b[i] = msg[i];
	    }
	    XSendEvent(dpy, DefaultRootWindow(dpy), False, 
		       SubstructureRedirectMask, &ev);
	    XFlush(dpy);
	}
    }

    exit(0);
}


#if 0
char*
getToken(char *str, int i, char *buf)
{
    
}


proplist_t
readBlackBoxStyle(char *path)
{
    FILE *f;
    char buffer[128], char token[128];
    proplist_t style;
    proplist_t p;
    
    f = fopen(path, "r");
    if (!f) {
	perror(path);
	return NULL;
    }

    while (1) {
	if (!fgets(buffer, 127, f))
	    break;

	if (strncasecmp(buffer, "menu.title:", 11)==0) {
	    
	}
    }
}
#endif
