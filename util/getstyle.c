/* getstyle.c - outputs style related options from WindowMaker to stdout
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


#define PROG_VERSION "getstyle (Window Maker) 0.6"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <assert.h>
#include <WINGs/WUtil.h>

#ifndef PATH_MAX
#define PATH_MAX  1024
#endif

#include "../src/wconfig.h"

/* table of style related options */
static char *options[] = {
    "TitleJustify",	
    "ClipTitleFont",
    "WindowTitleFont",	
    "MenuTitleFont",	
    "MenuTextFont",	
    "IconTitleFont",
    "DisplayFont",
    "LargeDisplayFont",
    "WindowTitleExtendSpace",
    "MenuTitleExtendSpace",
    "MenuTextExtendSpace",
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
    "ResizebarBack",
    "MenuTitleColor",	
    "MenuTextColor",	
    "MenuDisabledColor", 
    "MenuTitleBack",	
    "MenuTextBack",
    "IconBack",	
    "IconTitleColor",
    "IconTitleBack",
    "MenuStyle",
    "WindowTitleExtendSpace",
    "MenuTitleExtendSpace",
    "MenuTextExtendSpace",
    NULL
};


/* table of theme related options */
static char *theme_options[] = {
    "WorkspaceBack",
    "NormalCursor",
    "ArrowCursor",
    "MoveCursor",
    "ResizeCursor",
    "TopLeftResizeCursor",
    "TopRightResizeCursor",
    "BottomLeftResizeCursor",
    "BottomRightResizeCursor",
    "VerticalResizeCursor",
    "HorizontalResizeCursor",
    "WaitCursor",
    "QuestionCursor",
    "TextCursor",
    "SelectCursor",
    NULL
};




char *ProgName;

WMPropList *PixmapPath = NULL;

char *ThemePath = NULL;


void
print_help()
{
    printf("Usage: %s [OPTIONS] [FILE]\n", ProgName);
    puts("Retrieves style/theme configuration and output to FILE or to stdout");
    puts("");
    puts("  -t, --theme-options	output theme related options when producing a style file");
    puts("  -p, --pack		produce output as a theme pack");
    puts("  --help		display this help and exit");
    puts("  --version		output version information and exit");
}


char*
globalDefaultsPathForDomain(char *domain)
{
    static char path[1024];

    sprintf(path, "%s/WindowMaker/%s", SYSCONFDIR, domain);

    return path;
}


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
abortar(char *reason)
{
    char buffer[4000];

    printf("%s: %s\n", ProgName, reason);

    if (ThemePath) {
	printf("Removing unfinished theme pack\n");
	sprintf(buffer, "/bin/rm -fr \"%s\"", ThemePath);
	
	if (system(buffer)!=0) {
	    printf("%s: could not execute command %s\n", ProgName, buffer);
	}
    }
    exit(1);
}




char*
wgethomedir()
{
    char *home = getenv("HOME");
    struct passwd *user;

    if (home)
      return home;
    
    user = getpwuid(getuid());
    if (!user) {
	char buffer[80];
	
	sprintf(buffer, "could not get password entry for UID %i", getuid()); 
	perror(buffer);
	return "/";
    }
    if (!user->pw_dir) {
	return "/";
    } else {
	return user->pw_dir;
    }
}


void*
mymalloc(int size)
{
    void *tmp;
    
    tmp = malloc(size);
    if (!tmp) {
	abortar("out of memory");
    }
    
    return tmp;
}


char*
mystrdup(char *str)
{
    char *tmp;
    
    tmp = mymalloc(strlen(str)+1);

    strcpy(tmp, str);

    return tmp;
}


static char*
getuserhomedir(char *username)
{
    struct passwd *user;
    
    user = getpwnam(username);
    if (!user) {
	char buffer[100];

	sprintf(buffer,"could not get password entry for user %s", username);
	perror(buffer);
	return NULL;
    }
    if (!user->pw_dir) {
	return "/";
    } else {
	return user->pw_dir;
    }
}




char*
wexpandpath(char *path)
{
    char buffer2[PATH_MAX+2];
    char buffer[PATH_MAX+2];
    int i;

    memset(buffer, 0, PATH_MAX+2);
    
    if (*path=='~') {
	char *home;
	
	path++;
	if (*path=='/' || *path==0) {
	    home = wgethomedir();
	    strcat(buffer, home);
	} else {
	    int j;
	    j = 0;
	    while (*path!=0 && *path!='/') {
		buffer2[j++] = *path;
		buffer2[j] = 0;
		path++;
	    }
	    home = getuserhomedir(buffer2);
	    if (!home)
		return NULL;
	    strcat(buffer, home);
	}
    }
    
    i = strlen(buffer);

    while (*path!=0) {
	char *tmp;
	
	if (*path=='$') {
	    int j = 0;
	    path++;
	    /* expand $(HOME) or $HOME style environment variables */
	    if (*path=='(') {
		path++;
		while (*path!=0 && *path!=')') {
		    buffer2[j++] = *(path++);
		    buffer2[j] = 0;
                }
                if (*path==')')
                    path++;
		tmp = getenv(buffer2);
		if (!tmp) {
		    buffer[i] = 0;
		    strcat(buffer, "$(");
		    strcat(buffer, buffer2);
		    strcat(buffer, ")");
		    i += strlen(buffer2)+3;
		} else {
		    strcat(buffer, tmp);
		    i += strlen(tmp);
		}
	    } else {
		while (*path!=0 && *path!='/') {
		    buffer2[j++] = *(path++);
		    buffer2[j] = 0;
		}
		tmp = getenv(buffer2);
		if (!tmp) {
		    strcat(buffer, "$");
		    strcat(buffer, buffer2);
		    i += strlen(buffer2)+1;
		} else {
		    strcat(buffer, tmp);
		    i += strlen(tmp);
		}
	    }	    
	} else {
	    buffer[i++] = *path;
	    path++;
	}
    }
    
    return mystrdup(buffer);
}



char*
wfindfileinarray(WMPropList *paths, char *file)
{
    int i;
    char *path;
    int len, flen;
    char *fullpath;

    if (!file)
	return NULL;
    
    if (*file=='/' || *file=='~' || !paths || !WMIsPLArray(paths) 
	|| WMGetPropListItemCount(paths)==0) {
	if (access(file, R_OK)<0) {
	    fullpath = wexpandpath(file);
	    if (!fullpath)
		return NULL;
	    
	    if (access(fullpath, R_OK)<0) {
		free(fullpath);
		return NULL;
	    } else {
		return fullpath;
	    }
	} else {
	    return mystrdup(file);
	}
    }

    flen = strlen(file);
    for (i=0; i < WMGetPropListItemCount(paths); i++) {
	WMPropList *tmp;
	char *dir;

	tmp = WMGetFromPLArray(paths, i);
	if (!WMIsPLString(tmp) || !(dir = WMGetFromPLString(tmp)))
	    continue;

	len = strlen(dir);
	path = mymalloc(len+flen+2);
	path = memcpy(path, dir, len);
	path[len]=0;
	strcat(path, "/");
	strcat(path, file);
	/* expand tilde */
	fullpath = wexpandpath(path);
	free(path);
	if (fullpath) {
	    /* check if file is readable */
	    if (access(fullpath, R_OK)==0) {
		return fullpath;
	    }
	    free(fullpath);
	}
    }
    return NULL;
}




void
copyFile(char *dir, char *file)
{
    char buffer[4000];
    
    sprintf(buffer, "/bin/cp \"%s\" \"%s\"", file, dir);
    if (system(buffer)!=0) {
	printf("%s: could not copy file %s\n", ProgName, file);
    }
}


void
findCopyFile(char *dir, char *file)
{
    char *fullPath;

    fullPath = wfindfileinarray(PixmapPath, file);
    if (!fullPath) {
	char buffer[4000];

	sprintf(buffer, "coould not find file %s", file);
	abortar(buffer);
    }
    copyFile(dir, fullPath);
    free(fullPath);
}


char*
makeThemePack(WMPropList *style, char *themeName)
{
    WMPropList *keys;
    WMPropList *key;
    WMPropList *value;
    int i;
    char *themeDir;

    themeDir = mymalloc(strlen(themeName)+50);
    sprintf(themeDir, "%s.themed", themeName);
    ThemePath = themeDir;
    {
	char *tmp;
	
	tmp = mymalloc(strlen(themeDir)+20);
	sprintf(tmp, "/bin/mkdir \"%s\"", themeDir);
	if (system(tmp)!=0) {
	    printf("%s: could not create directory %s. Probably there's already a theme with that name in this directory.\n", ProgName, themeDir);
	    exit(1);
	}
	free(tmp);
    }
    keys = WMGetPLDictionaryKeys(style);

    for (i = 0; i < WMGetPropListItemCount(keys); i++) {
	key = WMGetFromPLArray(keys, i);

	value = WMGetFromPLDictionary(style, key);
	if (value && WMIsPLArray(value) && WMGetPropListItemCount(value) > 2) {
	    WMPropList *type;
	    char *t;

	    type = WMGetFromPLArray(value, 0);
	    t = WMGetFromPLString(type);
	    if (t == NULL)
		continue;
	    
	    if (strcasecmp(t, "tpixmap")==0
		|| strcasecmp(t, "spixmap")==0
		|| strcasecmp(t, "cpixmap")==0
		|| strcasecmp(t, "mpixmap")==0
		|| strcasecmp(t, "tdgradient")==0
		|| strcasecmp(t, "tvgradient")==0
		|| strcasecmp(t, "thgradient")==0) {
		WMPropList *file;
		char *p;
		char *newPath;

		file = WMGetFromPLArray(value, 1);

		p = strrchr(WMGetFromPLString(file), '/');
		if (p) {
		    copyFile(themeDir, WMGetFromPLString(file));

		    newPath = mystrdup(p+1);
		    WMDeleteFromPLArray(value, 1);
		    WMInsertInPLArray(value, 1, WMCreatePLString(newPath));
		    free(newPath);
		} else {
		    findCopyFile(themeDir, WMGetFromPLString(file));
		}
	    } else if (strcasecmp(t, "bitmap")==0) {
		WMPropList *file;
		char *p;
		char *newPath;

		file = WMGetFromPLArray(value, 1);

		p = strrchr(WMGetFromPLString(file), '/');
		if (p) {
		    copyFile(themeDir, WMGetFromPLString(file));

		    newPath = mystrdup(p+1);
		    WMDeleteFromPLArray(value, 1);
		    WMInsertInPLArray(value, 1, WMCreatePLString(newPath));
		    free(newPath);
		} else {
		    findCopyFile(themeDir, WMGetFromPLString(file));
		}
		
		
		file = WMGetFromPLArray(value, 2);

		p = strrchr(WMGetFromPLString(file), '/');
		if (p) {
		    copyFile(themeDir, WMGetFromPLString(file));

		    newPath = mystrdup(p+1);
		    WMDeleteFromPLArray(value, 2);
		    WMInsertInPLArray(value, 2, WMCreatePLString(newPath));
		    free(newPath);
		} else {
		    findCopyFile(themeDir, WMGetFromPLString(file));
		}
	    }
	}
    }

    return themeDir;
}


int 
main(int argc, char **argv)
{
    WMPropList *prop, *style, *key, *val;
    char *path;
    int i, theme_too=0;
    int make_pack = 0;
    char *style_file = NULL;


    ProgName = argv[0];

    if (argc>1) {
	for (i=1; i<argc; i++) {
	    if (strcmp(argv[i], "-p")==0
		|| strcmp(argv[i], "--pack")==0) {
		make_pack = 1;
		theme_too = 1;
	    } else if (strcmp(argv[i], "-t")==0
		       || strcmp(argv[i], "--theme-options")==0) {
                theme_too++;
	    } else if (strcmp(argv[i], "--help")==0) {
		print_help();
		exit(0);
	    } else if (strcmp(argv[i], "--version")==0) {
		puts(PROG_VERSION);
		exit(0);
	    } else {
		if (style_file!=NULL) {
		    printf("%s: invalid argument '%s'\n", argv[0], 
			   style_file[0]=='-' ? style_file : argv[i]);
		    printf("Try '%s --help' for more information\n", argv[0]);
		    exit(1);
		}
                style_file = argv[i];
            }
        }
    }
    
    if (make_pack && !style_file) {
	printf("%s: you must supply a name for the theme pack\n", ProgName);
	exit(1);
    }

    WMPLSetCaseSensitive(False);

    path = defaultsPathForDomain("WindowMaker");

    prop = WMReadPropListFromFile(path);
    if (!prop) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    /* get global value */
    path = globalDefaultsPathForDomain("WindowMaker");
    val = WMReadPropListFromFile(path);
    if (val) {
	WMMergePLDictionaries(val, prop);
	WMReleasePropList(prop);
	prop = val;
    }

    style = WMCreatePLDictionary(NULL, NULL, NULL);


    for (i=0; options[i]!=NULL; i++) {
        key = WMCreatePLString(options[i]);

        val = WMGetFromPLDictionary(prop, key);
        if (val)
            WMPutInPLDictionary(style, key, val);
    }

    val = WMGetFromPLDictionary(prop, WMCreatePLString("PixmapPath"));
    if (val)
    	PixmapPath = val;

    if (theme_too) {
        for (i=0; theme_options[i]!=NULL; i++) {
            key = WMCreatePLString(theme_options[i]);

            val = WMGetFromPLDictionary(prop, key);
            if (val)
                WMPutInPLDictionary(style, key, val);
        }
    }

    if (make_pack) {
	char *path;

	makeThemePack(style, style_file);

	path = mymalloc(strlen(ThemePath)+32);
	strcpy(path, ThemePath);
	strcat(path, "/style");
        WMWritePropListToFile(style, path, False);
        wfree(path);
    } else {
	if (style_file) {
	    WMWritePropListToFile(style, style_file, False);
	} else {
	    puts(WMGetPropListDescription(style, True));
	}
    }
    exit(0);
}


