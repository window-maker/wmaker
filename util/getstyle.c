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
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX  1024
#endif

#include "../src/wconfig.h"

/* table of style related options */
static char *options[] = {
    "TitleJustify",	
    "WindowTitleFont",	
    "MenuTitleFont",	
    "MenuTextFont",	
    "IconTitleFont",	
    "DisplayFont",	
    "HighlightColor",	
    "HighlightTextColor",
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
    "IconTitleColor",
    "IconTitleBack",
    NULL
};


/* table of theme related options */
static char *theme_options[] = {
    "WorkspaceBack",
    NULL
};




char *ProgName;

char *PixmapPath = NULL;

char *ThemePath = NULL;


void
print_help()
{
    printf("usage: %s [-options] [<style file>]\n", ProgName);
    puts("options:");
    puts(" -h	print help");
    puts(" -t	get theme options too");
    puts(" -p	produce a theme pack");
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
abortar(char *reason)
{
    char buffer[4000];

    printf("%s: %s\n", ProgName, reason);

    if (ThemePath) {
	printf("Removing unfinished theme pack\n");
	sprintf(buffer, "/bin/rm -fr %s", ThemePath);
	
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
wmalloc(int size)
{
    void *tmp;
    
    tmp = malloc(size);
    if (!tmp) {
	abortar("out of memory");
    }
    
    return tmp;
}


char*
wstrdup(char *str)
{
    char *tmp;
    
    tmp = wmalloc(strlen(str)+1);

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
    
    return wstrdup(buffer);
}



char*
wfindfileinarray(proplist_t paths, char *file)
{
    int i;
    char *path;
    int len, flen;
    char *fullpath;

    if (!file)
	return NULL;
    
    if (*file=='/' || *file=='~' || !paths || !PLIsArray(paths) 
	|| PLGetNumberOfElements(paths)==0) {
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
	    return wstrdup(file);
	}
    }

    flen = strlen(file);
    for (i=0; i < PLGetNumberOfElements(paths); i++) {
	proplist_t tmp;
	char *dir;

	tmp = PLGetArrayElement(paths, i);
	if (!PLIsString(tmp) || !(dir = PLGetString(tmp)))
	    continue;

	len = strlen(dir);
	path = wmalloc(len+flen+2);
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
    
    sprintf(buffer, "/bin/cp %s %s", file, dir);
    if (system(buffer)!=0) {
	printf("%s: could not copy file %s\n", ProgName, file);
    }
}


void
findCopyFile(char *dir, char *file)
{
    char *fullPath;

    fullPath = wfindfileinarray(PixmapPath, file);
    copyFile(dir, fullPath);
    free(fullPath);
}


char*
makeThemePack(proplist_t style, char *themeName)
{
    proplist_t keys;
    proplist_t key;
    proplist_t value;
    int i;
    char *themeDir;

    themeDir = wmalloc(strlen(themeName)+50);
    sprintf(themeDir, "%s.themed", themeName);
    ThemePath = themeDir;
    {
	char *tmp;
	
	tmp = wmalloc(strlen(themeDir)+20);
	sprintf(tmp, "/bin/mkdir %s", themeDir);
	if (system(tmp)!=0) {
	    printf("%s: could not create directory %s\n", ProgName, themeDir);
	    exit(1);
	}
	free(tmp);
    }
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
		      || strcasecmp(t, "cpixmap")==0
		      || strcasecmp(t, "tdgradient")==0
		      || strcasecmp(t, "tvgradient")==0
		      || strcasecmp(t, "thgradient")==0)) {
		proplist_t file;
		char *p;
		char *newPath;

		file = PLGetArrayElement(value, 1);

		p = strrchr(PLGetString(file), '/');
		if (p) {
		    copyFile(themeDir, PLGetString(file));

		    newPath = wstrdup(p+1);
		    PLRemoveArrayElement(value, 1);
		    PLInsertArrayElement(value, PLMakeString(newPath), 1);
		    free(newPath);
		} else {
		    findCopyFile(themeDir, PLGetString(file));
		}
	    }
	}
    }

    return themeDir;
}


int 
main(int argc, char **argv)
{
    proplist_t prop, style, key, val;
    char *path;
    int i, theme_too=0;
    int make_pack = 0;
    char *style_file = NULL;


    ProgName = argv[0];

    if (argc>1) {
	for (i=1; i<argc; i++) {
	    if (strcmp(argv[i], "-p")==0) {
		make_pack = 1;
		theme_too = 1;
	    } else if (strcmp(argv[i], "-t")==0) {
                theme_too++;
            } else if (argv[i][0] != '-') {
                style_file = argv[i];
            } else {
                print_help();
                exit(1);
            }
        }
    }
    
    if (make_pack && !style_file) {
	printf("%s: you must supply a name for the theme pack\n", ProgName);
	exit(1);
    }

    PLSetStringCmpHook(StringCompareHook);

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

    val = PLGetDictionaryEntry(prop, PLMakeString("PixmapPath"));
    PixmapPath = PLGetString(val);

    if (theme_too) {
        for (i=0; theme_options[i]!=NULL; i++) {
            key = PLMakeString(theme_options[i]);

            val = PLGetDictionaryEntry(prop, key);
            if (val)
                PLInsertDictionaryEntry(style, key, val);
        }	
    }

    if (make_pack) {
	char *path;
	char *themeDir;

	makeThemePack(style, style_file);

	path = wmalloc(strlen(ThemePath)+32);
	strcpy(path, ThemePath);
	strcat(path, "/style");
	PLSetFilename(style, PLMakeString(path));
	PLSave(style, NO);
    } else {
	if (style_file) {
	    val = PLMakeString(style_file);
	    PLSetFilename(style, val);
	    PLSave(style, NO);
	} else {
	    puts(PLGetDescriptionIndent(style, 0));
	}
    }
    exit(0);
}


