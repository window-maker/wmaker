/*
 *  WindowMaker miscelaneous function library
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "../src/config.h"

#include "WUtil.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX  1024
#endif


char*
wgethomedir()
{
    char *home = getenv("HOME");
    struct passwd *user;

    if (home)
      return home;
    
    user = getpwuid(getuid());
    if (!user) {
	wsyserror("could not get password entry for UID %i", getuid());
	return "/";
    }
    if (!user->pw_dir) {
	return "/";
    } else {
	return user->pw_dir;
    }
}


static char*
getuserhomedir(char *username)
{
    struct passwd *user;
    
    user = getpwnam(username);
    if (!user) {
	wsyserror("could not get password entry for user %s", username);
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



/*
 *----------------------------------------------------------------------
 * findfile--
 * 	Finds a file in a : separated list of paths. ~ expansion is also 
 * done.
 * 
 * Returns:
 * 	The complete path for the file (in a newly allocated string) or
 * NULL if the file was not found.
 * 
 * Side effects:
 * 	A new string is allocated. It must be freed later.
 * 
 *---------------------------------------------------------------------- 
 */
char*
wfindfile(char *paths, char *file)
{
    char *path;
    char *tmp;
    int done;
    int len, flen;
    char *fullpath;

    if (!file)
	return NULL;
    
    if (*file=='/' || *file=='~' || *file=='$' || !paths) {
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
    tmp = paths;
    done = 0;
    while (!done) {
	len = strcspn(tmp, ":");
	if (len==0) done=1;
	path = wmalloc(len+flen+2);
	path = memcpy(path, tmp, len);
	path[len]=0;
	strcat(path, "/");
	strcat(path, file);
	fullpath = wexpandpath(path);
	free(path);
	if (fullpath) {
	    if (access(fullpath, R_OK)==0) {
		return fullpath;
	    }
	    free(fullpath);
	}
	tmp=&(tmp[len+1]);
	if (*tmp==0) break;
    }
    return NULL;
}


char*
wfindfileinlist(char **path_list, char *file)
{
    int i;
    char *path;
    int len, flen;
    char *fullpath;

    if (!file)
	return NULL;
    
    if (*file=='/' || *file=='~' || !path_list) {
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
    for (i=0; path_list[i]!=NULL; i++) {
	len = strlen(path_list[i]);
	path = wmalloc(len+flen+2);
	path = memcpy(path, path_list[i], len);
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



