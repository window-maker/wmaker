/*
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

#ifndef WMWINDOW_H_
#define WMWINDOW_H_

#include <wraster.h>

#include "wconfig.h"
#include "GNUstep.h"
#ifdef MWM_HINTS
# include "motif.h"
#endif
#include "texture.h"
#include "menu.h"
#include "application.h"

/* not defined in my X11 headers */
#ifndef UrgencyHint
#define UrgencyHint (1L << 8)
#endif


#define BORDER_TOP	1
#define BORDER_BOTTOM	2
#define BORDER_LEFT	4
#define BORDER_RIGHT	8
#define BORDER_ALL	(1|2|4|8)


#define CLIENT_EVENTS (StructureNotifyMask | PropertyChangeMask\
	| EnterWindowMask | LeaveWindowMask | ColormapChangeMask \
	| FocusChangeMask)

typedef enum {
    WFM_PASSIVE, WFM_NO_INPUT, WFM_LOCALLY_ACTIVE, WFM_GLOBALLY_ACTIVE
} FocusMode;


/*
 * window attribute flags.
 * 
 * Note: attributes that should apply to the application as a
 * whole should only access the flags from the app->main_window_desc->window_flags
 * This is to make sure that the application doesn't have many different
 * values for the same option. For example, imagine xfoo, which has 
 * foo.bar as leader and it a child window named foo.baz. If you set
 * StartHidden YES for foo.bar and NO for foo.baz we must *always* guarantee
 * that the value that will be used will be that of the leader foo.bar.
 * The attributes inspector must always save application wide options
 * in the name of the leader window, not the child.
 */
/*
 * All flags must have their default values = 0
 */
typedef struct {
    /* OpenStep */
    unsigned int no_titlebar:1;	       /* draw titlebar? */
    unsigned int no_resizable:1;
    unsigned int no_closable:1;
    unsigned int no_miniaturizable:1;

    /* decorations */
    unsigned int no_resizebar:1;       /* draw the bottom handle? */
    unsigned int no_close_button:1;    /* draw a close button? */
    unsigned int no_miniaturize_button:1; /* draw an iconify button? */
    
    unsigned int broken_close:1;       /* is the close button broken? */

    /* ours */
    unsigned int kill_close:1;	       /* can't send WM_DELETE_WINDOW */
	
    unsigned int no_shadeable:1;
    unsigned int omnipresent:1;
    unsigned int skip_window_list:1;
    unsigned int floating:1;	       /* put in NSFloatingWindowLevel */
    unsigned int no_bind_keys:1;       /* intercept wm kbd binds
					* while window is focused */
    unsigned int no_bind_mouse:1;      /* intercept mouse events
					* on client area while window
					* is focused */
    unsigned int no_hide_others:1;     /* hide window when doing hideothers */
    unsigned int no_appicon:1;	       /* make app icon */
    
    unsigned int dont_move_off:1;

    unsigned int no_focusable:1;
    
    unsigned int always_user_icon:1;   /* ignore client IconPixmap or
					* IconWindow */
    
    unsigned int start_miniaturized:1;
    unsigned int start_hidden:1;
    unsigned int dont_save_session:1;  /* do not save app's state in session */
    
    /*
     * emulate_app_icon must be automatically disabled for apps that can
     * generate their own appicons and for apps that have no_appicon=1
     */
    unsigned int emulate_appicon:1;
} WWindowAttributes;

/*
 * Window manager protocols that both the client as we understand.
 */
typedef struct {
    unsigned int TAKE_FOCUS:1;
    unsigned int SAVE_YOURSELF:1;
    unsigned int DELETE_WINDOW:1;

    /* WindowMaker specific */
    unsigned int MINIATURIZE_WINDOW:1;
#ifdef MONITOR_HEARTBEAT 
    unsigned int HEARTBEAT:1;
#endif
} WProtocols;


/*
 * Stores client window information. Each client window has it's
 * structure created when it's being first mapped.
 */
typedef struct WWindow {
    struct WWindow *prev;	       /* window focus list */
    struct WWindow *next;

    WScreen *screen_ptr;	       /* pointer to the screen structure */
    WWindowAttributes window_flags;    /* window attribute flags */
    struct InspectorPanel *inspector;  /*  pointer to attribute editor panel */

    struct WFrameWindow *frame;	       /* the frame window */
    int frame_x, frame_y;	       /* position of the frame in root*/

    struct {
	int x, y;
	unsigned int width, height;    /* original geometry of the window */
    } old_geometry;		       /* (before things like maximize) */

    /* client window info */
    short old_border_width;	       /* original border width of client_win*/
    Window client_win;		       /* the window we're managing */
    WObjDescriptor client_descriptor; /* dummy descriptor for client */
    struct {
	int x, y;		       /* position of *client* relative 
					* to root */
	unsigned int width, height;    /* size of the client window */
    } client;

    XSizeHints *normal_hints;	       /* WM_NORMAL_HINTS */
    XWMHints *wm_hints;		       /* WM_HINTS (optional) */
    char *wm_instance;		       /* instance of WM_CLASS */
    char *wm_class;    		       /* class of WM_CLASS */
    GNUstepWMAttributes *wm_gnustep_attr;/* GNUstep window attributes */

    int state;			       /* state as in ICCCM */
    
    Window transient_for;	       /* WM_TRANSIENT_FOR */
    Window group_id;		       /* the leader window of the group */
    Window client_leader;	       /* WM_CLIENT_LEADER if not
					internal_window */

    Window main_window;		       /* main window for the application */
      
    int cmap_window_no;
    Window *cmap_windows;
    
    /* protocols */
    WProtocols protocols;	       /* accepted WM_PROTOCOLS */
    
    FocusMode focus_mode;	       /* type of keyboard input focus */

#ifdef MONITOR_HEARTBEAT
    time_t last_beat;
#endif
    struct {
	/* state flags */
	unsigned int mapped:1;	
	unsigned int focused:1;
	unsigned int miniaturized:1;
	unsigned int hidden:1;
	unsigned int shaded:1;
	unsigned int maximized:2;

	unsigned int semi_focused:1;
	/* window type flags */
	unsigned int urgent:1;	       /* if wm_hints says this is urgent */
#ifdef SHAPE
	unsigned int shaped:1;
#endif
	
	/* info flags */
	unsigned int buttons_dont_fit:1;
	unsigned int rebuild_texture:1;/* the window was resized and 
					* gradients should be re-rendered */
	unsigned int needs_full_repaint:1;/* does a full repaint of the 
					   * window next time it's painted */
	unsigned int icon_moved:1;     /* icon for this window was moved
					* by the user */
	unsigned int ignore_next_unmap:1;
	unsigned int selected:1;       /* multiple window selection */
	unsigned int skip_next_animation:1;
	unsigned int internal_window:1;
	unsigned int changing_workspace:1;

        unsigned int inspector_open:1; /* attrib inspector is already open */
	
	unsigned int destroyed:1;      /* window was already destroyed */
    } flags;		/* state of the window */

    struct WIcon *icon;		       /* icon info for the window */
    int icon_x, icon_y;		       /* position of the icon */
} WWindow;


/*
 * Changes to this must update wWindowSaveState/getSavedState
 * 
 */
typedef struct WSavedState {
    int workspace;
    int miniaturized;
    int shaded;
    int hidden;
    int use_geometry;
    int x;
    int y;
    unsigned int w;
    unsigned int h;
} WSavedState;


typedef struct WWindowState {
    char *instance;
    char *class;
    char *command;
    pid_t pid;
    WSavedState *state;
    struct WWindowState *next;
} WWindowState;


typedef void* WMagicNumber;



void wWindowDestroy(WWindow *wwin);
WWindow *wWindowCreate();

#ifdef SHAPE
void wWindowSetShape(WWindow *wwin);
void wWindowClearShape(WWindow *wwin);
#endif

WWindow *wManageWindow(WScreen *scr, Window window);

void wUnmanageWindow(WWindow *wwin, int restore);

void wWindowFocus(WWindow *wwin);
void wWindowUnfocus(WWindow *wwin);
void wWindowConstrainSize(WWindow *wwin, int *nwidth, int *nheight);
void wWindowConfigure(WWindow *wwin, int req_x, int req_y, 
		      int req_width, int req_height);

void wWindowMove(WWindow *wwin, int req_x, int req_y);

void wWindowSynthConfigureNotify(WWindow *wwin);

WWindow *wWindowFor(Window window);


void wWindowConfigureBorders(WWindow *wwin);

void wWindowUpdateButtonImages(WWindow *wwin);

void wWindowSaveState(WWindow *wwin);

void wWindowChangeWorkspace(WWindow *wwin, int workspace);

void wWindowSetKeyGrabs(WWindow *wwin);

void wWindowResetMouseGrabs(WWindow *wwin);

WWindow *wManageInternalWindow(WScreen *scr, Window window, Window owner,
			       char *title, int x, int y, 
			       int width, int height);

void wWindowCheckAttributeSanity(WWindow *wwin, WWindowAttributes *flags);

void wWindowUpdateGNUstepAttr(WWindow *wwin, GNUstepWMAttributes *attr);

void wWindowMap(WWindow *wwin);

Bool wWindowCanReceiveFocus(WWindow *wwin);

void wWindowDeleteSavedStatesForPID(pid_t pid);

WMagicNumber wWindowAddSavedState(char *instance, char *class, char *command,
                                  pid_t pid, WSavedState *state);

WMagicNumber wWindowGetSavedState(Window win);

void wWindowDeleteSavedState(WMagicNumber id);


#endif
