/* 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997 Shige Abe and 
 *		       Alfredo K. Kojima
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "WindowMaker.h"
#include "window.h"
#include "actions.h"
#include "client.h"
#include "funcs.h"
#include "stacking.h"
#include "workspace.h"
#include "framewin.h"

/********* Global Variables *******/
extern WPreferences wPreferences;
extern Time LastTimestamp;


/*
 * FocusWindow
 *
 *  - Needs to check if already in the right workspace before
 *    calling wChangeWorkspace?
 *
 *  Order:
 *    Switch to correct workspace
 *    Unshade if shaded
 *    If iconified then deiconify else focus/raise.
 */
static void
focusWindow(WMenu *menu, WMenuEntry *entry)
{
    WWindow *wwin;
    WScreen *scr;
    int x, y, move=0;

    wwin = (WWindow*)entry->clientdata;
    scr = wwin->screen_ptr;

    wMakeWindowVisible(wwin);

    x = wwin->frame_x;
    y = wwin->frame_y;
        
    /* bring window back to visible area */
    move = wScreenBringInside(scr, &x, &y, wwin->frame->core->width, 
			      wwin->frame->core->height);

    if (move) {
	wWindowConfigure(wwin, x, y, wwin->client.width, wwin->client.height);
    }
}
 
/*
 *
 * Open switch menu 
 *
 */
void 
OpenSwitchMenu(WScreen *scr, int x, int y, int keyboard)
{
    WMenu *switchmenu = scr->switch_menu;
    WWindow *wwin;

    if (switchmenu) {
	if (switchmenu->flags.mapped) {
	    if (!switchmenu->flags.buttoned) {
		wMenuUnmap(switchmenu);
	    } else {
		wRaiseFrame(switchmenu->frame->core);
		
		if (keyboard)
		    wMenuMapAt(switchmenu, 0, 0, True);
		else
		    wMenuMapCopyAt(switchmenu, x-switchmenu->frame->core->width/2,
			       y-switchmenu->frame->top_width/2);
	    }
	} else {
	    wMenuMapAt(switchmenu, x-switchmenu->frame->core->width/2,
		       y-switchmenu->frame->top_width/2, keyboard);
	}
	return;
    }
    switchmenu = wMenuCreate(scr,_("Windows"),True);
    scr->switch_menu = switchmenu;
    
    
    wwin = scr->focused_window;
    while (wwin) {
	UpdateSwitchMenu(scr, wwin, ACTION_ADD);

        wwin = wwin->prev;
    }
    
    if (switchmenu) {
	if (!switchmenu->flags.realized)
	    wMenuRealize(switchmenu);
	wMenuMapAt(switchmenu, x-switchmenu->frame->core->width/2,
		   y-switchmenu->frame->top_width/2, keyboard);
    }
}

/*
 *
 * Update switch menu
 *
 *
 */
void 
UpdateSwitchMenu(WScreen *scr, WWindow *wwin, int action)
{
    WMenu *switchmenu = scr->switch_menu;
    WMenuEntry *entry;
    char title[MAX_MENU_TEXT_LENGTH+6];
    int i;
    int checkVisibility = 0;

    if (!wwin->screen_ptr->switch_menu)
      return;
    /*
     *  This menu is updated under the following conditions:
     *
     *    1.  When a window is created.
     *    2.  When a window is destroyed.
     *
     *    3.  When a window changes it's title.
     */
    if (action == ACTION_ADD) {
	char *t;
	if (wwin->flags.internal_window 
	    || wwin->window_flags.skip_window_list)
	  return;
	    
	if (wwin->frame->title)
	  sprintf(title, "%s", wwin->frame->title);
	else
	  sprintf(title, "%s", DEF_WINDOW_TITLE);
	t = ShrinkString(scr->menu_entry_font, title, MAX_WINDOWLIST_WIDTH);
	entry = wMenuAddCallback(switchmenu, t, focusWindow, wwin);
	free(t);

	entry->flags.indicator = 1;
	entry->rtext = wmalloc(MAX_WORKSPACENAME_WIDTH+8);
	if (wwin->window_flags.omnipresent)
	    sprintf(entry->rtext, "[*]");
	else
	    sprintf(entry->rtext, "[%s]", 
		    scr->workspaces[wwin->frame->workspace]->name);
	
	if (wwin->flags.hidden) {
	    entry->flags.indicator_type = MI_HIDDEN;
	    entry->flags.indicator_on = 1;
	} else if (wwin->flags.miniaturized) {
	    entry->flags.indicator_type = MI_MINIWINDOW;
	    entry->flags.indicator_on = 1;
	} else if (wwin->flags.focused) {
	    entry->flags.indicator_type = MI_DIAMOND;
	    entry->flags.indicator_on = 1;
	} else if (wwin->flags.shaded) {
	    entry->flags.indicator_type = MI_SHADED;
	    entry->flags.indicator_on = 1;
	}

	wMenuRealize(switchmenu);
	checkVisibility = 1;
    } else {
	char *t;
	for (i=0; i<switchmenu->entry_no; i++) {
	    entry = switchmenu->entries[i];
	    /* this is the entry that was changed */
	    if (entry->clientdata == wwin) {
		switch (action) {
		 case ACTION_REMOVE:
		    wMenuRemoveItem(switchmenu, i);
		    wMenuRealize(switchmenu);
		    checkVisibility = 1;
		    break;
		    
		 case ACTION_CHANGE:
		    if (entry->text)
		      free(entry->text);
		    
		    if (wwin->frame->title)
		      sprintf(title, "%s", wwin->frame->title);
		    else
		      sprintf(title, "%s", DEF_WINDOW_TITLE);

		    t = ShrinkString(scr->menu_entry_font, title, 
				     MAX_WINDOWLIST_WIDTH);
		    entry->text = t;
		    /* fall through to update workspace number */
		 case ACTION_CHANGE_WORKSPACE:
		    if (entry->rtext) {
			if (wwin->window_flags.omnipresent)
			    sprintf(entry->rtext, "[*]");
			else
			    sprintf(entry->rtext, "[%s]", 
				    scr->workspaces[wwin->frame->workspace]->name);
		    }
		    wMenuRealize(switchmenu);
		    checkVisibility = 1;
		    break;


		 case ACTION_CHANGE_STATE:
		    if (wwin->flags.hidden) {
			entry->flags.indicator_type = MI_HIDDEN;
			entry->flags.indicator_on = 1;
		    } else if (wwin->flags.miniaturized) {
			entry->flags.indicator_type = MI_MINIWINDOW;
			entry->flags.indicator_on = 1;
		    } else if (wwin->flags.shaded && !wwin->flags.focused) {
			entry->flags.indicator_type = MI_SHADED;
			entry->flags.indicator_on = 1;
		    } else {
			entry->flags.indicator_on = wwin->flags.focused;
			entry->flags.indicator_type = MI_DIAMOND;
		    }
		    wMenuPaint(switchmenu);
		    break;
		}
		break;
	    }
	}
    }
    if (checkVisibility) {
        int tmp;

        tmp = switchmenu->frame->top_width + 5;
        /* if menu got unreachable, bring it to a visible place */
        if (switchmenu->frame_x < tmp - (int)switchmenu->frame->core->width) {
            wMenuMove(switchmenu, tmp - (int)switchmenu->frame->core->width,
                      switchmenu->frame_y, False);
        }
    }
    wMenuPaint(switchmenu);
}



void
UpdateSwitchMenuWorkspace(WScreen *scr, int workspace)
{
    WMenu *menu = scr->switch_menu;
    int i;
    WWindow *wwin;
    
    if (!menu)
	return;
    
    for (i=0; i<menu->entry_no; i++) {
	wwin = (WWindow*)menu->entries[i]->clientdata;

	if (wwin->frame->workspace==workspace 
	    && !wwin->window_flags.omnipresent) {
	    if (wwin->window_flags.omnipresent)
		sprintf(menu->entries[i]->rtext, "[*]");
	    else
		sprintf(menu->entries[i]->rtext, "[%s]", 
			scr->workspaces[wwin->frame->workspace]->name);
	    menu->flags.realized = 0;
	}
    }
    if (!menu->flags.realized)
	wMenuRealize(menu);
}
