/* window.c - client window managing class
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
#ifdef KEEP_XKB_LOCK_STATUS
# include <X11/XKBlib.h>
#endif /* KEEP_XKB_LOCK_STATUS */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "WindowMaker.h"
#include "GNUstep.h"
#ifdef MWM_HINTS
# include "motif.h"
#endif
#include "wcore.h"
#include "framewin.h"
#include "texture.h"
#include "window.h"
#include "winspector.h"
#include "icon.h"
#include "properties.h"
#include "actions.h"
#include "client.h"
#include "funcs.h"
#include "keybind.h"
#include "stacking.h"
#include "defaults.h"
#include "workspace.h"

/****** Global Variables ******/

extern WShortKey wKeyBindings[WKBD_LAST];

#ifdef SHAPE
extern Bool wShapeSupported;
#endif

/* contexts */
extern XContext wWinContext;

/* cursors */
extern Cursor wCursor[WCUR_LAST];

/* protocol atoms */
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;

extern Atom _XA_WINDOWMAKER_STATE;

extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask

extern Time LastTimestamp;

/* superfluous... */
extern void DoWindowBirth(WWindow*);


/***** Local Stuff *****/


static WWindowState *windowState=NULL;



/* local functions */
static FocusMode getFocusMode(WWindow *wwin);

static int getSavedState(Window window, WSavedState **state);

static void setupGNUstepHints(WWindowAttributes *attribs, 
			      GNUstepWMAttributes *gs_hints);

#ifdef MWM_HINTS
static void setupMWMHints(WWindowAttributes *attribs, MWMHints *mwm_hints);
#endif

/* event handlers */


/* frame window (during window grabs) */
static void frameMouseDown(WObjDescriptor *desc, XEvent *event);

/* close button */
static void windowCloseClick(WCoreWindow *sender, void *data, XEvent *event);
static void windowCloseDblClick(WCoreWindow *sender, void *data, XEvent *event);

/* iconify button */
static void windowIconifyClick(WCoreWindow *sender, void *data, XEvent *event);


static void titlebarMouseDown(WCoreWindow *sender, void *data, XEvent *event);
static void titlebarDblClick(WCoreWindow *sender, void *data, XEvent *event);

static void resizebarMouseDown(WCoreWindow *sender, void *data, XEvent *event);






WWindow*
wWindowFor(Window window)
{
    WObjDescriptor *desc;

    if (window==None)
      return NULL;

    if (XFindContext(dpy, window, wWinContext, (XPointer*)&desc)==XCNOENT)
      return NULL;

    if (desc->parent_type==WCLASS_WINDOW)
      return desc->parent;
    else if (desc->parent_type==WCLASS_FRAME) {
	WFrameWindow *frame = (WFrameWindow*)desc->parent;
	if (frame->flags.is_client_window_frame)
	    return frame->child;
    }
    return NULL;
}


WWindow*
wWindowCreate()
{
    WWindow *wwin;
    
    wwin = wmalloc(sizeof(WWindow));
    wretain(wwin);

    memset(wwin, 0, sizeof(WWindow));

    wwin->client_descriptor.handle_mousedown = frameMouseDown;
    wwin->client_descriptor.parent = wwin;
    wwin->client_descriptor.self = wwin;
    wwin->client_descriptor.parent_type = WCLASS_WINDOW;
    return wwin;
}


void
wWindowDestroy(WWindow *wwin)
{
    int i;

    wwin->flags.destroyed = 1;

    for (i = 0; i < 4; i++) {
	if (wwin->screen_ptr->shortcutWindow[i] == wwin) {
	    wwin->screen_ptr->shortcutWindow[i] = NULL;
	}
    }

    if (wwin->normal_hints)
      free(wwin->normal_hints);
    
    if (wwin->wm_hints)
      XFree(wwin->wm_hints);
    
    if (wwin->wm_instance)
      XFree(wwin->wm_instance);

    if (wwin->wm_class)
      XFree(wwin->wm_class);

    if (wwin->wm_gnustep_attr)
      free(wwin->wm_gnustep_attr);

    if (wwin->cmap_windows)
      XFree(wwin->cmap_windows);

    XDeleteContext(dpy, wwin->client_win, wWinContext);

    if (wwin->frame)
	wFrameWindowDestroy(wwin->frame);

    if (wwin->icon) {
	RemoveFromStackList(wwin->icon->core);
        wIconDestroy(wwin->icon);
        if (wPreferences.auto_arrange_icons)
            wArrangeIcons(wwin->screen_ptr, True);
    }
    wrelease(wwin);
}




static void
setupGNUstepHints(WWindowAttributes *attribs, GNUstepWMAttributes *gs_hints)
{
    if (gs_hints->flags & GSWindowStyleAttr) {
	attribs->no_titlebar =
	    ((gs_hints->window_style & WMTitledWindowMask)?0:1);
	
	attribs->no_close_button = attribs->no_closable = 
	    ((gs_hints->window_style & WMClosableWindowMask)?0:1);
	
	attribs->no_miniaturize_button = attribs->no_miniaturizable =
	    ((gs_hints->window_style & WMMiniaturizableWindowMask)?0:1);
	
	attribs->no_resizebar = attribs->no_resizable =
	    ((gs_hints->window_style & WMResizableWindowMask)?0:1);
    } else {
	/* setup the defaults */
	attribs->no_titlebar = 0;
	attribs->no_closable = 0;
	attribs->no_miniaturizable = 0;
	attribs->no_resizable = 0;
	attribs->no_close_button = 0;
	attribs->no_miniaturize_button = 0;
	attribs->no_resizebar = 0;
    }
    
    if (gs_hints->extra_flags & GSNoApplicationIconFlag) {
	attribs->no_appicon = 1;
    }
}


#ifdef MWM_HINTS
static void 
setupMWMHints(WWindowAttributes *attribs, MWMHints *mwm_hints)
{

    /* 
     * We will ignore all decoration hints that have an equivalent as
     * functions, because wmaker does not distinguish decoration hints
     */
    
    if (mwm_hints->flags & MWM_HINTS_DECORATIONS) {
# ifdef DEBUG
	fprintf(stderr,"has decor hints [ ");
# endif
	attribs->no_titlebar = 1;
	attribs->no_close_button = 1;
	attribs->no_miniaturize_button = 1;
	attribs->no_resizebar = 1;
	
	if (mwm_hints->decorations & MWM_DECOR_ALL) {
# ifdef DEBUG
	    fprintf(stderr,"ALL ");
# endif
	    attribs->no_titlebar = 0;
	    attribs->no_close_button = 0;
	    attribs->no_closable = 0;
	    attribs->no_miniaturize_button = 0;
	    attribs->no_miniaturizable = 0;
	    attribs->no_resizebar = 0;
	    attribs->no_resizable = 0;
	}
/*
	if (mwm_hints->decorations & MWM_DECOR_BORDER) {
# ifdef DEBUG
	    fprintf(stderr,"(BORDER) ");
# endif
	}
 */

	if (mwm_hints->decorations & MWM_DECOR_RESIZEH) {
# ifdef DEBUG
	    fprintf(stderr,"RESIZEH ");
# endif
	    attribs->no_resizebar = 0;
	}

	if (mwm_hints->decorations & MWM_DECOR_TITLE) {
# ifdef DEBUG
	    fprintf(stderr,"TITLE+close ");
# endif
	    attribs->no_titlebar = 0;
	    attribs->no_close_button = 0;
	    attribs->no_closable = 0;
	}
/*
	if (mwm_hints->decorations & MWM_DECOR_MENU) {
# ifdef DEBUG
	    fprintf(stderr,"(MENU) ");
# endif
	}
 */

	if (mwm_hints->decorations & MWM_DECOR_MINIMIZE) {
# ifdef DEBUG
	    fprintf(stderr,"MINIMIZE ");
# endif
	    attribs->no_miniaturize_button = 0;
	    attribs->no_miniaturizable = 0;
	}
/*
	if (mwm_hints->decorations & MWM_DECOR_MAXIMIZE) {
# ifdef DEBUG
	    fprintf(stderr,"(MAXIMIZE) ");
# endif
	}
 */
# ifdef DEBUG
	fprintf(stderr,"]\n");
# endif
    }


    if (mwm_hints->flags & MWM_HINTS_FUNCTIONS) {
# ifdef DEBUG
	fprintf(stderr,"has function hints [ ");
# endif
	attribs->no_closable = 1;
	attribs->no_miniaturizable = 1;
	attribs->no_resizable = 1;
	
	if (mwm_hints->functions & MWM_FUNC_ALL) {
# ifdef DEBUG 
	    fprintf(stderr,"ALL ");
# endif
	    attribs->no_closable = 0;
	    attribs->no_miniaturizable = 0;
	    attribs->no_resizable = 0;
	}
	if (mwm_hints->functions & MWM_FUNC_RESIZE) {
# ifdef DEBUG 
	    fprintf(stderr,"RESIZE ");
# endif
	    attribs->no_resizable = 0;
	}
	/*
	if (mwm_hints->functions & MWM_FUNC_MOVE) {
# ifdef DEBUG 
	    fprintf(stderr,"(MOVE) ");
# endif
	}
	 */
	if (mwm_hints->functions & MWM_FUNC_MINIMIZE) {
# ifdef DEBUG 
	    fprintf(stderr,"MINIMIZE ");
# endif
	    attribs->no_miniaturizable = 0;
	}
	if (mwm_hints->functions & MWM_FUNC_MAXIMIZE) {
# ifdef DEBUG 
	    fprintf(stderr,"MAXIMIZE ");
	    /* a window must be resizable to be maximizable */
	    attribs->no_resizable = 0;
# endif
	}
	if (mwm_hints->functions & MWM_FUNC_CLOSE) {
# ifdef DEBUG 
	    fprintf(stderr,"CLOSE ");
# endif
	    attribs->no_closable = 0;
	}
# ifdef DEBUG 
	fprintf(stderr,"]\n");
# endif
    }
}
#endif /* MWM_HINTS */


void
wWindowCheckAttributeSanity(WWindow *wwin, WWindowAttributes *wflags)
{
    if (wflags->no_appicon)
	wflags->emulate_appicon = 0;

    if (wwin->main_window!=None) {
	WApplication *wapp = wApplicationOf(wwin->main_window);
	if (wapp && !wapp->flags.emulated)
	    wflags->emulate_appicon = 0;
    }
    
    if (wwin->transient_for!=None 
	&& wwin->transient_for!=wwin->screen_ptr->root_win)
	wflags->emulate_appicon = 0;
}



Bool
wWindowCanReceiveFocus(WWindow *wwin)
{
    if (!wwin->flags.mapped && !wwin->flags.shaded)
	return False;
    if (wwin->window_flags.no_focusable || wwin->flags.miniaturized)
	return False;
    if (wwin->frame->workspace != wwin->screen_ptr->current_workspace)
	return False;

    return True;
}


/*
 *----------------------------------------------------------------
 * wManageWindow--
 * 	reparents the window and allocates a descriptor for it.
 * Window manager hints and other hints are fetched to configure
 * the window decoration attributes and others. User preferences
 * for the window are used if available, to configure window 
 * decorations and some behaviour.
 * 
 * Returns:
 * 	the new window descriptor
 * 
 * Side effects:
 * 	The window is reparented and appropriate notification
 * is done to the client. Input mask for the window is setup.
 * 	The window descriptor is also associated with various window 
 * contexts and inserted in the head of the window list.
 * Event handler contexts are associated for some objects 
 * (buttons, titlebar and resizebar)
 * 
 * TODO:
 * 	Check if the window_flags setting is correct.
 *---------------------------------------------------------------- 
 */
WWindow*
wManageWindow(WScreen *scr, Window window)
{
    WWindow *wwin;
    int x, y;
    unsigned width, height;
    XWindowAttributes wattribs;    
    XSetWindowAttributes attribs;
    int iconic = 0;
    WWindowState *win_state;
#ifdef MWM_HINTS
    MWMHints *motif_hints = NULL;
#endif
    int window_level;
    int foo;
    int workspace = -1;
    char *title;

    /* mutex. */
    XGrabServer(dpy);
    XSync(dpy, 0);
    /* make sure the window is still there */
    if (!XGetWindowAttributes(dpy, window, &wattribs)) {
	XUngrabServer(dpy);
	return NULL;
    }
    wwin = wWindowCreate();

    XSaveContext(dpy, window, wWinContext, (XPointer)&wwin->client_descriptor);

#ifdef DEBUG
    printf("managing window %x\n", (unsigned)window);
#endif

#ifdef SHAPE
    if (wShapeSupported) {
	int junk;
	unsigned int ujunk;
	int b_shaped;
	
	XShapeSelectInput(dpy, window, ShapeNotifyMask);
	XShapeQueryExtents(dpy, window, &b_shaped, &junk, &junk, &ujunk, 
			   &ujunk, &junk, &junk, &junk, &ujunk, &ujunk);
	wwin->flags.shaped = b_shaped;
    }
#endif
    
    /*
     *--------------------------------------------------
     * 
     * Get hints and other information in properties
     *
     *-------------------------------------------------- 
     */
    wwin->wm_hints = XGetWMHints(dpy, window);
    PropGetWMClass(window, &wwin->wm_class, &wwin->wm_instance);

    /* setup descriptor */
    wwin->client_win = window;
    wwin->screen_ptr = scr;

    wwin->old_border_width = wattribs.border_width;

    attribs.event_mask = CLIENT_EVENTS;
    attribs.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
    attribs.save_under = False;
    XChangeWindowAttributes(dpy, window, CWEventMask|CWDontPropagate
			    |CWSaveUnder, &attribs);    
    XSetWindowBorderWidth(dpy, window, 0);

    /* fill in property/hint data */
    if (!wFetchName(dpy, window, &title)) {
	title = NULL;
    }
    /* get hints from GNUstep app */
    if (!PropGetGNUstepWMAttr(window, &wwin->wm_gnustep_attr)) {
	wwin->wm_gnustep_attr=NULL;
    }
    
#ifdef MWM_HINTS
    if (!PropGetMotifWMHints(window, &motif_hints))
	motif_hints = NULL;
#endif /* MWM_HINTS */

    if (!PropGetClientLeader(window, &wwin->client_leader)) {
	wwin->client_leader = None;
    } else {
	wwin->main_window = wwin->client_leader;
    }

    if (wwin->wm_hints)
	XFree(wwin->wm_hints);
	
    wwin->wm_hints = XGetWMHints(dpy, window);

    if (wwin->wm_hints)  {
	if ((wwin->wm_hints->flags&StateHint) 
	    && (wwin->wm_hints->initial_state == IconicState)) {
	    iconic = 1;
	    /* don't do iconify animation */
	    wwin->flags.skip_next_animation = 1;
	}
      
	if (wwin->wm_hints->flags & WindowGroupHint) {
	    wwin->group_id = wwin->wm_hints->window_group;
	    /* window_group has priority over CLIENT_LEADER */
	    wwin->main_window = wwin->group_id;
	} else {
	    wwin->group_id = None;
	}

	if (wwin->wm_hints->flags & UrgencyHint)
	  wwin->flags.urgent = 1;
    } else {
	wwin->group_id = None;
    }

    PropGetProtocols(window, &wwin->protocols);
    
    if (!XGetTransientForHint(dpy, window, &wwin->transient_for)) {
	wwin->transient_for = None;
    } else {
	if (wwin->transient_for==None || wwin->transient_for==window) {
	    wwin->transient_for = scr->root_win;
	} else {
	    WWindow *owner;
	    owner = wWindowFor(wwin->transient_for);
	    if (owner && owner->main_window!=None) {
		wwin->main_window = owner->main_window;
            } /*else {
		wwin->main_window = None;
            }*/

	    /* don't let transients start miniaturized if their owners
	     * are not */
	    if (owner && !owner->flags.miniaturized && iconic) {
		iconic = 0;
		if (wwin->wm_hints)
		    wwin->wm_hints->initial_state = NormalState;
	    }
	}
    }

    /* guess the focus mode */
    wwin->focus_mode = getFocusMode(wwin);

    /* get geometry stuff */
    wClientGetNormalHints(wwin, &wattribs, True, &x, &y, &width, &height);

    /* get colormap windows */
    GetColormapWindows(wwin);


    /*
     *--------------------------------------------------
     * 
     * Setup the decoration/window attributes and
     * geometry
     * 
     *--------------------------------------------------
     */
    /* sets global default stuff */
    wDefaultFillAttributes(scr, wwin->wm_instance, wwin->wm_class, 
			   &wwin->window_flags, True);
    /*
     * Decoration setting is done in this precedence (lower to higher)
     * - use default in the resource database
     * - guess some settings
     * - use GNUstep/external window attributes
     * - set hints specified for the app in the resource DB
     */
    wwin->window_flags.broken_close = 0;
    
    if (wwin->protocols.DELETE_WINDOW)
	wwin->window_flags.kill_close = 0;
    else
	wwin->window_flags.kill_close = 1;

    /* transients can't be iconified or maximized */
    if (wwin->transient_for) {
	wwin->window_flags.no_miniaturizable = 1;
	wwin->window_flags.no_miniaturize_button = 1;
#ifdef DEBUG
	printf("%x is transient for %x\n", (unsigned)window, 
	       (unsigned)wwin->transient_for);
#endif
    }

    /* if the window can't be resized, remove the resizebar */
    if (wwin->normal_hints->flags & (PMinSize|PMaxSize)
	&& (wwin->normal_hints->min_width==wwin->normal_hints->max_width)
	&& (wwin->normal_hints->min_height==wwin->normal_hints->max_height)) {
	wwin->window_flags.no_resizable = 1;
	wwin->window_flags.no_resizebar = 1;
    }
    
    /* set GNUstep window attributes */
    if (wwin->wm_gnustep_attr) {
	setupGNUstepHints(&wwin->window_flags, wwin->wm_gnustep_attr);
	if (wwin->wm_gnustep_attr->flags & GSWindowLevelAttr) {
	    window_level = wwin->wm_gnustep_attr->window_level;
	} else {
	    /* setup defaults */
	    window_level = WMNormalWindowLevel;
	}
    } else {
#ifdef MWM_HINTS
	if (motif_hints) {
	    setupMWMHints(&wwin->window_flags, motif_hints);
	}
#endif /* MWM_HINTS */
	if (wwin->window_flags.floating)
	    window_level = WMFloatingWindowLevel;
	else
	    window_level = WMNormalWindowLevel;
    }
#ifdef MWM_HINTS
    if (motif_hints)
	free(motif_hints);
#endif

    /*
     * Set attributes specified only for that window/class.
     * This might do duplicate work with the 1st wDefaultFillAttributes().
     * TODO: Do something about that.
     */
    wDefaultFillAttributes(scr, wwin->wm_instance, wwin->wm_class, 
			   &wwin->window_flags, False);


    /*
     * Sanity checks for attributes that depend on other attributes 
     */
    wWindowCheckAttributeSanity(wwin, &wwin->window_flags);

    wwin->window_flags.no_shadeable = wwin->window_flags.no_titlebar;

    /*
     * Make broken apps behave as a nice app.
     */
    if (wwin->window_flags.emulate_appicon) {
	wwin->main_window = wwin->client_win;
    }

    /*
     *------------------------------------------------------------ 
     * 
     * Setup the initial state of the window
     *
     *------------------------------------------------------------
     */
    if (wwin->window_flags.start_miniaturized
	&& !wwin->window_flags.no_miniaturizable) {
	wwin->flags.miniaturized = 1;
        iconic = 1;
    }

    /* if there is a saved state, restore it */
    win_state = NULL;
    if (wwin->main_window!=None/* && wwin->main_window!=window*/) {
        win_state = (WWindowState*)wWindowGetSavedState(wwin->main_window);
    } else {
        win_state = (WWindowState*)wWindowGetSavedState(window);
    }
    if (win_state && !(wwin->wm_hints && wwin->wm_hints->flags&StateHint &&
                       wwin->wm_hints->initial_state==WithdrawnState)) {
        if (win_state->state->hidden>0)
            wwin->flags.hidden = win_state->state->hidden;
        if (win_state->state->shaded>0 && !wwin->window_flags.no_shadeable)
            wwin->flags.shaded = win_state->state->shaded;
        if (win_state->state->miniaturized>0 &&
            !wwin->window_flags.no_miniaturizable) {
            wwin->flags.miniaturized = win_state->state->miniaturized;
            iconic = 1;
        }
        if (!wwin->window_flags.omnipresent) {
            int w = wDefaultGetStartWorkspace(scr, wwin->wm_instance,
                                              wwin->wm_class);
            if (w < 0 || w >= scr->workspace_count) {
                workspace = win_state->state->workspace;
                if (workspace>=scr->workspace_count)
                    workspace = scr->current_workspace;
            } else {
                workspace = w;
            }
        } else {
            workspace = scr->current_workspace;
        }
    }

    /* if we're restarting, restore saved state. This will overwrite previous */
    {
	WSavedState *wstate;

	if (getSavedState(window, &wstate)) {
	    wwin->flags.shaded = wstate->shaded;
	    wwin->flags.hidden = wstate->hidden;
	    wwin->flags.miniaturized = 0;
	    workspace = wstate->workspace;
	    free(wstate);
	}
    }


    /* set workspace on which the window starts */    
    if (workspace >= 0) {
	if (workspace > scr->workspace_count-1) {
	    wWorkspaceMake(scr, workspace - scr->workspace_count + 1);
	    workspace = scr->workspace_count - 1;
	}
    } else {
	int w;

	w = wDefaultGetStartWorkspace(scr, wwin->wm_instance, wwin->wm_class);
        if (w>=0 && w<scr->workspace_count && !wwin->window_flags.omnipresent) {
            workspace = w;
        } else {
            workspace = scr->current_workspace;
        }
    }

    /* setup window geometry */
    if (win_state && win_state->state->use_geometry) {
        width = win_state->state->w;
        height = win_state->state->h;
    }
    wWindowConstrainSize(wwin, &width, &height);

    /* do not ask for window placement if the window is
     * transient, during startup, if the initial workspace is another one
     * or if the window wants to
     * start iconic.
     * If geometry was saved, restore it. */
    if (win_state && win_state->state->use_geometry) {
        x = win_state->state->x;
        y = win_state->state->y;
    } else if (wwin->transient_for==None && !scr->flags.startup &&
               workspace==scr->current_workspace && !iconic &&
               !(wwin->normal_hints->flags & (USPosition|PPosition))) {
        PlaceWindow(wwin, &x, &y, width, height);
    } 
    
    if (wwin->window_flags.dont_move_off)
	wScreenBringInside(scr, &x, &y, width, height);
    
    /* 
     *--------------------------------------------------
     * 
     * Create frame, borders and do reparenting
     *
     *--------------------------------------------------
     */
    
    foo = WFF_LEFT_BUTTON | WFF_RIGHT_BUTTON;
    if (!wwin->window_flags.no_titlebar)
	foo |= WFF_TITLEBAR;
    if (!wwin->window_flags.no_resizebar)
	foo |= WFF_RESIZEBAR;

    wwin->frame = wFrameWindowCreate(scr, window_level, 
				     x, y, width, height, foo,
				     scr->window_title_texture,
				     (WTexture**)scr->resizebar_texture,
				     scr->window_title_pixel, 
				     &scr->window_title_gc, 
				     &scr->title_font);

    wwin->frame->flags.is_client_window_frame = 1;
    wwin->frame->flags.justification = wPreferences.title_justification;

    /* setup button images */
    wWindowUpdateButtonImages(wwin);
    
    /* hide unused buttons */
    foo = 0;
    if (wwin->window_flags.no_close_button)
	foo |= WFF_RIGHT_BUTTON;
    if (wwin->window_flags.no_miniaturize_button)
	foo |= WFF_LEFT_BUTTON;
    if (foo!=0)
	wFrameWindowHideButton(wwin->frame, foo);
    
    
    wwin->frame->child = wwin;

    wFrameWindowChangeTitle(wwin->frame, title ? title : DEF_WINDOW_TITLE);
    if (title)
	XFree(title);
    
    wwin->frame->workspace = workspace;
    
    wwin->frame->on_click_left = windowIconifyClick;
    
    wwin->frame->on_click_right = windowCloseClick;
    wwin->frame->on_dblclick_right = windowCloseDblClick;
    
    wwin->frame->on_mousedown_titlebar = titlebarMouseDown;
    wwin->frame->on_dblclick_titlebar = titlebarDblClick;
    
    wwin->frame->on_mousedown_resizebar = resizebarMouseDown;

    XReparentWindow(dpy, wwin->client_win, wwin->frame->core->window,
		    0, wwin->frame->top_width);

#if 0
    {
	int gx, gy;

	wClientGetGravityOffsets(wwin, &gx, &gy);
	/* set the position of the frame on screen */
	x += gx * FRAME_BORDER_WIDTH;
	y += gy * FRAME_BORDER_WIDTH;
	/* if gravity is to the south, account for the border sizes */
	if (gy > 0)
	    y -= wwin->frame->top_width + wwin->frame->bottom_width;
    }
#endif
    /* 
     * wWindowConfigure() will init the client window's size
     * (wwin->client.{width,height}) and all other geometry
     * related variables (frame_x,frame_y)
     */
    wWindowConfigure(wwin, x, y, width, height);

    /*
     *--------------------------------------------------
     * 
     * Setup descriptors and save window to internal
     * lists
     *
     *-------------------------------------------------- 
     */

    if (wwin->main_window!=None) {
	WApplication *app;
	WWindow *leader;

	/* Leader windows do not necessary set themselves as leaders.
	 * If this is the case, point the leader of this window to
	 * itself */
	leader = wWindowFor(wwin->main_window);
	if (leader && leader->main_window==None) {
	    leader->main_window = leader->client_win;
	}

	app = wApplicationCreate(scr, wwin->main_window);
        if (app) {
            app->last_workspace = workspace;

            /*
             * Do application specific stuff, like setting application
             * wide attributes.
             */

            if (wwin->flags.hidden) {
                /* if the window was set to hidden because it was hidden
                 * in a previous incarnation and that state was restored */
                app->flags.hidden = 1;
            }

            if (app->flags.hidden) {
                wwin->flags.hidden = 1;
            }
        }
    }

    /* setup the frame descriptor */
    wwin->frame->core->descriptor.handle_mousedown = frameMouseDown;
    wwin->frame->core->descriptor.parent = wwin;
    wwin->frame->core->descriptor.parent_type = WCLASS_WINDOW;

    /* don't let windows go away if we die */
    XAddToSaveSet(dpy, window);

    XLowerWindow(dpy, window);

    /* if window is in this workspace and should be mapped, then  map it */
    if (!iconic && (workspace == scr->current_workspace
		    || wwin->window_flags.omnipresent)
        && !wwin->flags.hidden
	&& !(wwin->wm_hints && (wwin->wm_hints->flags & StateHint)
	     && wwin->wm_hints->initial_state == WithdrawnState)) {
        /* The following "if" is to avoid crashing of clients that expect
         * WM_STATE set before they get mapped. Else WM_STATE is set later,
         * after the return from this function.
         */
        if (wwin->wm_hints && (wwin->wm_hints->flags & StateHint)) {
            wClientSetState(wwin, wwin->wm_hints->initial_state, None);
        } else {
            wClientSetState(wwin, NormalState, None);
        }
	if (wPreferences.superfluous && !wPreferences.no_animations
	    && !scr->flags.startup && wwin->transient_for==None
	    /* 
	     * The brain damaged idiotic non-click to focus modes will
	     * have trouble with this because:
	     * 
	     * 1. window is created and mapped by the client
	     * 2. window is mapped by wmaker in small size
	     * 3. window is animated to grow to normal size
	     * 4. this function returns to normal event loop
	     * 5. eventually, the EnterNotify event that would trigger
	     * the window focusing (if the mouse is over that window)
	     * will be processed by wmaker.
	     * But since this event will be rather delayed 
	     * (step 3 has a large delay) the time when the event ocurred
	     * and when it is processed, the client that owns that window
	     * will reject the XSetInputFocus() for it.
	     */
	    && (wPreferences.focus_mode==WKF_CLICK
		|| wPreferences.auto_focus)) {
	    DoWindowBirth(wwin);
	}
	XMapSubwindows(dpy, wwin->frame->core->window);
	wWindowMap(wwin);
    } else {
	XMapSubwindows(dpy, wwin->frame->core->window);
    }

    /* setup stacking descriptor */
    if (wPreferences.on_top_transients && wwin->transient_for!=None
	&& wwin->transient_for!=scr->root_win) {
	WWindow *tmp;
	tmp = wWindowFor(wwin->transient_for);
	if (tmp)
	    wwin->frame->core->stacking->child_of = tmp->frame->core;
    } else {
	wwin->frame->core->stacking->child_of = NULL;
    }


    if (!scr->focused_window) {
	/* first window on the list */
	wwin->next = NULL;
	wwin->prev = NULL;
	scr->focused_window = wwin;
    } else {
	WWindow *tmp;

	/* add window at beginning of focus window list */
	tmp = scr->focused_window;
	while (tmp->prev)
	  tmp = tmp->prev;
	tmp->prev = wwin;
	wwin->next = tmp;
	wwin->prev = NULL;
    }


    XUngrabServer(dpy);

    /*
     *--------------------------------------------------
     *
     * Final preparations before window is ready to go
     *
     *--------------------------------------------------
     */

    wFrameWindowChangeState(wwin->frame, WS_UNFOCUSED);


    if (!iconic && workspace == scr->current_workspace) {
	WWindow *tmp = wWindowFor(wwin->transient_for);

	if ((tmp && tmp->flags.focused) || wPreferences.auto_focus)
	    wSetFocusTo(scr, wwin);
    } else {
	wwin->flags.ignore_next_unmap = 1;
    }
    wWindowResetMouseGrabs(wwin);

    if (!wwin->window_flags.no_bind_keys) {
	wWindowSetKeyGrabs(wwin);
    }

    /*
     * Prevent window withdrawal when getting the 
     * unmap notifies generated during reparenting
     */
    wwin->flags.mapped=0;

    XSync(dpy, 0);

    wColormapInstallForWindow(wwin->screen_ptr, scr->cmap_window);

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_ADD);
    /*
     *--------------------------------------------------
     *  Cleanup temporary stuff
     *--------------------------------------------------
     */

    if (win_state)
        wWindowDeleteSavedState(win_state);

    return wwin;
}





WWindow*
wManageInternalWindow(WScreen *scr, Window window, Window owner,
		      char *title, int x, int y, int width, int height)
{
    WWindow *wwin;
    int foo;

    wwin = wWindowCreate();

    wwin->flags.internal_window = 1;
    wwin->window_flags.omnipresent = 1;
    wwin->window_flags.no_shadeable = 1;
    wwin->window_flags.no_resizable = 1;
    wwin->window_flags.no_miniaturizable = 1;
    
    wwin->focus_mode = WFM_PASSIVE;
    
    wwin->client_win = window;
    wwin->screen_ptr = scr;

    wwin->transient_for = owner;
    
    wwin->client.x = x;
    wwin->client.y = y;
    wwin->client.width = width;
    wwin->client.height = height;
   
    wwin->frame_x = wwin->client.x;
    wwin->frame_y = wwin->client.y;

    
    foo = WFF_RIGHT_BUTTON;
    foo |= WFF_TITLEBAR;

    wwin->frame = wFrameWindowCreate(scr, WMFloatingWindowLevel,
				     wwin->frame_x, wwin->frame_y,
				     width, height, foo,
				     scr->window_title_texture,
				     (WTexture**)scr->resizebar_texture,
				     scr->window_title_pixel, 
				     &scr->window_title_gc, 
				     &scr->title_font);
    
    XSaveContext(dpy, window, wWinContext, (XPointer)&wwin->client_descriptor);

    wwin->frame->flags.is_client_window_frame = 1;
    wwin->frame->flags.justification = wPreferences.title_justification;

    wFrameWindowChangeTitle(wwin->frame, title);

    /* setup button images */
    wWindowUpdateButtonImages(wwin);

    /* hide buttons */
    wFrameWindowHideButton(wwin->frame, WFF_RIGHT_BUTTON);
    
    wwin->frame->child = wwin;
    
    wwin->frame->workspace = wwin->screen_ptr->current_workspace;
        
    wwin->frame->on_click_right = windowCloseClick;
    
    wwin->frame->on_mousedown_titlebar = titlebarMouseDown;
    wwin->frame->on_dblclick_titlebar = titlebarDblClick;
    
    wwin->frame->on_mousedown_resizebar = resizebarMouseDown;

    wwin->client.y += wwin->frame->top_width;
    XReparentWindow(dpy, wwin->client_win, wwin->frame->core->window, 
		    0, wwin->frame->top_width);

    wWindowConfigure(wwin, wwin->frame_x, wwin->frame_y, 
		     wwin->client.width, wwin->client.height);

    /* setup the frame descriptor */
    wwin->frame->core->descriptor.handle_mousedown = frameMouseDown;
    wwin->frame->core->descriptor.parent = wwin;
    wwin->frame->core->descriptor.parent_type = WCLASS_WINDOW;


    XLowerWindow(dpy, window);
    XMapSubwindows(dpy, wwin->frame->core->window);
    
    /* setup stacking descriptor */
    if (wPreferences.on_top_transients && wwin->transient_for!=None
	&& wwin->transient_for!=scr->root_win) {
	WWindow *tmp;
	tmp = wWindowFor(wwin->transient_for);
	if (tmp)
	    wwin->frame->core->stacking->child_of = tmp->frame->core;
    } else {
	wwin->frame->core->stacking->child_of = NULL;
    }
    

    if (!scr->focused_window) {
	/* first window on the list */
	wwin->next = NULL;
	wwin->prev = NULL;
	scr->focused_window = wwin;
    } else {
	WWindow *tmp;

	/* add window at beginning of focus window list */
	tmp = scr->focused_window;
	while (tmp->prev)
	  tmp = tmp->prev;
	tmp->prev = wwin;
	wwin->next = tmp;
	wwin->prev = NULL;
    }

    wFrameWindowChangeState(wwin->frame, WS_UNFOCUSED);

/*    if (wPreferences.auto_focus)*/
	wSetFocusTo(scr, wwin);

    wWindowResetMouseGrabs(wwin);

    wWindowSetKeyGrabs(wwin);

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_ADD);    
    
    return wwin;
}


/*
 *---------------------------------------------------------------------- 
 * wUnmanageWindow--
 * 	Removes the frame window from a window and destroys all data
 * related to it. The window will be reparented back to the root window
 * if restore is True.
 * 
 * Side effects:
 * 	Everything related to the window is destroyed and the window
 * is removed from the window lists. Focus is set to the previous on the
 * window list.
 *---------------------------------------------------------------------- 
 */
void
wUnmanageWindow(WWindow *wwin, int restore)
{
    WCoreWindow *frame = wwin->frame->core;
    WWindow *owner;
    WWindow *newFocusedWindow;
    int wasNotFocused;
    WScreen *scr = wwin->screen_ptr;

    /* First close attribute editor window if open */
    if (wwin->flags.inspector_open) {
        WWindow *pwin = wwin->inspector->frame; /* the inspector window */
        (*pwin->frame->on_click_right)(NULL, pwin, NULL);
    }
    
    /* Close window menu if it's open for this window */
    if (wwin->flags.menu_open_for_me) {
	CloseWindowMenu(scr);
    }
    if (!wwin->flags.internal_window)
        XRemoveFromSaveSet(dpy, wwin->client_win);
    XSelectInput(dpy, wwin->client_win, NoEventMask);

    XUnmapWindow(dpy, frame->window);

    /* deselect window */
    wSelectWindow(wwin, False);
    
    /* remove all pending events on window */
    /* I think this only matters for autoraise */
    if (wPreferences.raise_delay) 
       WMDeleteTimerWithClientData(wwin->frame->core);

    XFlush(dpy);

    UpdateSwitchMenu(scr, wwin, ACTION_REMOVE);

    /* reparent the window back to the root */
    if (restore)
	wClientRestore(wwin);

    if (wwin->transient_for!=scr->root_win) {
	owner = wWindowFor(wwin->transient_for);
	if (owner) {
	    owner->flags.semi_focused = 0;
	    wFrameWindowChangeState(owner->frame, WS_UNFOCUSED);
	}
    }

    wasNotFocused = !wwin->flags.focused;
    
    /* remove from window focus list */
    if (!wwin->prev && !wwin->next) {
	/* was the only window */
	scr->focused_window = NULL;
	newFocusedWindow = NULL;
    } else {
	WWindow *tmp;

	if (wwin->prev)
	  wwin->prev->next = wwin->next;
	if (wwin->next)
	  wwin->next->prev = wwin->prev;
	else {
	    scr->focused_window = wwin->prev;
	    scr->focused_window->next = NULL;
	}
	
	/* if in click to focus mode and the window
	 * was a transient, focus the owner window  
	 */
	tmp = NULL;
	if (wPreferences.focus_mode==WKF_CLICK) {
	    tmp = wWindowFor(wwin->transient_for);
	    if (tmp && (!tmp->flags.mapped || tmp->window_flags.no_focusable)) {
		tmp = NULL;
	    }
	}
	/* otherwise, focus the next one in the focus list */
	if (!tmp) {
	    tmp = scr->focused_window;
	    while (tmp) {
		if (!tmp->window_flags.no_focusable 
		    && (tmp->flags.mapped || tmp->flags.shaded))
		    break;
		tmp = tmp->prev;
	    }
	}
	if (wPreferences.focus_mode==WKF_CLICK) {
	    newFocusedWindow = tmp;
	} else if (wPreferences.focus_mode==WKF_SLOPPY
		   || wPreferences.focus_mode==WKF_POINTER) {
            unsigned int mask;
            int foo;
            Window bar, win;

            /*  This is to let the root window get the keyboard input
             * if Sloppy focus mode and no other window get focus.
             * This way keybindings will not freeze.
             */
            tmp = NULL;
            if (XQueryPointer(dpy, scr->root_win, &bar, &win,
                              &foo, &foo, &foo, &foo, &mask))
                tmp = wWindowFor(win);
            if (tmp == wwin)
                tmp = NULL;
	    newFocusedWindow = tmp;
        } else {
	    newFocusedWindow = NULL;
	}
    }
#ifdef DEBUG
    printf("destroying window %x frame %x\n", (unsigned)wwin->client_win,
	   (unsigned)frame->window);
#endif
    if (!wasNotFocused)
	wSetFocusTo(scr, newFocusedWindow);
    wWindowDestroy(wwin);
    XFlush(dpy);
}

     
void
wWindowFocus(WWindow *wwin)
{
#ifdef KEEP_XKB_LOCK_STATUS
    if (wPreferences.modelock) {
        if (!wwin->flags.focused) {
            XkbLockGroup(dpy, XkbUseCoreKbd, wwin->languagemode);
        }
    }
#endif /* KEEP_XKB_LOCK_STATUS */

    wFrameWindowChangeState(wwin->frame, WS_FOCUSED);

    wwin->flags.focused=1;

    wWindowResetMouseGrabs(wwin);

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);
}


void
wWindowMap(WWindow *wwin)
{
    XMapWindow(dpy, wwin->frame->core->window);
    wwin->flags.mapped = 1;
}



void
wWindowUnfocus(WWindow *wwin)
{
#ifdef KEEP_XKB_LOCK_STATUS
    static XkbStateRec staterec;
    if (wPreferences.modelock) {
        if (wwin->flags.focused) {
	    XkbGetState(dpy,XkbUseCoreKbd,&staterec);
	    wwin->languagemode=staterec.compat_state&32?1:0;
	    XkbLockGroup(dpy,XkbUseCoreKbd,0); /* reset to workspace */
        }
    }
#endif /* KEEP_XKB_LOCK_STATUS */

    CloseWindowMenu(wwin->screen_ptr);

    wFrameWindowChangeState(wwin->frame, wwin->flags.semi_focused 
			    ? WS_PFOCUSED : WS_UNFOCUSED);

    if (wwin->transient_for!=None 
	&& wwin->transient_for!=wwin->screen_ptr->root_win) {
	WWindow *owner;
	owner = wWindowFor(wwin->transient_for);
	if (owner && owner->flags.semi_focused) {
	    owner->flags.semi_focused = 0;
	    if (owner->flags.mapped || owner->flags.shaded) {
		wWindowUnfocus(owner);
		wFrameWindowPaint(owner->frame);
	    }
	}
    }
    wwin->flags.focused=0;
    wWindowResetMouseGrabs(wwin);

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);
}



/*
 *----------------------------------------------------------------------
 * 
 * wWindowConstrainSize--
 * 	Constrains size for the client window, taking the maximal size,
 * window resize increments and other size hints into account.
 * 
 * Returns:
 * 	The closest size to what was given that the client window can 
 * have.
 * 
 *---------------------------------------------------------------------- 
 */
void
wWindowConstrainSize(WWindow *wwin, int *nwidth, int *nheight)
{
    XSizeHints *sizeh = wwin->normal_hints;
    int width = *nwidth;
    int height = *nheight;
    int winc = sizeh->width_inc;
    int hinc = sizeh->height_inc;

    if (width < sizeh->min_width)
      width = sizeh->min_width;
    if (height < sizeh->min_height)
      height = sizeh->min_height;

    if (width > sizeh->max_width) 
      width = sizeh->max_width;
    if (height > sizeh->max_height) 
      height = sizeh->max_height;
    
    /* aspect ratio code borrowed from olwm */
    if (sizeh->flags & PAspect) {
	/* adjust max aspect ratio */
	if (!(sizeh->max_aspect.x==1 && sizeh->max_aspect.y==1)
	    && width * sizeh->max_aspect.y > height * sizeh->max_aspect.x) {
	    if (sizeh->max_aspect.x > sizeh->max_aspect.y) {
		height = (width * sizeh->max_aspect.y) / sizeh->max_aspect.x;
		if (height > sizeh->max_height) {
		    height = sizeh->max_height;
		    width = (height*sizeh->max_aspect.x) / sizeh->max_aspect.y;
		}
	    } else {
		width = (height * sizeh->max_aspect.x) / sizeh->max_aspect.y;
		if (width > sizeh->max_width) {
		    width = sizeh->max_width;
		    height = (width*sizeh->max_aspect.y) / sizeh->max_aspect.x;
		}
	    }
	}

	/* adjust min aspect ratio */
	if (!(sizeh->min_aspect.x==1 && sizeh->min_aspect.y==1)
	    && width * sizeh->min_aspect.y < height * sizeh->min_aspect.x) {
	    if (sizeh->min_aspect.x > sizeh->min_aspect.y) {
		height = (width * sizeh->min_aspect.y) / sizeh->min_aspect.x;
		if (height < sizeh->min_height) {
		    height = sizeh->min_height;
		    width = (height*sizeh->min_aspect.x) / sizeh->min_aspect.y;
		}
	    } else {
		width = (height * sizeh->min_aspect.x) / sizeh->min_aspect.y;
		if (width > sizeh->min_width) {
		    width = sizeh->min_width;
		    height = (width*sizeh->min_aspect.y) / sizeh->min_aspect.x;
		}
	    }
	}
    }

    if (sizeh->base_width != 0) {
	width = (((width - sizeh->base_width) / winc) * winc) 
	    + sizeh->base_width;
    } else {
	width = (((width - sizeh->min_width) / winc) * winc)
	  + sizeh->min_width;
    }

    if (sizeh->base_width != 0) {
	height = (((height - sizeh->base_height) / hinc) * hinc)
	  + sizeh->base_height;
    } else {
	height = (((height - sizeh->min_height) / hinc) * hinc) 
	  + sizeh->min_height;
    }
    
    *nwidth = width;
    *nheight = height;
}


void
wWindowChangeWorkspace(WWindow *wwin, int workspace)
{
    WScreen *scr = wwin->screen_ptr;
    WApplication *wapp;

    if (workspace >= scr->workspace_count || workspace < 0 
	|| workspace == wwin->frame->workspace)
      return;

    if (workspace != scr->current_workspace) {
	/* Sent to other workspace. Unmap window */
	if ((wwin->flags.mapped||wwin->flags.shaded)
	    && !wwin->window_flags.omnipresent
	    && !wwin->flags.changing_workspace) {
	    
	    wapp = wApplicationOf(wwin->main_window);
	    if (wapp) {
		wapp->last_workspace = workspace;
	    }
	    XUnmapWindow(dpy, wwin->frame->core->window);
	    wwin->flags.mapped = 0;
	    wSetFocusTo(scr, NULL);
	}
    } else {
	/* brought to current workspace. Map window */
	if (!wwin->flags.mapped && 
	    !(wwin->flags.miniaturized || wwin->flags.hidden)) {
	    wWindowMap(wwin);
	}
    }
    if (!wwin->window_flags.omnipresent) {
	wwin->frame->workspace = workspace;
        UpdateSwitchMenu(scr, wwin, ACTION_CHANGE_WORKSPACE);
    }
}


void
wWindowSynthConfigureNotify(WWindow *wwin)
{
    XEvent sevent;

    sevent.type = ConfigureNotify;
    sevent.xconfigure.display = dpy;
    sevent.xconfigure.event = wwin->client_win;
    sevent.xconfigure.window = wwin->client_win;

    sevent.xconfigure.x = wwin->client.x;
    sevent.xconfigure.y = wwin->client.y;
    sevent.xconfigure.width = wwin->client.width;
    sevent.xconfigure.height = wwin->client.height;

    sevent.xconfigure.border_width = wwin->old_border_width;
    if (wwin->window_flags.no_titlebar)
	sevent.xconfigure.above = None;
    else
        sevent.xconfigure.above = wwin->frame->titlebar->window;

    sevent.xconfigure.override_redirect = False;
    XSendEvent(dpy, wwin->client_win, False, StructureNotifyMask, &sevent);
    XFlush(dpy);
}


/*
 *----------------------------------------------------------------------
 * wWindowConfigure--
 * 	Configures the frame, decorations and client window to the 
 * specified geometry. The geometry is not checked for validity,
 * wWindowConstrainSize() must be used for that. 
 * 	The size parameters are for the client window, but the position is
 * for the frame.
 * 	The client window receives a ConfigureNotify event, according
 * to what ICCCM says.
 * 
 * Returns:
 * 	None
 * 
 * Side effects:
 * 	Window size and position are changed and client window receives
 * a ConfigureNotify event.
 *---------------------------------------------------------------------- 
 */
void 
wWindowConfigure(wwin, req_x, req_y, req_width, req_height)
WWindow *wwin;
int req_x, req_y;		       /* new position of the frame */
int req_width, req_height;	       /* new size of the client */
{
    int synth_notify = False;
    int resize;
    
    resize = (req_width!=wwin->client.width
	      || req_height!=wwin->client.height);
    /* 
     * if the window is being moved but not resized then
     * send a synthetic ConfigureNotify
     */
    if ((req_x!=wwin->frame_x || req_y!=wwin->frame_y) && !resize) {
	synth_notify = True;
    }
        
    if (wwin->window_flags.dont_move_off)
	wScreenBringInside(wwin->screen_ptr, &req_x, &req_y, 
			   req_width, req_height);
    if (resize) {
	if (req_width < MIN_WINDOW_SIZE)
	    req_width = MIN_WINDOW_SIZE;
	if (req_height < MIN_WINDOW_SIZE)
	    req_height = MIN_WINDOW_SIZE;
	
	/* If growing, resize inner part before frame,
	 * if shrinking, resize frame before.
	 * This will prevent the frame (that can have a different color) 
	 * to be exposed, causing flicker */
	if (req_height > wwin->frame->core->height
	    || req_width > wwin->frame->core->width)
	    XResizeWindow(dpy, wwin->client_win, req_width, req_height);

	if (wwin->flags.shaded) {
	    wFrameWindowResize(wwin->frame, req_width, wwin->frame->core->height);
	    wwin->old_geometry.height = req_height;
	} else {
	    wFrameWindowResizeInternal(wwin->frame, req_width, req_height);
	}
	
	if (!(req_height > wwin->frame->core->height
	    || req_width > wwin->frame->core->width))
	    XResizeWindow(dpy, wwin->client_win, req_width, req_height);

	wwin->client.x = req_x;
	wwin->client.y = req_y + wwin->frame->top_width;
	wwin->client.width = req_width;
	wwin->client.height = req_height;

	XMoveWindow(dpy, wwin->frame->core->window, req_x, req_y);
    } else {
	wwin->client.x = req_x;
	wwin->client.y = req_y + wwin->frame->top_width;

	XMoveWindow(dpy, wwin->frame->core->window, req_x, req_y);
    }
    wwin->frame_x = req_x;
    wwin->frame_y = req_y;

#ifdef SHAPE
    if (wShapeSupported && wwin->flags.shaped && resize) {
	wWindowSetShape(wwin);
    }
#endif

    if (synth_notify)
	wWindowSynthConfigureNotify(wwin);
    XFlush(dpy);
}


void 
wWindowMove(wwin, req_x, req_y)
WWindow *wwin;
int req_x, req_y;		       /* new position of the frame */
{
#ifdef CONFIGURE_WINDOW_WHILE_MOVING
    int synth_notify = False;

    /* Send a synthetic ConfigureNotify event for every window movement. */
    if ((req_x!=wwin->frame_x || req_y!=wwin->frame_y)) {
	synth_notify = True;
    }
#else
    /* A single synthetic ConfigureNotify event is sent at the end of
     * a completed (opaque) movement in moveres.c */
#endif

    if (wwin->window_flags.dont_move_off)
	wScreenBringInside(wwin->screen_ptr, &req_x, &req_y,
			   wwin->frame->core->width, wwin->frame->core->height);

    wwin->client.x = req_x;
    wwin->client.y = req_y + wwin->frame->top_width;

    XMoveWindow(dpy, wwin->frame->core->window, req_x, req_y);

    wwin->frame_x = req_x;
    wwin->frame_y = req_y;

#ifdef CONFIGURE_WINDOW_WHILE_MOVING
    if (synth_notify)
	wWindowSynthConfigureNotify(wwin);
#endif
}


void 
wWindowUpdateButtonImages(WWindow *wwin)
{
    WScreen *scr = wwin->screen_ptr;
    Pixmap pixmap, mask;
    WFrameWindow *fwin = wwin->frame;
    
    if (wwin->window_flags.no_titlebar)
      return;

    /* miniaturize button */
    
    if (!wwin->window_flags.no_miniaturize_button) {
	if (wwin->wm_gnustep_attr
	    && wwin->wm_gnustep_attr->flags & GSMiniaturizePixmapAttr) {
	    pixmap = wwin->wm_gnustep_attr->miniaturize_pixmap;
	      
	    if (wwin->wm_gnustep_attr->flags&GSMiniaturizeMaskAttr) {
		mask = wwin->wm_gnustep_attr->miniaturize_mask;
	    } else {
		mask = None;
	    }

	    if (fwin->lbutton_image 
		&& (fwin->lbutton_image->image != pixmap
		    || fwin->lbutton_image->mask != mask)) {
		wPixmapDestroy(fwin->lbutton_image);
		fwin->lbutton_image = NULL;
	    }
	    
	    if (!fwin->lbutton_image) {
		fwin->lbutton_image = wPixmapCreate(scr, pixmap, mask);
		fwin->lbutton_image->client_owned = 1;
		fwin->lbutton_image->client_owned_mask = 1;
	    }
	} else {
	    if (fwin->lbutton_image && !fwin->lbutton_image->shared) {
		wPixmapDestroy(fwin->lbutton_image);
	    }
	    fwin->lbutton_image = scr->b_pixmaps[WBUT_ICONIFY];
	}
    }
    
    /* close button */

    if (!wwin->window_flags.no_close_button) {
	if (wwin->wm_gnustep_attr
	    && wwin->wm_gnustep_attr->flags&GSClosePixmapAttr) {
	    pixmap = wwin->wm_gnustep_attr->close_pixmap;
	    
	    if (wwin->wm_gnustep_attr->flags&GSCloseMaskAttr) {
		mask = wwin->wm_gnustep_attr->close_mask;
	    } else {
		mask = None;
	    }

	    if (fwin->rbutton_image
		&& (fwin->rbutton_image->image != pixmap
		    || fwin->rbutton_image->mask != mask)) {
		wPixmapDestroy(fwin->rbutton_image);
		fwin->rbutton_image = NULL;
	    }
	    
	    if (!fwin->rbutton_image) {
		fwin->rbutton_image = wPixmapCreate(scr, pixmap, mask);
		fwin->rbutton_image->client_owned = 1;
		fwin->rbutton_image->client_owned_mask = 1;
	    }
	} else if (wwin->window_flags.kill_close) {
	    if (fwin->rbutton_image && !fwin->rbutton_image->shared) {
		wPixmapDestroy(fwin->rbutton_image);
	    }
	    fwin->rbutton_image = scr->b_pixmaps[WBUT_KILL];
	} else if (wwin->window_flags.broken_close) {
	    if (fwin->rbutton_image && !fwin->rbutton_image->shared) {
		wPixmapDestroy(fwin->rbutton_image);
	    }
	    fwin->rbutton_image = scr->b_pixmaps[WBUT_BROKENCLOSE];
	} else {
	    if (fwin->rbutton_image && !fwin->rbutton_image->shared) {
		wPixmapDestroy(fwin->rbutton_image);
	    }
	    fwin->rbutton_image = scr->b_pixmaps[WBUT_CLOSE];
	}
    }

    /* force buttons to be redrawn */
    fwin->flags.need_texture_change = 1;
    wFrameWindowPaint(fwin);
}


/*
 *---------------------------------------------------------------------------
 * wWindowConfigureBorders--
 * 	Update window border configuration according to window_flags
 * 
 *--------------------------------------------------------------------------- 
 */
void 
wWindowConfigureBorders(WWindow *wwin)
{
    if (wwin->frame) {
	int flags;
	int newy, oldh;
	
	flags = WFF_LEFT_BUTTON|WFF_RIGHT_BUTTON;
	if (!wwin->window_flags.no_titlebar)
	    flags |= WFF_TITLEBAR;
	if (!wwin->window_flags.no_resizebar)
	    flags |= WFF_RESIZEBAR;

	oldh = wwin->frame->top_width;
	wFrameWindowUpdateBorders(wwin->frame, flags);
	if (oldh != wwin->frame->top_width) {
	    newy = wwin->frame_y + oldh - wwin->frame->top_width;
	    
	    XMoveWindow(dpy, wwin->client_win, 0, wwin->frame->top_width);
	    wWindowConfigure(wwin, wwin->frame_x, newy,
			     wwin->client.width, wwin->client.height);
	}
	
	flags = 0;
	if (!wwin->window_flags.no_miniaturize_button
	    && wwin->frame->flags.hide_left_button)
	    flags |= WFF_LEFT_BUTTON;

	if (!wwin->window_flags.no_close_button
	    && wwin->frame->flags.hide_right_button)
	    flags |= WFF_RIGHT_BUTTON;

	if (flags!=0) {
	    wWindowUpdateButtonImages(wwin);
	    wFrameWindowShowButton(wwin->frame, flags);
	}

	flags = 0;
	if (wwin->window_flags.no_miniaturize_button
	    && !wwin->frame->flags.hide_left_button)
	    flags |= WFF_LEFT_BUTTON;

	if (wwin->window_flags.no_close_button
	    && !wwin->frame->flags.hide_right_button)
	    flags |= WFF_RIGHT_BUTTON;

	if (flags!=0)
	    wFrameWindowHideButton(wwin->frame, flags);

#ifdef SHAPE
	if (wShapeSupported && wwin->flags.shaped) {
	    wWindowSetShape(wwin);
	}
#endif
    }
}


void
wWindowSaveState(WWindow *wwin)
{
    CARD32 data[9];
    
    memset(data, 0, sizeof(CARD32)*9);
    data[0] = wwin->frame->workspace;
    data[2] = wwin->flags.shaded;
    data[3] = wwin->flags.hidden;

    XChangeProperty(dpy, wwin->client_win, _XA_WINDOWMAKER_STATE,
		    _XA_WINDOWMAKER_STATE, 32, PropModeReplace,
		    (unsigned char *)data, 9);
}


static int
getSavedState(Window window, WSavedState **state)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    CARD32 *data;
    
    if (XGetWindowProperty(dpy, window, _XA_WINDOWMAKER_STATE, 0, 9,
			   True, _XA_WINDOWMAKER_STATE,
			   &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
			   (unsigned char **)&data)!=Success || !data)
      return 0;
    
    *state = malloc(sizeof(WSavedState));

    if (*state) {
	(*state)->workspace = data[0];
	(*state)->miniaturized = data[1];
	(*state)->shaded = data[2];
	(*state)->hidden = data[3];
	(*state)->use_geometry = data[4];
	(*state)->x = data[5];
	(*state)->y = data[6];
	(*state)->w = data[7];
	(*state)->h = data[8];
    }
    XFree(data);
    
    if (*state && type_ret==_XA_WINDOWMAKER_STATE)
      return 1;
    else 
      return 0;
}


#ifdef SHAPE
void
wWindowClearShape(WWindow *wwin)
{
    XShapeCombineMask(dpy, wwin->frame->core->window, ShapeBounding,
		      0, wwin->frame->top_width, None, ShapeSet);
    XFlush(dpy);
}

void 
wWindowSetShape(WWindow *wwin)
{
    XRectangle rect[2];
    int count;
#ifdef OPTIMIZE_SHAPE
    XRectangle *rects;
    XRectangle *urec;
    int ordering;

    /* only shape is the client's */
    if (wwin->window_flags.no_titlebar && wwin->window_flags.no_resizebar) {
	goto alt_code;
    }
    
    /* Get array of rectangles describing the shape mask */
    rects = XShapeGetRectangles(dpy, wwin->client_win, ShapeBounding,
				&count, &ordering);
    if (!rects) {
	goto alt_code;
    }

    urec = malloc(sizeof(XRectangle)*(count+2));
    if (!urec) {
	XFree(rects);
	goto alt_code;
    }

    /* insert our decoration rectangles in the rect list */
    memcpy(urec, rects, sizeof(XRectangle)*count);
    XFree(rects);

    if (!wwin->window_flags.no_titlebar) {
	urec[count].x = -1;
	urec[count].y = -1 - wwin->frame->top_width;
	urec[count].width = wwin->frame->core->width + 2;
	urec[count].height = wwin->frame->top_width + 1;
	count++;
    }
    if (!wwin->window_flags.no_resizebar) {
	urec[count].x = -1;
	urec[count].y = wwin->frame->core->height 
	    - wwin->frame->bottom_width - wwin->frame->top_width;
	urec[count].width = wwin->frame->core->width + 2;
	urec[count].height = wwin->frame->bottom_width + 1;
	count++;
    }

    /* shape our frame window */
    XShapeCombineRectangles(dpy, wwin->frame->core->window, ShapeBounding,
			    0, wwin->frame->top_width, urec, count,
			    ShapeSet, Unsorted);
    XFlush(dpy);
    free(urec);
    return;

alt_code:
#endif /* OPTIMIZE_SHAPE */
    count = 0;
    if (!wwin->window_flags.no_titlebar) {
        rect[count].x = -1;
        rect[count].y = -1;
	rect[count].width = wwin->frame->core->width + 2;
	rect[count].height = wwin->frame->top_width + 1;
        count++;
    }
    if (!wwin->window_flags.no_resizebar) {
	rect[count].x = -1;
	rect[count].y = wwin->frame->core->height - wwin->frame->bottom_width;
	rect[count].width = wwin->frame->core->width + 2;
	rect[count].height = wwin->frame->bottom_width + 1;
	count++;
    }
    if (count > 0) {
	XShapeCombineRectangles(dpy, wwin->frame->core->window, ShapeBounding,
				0, 0, rect, count, ShapeSet, Unsorted);
    }
    XShapeCombineShape(dpy, wwin->frame->core->window, ShapeBounding,
		       0, wwin->frame->top_width, wwin->client_win,
		       ShapeBounding, (count > 0 ? ShapeUnion : ShapeSet));
    XFlush(dpy);
}
#endif /* SHAPE */

/* ====================================================================== */

static FocusMode
getFocusMode(WWindow *wwin)
{
    FocusMode mode;

    if ((wwin->wm_hints) && (wwin->wm_hints->flags & InputHint)) {
	if (wwin->wm_hints->input == True) {
	    if (wwin->protocols.TAKE_FOCUS)
	      mode = WFM_LOCALLY_ACTIVE;
	    else
	      mode = WFM_PASSIVE;
	} else {
	    if (wwin->protocols.TAKE_FOCUS)
	      mode = WFM_GLOBALLY_ACTIVE;
	    else
	      mode = WFM_NO_INPUT;
	}
    } else {
	mode = WFM_PASSIVE;
    }
    return mode;
}


void
wWindowSetKeyGrabs(WWindow *wwin)
{
    int i;
    WShortKey *key;

    for (i=0; i<WKBD_LAST; i++) {
	key = &wKeyBindings[i];

	if (key->keycode==0)
	    continue;
	if (key->modifier!=AnyModifier) {
	    XGrabKey(dpy, key->keycode, key->modifier|LockMask,
		wwin->frame->core->window, True, GrabModeAsync, GrabModeAsync);
#ifdef NUMLOCK_HACK
	    /* Also grab all modifier combinations possible that include,
	     * LockMask, ScrollLockMask and NumLockMask, so that keygrabs
	     * work even if the NumLock/ScrollLock key is on.
	     */
	    wHackedGrabKey(key->keycode, key->modifier,
			   wwin->frame->core->window, True, GrabModeAsync, 
			   GrabModeAsync);
#endif
	}
	XGrabKey(dpy, key->keycode, key->modifier,
	     wwin->frame->core->window, True, GrabModeAsync, GrabModeAsync);
    }

    wRootMenuBindShortcuts(wwin->frame->core->window);
}



void
wWindowResetMouseGrabs(WWindow *wwin)
{
    XUngrabButton(dpy, AnyButton, AnyModifier, wwin->client_win);

    if (!wwin->window_flags.no_bind_mouse) {
	/* grabs for Meta+drag */
	wHackedGrabButton(AnyButton, MOD_MASK, wwin->client_win, 
			  True, ButtonPressMask, GrabModeSync, 
			  GrabModeAsync, None, wCursor[WCUR_ARROW]);
    }
    
    if (!wwin->flags.focused) {
	/* the passive grabs to focus the window */
	if (wPreferences.focus_mode == WKF_CLICK)
	    XGrabButton(dpy, AnyButton, AnyModifier, wwin->client_win,
			True, ButtonPressMask, GrabModeSync, GrabModeAsync,
			None, None);
    }
    XFlush(dpy);
}


void
wWindowUpdateGNUstepAttr(WWindow *wwin, GNUstepWMAttributes *attr)
{
    
    if (attr->flags & GSExtraFlagsAttr) {
	if (wwin->window_flags.broken_close != 
	    (attr->extra_flags & GSDocumentEditedFlag)) {
	    wwin->window_flags.broken_close = !wwin->window_flags.broken_close;

	    wWindowUpdateButtonImages(wwin);
	}
    }
    
}


WMagicNumber
wWindowAddSavedState(char *instance, char *class, char *command,
                     pid_t pid, WSavedState *state)
{
    WWindowState *wstate;

    wstate = malloc(sizeof(WWindowState));
    if (!wstate)
        return 0;

    memset(wstate, 0, sizeof(WWindowState));
    wstate->pid = pid;
    if (instance)
        wstate->instance = wstrdup(instance);
    if (class)
        wstate->class = wstrdup(class);
    if (command)
        wstate->command = wstrdup(command);
    wstate->state = state;

    wstate->next = windowState;
    windowState = wstate;

#ifdef DEBUG
    printf("Added WindowState with ID %p, for %s.%s : \"%s\"\n", wstate, instance,
           class, command);
#endif

    return wstate;
}


#define SAME(x, y) (((x) && (y) && !strcmp((x), (y))) || (!(x) && !(y)))


WMagicNumber
wWindowGetSavedState(Window win)
{
    char *instance, *class, *command=NULL;
    WWindowState *wstate = windowState;
    char **argv;
    int argc;

    if (!wstate)
        return NULL;

    if (XGetCommand(dpy, win, &argv, &argc)) {
	if (argc > 0)
	    command = FlattenStringList(argv, argc);
        XFreeStringList(argv);
    }
    if (!command)
        return NULL;

    if (PropGetWMClass(win, &class, &instance)) {
        while (wstate) {
            if (SAME(instance, wstate->instance) &&
                SAME(class, wstate->class) &&
                SAME(command, wstate->command)) {
                break;
            }
            wstate = wstate->next;
        }
    } else {
        wstate = NULL;
    }

#ifdef DEBUG
    printf("Read WindowState with ID %p, for %s.%s : \"%s\"\n", wstate, instance,
           class, command);
#endif

    if (command) free(command);
    if (instance) XFree(instance);
    if (class) XFree(class);

    return wstate;
}


void 
wWindowDeleteSavedState(WMagicNumber id)
{
    WWindowState *tmp, *wstate=(WWindowState*)id;

    if (!wstate || !windowState)
        return;

    tmp = windowState;
    if (tmp==wstate) {
        windowState = wstate->next;
#ifdef DEBUG
        printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
               wstate, wstate->instance, wstate->class, wstate->command);
#endif
        if (wstate->instance) free(wstate->instance);
        if (wstate->class)    free(wstate->class);
        if (wstate->command)  free(wstate->command);
        free(wstate->state);
        free(wstate);
    } else {
	while (tmp->next) {
	    if (tmp->next==wstate) {
		tmp->next=wstate->next;
#ifdef DEBUG
                printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
                       wstate, wstate->instance, wstate->class, wstate->command);
#endif
                if (wstate->instance) free(wstate->instance);
                if (wstate->class)    free(wstate->class);
                if (wstate->command)  free(wstate->command);
                free(wstate->state);
                free(wstate);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}


void 
wWindowDeleteSavedStatesForPID(pid_t pid)
{
    WWindowState *tmp, *wstate;

    if (!windowState)
        return;

    tmp = windowState;
    if (tmp->pid == pid) {
	wstate = windowState;
        windowState = tmp->next;
#ifdef DEBUG
        printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
               wstate, wstate->instance, wstate->class, wstate->command);
#endif
        if (wstate->instance) free(wstate->instance);
        if (wstate->class)    free(wstate->class);
        if (wstate->command)  free(wstate->command);
        free(wstate->state);
        free(wstate);
    } else {
	while (tmp->next) {
	    if (tmp->next->pid==pid) {
		wstate = tmp->next;
		tmp->next = wstate->next;
#ifdef DEBUG
                printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
                       wstate, wstate->instance, wstate->class, wstate->command);
#endif
                if (wstate->instance) free(wstate->instance);
                if (wstate->class)    free(wstate->class);
                if (wstate->command)  free(wstate->command);
                free(wstate->state);
                free(wstate);
		break;
	    }
	    tmp = tmp->next;
	}
    }    
}


/* ====================================================================== */

static void 
resizebarMouseDown(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = data;

#ifndef NUMLOCK_HACK
    if ((event->xbutton.state & ValidModMask)
	!= (event->xbutton.state & ~LockMask)) {
	wwarning(_("the NumLock, ScrollLock or similar key seems to be turned on.\n"\
		"Turn it off or some mouse actions and keyboard shortcuts will not work."));
    }
#endif
    
    event->xbutton.state &= ValidModMask;
    
    CloseWindowMenu(wwin->screen_ptr);

    if (wPreferences.focus_mode==WKF_CLICK 
	&& !(event->xbutton.state&ControlMask)) {
	wSetFocusTo(wwin->screen_ptr, wwin);
    }

    if (event->xbutton.button == Button1)
	wRaiseFrame(wwin->frame->core);

    if (event->xbutton.state & MOD_MASK) {
	/* move the window */
#if 0
	if (XGrabPointer(dpy, wwin->frame->resizebar->window, True,
			 ButtonMotionMask|ButtonReleaseMask|ButtonPressMask,
			 GrabModeAsync, GrabModeAsync, None, 
			 None, CurrentTime)!=GrabSuccess) {
#ifdef DEBUG0
	    wwarning("pointer grab failed for window move");
#endif
	    return;
	}
#endif
	wMouseMoveWindow(wwin, event);
	XUngrabPointer(dpy, CurrentTime);
    } else {
#if 0
        /* resize the window */
        if (XGrabPointer(dpy, wwin->frame->resizebar->window, True,
                         ButtonMotionMask|ButtonReleaseMask|ButtonPressMask,
                         GrabModeAsync, GrabModeAsync, None,
                         None, CurrentTime)!=GrabSuccess) {
#ifdef DEBUG0
            wwarning("pointer grab failed for window resize");
#endif
            return;
        }
#endif
        wMouseResizeWindow(wwin, event);
        XUngrabPointer(dpy, CurrentTime);
    }
}



static void 
titlebarDblClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = data;

    event->xbutton.state &= ValidModMask;

    if (event->xbutton.button==Button1) {
	if (event->xbutton.state == 0) {
	    if (!wwin->window_flags.no_shadeable) {
		/* shade window */
		if (wwin->flags.shaded)
		    wUnshadeWindow(wwin);
		else
		    wShadeWindow(wwin);
	    }
	} else {
	    int dir = 0;

	    if (event->xbutton.state & ControlMask)
		dir |= MAX_VERTICAL;
	    
            if (event->xbutton.state & ShiftMask) {
		dir |= MAX_HORIZONTAL;
                if (!(event->xbutton.state & ControlMask))
                    wSelectWindow(wwin, !wwin->flags.selected);
            }

	    /* maximize window */
	    if (dir !=0 && !wwin->window_flags.no_resizable) {
		if (wwin->flags.maximized) 
		    wUnmaximizeWindow(wwin);
		 else
		    wMaximizeWindow(wwin, dir);
	    }
	}
    } else if (event->xbutton.button==Button3) {
	if (event->xbutton.state & MOD_MASK) {
	    wHideOtherApplications(wwin);
	}
    } else if (event->xbutton.button==Button2) {
	wSelectWindow(wwin, !wwin->flags.selected);
    }
}


static void
frameMouseDown(WObjDescriptor *desc, XEvent *event)
{
    WWindow *wwin = desc->parent;

    event->xbutton.state &= ValidModMask;
    
    CloseWindowMenu(wwin->screen_ptr);

    if (wPreferences.focus_mode==WKF_CLICK
	&& !(event->xbutton.state&ControlMask)) {
	wSetFocusTo(wwin->screen_ptr, wwin);
    }
    if (event->xbutton.button == Button1) {
	wRaiseFrame(wwin->frame->core);
    }
    
    if (event->xbutton.state & MOD_MASK) {
	/* move the window */
	if (XGrabPointer(dpy, wwin->client_win, False, 
			 ButtonMotionMask|ButtonReleaseMask|ButtonPressMask,
			 GrabModeAsync, GrabModeAsync, None,
			 None, CurrentTime)!=GrabSuccess) {
#ifdef DEBUG0
	    wwarning("pointer grab failed for window move");
#endif
	    return;
	}
	if (event->xbutton.button == Button3 && !wwin->window_flags.no_resizable)
	    wMouseResizeWindow(wwin, event);
	else 
	    wMouseMoveWindow(wwin, event);
	XUngrabPointer(dpy, CurrentTime);
    }
}


static void 
titlebarMouseDown(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = (WWindow*)data;

#ifndef NUMLOCK_HACK
    if ((event->xbutton.state & ValidModMask)
	!= (event->xbutton.state & ~LockMask)) {
	wwarning(_("the NumLock, ScrollLock or similar key seems to be turned on.\n"\
		"Turn it off or some mouse actions and keyboard shortcuts will not work."));
    }
#endif

    event->xbutton.state &= ValidModMask;
    

    CloseWindowMenu(wwin->screen_ptr);
    
    if (wPreferences.focus_mode==WKF_CLICK 
	&& !(event->xbutton.state&ControlMask)) {
	wSetFocusTo(wwin->screen_ptr, wwin);
    }
    
    if (event->xbutton.button == Button1
	|| event->xbutton.button == Button2) {
	
	if (event->xbutton.button == Button1) {
	    if (event->xbutton.state & MOD_MASK) {
		wLowerFrame(wwin->frame->core);
	    } else {
		wRaiseFrame(wwin->frame->core);
	    }
	}
	if ((event->xbutton.state & ShiftMask)
	    && !(event->xbutton.state & ControlMask)) {
	    wSelectWindow(wwin, !wwin->flags.selected);
	    return;
	}
	/* move the window */
	wMouseMoveWindow(wwin, event);
	XUngrabPointer(dpy, CurrentTime);
    } else if (event->xbutton.button == Button3 && event->xbutton.state==0
	       && !wwin->flags.internal_window) {
	WObjDescriptor *desc;
	
	OpenWindowMenu(wwin, event->xbutton.x_root, 
		       wwin->frame_y+wwin->frame->top_width, False);

	/* allow drag select */
	desc = &wwin->screen_ptr->window_menu->menu->descriptor;
	event->xany.send_event = True;
	(*desc->handle_mousedown)(desc, event);
    }
}



static void
windowCloseClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = data;

    event->xbutton.state &= ValidModMask;

    CloseWindowMenu(wwin->screen_ptr);

    /* if control-click, kill the client */
    if (event->xbutton.state & ControlMask) {
	wClientKill(wwin);
    } else if (wwin->protocols.DELETE_WINDOW && event->xbutton.state==0) {
	/* send delete message */
	wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW, LastTimestamp);
    }
}


static void
windowCloseDblClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = data;

    CloseWindowMenu(wwin->screen_ptr);

    /* send delete message */
    if (wwin->protocols.DELETE_WINDOW) {
	wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW, LastTimestamp);
    } else {
	wClientKill(wwin);
    }
}


static void
windowIconifyClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WWindow *wwin = data;

    event->xbutton.state &= ValidModMask;
    
    CloseWindowMenu(wwin->screen_ptr);
    
    if (wwin->protocols.MINIATURIZE_WINDOW && event->xbutton.state==0) {
	wClientSendProtocol(wwin, _XA_GNUSTEP_WM_MINIATURIZE_WINDOW,
			    LastTimestamp);
    } else {
	WApplication *wapp;
	if ((event->xbutton.state & ControlMask) ||
	    (event->xbutton.button == Button3)) {
	    
	    wapp = wApplicationOf(wwin->main_window);
	    if (wapp && !wwin->window_flags.no_appicon)
		wHideApplication(wapp);
	} else if (event->xbutton.state==0) {
	    wIconifyWindow(wwin);
	}
    }
}
