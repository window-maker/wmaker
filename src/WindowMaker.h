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

#ifndef WINDOWMAKER_H_
#define WINDOWMAKER_H_

#include "wconfig.h"

#include <assert.h>

#include "WINGs.h"
#include "WUtil.h"

#if HAVE_LIBINTL_H && I18N
# include <libintl.h>
# define _(text) gettext(text)
#else
# define _(text) (text)
#endif




/* class codes */
typedef enum {
    WCLASS_UNKNOWN = 0,
	WCLASS_WINDOW = 1,	       /* managed client windows */
	WCLASS_MENU = 2,	       /* root menus */
	WCLASS_APPICON = 3,
	WCLASS_DUMMYWINDOW = 4,	       /* window that holds window group leader */
	WCLASS_MINIWINDOW = 5,
	WCLASS_DOCK_ICON = 6,
	WCLASS_PAGER = 7,
	WCLASS_TEXT_INPUT = 8,
	WCLASS_FRAME = 9
} WClassType;


/* generic window levels (a superset of the N*XTSTEP ones) */
enum {
    WMDesktopLevel = 0,
	WMSunkenLevel = 1,
	WMNormalLevel = 2,
	WMFloatingLevel = 3,
	WMDockLevel = 4,
	WMSubmenuLevel = 5,
	WMMainMenuLevel = 6,
	WMOuterSpaceLevel = 7
};
#define MAX_WINDOW_LEVELS 8

/*
 * WObjDescriptor will be used by the event dispatcher to
 * send events to a particular object through the methods in the 
 * method table. If all objects of the same class share the
 * same methods, the class method table should be used, otherwise
 * a new method table must be created for each object.
 * It is also assigned to find the parent structure of a given
 * window (like the WWindow or WMenu for a button)
 */

typedef struct WObjDescriptor {
    void *self;			       /* the object that will be called */    
				       /* event handlers */
    void (*handle_expose)(struct WObjDescriptor *sender, XEvent *event);
    
    void (*handle_mousedown)(struct WObjDescriptor *sender, XEvent *event);

    void (*handle_anything)(struct WObjDescriptor *sender, XEvent *event);

    void (*handle_enternotify)(struct WObjDescriptor *sender, XEvent *event);
    void (*handle_leavenotify)(struct WObjDescriptor *sender, XEvent *event);

    WClassType parent_type;	       /* type code of the parent */
    void *parent;		       /* parent object (WWindow or WMenu) */
} WObjDescriptor;


/* shutdown modes */
typedef enum {
    WSExitMode,
	WSLogoutMode,
	WSKillMode,
	WSRestartPreparationMode
} WShutdownMode;


/* internal buttons */
#define WBUT_CLOSE              0
#define WBUT_BROKENCLOSE        1
#define WBUT_ICONIFY            2
#define WBUT_KILL		3
#ifdef XKB_BUTTON_HINT
#define WBUT_XKBGROUP1      4
#define WBUT_XKBGROUP2      5
#define WBUT_XKBGROUP3      6
#define WBUT_XKBGROUP4      7
#define PRED_BPIXMAPS		8 /* reserved for 4 groups */
#else
#define PRED_BPIXMAPS		4 /* count of WBUT icons */
#endif /* XKB_BUTTON_HINT */

/* cursors */
#define WCUR_DEFAULT	0
#define WCUR_NORMAL 	0
#define WCUR_MOVE	1
#define WCUR_RESIZE	2
#define WCUR_WAIT	3
#define WCUR_ARROW	4
#define WCUR_QUESTION	5
#define WCUR_TEXT	6
#define WCUR_LAST	7


/* geometry displays */
#define WDIS_NEW	0	       /* new style */
#define WDIS_CENTER	1	       /* center of screen */
#define WDIS_TOPLEFT	2	       /* top left corner of screen */
#define WDIS_FRAME_CENTER 3	       /* center of the frame */


/* keyboard input focus mode */
#define WKF_CLICK	0
#define WKF_POINTER	1
#define WKF_SLOPPY	2

/* window placement mode */
#define WPM_MANUAL	0
#define WPM_CASCADE	1
#define WPM_SMART	2
#define WPM_RANDOM	3
#define WPM_AUTO        4

/* text justification */
#define WTJ_CENTER	0
#define WTJ_LEFT	1
#define WTJ_RIGHT	2

/* iconification styles */
#define WIS_ZOOM        0
#define WIS_TWIST       1
#define WIS_FLIP        2
#define WIS_NONE        3
#define WIS_RANDOM	4 /* secret */

/* switchmenu actions */
#define ACTION_ADD	0
#define ACTION_REMOVE	1
#define ACTION_CHANGE	2
#define ACTION_CHANGE_WORKSPACE 3
#define ACTION_CHANGE_STATE	4


/* speeds */
#define SPEED_ULTRAFAST 0
#define SPEED_FAST	1
#define SPEED_MEDIUM	2
#define SPEED_SLOW	3
#define SPEED_ULTRASLOW 4


/* window states */
#define WS_FOCUSED	0
#define WS_UNFOCUSED	1
#define WS_PFOCUSED	2

/* clip title colors */
#define CLIP_NORMAL    0
#define CLIP_COLLAPSED 1


/* icon yard position */
#define	IY_VERT		1
#define	IY_HORIZ	0
#define	IY_TOP		2
#define	IY_BOTTOM	0
#define	IY_RIGHT	4
#define	IY_LEFT		0


/* menu styles */
#define MS_NORMAL		0
#define MS_SINGLE_TEXTURE	1
#define MS_FLAT			2


/* workspace display position */
#define WD_NONE		0
#define WD_CENTER	1
#define WD_TOP		2
#define WD_BOTTOM	3
#define WD_TOPLEFT	4
#define WD_TOPRIGHT	5
#define WD_BOTTOMLEFT	6
#define WD_BOTTOMRIGHT	7


/* program states */
#define WSTATE_NORMAL		0
#define WSTATE_NEED_EXIT	1
#define WSTATE_NEED_RESTART	2
#define WSTATE_EXITING		3
#define WSTATE_RESTARTING	4
#define WSTATE_MODAL		5


#define WCHECK_STATE(state)	(state == WProgramState)
#define WCHANGE_STATE(nstate)	\
	if (WProgramState == WSTATE_NORMAL\
		|| nstate != WSTATE_MODAL)\
		 WProgramState = (nstate)


/* notifications */

#ifdef MAINFILE
#define NOTIFICATION(n) char *WN##n = #n
#else
#define NOTIFICATION(n) extern char *WN##n
#endif

NOTIFICATION(WindowAppearanceSettingsChanged);

NOTIFICATION(IconAppearanceSettingsChanged);

NOTIFICATION(IconTileSettingsChanged);

NOTIFICATION(MenuAppearanceSettingsChanged);

NOTIFICATION(MenuTitleAppearanceSettingsChanged);


/* appearance settings clientdata flags */
enum {
    WFontSettings = 1 << 0,
	WTextureSettings = 1 << 1,
	WColorSettings = 1 << 2
};



typedef struct {
    int x1, y1;
    int x2, y2;
} WArea;

typedef struct WCoord {
    int x, y;
} WCoord;

typedef struct WPreferences {
    char *pixmap_path;		       /* : separate list of */
				       /* paths to find pixmaps */
    char *icon_path;		       /* : separated list of */
				       /* paths to find icons */
    
    char *logger_shell;		       /* shell to log child stdi/o */

    RImage *button_images;	       /* titlebar button images */

    char smooth_workspace_back;
    char size_display;		       /* display type for resize geometry */
    char move_display;		       /* display type for move geometry */
    char window_placement;	       /* window placement mode */
    char colormap_mode;		       /* colormap focus mode */
    char focus_mode;		       /* window focusing mode */
    
    char opaque_move;		       /* update window position during */
				       /* move */

    char wrap_menus;		       /* wrap menus at edge of screen */
    char scrollable_menus;	       /* let them be scrolled */
    char align_menus;		       /* align menu with their parents */
    
    char use_saveunders;	       /* turn on SaveUnders for menus,
					* icons etc. */ 
    char no_window_over_dock;

    char no_window_over_icons;

    WCoord window_place_origin;	       /* Offset for windows placed on
                                        * screen */

    char constrain_window_size;	       /* don't let windows get bigger than 
					* screen */

    char circ_raise;		       /* raise window when Alt-tabbing */

    char ignore_focus_click;

    char open_transients_with_parent;  /* open transient window in
					same workspace as parent */
    char title_justification;	       /* titlebar text alignment */

    char multi_byte_text;
#ifdef KEEP_XKB_LOCK_STATUS
    char modelock;
#endif

    char no_dithering;		       /* use dithering or not */
    
    char no_sound;		       /* enable/disable sound */
    char no_animations;		       /* enable/disable animations */
    
    char no_autowrap;		       /* wrap workspace when window is moved
					* to the edge */
    
    char auto_arrange_icons;	       /* automagically arrange icons */
    
    char icon_box_position;	       /* position to place icons */
    
    char iconification_style;          /* position to place icons */
    
    char disable_root_mouse;	       /* disable button events in root window */
    
    char auto_focus;		       /* focus window when it's mapped */


    char *icon_back_file;	       /* background image for icons */

    WCoord *root_menu_pos;	       /* initial position of the root menu*/
    WCoord *app_menu_pos;

    WCoord *win_menu_pos;
    
    char icon_yard;		       /* aka iconbox */

    int raise_delay;		       /* delay for autoraise. 0 is disabled */

    int cmap_size;		       /* size of dithering colormap in colors
					* per channel */

    int icon_size;		       /* size of the icon */

    char menu_style;		       /* menu decoration style */

    char workspace_name_display_position;
    
    unsigned int modifier_mask;	       /* mask to use as kbd modifier */


    char ws_advance;                   /* Create new workspace and advance */

    char ws_cycle;                     /* Cycle existing workspaces */

    char save_session_on_exit;	       /* automatically save session on exit */

    char sticky_icons;		       /* If miniwindows will be onmipresent */

    char dont_confirm_kill;	       /* do not confirm Kill application */

    char disable_miniwindows;

    char dont_blink;		       /* do not blink icon selection */
    
    /* Appearance options */
    char new_style;		       /* Use newstyle buttons */
    char superfluous;		       /* Use superfluous things */

    /* root window mouse bindings */
    char select_button;		       /* button for window selection */
    char windowl_button;	       /* button for window list menu */
    char menu_button;		       /* button for app menu */

    /* balloon text */
    char window_balloon;
    char miniwin_balloon;
    char appicon_balloon;
    char help_balloon;
        
#ifdef WEENDOZE_CYCLE
    char windoze_cycling;	       /* Windoze 95 style Alt+Tabbing */
    char popup_switchmenu;	       /* Popup the switchmenu when Alt+Tabbing */
#endif /* WEENDOZE_CYCLE */
    /* some constants */
    int dblclick_time;		       /* double click delay time in ms */

    /* animate menus */
    char menu_scroll_speed;	       /* how fast menus are scrolled */

    /* animate icon sliding */
    char icon_slide_speed;	       /* icon slide animation speed */

    /* shading animation */
    char shade_speed;

    int edge_resistance;
    char attract;

    struct {
        unsigned int nodock:1;	       /* don't display the dock */
        unsigned int noclip:1;        /* don't display the clip */
	unsigned int nocpp:1;	       /* don't use cpp */
	unsigned int noupdates:1;      /* don't require ~/GNUstep (-static) */
    } flags;			       /* internal flags */
} WPreferences;



/****** Global Variables  ******/
extern Display	*dpy;
extern char *ProgName;
extern unsigned int ValidModMask;
extern char WProgramState;

/****** Global Functions ******/
extern void wAbort(Bool dumpCore);

#endif
