/* usermenu.c- user defined menu
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
 * User defined menu is good, but beer's always better
 * if someone wanna start hacking something, He heard...
 * TODO
 *  - enhance commands. (eg, exit, hide, list all app's member
 *    window and etc)
 *  - cache menu... dunno.. if people really use this feature :P
 *  - Violins, senseless violins!
 *  that's all, right now :P
 *  - external! WINGs menu editor.
 *  TODONOT
 *  - allow applications to share their menu. ] think it
 *    looks wierd since there still are more than 1 appicon.
 *  
 *  Syntax...
 *  (
 *    "Program Name",
 *    ("Command 1", SHORTCUT, 1),
 *    ("Command 2", SHORTCUT, 2, ("Allowed_instant_1", "Allowed_instant_2")),
 *    ("Command 3", SHORTCUT, (3,4,5), ("Allowed_instant_1")),
 *    (
 *      "Submenu",
 *      ("Kill Command", KILL),
 *      ("Hide Command", HIDE),
 *      ("Hide Others Command", HIDE_OTHERS),
 *      ("Members", MEMBERS),
 *      ("Exit Command", EXIT)
 *    )
 *  )
 *  
 *  Tips:
 *  - If you don't want short cut keys to be listed
 *    in the right side of entries, you can just put them
 *    in array instead of using the string directly.
 *  
 */

#include "wconfig.h"

#ifdef USER_MENU

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "menu.h"
#include "actions.h"
#include "funcs.h"
#include "keybind.h"

#include "framewin.h"


extern proplist_t ReadProplistFromFile(char *file);
/*** var ***/
extern WPreferences wPreferences;

typedef struct {
    WScreen *screen;
    WShortKey *key;
    int key_no;
} WUserMenuData;


static void
notifyClient(WMenu *menu, WMenuEntry *entry)
{
    XEvent event;
    WUserMenuData *data = entry->clientdata;
    WScreen *scr = data->screen;
    Window window;
    int i;

    window=scr->focused_window->client_win;

    for(i=0;i<data->key_no;i++) {
        event.xkey.type = KeyPress;
        event.xkey.display = dpy;
        event.xkey.window = window;
        event.xkey.root = DefaultRootWindow(dpy);
        event.xkey.subwindow = (Window)None;
        event.xkey.x = 0x0;
        event.xkey.y = 0x0;
        event.xkey.x_root = 0x0;
        event.xkey.y_root = 0x0;
        event.xkey.keycode = data->key[i].keycode;
        event.xkey.state = data->key[i].modifier;
        event.xkey.same_screen = True;
        event.xkey.time = CurrentTime;
        if (XSendEvent(dpy, window, False, KeyPressMask, &event)) {
            event.xkey.type = KeyRelease;
            event.xkey.time = CurrentTime;
            XSendEvent(dpy, window, True, KeyReleaseMask, &event);
        }
    }
}

static void
removeUserMenudata(void *menudata)
{
    WUserMenuData *data = menudata;
    if(data->key) free(data->key);
    free(data);
}


static WUserMenuData*
convertShortcuts(WScreen *scr, proplist_t shortcut)
{
    WUserMenuData *data;
    KeySym ksym;
    char *k;
    char *buffer;
    char buf[128], *b;
    int keycount,i,j,mod;

    if (PLIsString(shortcut)) {
        keycount = 1;
    }
    else if (PLIsArray(shortcut)) {
        keycount = PLGetNumberOfElements(shortcut);
    }
    else return NULL;
    /*for (i=0;i<keycount;i++){*/

    data = wmalloc(sizeof(WUserMenuData));
    if (!data) return NULL;
    data->key = wmalloc(sizeof(WShortKey)*keycount);
    if (!data->key) {
        free(data);
        return NULL;
    }


    for (i=0,j=0;i<keycount;i++) {
        data->key[j].modifier = 0;
        if (PLIsArray(shortcut)) {
            strcpy(buf, PLGetString(PLGetArrayElement(shortcut, i)));
        } else {
            strcpy(buf, PLGetString(shortcut));
        }
        b = (char*)buf;

        while ((k = strchr(b, '+'))!=NULL) {
            *k = 0;
            mod = wXModifierFromKey(b);
            if (mod<0) {
                break;
            }
            data->key[j].modifier |= mod;
            b = k+1;
        }

        ksym = XStringToKeysym(b);
        if (ksym==NoSymbol) {
            continue;
        }

        data->key[j].keycode = XKeysymToKeycode(dpy, ksym);
        if (data->key[j].keycode) {
            j++;
        }
    }

keyover:
    
    /* get key */
    if (!j) {
        puts("fatal j");
        free(data->key);
        free(data);
        return NULL;
    }
    data->key_no = j;
    data->screen = scr;
    
    return data;
}

static WMenu*
configureUserMenu(WScreen *scr, proplist_t plum)
{
    char *mtitle;
    WMenu *menu=NULL;
    proplist_t elem, title, command, params;
    int count,i;
    WUserMenuData *data;

    if (!plum) return NULL;
    if (!PLIsArray(plum)) {
        return NULL;
    }

    count = PLGetNumberOfElements(plum);
    if (!count) return NULL;

    elem = PLGetArrayElement(plum, 0);
    if (!PLIsString(elem)) {
        return NULL;
    }
    
    mtitle = PLGetString(elem);
    
    menu=wMenuCreateForApp(scr, mtitle, True);
    
    for(i=1; i<count; i++) {
        elem = PLGetArrayElement(plum,i);
        if(PLIsArray(PLGetArrayElement(elem,1))) {
            WMenu *submenu;
            WMenuEntry *mentry;
            
            submenu = configureUserMenu(scr,elem);
            if (submenu)
                mentry = wMenuAddCallback(menu, submenu->frame->title,
                        NULL, NULL);
            wMenuEntrySetCascade(menu, mentry, submenu);
        }
        else {
            int idx = 0;
            proplist_t instances=0;

            title = PLGetArrayElement(elem,idx++);
            command = PLGetArrayElement(elem,idx++);
            if (PLGetNumberOfElements(elem) >= 3)
                params = PLGetArrayElement(elem,idx++);
            
            if (!title || !command)
                return menu;

            if (!strcmp("SHORTCUT",PLGetString(command))) {
                WMenuEntry *entry;

                data = convertShortcuts(scr, params);
                if (data){
                    entry = wMenuAddCallback(menu, PLGetString(title),
                                    notifyClient, data);

                    if (entry) {
                        if (PLIsString(params)) {
                            entry->rtext = GetShortcutString(PLGetString(params));
                        }
                        entry->free_cdata = removeUserMenudata;

                        if (PLGetNumberOfElements(elem) >= 4) {
                            instances = PLGetArrayElement(elem,idx++);
                            if(PLIsArray(instances))
				if (instances && PLGetNumberOfElements(instances)
				    && PLIsArray(instances)){
				    entry->instances = PLRetain(instances);
                            }
                        }
                    }
                }
            }


        }
    }
    return menu;
}

void
wUserMenuRefreshInstances(WMenu *menu, WWindow *wwin)
{
    WMenuEntry* entry;
    int i,j,count,paintflag;
    
    paintflag=0;
    
    if(!menu) return;

    for (i=0; i<menu->entry_no; i++) {
        if (menu->entries[i]->instances){
            proplist_t ins;
            int oldflag;
            count = PLGetNumberOfElements(menu->entries[i]->instances);

            oldflag = menu->entries[i]->flags.enabled;
            menu->entries[i]->flags.enabled = 0;
            for (j=0; j<count;j++) {
                ins = PLGetArrayElement(menu->entries[i]->instances,j);
                if (!strcmp(wwin->wm_instance,PLGetString(ins))) {
                    menu->entries[i]->flags.enabled = 1;
                    break;
                }
            }
            if (oldflag != menu->entries[i]->flags.enabled)
                paintflag=1;
        }
    }
    for (i=0; i < menu->cascade_no; i++) {
        if (!menu->cascades[i]->flags.brother)
	    wUserMenuRefreshInstances(menu->cascades[i], wwin);
        else
	    wUserMenuRefreshInstances(menu->cascades[i]->brother, wwin);
    }

    if (paintflag)
        wMenuPaint(menu);
}


static WMenu*
readUserMenuFile(WScreen *scr, char *file_name)
{
    WMenu *menu;
    char *mtitle;
    proplist_t plum, elem, title, command, params;
    int count,i;

    menu=NULL;
    plum = ReadProplistFromFile(file_name);
    /**/
    
    if(plum){
        menu = configureUserMenu(scr, plum);
        PLRelease(plum);
    }
    return menu;
}


WMenu*
wUserMenuGet(WScreen *scr, WWindow *wwin)
{
    WMenu *menu = NULL;
    char buffer[100];
    char *path = NULL;
    char *tmp;
    if ( wwin->wm_instance && wwin->wm_class ) {
        tmp=wmalloc(strlen(wwin->wm_instance)+strlen(wwin->wm_class)+7);
        sprintf(tmp,"%s.%s.menu",wwin->wm_instance,wwin->wm_class);
        path = wfindfile(DEF_USER_MENU_PATHS,tmp);
        free(tmp);

        if (!path) return NULL;
        
        if (wwin) {
            menu = readUserMenuFile(scr, path);
        }

        free(path);
    }
    return menu;
}

#endif /* USER_MENU */
