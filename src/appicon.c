/* appicon.c- icon for applications (not mini-window)
 *
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
 *  Copyright (c) 1998       Dan Pascu
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
#include <string.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "actions.h"
#include "stacking.h"
#include "dock.h"
#include "funcs.h"
#include "defaults.h"
#include "workspace.h"
#include "superfluous.h"
#include "menu.h"
#include "framewin.h"
#include "dialog.h"
#include "client.h"

/* 
 * icon_file for the dock is got from the preferences file by
 * using the classname/instancename
 */

/**** Global variables ****/
extern Cursor wCursor[WCUR_LAST];
extern WPreferences wPreferences;

#define MOD_MASK       wPreferences.modifier_mask

void appIconMouseDown(WObjDescriptor *desc, XEvent *event);
static void iconDblClick(WObjDescriptor *desc, XEvent *event);
static void iconExpose(WObjDescriptor *desc, XEvent *event);



WAppIcon*
wAppIconCreateForDock(WScreen *scr, char *command, char *wm_instance,
		      char *wm_class, int tile)
{
    WAppIcon *dicon;
    char *path;

    dicon = wmalloc(sizeof(WAppIcon));
    wretain(dicon);
    memset(dicon, 0, sizeof(WAppIcon));
    dicon->yindex = -1;
    dicon->xindex = -1;

    dicon->prev = NULL;
    dicon->next = scr->app_icon_list;
    if (scr->app_icon_list) {
        scr->app_icon_list->prev = dicon;
    }
    scr->app_icon_list = dicon;

    if (command) {
	dicon->command = wstrdup(command);	
    }
    if (wm_class)
      dicon->wm_class = wstrdup(wm_class);
    if (wm_instance)
      dicon->wm_instance = wstrdup(wm_instance);

    path = wDefaultGetIconFile(scr, wm_instance, wm_class, True);
    if (!path && command) {
	wApplicationExtractDirPackIcon(scr, command, wm_instance, wm_class);

	path = wDefaultGetIconFile(scr, wm_instance, wm_class, False);
    }
    
    if (path)
	path = FindImage(wPreferences.icon_path, path);

    dicon->icon = wIconCreateWithIconFile(scr, path, tile);
    if (path)
	free(path);
#ifdef REDUCE_APPICONS
    dicon->num_apps = 0;
#endif
#ifdef DEMATERIALIZE_ICON
    {
        XSetWindowAttributes attribs;
        attribs.save_under = True;
        XChangeWindowAttributes(dpy, dicon->icon->core->window,
                CWSaveUnder, &attribs);
    }
#endif

    /* will be overriden by dock */
    dicon->icon->core->descriptor.handle_mousedown = appIconMouseDown;
    dicon->icon->core->descriptor.handle_expose = iconExpose;
    dicon->icon->core->descriptor.parent_type = WCLASS_APPICON;
    dicon->icon->core->descriptor.parent = dicon;
    AddToStackList(dicon->icon->core);

    return dicon;
}



WAppIcon*
wAppIconCreate(WWindow *leader_win)
{
    WAppIcon *aicon;
#ifdef REDUCE_APPICONS
    WAppIcon *atmp;
    WAppIconAppList *applist;
    char *tinstance, *tclass;
#endif
    WScreen *scr = leader_win->screen_ptr;

    aicon = wmalloc(sizeof(WAppIcon));
    wretain(aicon);
    memset(aicon, 0, sizeof(WAppIcon));
#ifdef REDUCE_APPICONS
    applist = wmalloc(sizeof(WAppIconAppList));
    memset(applist, 0, sizeof(WAppIconAppList));
    applist->wapp = wApplicationOf(leader_win->main_window);
    aicon->applist = applist;
    if (applist->wapp == NULL) {
	/* Something's wrong. wApplicationOf() should always return a
	 * valid structure.  Rather than violate assumptions, bail. -cls
	 */
	free(applist);
	wrelease(aicon);
	return NULL;
    }
#endif

    aicon->yindex = -1;
    aicon->xindex = -1;

    aicon->prev = NULL;
    aicon->next = scr->app_icon_list;
    if (scr->app_icon_list) {
#ifndef REDUCE_APPICONS
        scr->app_icon_list->prev = aicon;
#else
	/* If we aren't going to have a match, jump straight to new appicon */
	if (leader_win->wm_class == NULL || leader_win->wm_class == NULL)
	    atmp = NULL;
	else
	atmp = scr->app_icon_list;

	while (atmp != NULL) {
	    if ((tinstance = atmp->wm_instance) == NULL)
		tinstance = "";
	    if ((tclass = atmp->wm_class) == NULL)
		tclass = "";
	    if ((strcmp(leader_win->wm_class, tclass) == 0) &&
		(strcmp(leader_win->wm_instance, tinstance) == 0))
	    {
		/* We have a winner */
		wrelease(aicon);	
		atmp->num_apps++;
		applist->next = atmp->applist;
		if (atmp->applist)
		    atmp->applist->prev = applist;
		atmp->applist = applist;

		if (atmp->docked) {
		    wDockSimulateLaunch(atmp->dock, atmp);
		}

		return atmp;
	    }
	    atmp = atmp->next;
	}
	if (atmp == NULL) {
	    scr->app_icon_list->prev = aicon;
	}
#endif /* REDUCE_APPICONS */
    }
    scr->app_icon_list = aicon;

    if (leader_win->wm_class)
        aicon->wm_class = wstrdup(leader_win->wm_class);
    if (leader_win->wm_instance)
        aicon->wm_instance = wstrdup(leader_win->wm_instance);
#ifdef REDUCE_APPICONS
    aicon->num_apps = 1;
#endif

    aicon->icon = wIconCreate(leader_win);
#ifdef DEMATERIALIZE_ICON
    {
        XSetWindowAttributes attribs;
        attribs.save_under = True;
        XChangeWindowAttributes(dpy, aicon->icon->core->window,
                CWSaveUnder, &attribs);
    }
#endif

    /* will be overriden if docked */
    aicon->icon->core->descriptor.handle_mousedown = appIconMouseDown;
    aicon->icon->core->descriptor.handle_expose = iconExpose;
    aicon->icon->core->descriptor.parent_type = WCLASS_APPICON;
    aicon->icon->core->descriptor.parent = aicon;
    AddToStackList(aicon->icon->core);
    aicon->icon->show_title = 0;
    wIconUpdate(aicon->icon);

    return aicon;
}


void
wAppIconDestroy(WAppIcon *aicon)
{
    WScreen *scr = aicon->icon->core->screen_ptr;
#ifdef REDUCE_APPICONS
    WAppIconAppList *aptmp;
#endif

    RemoveFromStackList(aicon->icon->core);
    wIconDestroy(aicon->icon);
    if (aicon->command)
	free(aicon->command);
#ifdef OFFIX_DND
    if (aicon->dnd_command)
	free(aicon->dnd_command);
#endif
    if (aicon->wm_instance)
      free(aicon->wm_instance);
    if (aicon->wm_class)
      free(aicon->wm_class);
#ifdef REDUCE_APPICONS
    /* There should never be a list but just in case */
    if (aicon->applist != NULL) {
	aptmp = aicon->applist;
    	while (aptmp->next) {
	    aptmp = aptmp->next;
	    free(aptmp->prev);
	}
	free(aptmp);
    }
#endif

    if (aicon == scr->app_icon_list) {
        if (aicon->next)
            aicon->next->prev = NULL;
        scr->app_icon_list = aicon->next;
    }
    else {
        if (aicon->next)
            aicon->next->prev = aicon->prev;
        if (aicon->prev)
            aicon->prev->next = aicon->next;
    }

    aicon->destroyed = 1;
    wrelease(aicon);
}



#ifdef NEWAPPICON
static void
drawCorner(WIcon *icon, WWindow *wwin, int active)
{
    WScreen *scr = wwin->screen_ptr;
    XPoint points[3];
    GC gc;

    points[0].x = 2;
    points[0].y = 2;
    points[1].x = 12;
    points[1].y = 2;
    points[2].x = 2;
    points[2].y = 12;
    if (active) {
	gc=scr->focused_texture->any.gc;
    } else {
	gc=scr->unfocused_texture->any.gc;
    }
    XFillPolygon(dpy, icon->core->window, gc, points, 3,
		 Convex, CoordModeOrigin);
}
#endif /* NEWAPPICON */


void 
wAppIconMove(WAppIcon *aicon, int x, int y)
{
    XMoveWindow(dpy, aicon->icon->core->window, x, y);
    aicon->x_pos = x;
    aicon->y_pos = y;
}


#ifdef WS_INDICATOR
static void
updateDockNumbers(WScreen *scr)
{
   int length;
   char *ws_numbers;
   GC numbers_gc;
   XGCValues my_gc_values;
   unsigned long my_v_mask = (GCForeground);
   WAppIcon *dicon = scr->dock->icon_array[0];

   my_gc_values.foreground = scr->white_pixel;
   numbers_gc = XCreateGC(dpy, dicon->icon->core->window,
			  my_v_mask, &my_gc_values);
   
   ws_numbers = malloc(20);
   sprintf(ws_numbers, "%i [ %i ]", scr->current_workspace+1, 
	   ((scr->current_workspace/10)+1));
   length = strlen(ws_numbers);

   XClearArea(dpy, dicon->icon->core->window, 2, 2, 50, 
	      scr->icon_title_font->y+1, False);

#ifndef I18N_MB
   XSetFont(dpy, numbers_gc, scr->icon_title_font->font->fid);
   XSetForeground(dpy, numbers_gc, scr->black_pixel);
   XDrawString(dpy, dicon->icon->core->window,
               numbers_gc, 4, scr->icon_title_font->y+3, ws_numbers, length);
   XSetForeground(dpy, numbers_gc, scr->white_pixel);
   XDrawString(dpy, dicon->icon->core->window,
               numbers_gc, 3, scr->icon_title_font->y+2, ws_numbers, length);
#else
   XSetForeground(dpy, numbers_gc, scr->black_pixel);
   XmbDrawString(dpy, dicon->icon->core->window,
                 scr->icon_title_font->font, numbers_gc, 4,
                 scr->icon_title_font->y+3, ws_numbers, length);
   XSetForeground(dpy, numbers_gc, scr->white_pixel);
   XmbDrawString(dpy, dicon->icon->core->window,
                 scr->icon_title_font->font, numbers_gc, 3,
                 scr->icon_title_font->y+2, ws_numbers, length);
#endif /* I18N_MB */

   XFreeGC(dpy, numbers_gc);
   free(ws_numbers);
}
#endif /* WS_INDICATOR */


void
wAppIconPaint(WAppIcon *aicon)
{
    WScreen *scr = aicon->icon->core->screen_ptr;

    wIconPaint(aicon->icon);


# ifdef WS_INDICATOR
    if (aicon->docked && scr->dock && aicon->yindex==0)
	updateDockNumbers(scr);
# endif
    if (scr->dock_dots && aicon->docked && !aicon->running
	&& aicon->command!=NULL) {
	XSetClipMask(dpy, scr->copy_gc, scr->dock_dots->mask);
	XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
	XCopyArea(dpy, scr->dock_dots->image, aicon->icon->core->window, 
		  scr->copy_gc, 0, 0, scr->dock_dots->width,
		  scr->dock_dots->height, 0, 0);
    }

#ifdef HIDDENDOT
    {
        WApplication *wapp;
	wapp = wApplicationOf(aicon->main_window);
	if (wapp) {
	    if (wapp->flags.hidden) {
		XSetClipMask(dpy, scr->copy_gc, scr->dock_dots->mask);
		XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
		XCopyArea(dpy, scr->dock_dots->image, 
			  aicon->icon->core->window, 
			  scr->copy_gc, 0, 0, 7,
			  scr->dock_dots->height, 0, 0);
	    }
	}
    }
#endif /* HIDDENDOT */

#ifdef NEWAPPICON
    if (!wPreferences.strict_ns && aicon->icon->owner!=NULL) {
	int active=0;
	if (aicon->main_window==None) {
	    active = (aicon->icon->owner->flags.focused ? 1 : 0);
	} else {
	    WApplication *wapp;

	    wapp = wApplicationOf(aicon->main_window);
	    if (!wapp) {
		active = aicon->icon->owner->flags.focused;
		wwarning("error in wAppIconPaint(). Please report it");
	    } else {
		active = wapp->main_window_desc->flags.focused;
		if (wapp->main_window_desc->flags.hidden
		    || !wapp->main_window_desc->flags.mapped)
		  active = -1;
	    }
	}
	if (active>=0)
	  drawCorner(aicon->icon, aicon->icon->owner, active);
    }
#endif /* NEWAPPICON */

    XSetClipMask(dpy, scr->copy_gc, None);
    if (aicon->launching) {
	XFillRectangle(dpy, aicon->icon->core->window, scr->stipple_gc, 
		       0, 0, wPreferences.icon_size, wPreferences.icon_size);
    }
}



#define canBeDocked(wwin)  ((wwin) && ((wwin)->wm_class||(wwin)->wm_instance))

#ifdef REDUCE_APPICONS
unsigned int
wAppIconReduceAppCount(WApplication *wapp) 
{
    WAppIconAppList *applist;

    if (wapp == NULL)
	return 0;

    if (wapp->app_icon == NULL) 
	return 0;

    /* If given a main window, check the applist 
     * and remove the if it exists 
     */
	applist = wapp->app_icon->applist;
	while (applist != NULL) {
	        if (applist->wapp == wapp) {
		    /* If this app owns the appicon, change the appicon's 
		     * owner to the next app in the list or NULL
		     */
	    if (wapp->app_icon->icon->owner == applist->wapp->main_window_desc)
{
			if (applist->next) {
			    wapp->app_icon->icon->owner = applist->next->wapp->main_window_desc;
			} else if (applist->prev) {
			    wapp->app_icon->icon->owner = applist->prev->wapp->main_window_desc;
			} else {
			    wapp->app_icon->icon->owner = NULL;
			}
		    }
		    if (applist->prev)
			applist->prev->next = applist->next;

		    if (applist->next)
			applist->next->prev = applist->prev;

	    if (applist == wapp->app_icon->applist)
		    	wapp->app_icon->applist = applist->next;

		    free(applist);

		    if (wapp->app_icon->applist != NULL)
		    	wapp->app_icon->main_window = wapp->app_icon->applist->wapp->main_window;

		    return (--wapp->app_icon->num_apps);
	    	}
	    applist = applist->next;
	}
    return (--wapp->app_icon->num_apps);
}
#endif


static void
hideCallback(WMenu *menu, WMenuEntry *entry)
{
    WApplication *wapp = (WApplication*)entry->clientdata;
    
    if (wapp->flags.hidden) {
	wWorkspaceChange(menu->menu->screen_ptr, wapp->last_workspace);
	wUnhideApplication(wapp, False, False);
    } else {
	wHideApplication(wapp);
    }
}


static void
unhideHereCallback(WMenu *menu, WMenuEntry *entry)
{
    WApplication *wapp = (WApplication*)entry->clientdata;
    
    wUnhideApplication(wapp, False, True);
}


static void
setIconCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *icon = ((WApplication*)entry->clientdata)->app_icon;
    char *file=NULL;
    WScreen *scr;
    int result;

    assert(icon!=NULL);

    if (icon->editing)
	return;
    icon->editing = 1;
    scr = icon->icon->core->screen_ptr;
    
    wretain(icon);

    result = wIconChooserDialog(scr, &file, icon->wm_instance, icon->wm_class);
    
    if (result && !icon->destroyed) {
	if (file[0]==0) {
	    free(file);
	    file = NULL;
	}
	if (!wIconChangeImageFile(icon->icon, file)) {
	    wMessageDialog(scr, _("Error"),
			   _("Could not open specified icon file"), 
			   _("OK"), NULL, NULL);
	} else {
	    wDefaultChangeIcon(scr, icon->wm_instance, icon->wm_class, file);
	    wAppIconPaint(icon);
	}
	if (file)
	    free(file);
    }
    icon->editing = 0;
    wrelease(icon);
}


static void
killCallback(WMenu *menu, WMenuEntry *entry)
{
    WApplication *wapp = (WApplication*)entry->clientdata;
    char *buffer;

    if (!WCHECK_STATE(WSTATE_NORMAL))
	return;

    WCHANGE_STATE(WSTATE_MODAL);
    
    assert(entry->clientdata!=NULL);

    buffer = wstrappend(wapp->app_icon ? wapp->app_icon->wm_class : NULL,
			_(" will be forcibly closed.\n"
			  "Any unsaved changes will be lost.\n"
			  "Please confirm."));

    wretain(wapp->main_window_desc);
    if (wPreferences.dont_confirm_kill
	|| wMessageDialog(menu->frame->screen_ptr, _("Kill Application"),
			  buffer, _("Yes"), _("No"), NULL)==WAPRDefault) {
	if (!wapp->main_window_desc->flags.destroyed)
	    wClientKill(wapp->main_window_desc);
    }
    wrelease(wapp->main_window_desc);

    free(buffer);

    WCHANGE_STATE(WSTATE_NORMAL);
}


static WMenu*
createApplicationMenu(WScreen *scr)
{
    WMenu *menu;
    
    menu = wMenuCreate(scr, NULL, False);
    wMenuAddCallback(menu, _("Unhide Here"), unhideHereCallback, NULL);
    wMenuAddCallback(menu, _("Hide"), hideCallback, NULL);
    wMenuAddCallback(menu, _("Set Icon..."), setIconCallback, NULL);
    wMenuAddCallback(menu, _("Kill"), killCallback, NULL);

    return menu;
}


static void
openApplicationMenu(WApplication *wapp, int x, int y)
{
    WMenu *menu;
    WScreen *scr = wapp->main_window_desc->screen_ptr;
    int i;
    
    if (!scr->icon_menu) {
	scr->icon_menu = createApplicationMenu(scr);
	free(scr->icon_menu->entries[1]->text);
    }

    menu = scr->icon_menu;

    if (wapp->flags.hidden) {
	menu->entries[1]->text = _("Unhide");
    } else {
	menu->entries[1]->text = _("Hide");
    }

    menu->flags.realized = 0;
    wMenuRealize(menu);

    x -= menu->frame->core->width/2;
    if (x + menu->frame->core->width > scr->scr_width)
	x = scr->scr_width - menu->frame->core->width;
    if (x < 0)
	x = 0;

    /* set client data */
    for (i = 0; i < menu->entry_no; i++) {
	menu->entries[i]->clientdata = wapp;
    }
    wMenuMapAt(menu, x, y, False);
}


/******************************************************************/

static void 
iconExpose(WObjDescriptor *desc, XEvent *event)
{
    wAppIconPaint(desc->parent);
}


static void
iconDblClick(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *aicon = desc->parent;
    WApplication *wapp;
    WScreen *scr = aicon->icon->core->screen_ptr;
    int unhideHere;

#ifndef REDUCE_APPICONS
    assert(aicon->icon->owner!=NULL);
#else
    if (aicon->icon->owner == NULL) {
       fprintf(stderr, "Double-click disabled: missing main window.\n");
       return;
    }
#endif 

    wapp = wApplicationOf(aicon->icon->owner->main_window);
#ifdef DEBUG0
    if (!wapp) {
	wwarning("could not find application descriptor for app icon!!");
	return;
    }
#endif
#ifdef REDUCE_APPICONS
    if (!wapp) {
       fprintf(stderr, "Double-click disabled: missing wapp.\n");
       return;
    }
#endif /* REDUCE_APPICONS */

    unhideHere = (event->xbutton.state & ShiftMask);

    /* go to the last workspace that the user worked on the app */
    if (!unhideHere && wapp->last_workspace != scr->current_workspace)
	wWorkspaceChange(scr, wapp->last_workspace);

    wUnhideApplication(wapp, event->xbutton.button==Button2, unhideHere);

    if (event->xbutton.state & MOD_MASK) {
	wHideOtherApplications(aicon->icon->owner);
    }
}


void 
appIconMouseDown(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *aicon = desc->parent;
    WIcon *icon = aicon->icon;
    XEvent ev;
    int x=aicon->x_pos, y=aicon->y_pos;
    int dx=event->xbutton.x, dy=event->xbutton.y;
    int grabbed=0;
    int done=0;
    int superfluous = wPreferences.superfluous; /* we catch it to avoid problems */
    WScreen *scr = icon->core->screen_ptr;
    WWorkspace *workspace = scr->workspaces[scr->current_workspace];
    int shad_x = 0, shad_y = 0, docking=0, dockable, collapsed = 0;
    int ix, iy;
    int clickButton = event->xbutton.button;
    Pixmap ghost = None;

    if (aicon->editing || WCHECK_STATE(WSTATE_MODAL))
	return;
    
    if (IsDoubleClick(scr, event)) {
	iconDblClick(desc, event);
	return;
    }

    if (event->xbutton.button == Button3) {
	WObjDescriptor *desc;
	WApplication *wapp = wApplicationOf(aicon->icon->owner->main_window);

	if (!wapp)
	    return;

	openApplicationMenu(wapp, event->xbutton.x_root, 
			    event->xbutton.y_root);

	/* allow drag select of menu */
	desc = &scr->icon_menu->menu->descriptor;
	event->xbutton.send_event = True;
	(*desc->handle_mousedown)(desc, event);
	return;
    }

#ifdef DEBUG
    puts("Moving icon");
#endif
    if (event->xbutton.state & MOD_MASK)
	wLowerFrame(icon->core);
    else
	wRaiseFrame(icon->core);

    
    if (XGrabPointer(dpy, icon->core->window, True, ButtonMotionMask
		     |ButtonReleaseMask|ButtonPressMask, GrabModeAsync,
		     GrabModeAsync, None, None, CurrentTime) !=GrabSuccess) {
	wwarning("pointer grab failed for appicon move");
    }

    if (wPreferences.flags.nodock && wPreferences.flags.noclip)
      dockable = 0;
    else
      dockable = canBeDocked(icon->owner);


    while (!done) {
	WMMaskEvent(dpy, PointerMotionMask|ButtonReleaseMask|ButtonPressMask
		    |ButtonMotionMask|ExposureMask, &ev);
	switch (ev.type) {
	 case Expose:
	    WMHandleEvent(&ev);
	    break;

	 case MotionNotify:
	    if (!grabbed) {
		if (abs(dx-ev.xmotion.x)>=MOVE_THRESHOLD
		    || abs(dy-ev.xmotion.y)>=MOVE_THRESHOLD) {
		    XChangeActivePointerGrab(dpy, ButtonMotionMask
					    |ButtonReleaseMask|ButtonPressMask, 
					     wCursor[WCUR_MOVE], CurrentTime);
		    grabbed=1;
		} else {
		    break;
		}
	    }
	    x = ev.xmotion.x_root - dx;
	    y = ev.xmotion.y_root - dy;
	    XMoveWindow(dpy, icon->core->window, x, y);

	    if (dockable) {
                if (scr->dock && wDockSnapIcon(scr->dock, aicon, x, y,
                                               &ix, &iy, False)) {
                    shad_x = scr->dock->x_pos + ix*wPreferences.icon_size;
		    shad_y = scr->dock->y_pos + iy*wPreferences.icon_size;

                    if (scr->last_dock != scr->dock && collapsed) {
                        scr->last_dock->collapsed = 1;
                        wDockHideIcons(scr->last_dock);
                        collapsed = 0;
                    }
                    if (!collapsed && (collapsed = scr->dock->collapsed)) {
                        scr->dock->collapsed = 0;
                        wDockShowIcons(scr->dock);
                    }

                    if (scr->dock->auto_raise_lower)
                        wDockRaise(scr->dock);

                    scr->last_dock = scr->dock;

		    XMoveWindow(dpy, scr->dock_shadow, shad_x, shad_y);
		    if (!docking) {
			Window wins[2];

			wins[0] = icon->core->window;
			wins[1] = scr->dock_shadow;
			XRestackWindows(dpy, wins, 2);
                        if (superfluous) {
			if (icon->pixmap!=None)
			    ghost = MakeGhostIcon(scr, icon->pixmap);
			else
			    ghost = MakeGhostIcon(scr, icon->core->window);
			XSetWindowBackgroundPixmap(dpy, scr->dock_shadow,
						   ghost);
			XClearWindow(dpy, scr->dock_shadow);
                        }
                        XMapWindow(dpy, scr->dock_shadow);
		    }
		    docking = 1;
                } else if (workspace->clip &&
                           wDockSnapIcon(workspace->clip, aicon, x, y,
                                         &ix, &iy, False)) {
                    shad_x = workspace->clip->x_pos + ix*wPreferences.icon_size;
		    shad_y = workspace->clip->y_pos + iy*wPreferences.icon_size;
		    
                    if (scr->last_dock != workspace->clip && collapsed) {
                        scr->last_dock->collapsed = 1;
                        wDockHideIcons(scr->last_dock);
                        collapsed = 0;
                    }
                    if (!collapsed && (collapsed = workspace->clip->collapsed)) {
                        workspace->clip->collapsed = 0;
                        wDockShowIcons(workspace->clip);
                    }

                    if (workspace->clip->auto_raise_lower)
                        wDockRaise(workspace->clip);

                    scr->last_dock = workspace->clip;

		    XMoveWindow(dpy, scr->dock_shadow, shad_x, shad_y);
		    if (!docking) {
			Window wins[2];

			wins[0] = icon->core->window;
			wins[1] = scr->dock_shadow;
			XRestackWindows(dpy, wins, 2);
                        if (superfluous) {
			if (icon->pixmap!=None)
			    ghost = MakeGhostIcon(scr, icon->pixmap);
			else
			    ghost = MakeGhostIcon(scr, icon->core->window);
			XSetWindowBackgroundPixmap(dpy, scr->dock_shadow,
						   ghost);
			XClearWindow(dpy, scr->dock_shadow);
                        }
			XMapWindow(dpy, scr->dock_shadow);
		    }
                    docking = 1;
		} else if (docking) {
		    XUnmapWindow(dpy, scr->dock_shadow);
                    docking = 0;
		}
            }

	    break;

	 case ButtonPress:
	    break;

	 case ButtonRelease:
	    if (ev.xbutton.button != clickButton)
		break;
	    XUngrabPointer(dpy, CurrentTime);

            if (docking) {
		Bool docked;

		/* icon is trying to be docked */
		SlideWindow(icon->core->window, x, y, shad_x, shad_y);
		XUnmapWindow(dpy, scr->dock_shadow);
                docked = wDockAttachIcon(scr->last_dock, aicon, ix, iy);
                if (scr->last_dock->auto_collapse) {
                    collapsed = 0;
                }
                if (workspace->clip &&
                    workspace->clip != scr->last_dock &&
                    workspace->clip->auto_raise_lower)
                    wDockLower(workspace->clip);

		if (!docked) {
		    /* If icon could not be docked, slide it back to the old
		     * position */
		    SlideWindow(icon->core->window, x, y, aicon->x_pos, 
				aicon->y_pos);
		}
            } else {
		XMoveWindow(dpy, icon->core->window, x, y);
		aicon->x_pos = x;
		aicon->y_pos = y;
                if (workspace->clip && workspace->clip->auto_raise_lower)
                    wDockLower(workspace->clip);
            }
	    if (collapsed) {
		scr->last_dock->collapsed = 1;
		wDockHideIcons(scr->last_dock);
		collapsed = 0;
	    }
	    if (superfluous) {
		if (ghost!=None)
		    XFreePixmap(dpy, ghost);
		XSetWindowBackground(dpy, scr->dock_shadow, scr->white_pixel);
	    }

	    if (wPreferences.auto_arrange_icons)
		wArrangeIcons(scr, True);

	    done = 1;
	    break;
	}
    }
#ifdef DEBUG
    puts("End icon move");
#endif
    
}
