/* plugin.c- plugin
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
 */

#include "plugin.h"

#ifdef TEXTURE_PLUGIN
# ifdef HAVE_DLFCN_H
#  include <dlfcn.h>
# endif
#endif

#include <proplist.h>

WFunction *
wPluginCreateFunction(int type, char *library_name,
        char *init_proc_name, char *proc_name, char *free_data_proc_name,
        proplist_t pl_arg, void *init_data) {
    WFunction *function;
    _DL_InitDataProc *initProc;

    function = wmalloc(sizeof(WFunction));
    bzero(function, sizeof(WFunction));

    function->handle = dlopen(library_name, RTLD_LAZY);
    if (!function->handle) {
        wwarning(_("library \"%s\" cound not be opened."), library_name);
        free(function);
        return NULL;
    }

    function->proc.any = dlsym(function->handle, proc_name);
    if (!function->proc.any) {
        wwarning(_("function \"%s\" not found in library \"%s\""), proc_name, library_name);
        dlclose(function->handle);
        free(function);
        return NULL;
    }

    if (free_data_proc_name) {
        function->freeData = dlsym(function->handle, free_data_proc_name);
        if (!function->freeData) {
            wwarning(_("function \"%s\" not found in library \"%s\""), free_data_proc_name, library_name);
            dlclose(function->handle);
            free(function);
            return NULL;
        }
    }

    if (pl_arg) function->arg = PLDeepCopy(pl_arg);
    function->data = init_data;
    if (init_proc_name) {
        initProc = dlsym(function->handle, init_proc_name);
        if (initProc) {
            initProc(function->arg, &function->data);
        } else {
            /* Where's my english teacher? -- ]d
            wwarning(_("?"),?);
            */
            dlclose(function->handle);
            free(function);
        }
    }

    function->type = type;
    return function;
}

void
wPluginDestroyFunction(WFunction *function) {
    if (!function) return;
    if (function->data) {
        if (function->freeData) {
            function->freeData(&function->data);
        } else {
            free(function->data);
        }
    }
    if (function->arg) PLRelease(function->arg);
    free(function);
    return;
}

/* hmmmm, need another function to pack a va_list 
 * but better move on something else for now :D */
