/* plugin.h- plugin
 *
 *  Window Maker window manager
 *
 *  Copyright (c) hmmm... Should I put everybody's name here?
 *  Where's my lawyer?? -- ]d :D
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
 * 
 * * * * * * * * *
 * Do you think I should move this code into another file? -- ]d
 *
 * BTW, should this file be able to be included by any plugin file that
 * want _DL ?
 */


#ifndef WMPLUGIN_H
#define WMPLUGIN_H

#include "wconfig.h"

#include <WINGs.h>
#include <proplist.h>

#define W_FUNCTION_ANY 0
#define W_FUNCTION_DRAWSTRING 1

typedef struct _WPluginData {
    int size;
    void **array;
} WPluginData;

typedef void (*_DL_AnyProc)(proplist_t);

/* first 3 must == WS_FOCUSED WS_UNFOCUSED WS_PFOCUSED -- ]d */
#ifdef DRAWSTRING_PLUGIN
#define W_STRING_FTITLE 0
#define W_STRING_UTITLE 1
#define W_STRING_PTITLE 2
#define W_STRING_MTITLE 3
#define W_STRING_MTEXT  4
#define W_STRING_MEMBERS 5

typedef void _DL_DrawStringProc(proplist_t, Drawable, int, int, char*, int, WPluginData*);
typedef void _DL_WidthStringProc(char*, int, WPluginData*, int*, int*, int*);
#endif

typedef void _DL_FreeDataProc(proplist_t pl, WPluginData *free_data);

typedef int _DL_InitDataProc(proplist_t pl, WPluginData *init_data);
                                        /* prototype for function initializer */

#define W_DSPROC_DRAWSTRING 0
#define W_DSPROC_WIDTHOFSTRING 1
#define W_DSPROC_MEMBERS 2

typedef struct _WFunction {
    int type;
    void *handle;
    proplist_t arg;
    WPluginData *data;
    _DL_FreeDataProc *freeData;
    union {
        _DL_AnyProc **any;
#ifdef DRAWSTRING_PLUGIN
        _DL_DrawStringProc **drawString;
        _DL_WidthStringProc **widthOfString;
#endif
    } proc;
    /*
    char *libraryName;
    char *procName;
    char *freeDataProcName;
    */
} WFunction;

/* for init_data, pass something like 
 * p = wmalloc(sizeof(void *) * 3)
 * and let p[0]=display p[1]=colormap p[2]=cache (for keeping local data
 * for each instance of function in each WFunction) to the initializing
 * code for drawstring function... may be I can change this to a variable
 * packer function? or use va_list? I dunno...
 *
 * --]d
 */

WFunction* wPluginCreateFunction(int type, char *library_name,
        char *init_proc_name, WPluginData *funcs, char *free_data_proc_name,
        proplist_t pl_arg, WPluginData *init_data);

void wPluginDestroyFunction(WFunction *function);

WPluginData* wPluginPackData(int members, ...);

#endif
