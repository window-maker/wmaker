/* workspace.c- Workspace management
 * 
 *  Window Maker window manager
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
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

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
#ifdef GNOME_STUFF
#include "gnome.h"
#endif
#ifdef KWM_HINTS
#include "kwm.h"
#endif

#include <proplist.h>


extern WPreferences wPreferences;
extern XContext wWinContext;


static proplist_t dWorkspaces=NULL;
static proplist_t dClip, dName;

#ifdef VIRTUAL_DESKTOP
static BOOL initVDesk = False;
#endif

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
	wspace->name = NULL;

#ifdef KWM_HINTS
	if (scr->flags.kwm_syncing_count) {
	    wspace->name = wKWMGetWorkspaceName(scr, scr->workspace_count-1);
	}
#endif
	if (!wspace->name) {
	    wspace->name = wmalloc(strlen(_("Workspace %i"))+8);
	    sprintf(wspace->name, _("Workspace %i"), scr->workspace_count);
	}


    if (!wPreferences.flags.noclip) {
        wspace->clip = wDockCreate(scr, WM_CLIP);
    } else
        wspace->clip = NULL;

	list = wmalloc(sizeof(WWorkspace*)*scr->workspace_count);

	for (i=0; i<scr->workspace_count-1; i++) {
	    list[i] = scr->workspaces[i];
	}
	list[i] = wspace;
	if (scr->workspaces)
	    wfree(scr->workspaces);
	scr->workspaces = list;

	wWorkspaceMenuUpdate(scr, scr->workspace_menu);
	wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);
#ifdef VIRTUAL_DESKTOP
    wspace->view_x = wspace->view_y = 0;
    wspace->height = scr->scr_height * 2;
    wspace->width = scr->scr_width * 2;
#endif
#ifdef GNOME_STUFF
	wGNOMEUpdateWorkspaceHints(scr);
#endif
#ifdef KWM_HINTS
	if (!scr->flags.kwm_syncing_count) {
	    wKWMUpdateWorkspaceCountHint(scr);
	    wKWMUpdateWorkspaceNameHint(scr, scr->workspace_count-1);
	}
#ifdef not_used
	wKWMSetUsableAreaHint(scr, scr->workspace_count-1);
#endif
#endif
	XFlush(dpy);

	return scr->workspace_count-1;
    }
    return -1;
}



Bool
wWorkspaceDelete(WScreen *scr, int workspace)
{
    WWindow *tmp;
    WWorkspace **list;
    int i, j;


    if (workspace<=0)
	return False;

    /* verify if workspace is in use by some window */
    tmp = scr->focused_window;
    while (tmp) {
	if (!IS_OMNIPRESENT(tmp) && tmp->frame->workspace==workspace)
	    return False;
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
		wfree(scr->workspaces[i]->name);
	    wfree(scr->workspaces[i]);
	}
    }
    wfree(scr->workspaces);
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

#ifdef GNOME_STUFF
    wGNOMEUpdateWorkspaceHints(scr);
#endif
#ifdef KWM_HINTS
    wKWMUpdateWorkspaceCountHint(scr);
#endif

    if (scr->current_workspace >= scr->workspace_count)
	wWorkspaceChange(scr, scr->workspace_count-1);

    return True;
}


typedef struct WorkspaceNameData {
    int count;
    RImage *back;
    RImage *text;
    time_t timeout;
} WorkspaceNameData;



static void
hideWorkpaceName(void *data)
{
    WScreen *scr = (WScreen*)data;

    if (!scr->workspace_name_data || scr->workspace_name_data->count == 0
	|| time(NULL) > scr->workspace_name_data->timeout) {
	XUnmapWindow(dpy, scr->workspace_name);

	if (scr->workspace_name_data) {
	    RDestroyImage(scr->workspace_name_data->back);
	    RDestroyImage(scr->workspace_name_data->text);
	    wfree(scr->workspace_name_data);

	    scr->workspace_name_data = NULL;
	}
	scr->workspace_name_timer = NULL;
    } else {
	RImage *img = RCloneImage(scr->workspace_name_data->back);
	Pixmap pix;

	scr->workspace_name_timer = 
	    WMAddTimerHandler(WORKSPACE_NAME_FADE_DELAY, hideWorkpaceName,
			      scr);

	RCombineImagesWithOpaqueness(img, scr->workspace_name_data->text,
				     scr->workspace_name_data->count*255/10);

	RConvertImage(scr->rcontext, img, &pix);

	RDestroyImage(img);

	XSetWindowBackgroundPixmap(dpy, scr->workspace_name, pix);
	XClearWindow(dpy, scr->workspace_name);
	XFreePixmap(dpy, pix);
	XFlush(dpy);

	scr->workspace_name_data->count--;
    }
}



static void
showWorkspaceName(WScreen *scr, int workspace)
{
    WorkspaceNameData *data;
    RXImage *ximg;
    Pixmap text, mask;
    int w, h;
    int px, py;
    char *name = scr->workspaces[workspace]->name;
    int len = strlen(name);
    int x, y;

    if (wPreferences.workspace_name_display_position == WD_NONE
	|| scr->workspace_count < 2)
	return;

    if (scr->workspace_name_timer) {
	WMDeleteTimerHandler(scr->workspace_name_timer);
	XUnmapWindow(dpy, scr->workspace_name);
	XFlush(dpy);
    }
    scr->workspace_name_timer = WMAddTimerHandler(WORKSPACE_NAME_DELAY,
						  hideWorkpaceName, scr);

    if (scr->workspace_name_data) {
	RDestroyImage(scr->workspace_name_data->back);
	RDestroyImage(scr->workspace_name_data->text);
	wfree(scr->workspace_name_data);
    }

    data = wmalloc(sizeof(WorkspaceNameData));

    w = WMWidthOfString(scr->workspace_name_font, name, len);
    h = WMFontHeight(scr->workspace_name_font);

    switch (wPreferences.workspace_name_display_position) {
     case WD_TOP:
	px = (scr->scr_width - (w+4))/2;
	py = 0;
	break;
     case WD_BOTTOM:
	px = (scr->scr_width - (w+4))/2;
	py = scr->scr_height - (h+4);
	break;
     case WD_TOPLEFT:
	px = 0;
	py = 0;
	break;
     case WD_TOPRIGHT:
	px = scr->scr_width - (w+4);
	py = 0;
	break;
     case WD_BOTTOMLEFT:
	px = 0;
	py = scr->scr_height - (h+4);
	break;
     case WD_BOTTOMRIGHT:
	px = scr->scr_width - (w+4);
	py = scr->scr_height - (h+4);
	break;
     case WD_CENTER:
     default:
	px = (scr->scr_width - (w+4))/2;
	py = (scr->scr_height - (h+4))/2;
	break;
    }
    XResizeWindow(dpy, scr->workspace_name, w+4, h+4);
    XMoveWindow(dpy, scr->workspace_name, px, py);

    text = XCreatePixmap(dpy, scr->w_win, w+4, h+4, scr->w_depth);
    mask = XCreatePixmap(dpy, scr->w_win, w+4, h+4, 1);

    XSetForeground(dpy, scr->draw_gc, scr->black_pixel);
    XFillRectangle(dpy, text, scr->draw_gc, 0, 0, w+4, h+4);

    XSetForeground(dpy, scr->mono_gc, 0);
    XFillRectangle(dpy, mask, scr->mono_gc, 0, 0, w+4, h+4);

    XSetForeground(dpy, scr->mono_gc, 1);
    for (x = 0; x <= 4; x++) {
	for (y = 0; y <= 4; y++) {
	    WMDrawString(scr->wmscreen, mask, scr->mono_gc,
			 scr->workspace_name_font, x, y, name, len);
	}
    }

    XSetForeground(dpy, scr->draw_gc, scr->white_pixel);
    WMDrawString(scr->wmscreen, text, scr->draw_gc, scr->workspace_name_font, 
		 2, 2, scr->workspaces[workspace]->name,
		 strlen(scr->workspaces[workspace]->name));
#ifdef SHAPE
    XShapeCombineMask(dpy, scr->workspace_name, ShapeBounding, 0, 0, mask,
		      ShapeSet);
#endif
    XSetWindowBackgroundPixmap(dpy, scr->workspace_name, text);
    XClearWindow(dpy, scr->workspace_name);

    data->text = RCreateImageFromDrawable(scr->rcontext, text, None);

    XFreePixmap(dpy, text);
    XFreePixmap(dpy, mask);

    if (!data->text) {
	XMapRaised(dpy, scr->workspace_name);
	XFlush(dpy);

	goto erro;
    }

    ximg = RGetXImage(scr->rcontext, scr->root_win, px, py,
		      data->text->width, data->text->height);

    if (!ximg || !ximg->image) {
	goto erro;
    }

    XMapRaised(dpy, scr->workspace_name);
    XFlush(dpy);

    data->back = RCreateImageFromXImage(scr->rcontext, ximg->image, NULL);
    RDestroyXImage(scr->rcontext, ximg);

    if (!data->back) {
	goto erro;
    }

    data->count = 10;

    /* set a timeout for the effect */
    data->timeout = time(NULL) + 2 +
	(WORKSPACE_NAME_DELAY + WORKSPACE_NAME_FADE_DELAY*data->count)/1000;

    scr->workspace_name_data = data;


    return;

erro:
    if (scr->workspace_name_timer)
	WMDeleteTimerHandler(scr->workspace_name_timer);

    if (data->text)
	RDestroyImage(data->text);
    if (data->back)
	RDestroyImage(data->back);
    wfree(data);

    scr->workspace_name_data = NULL;

    scr->workspace_name_timer = WMAddTimerHandler(WORKSPACE_NAME_DELAY +
						  10*WORKSPACE_NAME_FADE_DELAY,
						  hideWorkpaceName, scr);
}


void
wWorkspaceChange(WScreen *scr, int workspace)
{
    if (scr->flags.startup || scr->flags.startup2) {
	return;
    }

    if (workspace != scr->current_workspace) {
        wWorkspaceForceChange(scr, workspace);
    } /*else {
	showWorkspaceName(scr, workspace);
    }*/
}


void
wWorkspaceRelativeChange(WScreen *scr, int amount)
{
    int w;

    w = scr->current_workspace + amount;

    if (amount < 0) {

	if (w >= 0)
	    wWorkspaceChange(scr, w);
	else if (wPreferences.ws_cycle)
	    wWorkspaceChange(scr, scr->workspace_count + w);

    } else if (amount > 0) {

	if (w < scr->workspace_count)
	    wWorkspaceChange(scr, w);
	else if (wPreferences.ws_advance)
	    wWorkspaceChange(scr, WMIN(w, MAX_WORKSPACES-1));
	else if (wPreferences.ws_cycle)
	    wWorkspaceChange(scr, w % scr->workspace_count);
    }
}



void
wWorkspaceForceChange(WScreen *scr, int workspace)
{
    WWindow *tmp, *foc=NULL, *foc2=NULL;
    
    if (workspace >= MAX_WORKSPACES || workspace < 0)
	return;

    SendHelperMessage(scr, 'C', workspace+1, NULL);

    if (workspace > scr->workspace_count-1) {
	wWorkspaceMake(scr, workspace - scr->workspace_count + 1);
    }

    wClipUpdateForWorkspaceChange(scr, workspace);

    scr->current_workspace = workspace;

    wWorkspaceMenuUpdate(scr, scr->workspace_menu);

    wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);

    if ((tmp = scr->focused_window)!= NULL) {
	if ((IS_OMNIPRESENT(tmp) && !WFLAGP(tmp, skip_window_list))
	    || tmp->flags.changing_workspace) {
	    foc = tmp;
	}
 
	/* foc2 = tmp; will fix annoyance with gnome panel
	 * but will create annoyance for every other application 
	 */

	while (tmp) {
	    if (tmp->frame->workspace!=workspace && !tmp->flags.selected) {
		/* unmap windows not on this workspace */
		if ((tmp->flags.mapped||tmp->flags.shaded)
		    && !IS_OMNIPRESENT(tmp)
		    && !tmp->flags.changing_workspace) {
		    
		    wWindowUnmap(tmp);
		}
                /* also unmap miniwindows not on this workspace */
                if (tmp->flags.miniaturized && !IS_OMNIPRESENT(tmp) 
		    && tmp->icon) {
                    if (!wPreferences.sticky_icons) {
                        XUnmapWindow(dpy, tmp->icon->core->window);
			tmp->icon->mapped = 0;
		    }
#if 0
		    else {
			tmp->icon->mapped = 1;
			/* Why is this here? -Alfredo */
                        XMapWindow(dpy, tmp->icon->core->window);
		    }
#endif
                }
		/* update current workspace of omnipresent windows */
		if (IS_OMNIPRESENT(tmp)) {
		    WApplication *wapp = wApplicationOf(tmp->main_window);

		    tmp->frame->workspace = workspace;

		    if (wapp) {
			wapp->last_workspace = workspace;
		    }
		    if (!foc2)
			foc2 = tmp;
		}
            } else {
		/* change selected windows' workspace */
		if (tmp->flags.selected) {
		    wWindowChangeWorkspace(tmp, workspace);
                    if (!tmp->flags.miniaturized && !foc) {
                        foc = tmp;
                    }
                } else {
		    if (!tmp->flags.hidden) {
			if (!(tmp->flags.mapped || tmp->flags.miniaturized)) {
			    /* remap windows that are on this workspace */
			    wWindowMap(tmp);
			    if (!foc && !WFLAGP(tmp, skip_window_list))
				foc = tmp;
			}
			/* Also map miniwindow if not omnipresent */
			if (!wPreferences.sticky_icons &&
			    tmp->flags.miniaturized &&
			    !IS_OMNIPRESENT(tmp) && tmp->icon) {
			    tmp->icon->mapped = 1;
			    XMapWindow(dpy, tmp->icon->core->window);
			}
		    }
                }
	    }
	    tmp = tmp->prev;
	}

	if (!foc)
	    foc = foc2;

	if (scr->focused_window->flags.mapped && !foc) {
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
        if (scr->workspaces[workspace]->clip->auto_collapse ||
            scr->workspaces[workspace]->clip->auto_raise_lower) {
            /* to handle enter notify. This will also */
            XUnmapWindow(dpy, scr->clip_icon->icon->core->window);
            XMapWindow(dpy, scr->clip_icon->icon->core->window);
        } else {
            wClipIconPaint(scr->clip_icon);
        }
    }

    showWorkspaceName(scr, workspace);

#ifdef GNOME_STUFF
    wGNOMEUpdateCurrentWorkspaceHint(scr);
#endif
#ifdef KWM_HINTS
    wKWMUpdateCurrentWorkspaceHint(scr);
#endif
/*   XSync(dpy, False); */
}

#ifdef VIRTUAL_DESKTOP
/* TODO:
* 1) Allow border around each window so the scrolling
* won't just stop at the border.
* 2) Make pager.
*/

void wWorkspaceManageEdge(WScreen *scr)
{
    int w;
    int vmask;
    XSetWindowAttributes attribs;

    puts("wWorkspaceManageEdge()");
    if (wPreferences.vedge_thickness) {
        initVDesk = True;
        for (w = 0; w < scr->workspace_count; w++) {
            puts("reset workspace");
            wWorkspaceSetViewPort(scr, w, 0, 0);
        }

        vmask = CWEventMask|CWOverrideRedirect;
        attribs.event_mask = (EnterWindowMask | LeaveWindowMask | VisibilityChangeMask);
        attribs.override_redirect = True;
        scr->virtual_edge_u =
            XCreateWindow(dpy, scr->root_win, 0, 0,
                    scr->scr_width, wPreferences.vedge_thickness, 0,
                    CopyFromParent, InputOnly, CopyFromParent, vmask, &attribs);
        scr->virtual_edge_d =
            XCreateWindow(dpy, scr->root_win, 0, scr->scr_height-wPreferences.vedge_thickness,
                    scr->scr_width, wPreferences.vedge_thickness, 0,
                    CopyFromParent, InputOnly, CopyFromParent, vmask, &attribs);
        scr->virtual_edge_l =
            XCreateWindow(dpy, scr->root_win, 0, 0,
                    wPreferences.vedge_thickness, scr->scr_height, 0,
                    CopyFromParent, InputOnly, CopyFromParent, vmask, &attribs);
        scr->virtual_edge_r =
            XCreateWindow(dpy, scr->root_win, scr->scr_width-wPreferences.vedge_thickness, 0,
                    wPreferences.vedge_thickness, scr->scr_height, 0,
                    CopyFromParent, InputOnly, CopyFromParent, vmask, &attribs);
        XMapWindow(dpy, scr->virtual_edge_u);
        XMapWindow(dpy, scr->virtual_edge_d);
        XMapWindow(dpy, scr->virtual_edge_l);
        XMapWindow(dpy, scr->virtual_edge_r);
        wWorkspaceRaiseEdge(scr);
    }
}

void wWorkspaceRaiseEdge(WScreen *scr)
{
    if (wPreferences.vedge_thickness && initVDesk) {
        XRaiseWindow(dpy, scr->virtual_edge_u);
        XRaiseWindow(dpy, scr->virtual_edge_d);
        XRaiseWindow(dpy, scr->virtual_edge_l);
        XRaiseWindow(dpy, scr->virtual_edge_r);
    }
}

void wWorkspaceResizeViewPort(WScreen *scr, int workspace, int width, int height)
{
    scr->workspaces[workspace]->width = WMAX(width,scr->scr_width);
    scr->workspaces[workspace]->height = WMAX(height,scr->scr_height);
}

void updateWorkspaceGeometry(WScreen *scr, int workspace, int *view_x, int *view_y) {
    int most_left, most_right, most_top, most_bottom;
    WWindow *wwin;

    /* adjust workspace layout */
    wwin = scr->focused_window;
    most_right = scr->scr_width;
    most_bottom = scr->scr_height;
    most_left = 0;
    most_top = 0;
    for(;wwin; wwin = wwin->prev) {
        if (wwin->frame->workspace == workspace) {
            if (!wwin->flags.miniaturized
                    && !wwin->flags.hidden) {
                if (wwin->frame_x < most_left) { /* record positions, should this be cached? */
                    most_left = wwin->frame_x;
                }
                if ((int)wwin->frame_x + (int)wwin->frame->core->width > most_right) {
                    most_right = wwin->frame_x + wwin->frame->core->width;
                }
                if (wwin->frame_y < most_top) {
                    most_top = wwin->frame_y;
                }
                if (wwin->frame_y + wwin->frame->core->height > most_bottom) {
                    most_bottom = wwin->frame_y + wwin->frame->core->height;
                }
            }
        }
    }

    scr->workspaces[workspace]->width = WMAX(most_right, scr->scr_width) - WMIN(most_left, 0);
    scr->workspaces[workspace]->height = WMAX(most_bottom, scr->scr_height) - WMIN(most_top, 0);

    *view_x += -most_left - scr->workspaces[workspace]->view_x;
    scr->workspaces[workspace]->view_x = -most_left;

    *view_y += -most_top - scr->workspaces[workspace]->view_y;
    scr->workspaces[workspace]->view_y = -most_top;

}

Bool wWorkspaceSetViewPort(WScreen *scr, int workspace, int view_x, int view_y)
{
    Bool adjust_flag = False;
    int diff_x, diff_y;
    WWindow *wwin;

    printf("wWorkspaceSetViewPort %d %d\n", view_x, view_y);

    updateWorkspaceGeometry(scr, workspace, &view_x, &view_y);

    if (view_x + scr->scr_width > scr->workspaces[workspace]->width) {
        /* puts("right edge of vdesk"); */
        view_x = scr->workspaces[workspace]->width - scr->scr_width;
    }
    if (view_x < 0) {
        /* puts("left edge of vdesk"); */
        view_x = 0;
    }
    if (view_y + scr->scr_height > scr->workspaces[workspace]->height) {
        /* puts("right edge of vdesk"); */
        view_y = scr->workspaces[workspace]->height - scr->scr_height;
    }
    if (view_y < 0) {
        /* puts("left edge of vdesk"); */
        view_y = 0;
    }

    diff_x = scr->workspaces[workspace]->view_x - view_x;
    diff_y = scr->workspaces[workspace]->view_y - view_y;
    if (!diff_x && !diff_y)
        return False;

    scr->workspaces[workspace]->view_x = view_x;
    scr->workspaces[workspace]->view_y = view_y;


    for( wwin = scr->focused_window; wwin; wwin = wwin->prev) {
        if (wwin->frame->workspace == workspace) {
            wWindowMove(wwin, wwin->frame_x + diff_x, wwin->frame_y + diff_y);
            wWindowSynthConfigureNotify(wwin);
        }
    }

    return adjust_flag;
}

void wWorkspaceGetViewPosition(WScreen *scr, int workspace, int *view_x, int *view_y) {
    if (view_x) *view_x = scr->workspaces[workspace]->view_x;
    if (view_y) *view_y = scr->workspaces[workspace]->view_y;
}

#endif

static void
switchWSCommand(WMenu *menu, WMenuEntry *entry)
{
    wWorkspaceChange(menu->frame->screen_ptr, (long)entry->clientdata);
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
    wfree(scr->workspaces[workspace]->name);
    scr->workspaces[workspace]->name = wstrdup(buf);

    if (scr->clip_ws_menu) {
	if (strcmp(scr->clip_ws_menu->entries[workspace+2]->text, buf)!=0) {
	    wfree(scr->clip_ws_menu->entries[workspace+2]->text);
	    scr->clip_ws_menu->entries[workspace+2]->text = wstrdup(buf);
	    wMenuRealize(scr->clip_ws_menu);
	}
    }
    if (scr->workspace_menu) {
	if (strcmp(scr->workspace_menu->entries[workspace+2]->text, buf)!=0) {
	    wfree(scr->workspace_menu->entries[workspace+2]->text);
	    scr->workspace_menu->entries[workspace+2]->text = wstrdup(buf);
	    wMenuRealize(scr->workspace_menu);
	}
    }

    UpdateSwitchMenuWorkspace(scr, workspace);

    if (scr->clip_icon)
        wClipIconPaint(scr->clip_icon);
    
#ifdef GNOME_STUFF
    wGNOMEUpdateWorkspaceNamesHint(scr);
#endif
#ifdef KWM_HINTS
    wKWMUpdateWorkspaceNameHint(scr, workspace);
#endif
}




/* callback for when menu entry is edited */
static void
onMenuEntryEdited(WMenu *menu, WMenuEntry *entry)
{
    char *tmp;

    tmp = entry->text;
    wWorkspaceRename(menu->frame->screen_ptr, (long)entry->clientdata, tmp);
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
    int i;
    long ws;
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
    int i, j, wscount;

    make_keys();

    parr = PLGetDictionaryEntry(scr->session_state, dWorkspaces);

    if (!parr)
	return;
    
    wscount = scr->workspace_count;
    for (i=0; i < WMIN(PLGetNumberOfElements(parr), MAX_WORKSPACES); i++) {
        wks_state = PLGetArrayElement(parr, i);
        if (PLIsDictionary(wks_state))
            pstr = PLGetDictionaryEntry(wks_state, dName);
        else
            pstr = wks_state;
	if (i >= scr->workspace_count)
	    wWorkspaceNew(scr);
	if (scr->workspace_menu) {
	    wfree(scr->workspace_menu->entries[i+2]->text);
	    scr->workspace_menu->entries[i+2]->text = wstrdup(PLGetString(pstr));
	    scr->workspace_menu->flags.realized = 0;
	}
	wfree(scr->workspaces[i]->name);
        scr->workspaces[i]->name = wstrdup(PLGetString(pstr));
        if (!wPreferences.flags.noclip) {
            clip_state = PLGetDictionaryEntry(wks_state, dClip);
            if (scr->workspaces[i]->clip)
                wDockDestroy(scr->workspaces[i]->clip);
            scr->workspaces[i]->clip = wDockRestoreState(scr, clip_state,
                                                         WM_CLIP);
            if (i>0)
                wDockHideIcons(scr->workspaces[i]->clip);

            /* We set the global icons here, because scr->workspaces[i]->clip
             * was not valid in wDockRestoreState().
             * There we only set icon->omnipresent to know which icons we
             * need to set here.
             */
            for (j=0; j<scr->workspaces[i]->clip->max_icons; j++) {
                WAppIcon *aicon = scr->workspaces[i]->clip->icon_array[j];

                if (aicon && aicon->omnipresent) {
                    aicon->omnipresent = 0;
                    wClipMakeIconOmnipresent(aicon, True);
                    XMapWindow(dpy, aicon->icon->core->window);
                }
            }
        }
#ifdef KWM_HINTS
	wKWMUpdateWorkspaceNameHint(scr, i);
#endif
    }
#ifdef GNOME_STUFF
    wGNOMEUpdateWorkspaceNamesHint(scr);
#endif
}


