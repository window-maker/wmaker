/* workspace.c- Workspace management
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
#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "icon.h"
#include "funcs.h"
#include "menu.h"
#include "application.h"
#include "dock.h"
#include "actions.h"
#include "workspace.h"
#include "appicon.h"

#include <proplist.h>


extern WPreferences wPreferences;
extern XContext wWinContext;


static proplist_t dWorkspaces=NULL;
static proplist_t dClip, dName;


static void
make_keys()
{
    if (dWorkspaces!=NULL)
	return;
    
    dWorkspaces = PLMakeString("Workspaces");
    dName = PLMakeString("Name");
    dClip = PLMakeString("Clip");
}


void
wWorkspaceMake(WScreen *scr, int count)
{
    while (count>0) {
	wWorkspaceNew(scr);
	count--;
    }
}


int
wWorkspaceNew(WScreen *scr)
{
    WWorkspace *wspace, **list;
    int i;
    
    if (scr->workspace_count < MAX_WORKSPACES) {
	scr->workspace_count++;

	wspace = wmalloc(sizeof(WWorkspace));

        wspace->name = wmalloc(strlen(_("Workspace %i"))+8);
        sprintf(wspace->name, _("Workspace %i"), scr->workspace_count);
        if (!wPreferences.flags.noclip) {
            wspace->clip = wDockCreate(scr, WM_CLIP);
	} else
            wspace->clip = NULL;

	
	list = wmalloc(sizeof(WWorkspace*)*scr->workspace_count);
	
	for (i=0; i<scr->workspace_count-1; i++) {
	    list[i] = scr->workspaces[i];
	}
	list[i] = wspace;
	free(scr->workspaces);
	scr->workspaces = list;

	wWorkspaceMenuUpdate(scr, scr->workspace_menu);
	wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);
	
	return scr->workspace_count-1;
    }
    return -1;
}



void
wWorkspaceDelete(WScreen *scr, int workspace)
{
    WWindow *tmp;
    WWorkspace **list;
    int i, j;

    if (workspace<=0)
	return;

    /* verify if workspace is in use by some window */
    tmp = scr->focused_window;
    while (tmp) {
	if (tmp->frame->workspace==workspace)
	    return;
	tmp = tmp->prev;
    }
    
    if (!wPreferences.flags.noclip) {
        wDockDestroy(scr->workspaces[workspace]->clip);
        scr->workspaces[workspace]->clip = NULL;
    }
    
    list = wmalloc(sizeof(WWorkspace*)*(scr->workspace_count-1));
    j = 0;
    for (i=0; i<scr->workspace_count; i++) {
	if (i!=workspace)
	    list[j++] = scr->workspaces[i];
	else {
	    if (scr->workspaces[i]->name)
		free(scr->workspaces[i]->name);
	    free(scr->workspaces[i]);
	}
    }
    free(scr->workspaces);
    scr->workspaces = list;

    scr->workspace_count--;


    /* update menu */
    wWorkspaceMenuUpdate(scr, scr->workspace_menu);
    /* clip workspace menu */
    wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);

    /* update also window menu */
    if (scr->workspace_submenu) {
	WMenu *menu = scr->workspace_submenu;

        i = menu->entry_no;
        while (i>scr->workspace_count)
            wMenuRemoveItem(menu, --i);
	wMenuRealize(menu);
    }
    /* and clip menu */
    if (scr->clip_submenu) {
        WMenu *menu = scr->clip_submenu;

        i = menu->entry_no;
        while (i>scr->workspace_count)
            wMenuRemoveItem(menu, --i);
	wMenuRealize(menu);
    }
}


void
wWorkspaceChange(WScreen *scr, int workspace)
{
    if (workspace != scr->current_workspace) {
        wWorkspaceForceChange(scr, workspace);
    }
}

void
wWorkspaceForceChange(WScreen *scr, int workspace)
{
    WWindow *tmp, *foc=NULL;
#ifdef EXPERIMENTAL
    WWorkspaceTexture *wsback;
#endif
    
    if (workspace >= MAX_WORKSPACES || workspace < 0)
	return;

#ifdef EXPERIMENTAL
    /* update the background for the workspace */
    if (workspace < scr->wspaceTextureCount
	&& scr->wspaceTextures[workspace]) {
	wsback = scr->wspaceTextures[workspace];
    } else {
	/* check if the previous workspace used the default background */
	if (scr->current_workspace < 0
	    || scr->current_workspace >= scr->wspaceTextureCount
	    || scr->wspaceTextures[scr->current_workspace]==NULL) {
	    wsback = NULL;
	} else {
	    wsback = scr->defaultTexure;
	}
    }
    if (wsback) {
	if (wsback->pixmap!=None) {
	    XSetWindowBackgroundPixmap(dpy, scr->root_win, wsback->pixmap);
	} else {
	    XSetWindowBackground(dpy, scr->root_win, wsback->solid);
	}
	XClearWindow(dpy, scr->root_win);
	XFlush(dpy);
    }
#endif /* EXPERIMENTAL */

    if (workspace > scr->workspace_count-1) {
	wWorkspaceMake(scr, workspace - scr->workspace_count + 1);
    }

    wClipUpdateForWorkspaceChange(scr, workspace);

    scr->current_workspace = workspace;

    wWorkspaceMenuUpdate(scr, scr->workspace_menu);

    wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);

    if ((tmp = scr->focused_window)!= NULL) {	
	while (tmp) {
	    if (tmp->frame->workspace!=workspace && !tmp->flags.selected) {
		/* unmap windows not on this workspace */
		if ((tmp->flags.mapped||tmp->flags.shaded)
		    && !tmp->window_flags.omnipresent
		    && !tmp->flags.changing_workspace) {
		    XUnmapWindow(dpy, tmp->frame->core->window);
		    tmp->flags.mapped = 0;
		}
                /* also unmap miniwindows not on this workspace */
                if (tmp->flags.miniaturized &&
                    !tmp->window_flags.omnipresent && tmp->icon) {
                    if (!wPreferences.sticky_icons) {
                        XUnmapWindow(dpy, tmp->icon->core->window);
			tmp->icon->mapped = 0;
		    } else {
			tmp->icon->mapped = 1;
			/* Why is this here? -Alfredo */
                        XMapWindow(dpy, tmp->icon->core->window);
		    }
                }
		/* update current workspace of omnipresent windows */
		if (tmp->window_flags.omnipresent) {		    
		    WApplication *wapp = wApplicationOf(tmp->main_window);

		    tmp->frame->workspace = workspace;

		    if (wapp) {
			wapp->last_workspace = workspace;
		    }
		}
            } else {
		/* change selected windows' workspace */
		if (tmp->flags.selected) {
		    wWindowChangeWorkspace(tmp, workspace);
                    if (!tmp->flags.miniaturized) {
                        foc = tmp;
                    }
                } else {
		    if (!tmp->flags.hidden) {
			if (!(tmp->flags.mapped || tmp->flags.miniaturized)) {
			    /* remap windows that are on this workspace */
			    XMapWindow(dpy, tmp->frame->core->window);
			    if (!foc)
				foc = tmp;
			    if (!tmp->flags.shaded)
				tmp->flags.mapped = 1;
			}
			/* Also map miniwindow if not omnipresent */
			if (!wPreferences.sticky_icons &&
			    tmp->flags.miniaturized &&
			    !tmp->window_flags.omnipresent && tmp->icon) {
			    tmp->icon->mapped = 1;
			    XMapWindow(dpy, tmp->icon->core->window);
			}
		    }
                }
	    }
	    tmp = tmp->prev;
	}

	if (scr->focused_window->flags.mapped) {
	    foc = scr->focused_window;
	}
	if (wPreferences.focus_mode == WKF_CLICK) {
	    wSetFocusTo(scr, foc);
	} else {
            unsigned int mask;
            int foo;
            Window bar, win;
	    WWindow *tmp;

	    tmp = NULL;
            if (XQueryPointer(dpy, scr->root_win, &bar, &win,
                              &foo, &foo, &foo, &foo, &mask)) {
		tmp = wWindowFor(win);
	    }
	    if (!tmp && wPreferences.focus_mode == WKF_SLOPPY) {
		wSetFocusTo(scr, foc);
	    } else {
		wSetFocusTo(scr, tmp);
	    }
	}
    }

    /* We need to always arrange icons when changing workspace, even if
     * no autoarrange icons, because else the icons in different workspaces
     * can be superposed.
     * This can be avoided if appicons are also workspace specific.
     */
    if (!wPreferences.sticky_icons)
        wArrangeIcons(scr, False);

    if (scr->dock)
        wAppIconPaint(scr->dock->icon_array[0]);
    if (scr->clip_icon) {
        if (scr->workspaces[workspace]->clip->auto_collapse) {
            /* to handle enter notify. This will also */
            XUnmapWindow(dpy, scr->clip_icon->icon->core->window);
            XMapWindow(dpy, scr->clip_icon->icon->core->window);
        } else {
            wClipIconPaint(scr->clip_icon);
        }
    }
}


static void
switchWSCommand(WMenu *menu, WMenuEntry *entry)
{
    wWorkspaceChange(menu->frame->screen_ptr, (int)entry->clientdata);
}



static void
deleteWSCommand(WMenu *menu, WMenuEntry *entry)
{
    wWorkspaceDelete(menu->frame->screen_ptr, 
		     menu->frame->screen_ptr->workspace_count-1);
}



static void
newWSCommand(WMenu *menu, WMenuEntry *foo)
{
    int ws;

    ws = wWorkspaceNew(menu->frame->screen_ptr);
    /* autochange workspace*/
    if (ws>=0)
	wWorkspaceChange(menu->frame->screen_ptr, ws);
    
    
    /*
    if (ws<9) {
	int kcode;
	if (wKeyBindings[WKBD_WORKSPACE1+ws]) {
	    kcode = wKeyBindings[WKBD_WORKSPACE1+ws]->keycode;
	    entry->rtext = 
	      wstrdup(XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0)));
	}
    }*/
}


static char*
cropline(char *line)
{
    char *start, *end;
    
    if (strlen(line)==0)
	return line;
    
    start = line;
    end = &(line[strlen(line)])-1;
    while (isspace(*line) && *line!=0) line++;
    while (isspace(*end) && end!=line) {
	*end=0;
	end--;
    }
    return line;
}


void
wWorkspaceRename(WScreen *scr, int workspace, char *name)
{
    char buf[MAX_WORKSPACENAME_WIDTH+1];
    char *tmp;

    if (workspace >= scr->workspace_count)
	return;

    /* trim white spaces */
    tmp = cropline(name);

    if (strlen(tmp)==0) {
	sprintf(buf, _("Workspace %i"), workspace+1);
    } else {
	strncpy(buf, tmp, MAX_WORKSPACENAME_WIDTH);
    }
    buf[MAX_WORKSPACENAME_WIDTH] = 0;

    /* update workspace */
    free(scr->workspaces[workspace]->name);
    scr->workspaces[workspace]->name = wstrdup(buf);

    if (scr->clip_ws_menu) {
	if (strcmp(scr->clip_ws_menu->entries[workspace+2]->text, buf)!=0) {
	    free(scr->clip_ws_menu->entries[workspace+2]->text);
	    scr->clip_ws_menu->entries[workspace+2]->text = wstrdup(buf);
	    wMenuRealize(scr->clip_ws_menu);
	}
    }
    if (scr->workspace_menu) {
	if (strcmp(scr->workspace_menu->entries[workspace+2]->text, buf)!=0) {
	    free(scr->workspace_menu->entries[workspace+2]->text);
	    scr->workspace_menu->entries[workspace+2]->text = wstrdup(buf);
	    wMenuRealize(scr->workspace_menu);
	}
    }
    UpdateSwitchMenuWorkspace(scr, workspace);
    if (scr->clip_icon)
        wClipIconPaint(scr->clip_icon);
}




/* callback for when menu entry is edited */
static void
onMenuEntryEdited(WMenu *menu, WMenuEntry *entry)
{
    char *tmp;

    tmp = entry->text;
    wWorkspaceRename(menu->frame->screen_ptr, (int)entry->clientdata, tmp);
}


WMenu*
wWorkspaceMenuMake(WScreen *scr, Bool titled)
{
    WMenu *wsmenu;

    wsmenu = wMenuCreate(scr, titled ? _("Workspaces") : NULL, False);
    if (!wsmenu) {
	wwarning(_("could not create Workspace menu"));
	return NULL;
    }
    
    /* callback to be called when an entry is edited */
    wsmenu->on_edit = onMenuEntryEdited;
    
    wMenuAddCallback(wsmenu, _("New"), newWSCommand, NULL);
    wMenuAddCallback(wsmenu, _("Destroy Last"), deleteWSCommand, NULL);

    return wsmenu;
}



void
wWorkspaceMenuUpdate(WScreen *scr, WMenu *menu)
{
    int i, ws;
    char title[MAX_WORKSPACENAME_WIDTH+1];
    WMenuEntry *entry;
    int tmp;

    if (!menu)
	return;

    if (menu->entry_no < scr->workspace_count+2) {
	/* new workspace(s) added */
	i = scr->workspace_count-(menu->entry_no-2);
	ws = menu->entry_no - 2;
	while (i>0) {
	    strcpy(title, scr->workspaces[ws]->name);

	    entry = wMenuAddCallback(menu, title, switchWSCommand, (void*)ws);
	    entry->flags.indicator = 1;
	    entry->flags.editable = 1;

	    i--;
	    ws++;
	}
    } else if (menu->entry_no > scr->workspace_count+2) {
	/* removed workspace(s) */
	for (i = menu->entry_no-1; i >= scr->workspace_count+2; i--) {
	    wMenuRemoveItem(menu, i);
	}
    }
    wMenuRealize(menu);
    
    for (i=0; i<scr->workspace_count; i++) {	
	menu->entries[i+2]->flags.indicator_on = 0;
    }
    menu->entries[scr->current_workspace+2]->flags.indicator_on = 1;

    /* don't let user destroy current workspace */
    if (scr->current_workspace == scr->workspace_count-1) {
	wMenuSetEnabled(menu, 1, False);
    } else {
	wMenuSetEnabled(menu, 1, True);
    }

    tmp = menu->frame->top_width + 5;
    /* if menu got unreachable, bring it to a visible place */
    if (menu->frame_x < tmp - (int)menu->frame->core->width)
	wMenuMove(menu, tmp - (int)menu->frame->core->width, menu->frame_y, False);
    
    wMenuPaint(menu);
}


void
wWorkspaceSaveState(WScreen *scr, proplist_t old_state)
{
    proplist_t parr, pstr;
    proplist_t wks_state, old_wks_state;
    proplist_t foo, bar;
    int i;

    make_keys();

    old_wks_state = PLGetDictionaryEntry(old_state, dWorkspaces);
    parr = PLMakeArrayFromElements(NULL);
    for (i=0; i < scr->workspace_count; i++) {
        pstr = PLMakeString(scr->workspaces[i]->name);
        wks_state = PLMakeDictionaryFromEntries(dName, pstr, NULL);
        PLRelease(pstr);
        if (!wPreferences.flags.noclip) {
            pstr = wClipSaveWorkspaceState(scr, i);
            PLInsertDictionaryEntry(wks_state, dClip, pstr);
            PLRelease(pstr);
        } else if (old_wks_state!=NULL) {
            if ((foo = PLGetArrayElement(old_wks_state, i))!=NULL) {
                if ((bar = PLGetDictionaryEntry(foo, dClip))!=NULL) {
                    PLInsertDictionaryEntry(wks_state, dClip, bar);
                }
            }
        }
        PLAppendArrayElement(parr, wks_state);
        PLRelease(wks_state);
    }
    PLInsertDictionaryEntry(scr->session_state, dWorkspaces, parr);
    PLRelease(parr);
}


void
wWorkspaceRestoreState(WScreen *scr)
{
    proplist_t parr, pstr, wks_state;
    proplist_t clip_state;
    int i, wscount;

    make_keys();

    parr = PLGetDictionaryEntry(scr->session_state, dWorkspaces);

    if (!parr)
	return;
    
    wscount = scr->workspace_count;
    for (i=0; i < PLGetNumberOfElements(parr); i++) {
        wks_state = PLGetArrayElement(parr, i);
        if (PLIsDictionary(wks_state))
            pstr = PLGetDictionaryEntry(wks_state, dName);
        else
            pstr = wks_state;
	if (i >= scr->workspace_count)
	    wWorkspaceNew(scr);
	if (scr->workspace_menu) {
	    free(scr->workspace_menu->entries[i+2]->text);
	    scr->workspace_menu->entries[i+2]->text = wstrdup(PLGetString(pstr));
	    scr->workspace_menu->flags.realized = 0;
	}
	free(scr->workspaces[i]->name);
        scr->workspaces[i]->name = wstrdup(PLGetString(pstr));
        if (!wPreferences.flags.noclip) {
            clip_state = PLGetDictionaryEntry(wks_state, dClip);
            if (scr->workspaces[i]->clip)
                wDockDestroy(scr->workspaces[i]->clip);
            scr->workspaces[i]->clip = wDockRestoreState(scr, clip_state,
                                                          WM_CLIP);
            if (i>0)
                wDockHideIcons(scr->workspaces[i]->clip);
        }
    }
}


