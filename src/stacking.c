/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "WindowMaker.h"
#include "screen.h"
#include "window.h"
#include "funcs.h"
#include "actions.h"
#include "properties.h"
#include "stacking.h"
#ifdef KWM_HINTS
#include "kwm.h"
#endif

/*** Global Variables ***/
extern XContext wStackContext;

extern WPreferences wPreferences;


/*
 *---------------------------------------------------------------------- 
 * RemakeStackList--
 * 	Remakes the stacking_list for the screen, getting the real
 * stacking order from the server and reordering windows that are not
 * in the correct stacking.
 * 
 * Side effects:
 * 	The stacking order list and the actual window stacking 
 * may be changed (corrected)
 * 
 *---------------------------------------------------------------------- 
 */
void
RemakeStackList(WScreen *scr)
{
    Window *windows;
    unsigned int nwindows;
    Window junkr, junkp;
    WCoreWindow *frame;
    WCoreWindow *onbotw[MAX_WINDOW_LEVELS];
    int level;
    int i, c;

    if (!XQueryTree(dpy, scr->root_win, &junkr, &junkp, &windows, &nwindows)) {
	wwarning(_("could not get window list!!"));
	return;
    } else {
	for (i=0; i<MAX_WINDOW_LEVELS; i++) {
	    scr->stacking_list[i] = NULL;
	    onbotw[i] = NULL;
/*	    scr->window_level_count[i] = 0;*/
	}
	/* verify list integrity */
	c=0;
	for (i=0; i<nwindows; i++) {
	    if (XFindContext(dpy, windows[i], wStackContext, (XPointer*)&frame)
		==XCNOENT) {
		continue;
	    }
	    if (!frame) continue;
	    c++;
	    level = frame->stacking->window_level;
	    if (onbotw[level])
	      onbotw[level]->stacking->above = frame;
	    frame->stacking->under = onbotw[level];
	    frame->stacking->above = NULL;
	    onbotw[level] = frame;
/*	    scr->window_level_count[level]++;*/
        }
        XFree(windows);
#ifdef DEBUG
	if (c!=scr->window_count) {
	    puts("Found different number of windows than in window lists!!!");
	}
#endif
	scr->window_count = c;
    }
    /* now, just concatenate the lists */
    for (i=0; i<MAX_WINDOW_LEVELS; i++) {
	scr->stacking_list[i] = onbotw[i];
	if (onbotw[i])
	  onbotw[i]->stacking->above = NULL;
    }
    
    CommitStacking(scr);
}

/*
 *----------------------------------------------------------------------
 * CommitStacking--
 * 	Reorders the actual window stacking, so that it has the stacking
 * order in the internal window stacking lists. It does the opposite
 * of RemakeStackList().
 * 
 * Side effects:
 * 	Windows may be restacked.
 *---------------------------------------------------------------------- 
 */
void
CommitStacking(WScreen *scr)
{
    WCoreWindow *tmp;
    int nwindows;
    Window *windows;
    int i, level;
    
    nwindows = scr->window_count;
    windows = wmalloc(sizeof(Window)*nwindows);
    i=0;
    for (level=MAX_WINDOW_LEVELS-1; level>=0; level--) {
	tmp = scr->stacking_list[level];
	while (tmp) {
#ifdef DEBUG
	    if (i>=nwindows) {
		puts("Internal inconsistency! window_count is incorrect!!!");
		printf("window_count says %i windows\n", nwindows);
		free(windows);
		return;
	    }
#endif
	    windows[i++] = tmp->window;
	    tmp = tmp->stacking->under;
	}
    }
    XRestackWindows(dpy, windows, i);
    free(windows);
    
#ifdef KWM_HINTS
    wKWMBroadcastStacking(scr);
#endif
}

/*
 *----------------------------------------------------------------------
 * moveFrameToUnder--
 * 	Reestacks windows so that "frame" is under "under".
 * 
 * Returns:
 *	None
 * 
 * Side effects:
 * 	Changes the stacking order of frame.
 *----------------------------------------------------------------------
 */
static void
moveFrameToUnder(WCoreWindow *under, WCoreWindow *frame)
{
    Window wins[2];

    wins[0] = under->window;
    wins[1] = frame->window;
    XRestackWindows(dpy, wins, 2);

#ifdef KWM_HINTS
    wKWMBroadcastStacking(under->screen_ptr);
#endif
}

/*
 *----------------------------------------------------------------------
 * wRaiseFrame--
 * 	Raises a frame taking the window level and the on_top flag
 * into account. 
 * 
 * Returns:
 * 	None
 * 
 * Side effects:
 * 	Window stacking order and window list is changed.
 * 
 *---------------------------------------------------------------------- 
 */
void 
wRaiseFrame(WCoreWindow *frame)
{
    WCoreWindow *wlist = frame, *wlist_above;
    int level = frame->stacking->window_level;
    int i;

    /* already on top */
    if (frame->stacking->above == NULL) {
	return;
    }

    /* insert on top of other windows */
#if 1
    while (wlist) {
	if (wlist == (wlist_above = wlist->stacking->above)) {
	    wwarning("You just found a bug in wmaker. Please try to figure what type of raising/lowering operations you did with which applications and report. Please give complete information about how to reproduce it.");
	    break;
	} else {
	    wlist=wlist_above;
	}
    }
#else
    while (wlist)
	wlist = wlist->stacking->above;
#endif
    /* window is inserted before the point found */
    if (wlist==NULL) {
	/* top most window (last on the list) */
	if (frame->stacking->under)
	  frame->stacking->under->stacking->above = frame->stacking->above;
	if (frame->stacking->above)
	  frame->stacking->above->stacking->under = frame->stacking->under;
	
	frame->stacking->above = NULL;
	frame->stacking->under = frame->screen_ptr->stacking_list[level];
	frame->screen_ptr->stacking_list[level]->stacking->above=frame;
	frame->screen_ptr->stacking_list[level] = frame;
    } else if (frame!=wlist) {
	if (frame->stacking->under)
	  frame->stacking->under->stacking->above = frame->stacking->above;
	if (frame->stacking->above)
	  frame->stacking->above->stacking->under = frame->stacking->under;
	
	frame->stacking->above = wlist;
	frame->stacking->under = wlist->stacking->under;
	if (wlist->stacking->under)
	  wlist->stacking->under->stacking->above = frame;
	wlist->stacking->under = frame;
    }
#ifdef removed
    if (wPreferences.on_top_transients)
#endif
    {
	/* raise transients under us from bottom to top 
	 * so that the order is kept */
	again:
	wlist = frame->stacking->under;
	while (wlist && wlist->stacking->under)
	  wlist = wlist->stacking->under;
	while (wlist && wlist!=frame) {
	    if (wlist->stacking->child_of == frame) {
		wRaiseFrame(wlist);
		goto again;
	    }
	    wlist = wlist->stacking->above;
	}
# if 0
	again:
	wlist = frame->stacking->under;
	while (wlist) {
	    if (wlist->stacking->child_of == frame) {
		/* transient for us */
		wRaiseFrame(wlist);
		goto again; /* need this or we'll get in a loop */
	    } 
	    wlist = wlist->stacking->under;
	}
#endif
    }
    /* try to optimize things a little */
    if (frame->stacking->above == NULL) {
	WCoreWindow *above=NULL;

	for (i=level+1; i<MAX_WINDOW_LEVELS; i++) {
	    if (frame->screen_ptr->stacking_list[i]!=NULL) {
		/* can't optimize */
		above = frame->screen_ptr->stacking_list[i];
		while (above->stacking->under)
		    above = above->stacking->under;
		break;
	    }
	}
	if (!above) {
	    XRaiseWindow(dpy, frame->window);
	} else {
	    moveFrameToUnder(above, frame);
	}
    } else {
	moveFrameToUnder(frame->stacking->above, frame);
    }
#ifdef KWM_HINTS
    {
	WWindow *wwin = wWindowFor(frame->window);

	if (wwin)
	    wKWMSendEventMessage(wwin, WKWMRaiseWindow);
    }
#endif
}


void
wRaiseLowerFrame(WCoreWindow *frame)
{
    if (!frame->stacking->above
	||(frame->stacking->window_level
	   !=frame->stacking->above->stacking->window_level)) {

	wLowerFrame(frame);
    } else {
	WCoreWindow *scan = frame->stacking->above;
	WWindow *frame_wwin = (WWindow*) frame->descriptor.parent;

	while (scan) {

	    if (scan->descriptor.parent_type == WCLASS_WINDOW) {
		WWindow *scan_wwin = (WWindow*) scan->descriptor.parent;
		
		if (wWindowObscuresWindow(scan_wwin, frame_wwin)
		    && scan_wwin->flags.mapped) {
		    break;
		}
	    }
	    scan = scan->stacking->above;
	}

	if (scan) {
	    wRaiseFrame(frame);
	} else {
	    wLowerFrame(frame);
	}
    }
}


void 
wLowerFrame(WCoreWindow *frame)
{
    WScreen *scr=frame->screen_ptr;
    WCoreWindow *prev, *wlist=frame;
    int level = frame->stacking->window_level;
    int i;

    /* already in bottom */
    if (wlist->stacking->under==NULL) {
	return;
    }
#ifdef removed
    if (wPreferences.on_top_transients &&
	wlist->stacking->under==wlist->stacking->child_of) {
	return;
    }
#else
    if (wlist->stacking->under==wlist->stacking->child_of) {
	return;
    }
#endif
    prev = wlist;
    /* remove from the list */
    if (scr->stacking_list[level] == frame) {
	/* it was the top window */
	scr->stacking_list[level] = frame->stacking->under;
	scr->stacking_list[level]->stacking->above = NULL;
    } else {
	if (frame->stacking->under)
	  frame->stacking->under->stacking->above = frame->stacking->above;
	if (frame->stacking->above)
	  frame->stacking->above->stacking->under = frame->stacking->under;
    }
    wlist = scr->stacking_list[level];
    /* look for place to put this window */
#ifdef removed
    if (wPreferences.on_top_transients)
#endif
    {
	WCoreWindow *owner = frame->stacking->child_of;
	
	if (owner != wlist) {
	    while (wlist->stacking->under) {
		/* if this is a transient, it should not be placed under 
		 * it's owner */
		if (owner == wlist->stacking->under)
		  break;
		wlist = wlist->stacking->under;
	    }
	}
    } 
#ifdef removed
    else {
	while (wlist->stacking->under) {
	    wlist = wlist->stacking->under;
	}
    }
#endif
    /* insert under the place found */
    frame->stacking->above = wlist;
    frame->stacking->under = wlist->stacking->under;
    if (wlist->stacking->under)
      wlist->stacking->under->stacking->above = frame;
    wlist->stacking->under = frame;

    /* try to optimize things a little */
    if (frame->stacking->above == NULL) {
	WCoreWindow *above = NULL;
	for (i=level-1; i>=0; i--) {
	    if (scr->stacking_list[i]!=NULL) {
		/* can't optimize */
		above = scr->stacking_list[i];
		while (above->stacking->under)
		  above = above->stacking->under;
		break;
	    }
	}
	if (!above) {
	    XLowerWindow(dpy, frame->window);
	} else {
	    moveFrameToUnder(above, frame);
	}
    } else {
	moveFrameToUnder(frame->stacking->above, frame);
    }
#ifdef KWM_HINTS
    {
	WWindow *wwin = wWindowFor(frame->window);

	if (wwin)
	    wKWMSendEventMessage(wwin, WKWMLowerWindow);
    }
#endif
}


/*
 *----------------------------------------------------------------------
 * AddToStackList--
 * 	Inserts the frame in the top of the stacking list. The
 * stacking precedence is obeyed.
 * 
 * Returns:
 * 	None
 * 
 * Side effects:
 * 	The frame is added to it's screen's window list.
 *---------------------------------------------------------------------- 
 */
void
AddToStackList(WCoreWindow *frame)
{
    WCoreWindow *prev, *tmpw, *wlist;
    int index = frame->stacking->window_level;

    frame->screen_ptr->window_count++;
/*    frame->screen_ptr->window_level_count[index]++;*/
    XSaveContext(dpy, frame->window, wStackContext, (XPointer)frame);
    tmpw = frame->screen_ptr->stacking_list[index];
    if (!tmpw) {
	frame->screen_ptr->stacking_list[index] = frame;
	frame->stacking->above = NULL;
	frame->stacking->under = NULL;
	CommitStacking(frame->screen_ptr);	
	return;
    }
    prev = tmpw;
    /* check if this is a transient owner */
#ifdef removed
    if (wPreferences.on_top_transients)
#endif
    {
	WCoreWindow *trans = NULL;
	  
	wlist = frame->screen_ptr->stacking_list[index];
	while (wlist) {
	    if (wlist->stacking->child_of == frame)
	      trans = wlist;
	    wlist = wlist->stacking->under;
	}
	
	frame->stacking->above = trans;
	if (trans) {
	    frame->stacking->under = trans->stacking->under;
	    if (trans->stacking->under) {
		trans->stacking->under->stacking->above = frame;
	    }
	    trans->stacking->under = frame;
	} else {
	    frame->stacking->under = tmpw;
	    tmpw->stacking->above = frame;
	    frame->screen_ptr->stacking_list[index] = frame;	    
	}
    }
#ifdef removed
    else {
	/* put on top of the stacking list */
	frame->stacking->above = NULL;
	frame->stacking->under = tmpw;
	tmpw->stacking->above = frame;
	frame->screen_ptr->stacking_list[index] = frame;
    }
#endif
    CommitStacking(frame->screen_ptr);
}


/*
 *----------------------------------------------------------------------
 * MoveInStackListAbove--
 * 	Moves the frame above "next".
 * 
 * Returns:
 * 	None
 * 
 * Side effects:
 * 	Stacking order may be changed.
 *      Window level for frame may be changed.
 *---------------------------------------------------------------------- 
 */
void
MoveInStackListAbove(WCoreWindow *next, WCoreWindow *frame)
{
    WCoreWindow *tmpw;
    int index;

    if (!next || frame->stacking->under == next)
        return;

    if (frame->stacking->window_level != next->stacking->window_level)
        ChangeStackingLevel(frame, next->stacking->window_level);

    index = frame->stacking->window_level;

    tmpw = frame->screen_ptr->stacking_list[index];
    if (tmpw == frame)
        frame->screen_ptr->stacking_list[index] = frame->stacking->under;
    if (frame->stacking->under)
        frame->stacking->under->stacking->above = frame->stacking->above;
    if (frame->stacking->above)
        frame->stacking->above->stacking->under = frame->stacking->under;
    if (next->stacking->above)
        next->stacking->above->stacking->under = frame;
    frame->stacking->under = next;
    frame->stacking->above = next->stacking->above;
    next->stacking->above = frame;
    if (tmpw == next)
        frame->screen_ptr->stacking_list[index] = frame;

    /* try to optimize things a little */
    if (frame->stacking->above == NULL) {
        WCoreWindow *above=NULL;
        int i;

        for (i=index+1; i<MAX_WINDOW_LEVELS; i++) {
            if (frame->screen_ptr->stacking_list[i]!=NULL) {
                /* can't optimize */
                above = frame->screen_ptr->stacking_list[i];
                while (above->stacking->under)
                    above = above->stacking->under;
                break;
            }
        }
        if (!above) {
            XRaiseWindow(dpy, frame->window);
        } else {
            moveFrameToUnder(above, frame);
        }
    } else {
        moveFrameToUnder(frame->stacking->above, frame);
    }
}


/*
 *----------------------------------------------------------------------
 * MoveInStackListUnder--
 * 	Moves the frame to under "prev".
 * 
 * Returns:
 * 	None
 * 
 * Side effects:
 * 	Stacking order may be changed.
 *      Window level for frame may be changed.
 *---------------------------------------------------------------------- 
 */
void
MoveInStackListUnder(WCoreWindow *prev, WCoreWindow *frame)
{
    WCoreWindow *tmpw;
    int index;

    if (!prev || frame->stacking->above == prev)
        return;

    if (frame->stacking->window_level != prev->stacking->window_level)
        ChangeStackingLevel(frame, prev->stacking->window_level);

    index = frame->stacking->window_level;

    tmpw = frame->screen_ptr->stacking_list[index];
    if (tmpw == frame)
        frame->screen_ptr->stacking_list[index] = frame->stacking->under;
    if (frame->stacking->under)
        frame->stacking->under->stacking->above = frame->stacking->above;
    if (frame->stacking->above)
        frame->stacking->above->stacking->under = frame->stacking->under;
    if (prev->stacking->under)
        prev->stacking->under->stacking->above = frame;
    frame->stacking->above = prev;
    frame->stacking->under = prev->stacking->under;
    prev->stacking->under = frame;
    moveFrameToUnder(prev, frame);
}


void
RemoveFromStackList(WCoreWindow *frame)
{
    int index = frame->stacking->window_level;

    if (XDeleteContext(dpy, frame->window, wStackContext)==XCNOENT) {
#ifdef DEBUG0
	wwarning("RemoveFromStackingList(): window not in list ");
#endif
	return;
    }
    /* remove from the window stack list */
    if (frame->stacking->under)
      frame->stacking->under->stacking->above = frame->stacking->above;
    if (frame->stacking->above)
      frame->stacking->above->stacking->under = frame->stacking->under;
    else /* this was the first window on the list */
      frame->screen_ptr->stacking_list[index] = frame->stacking->under;

    frame->screen_ptr->window_count--;
/*    frame->screen_ptr->window_level_count[index]--;*/
}


void
ChangeStackingLevel(WCoreWindow *frame, int new_level)
{
    int old_level;
    if (frame->stacking->window_level == new_level)
      return;
    old_level = frame->stacking->window_level;

    RemoveFromStackList(frame);
    frame->stacking->window_level = new_level;
    AddToStackList(frame);
    if (old_level > new_level) {
	wRaiseFrame(frame);
    } else {
	wLowerFrame(frame);
    }
}
