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
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef False
# define False 0
#endif
#ifndef True
# define True 1
#endif

extern void wAbort(int);

static int Aborting=0; /* if we're in the middle of an emergency exit */


static WMHashTable *table = NULL;


void *wmalloc(size_t size)
{
    void *tmp;
    
    tmp = malloc(size);
    if (tmp == NULL) {
	wwarning("malloc() failed. Retrying after 2s.");
	sleep(2);
	tmp = malloc(size);
	if (tmp == NULL) {
	    if (Aborting) {
		puts("Real Bad Error: recursive malloc() failure.");
		exit(-1);
	    } else {
		wfatal("virtual memory exhausted");
		Aborting=1;
		wAbort(False);
	    }
	}
    }
    return tmp;
}


void *wrealloc(void *ptr, size_t newsize)
{
    void *nptr;

    if (!ptr) {
	nptr = malloc(newsize);
    } else {
	nptr=realloc(ptr, newsize);
    }
    if (nptr==NULL) {
	printf("Could not do realloc");
	return NULL;
    }
    return nptr;
}


void*
wretain(void *ptr)
{
    int *refcount;
    
    if (!table) {
	table = WMCreateHashTable(WMIntHashCallbacks);
    }
        
    refcount = WMHashGet(table, ptr);
    if (!refcount) {
	refcount = wmalloc(sizeof(int));
	*refcount = 1;
	WMHashInsert(table, ptr, refcount);
#ifdef VERBOSE
	printf("== %i (%p)\n", *refcount, ptr);
#endif
    } else {
	(*refcount)++;
#ifdef VERBOSE
	printf("+ %i (%p)\n", *refcount, ptr);
#endif
    }
    
    return ptr;
}


void
wrelease(void *ptr)
{
    int *refcount;
    
    refcount = WMHashGet(table, ptr);
    if (!refcount) {
	wwarning("trying to release unexisting data %p", ptr);
    } else {
	(*refcount)--;
	if (*refcount < 1) {
#ifdef VERBOSE
	    printf("RELEASING %p\n", ptr);
#endif
	    WMHashRemove(table, ptr);
	    free(refcount);
	    free(ptr);
	}
#ifdef VERBOSE
	else {
	    printf("- %i (%p)\n", *refcount, ptr);
	}
#endif
    }
}


char*
wstrdup(char *str)
{
    assert(str!=NULL);

    return strcpy(wmalloc(strlen(str)+1), str);
}


char*
wstrappend(char *dst, char *src)
{
    char *str;
    
    if (!dst)
	return wstrdup(src);
    else if (!src)
	return wstrdup(dst);
    
    str = wmalloc(strlen(dst)+strlen(src)+1);
    strcpy(str, dst);
    strcat(str, src);

    return str;
}

