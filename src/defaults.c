/* defaults.c - manage configuration through defaults db
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <wraster.h>


#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "texture.h"
#include "screen.h"
#include "resources.h"
#include "defaults.h"
#include "keybind.h"
#include "xmodifier.h"
#include "icon.h"
#include "funcs.h"
#include "actions.h"
#include "dock.h"
#include "workspace.h"


/*
 * Our own proplist reader parser. This one will not accept any
 * syntax errors and is more descriptive in the error messages.
 * It also doesn't seem to crash.
 */
extern proplist_t ReadProplistFromFile(char *file);


/***** Global *****/

extern WDDomain *WDWindowMaker;
extern WDDomain *WDWindowAttributes;
extern WDDomain *WDRootMenu;

extern int wScreenCount;

/*
extern proplist_t wDomainName;
extern proplist_t wAttributeDomainName;
 */
extern WPreferences wPreferences;

extern WShortKey wKeyBindings[WKBD_LAST];

typedef struct {
    char *key;
    char *default_value;
    void *extra_data;
    void *addr;
    int (*convert)();
    int (*update)();
    proplist_t plkey;
    proplist_t plvalue;		       /* default value */
} WDefaultEntry;


/* used to map strings to integers */
typedef struct {
    char *string;
    short value;
    char is_alias;
} WOptionEnumeration;



/* type converters */
static int getBool();
static int getInt();
static int getCoord();
#if 0
/* this is not used yet */
static int getString();
#endif
static int getPathList();
static int getEnum();
static int getTexture();
static int getWSBackground();
static int getWSSpecificBackground();
static int getFont();
static int getColor();
static int getKeybind();
static int getModMask();


/* value setting functions */
static int setJustify();
static int setIfDockPresent();
static int setStickyIcons();
/*
static int setPositive();
*/
static int setWidgetColor();
static int setIconTile();
static int setWinTitleFont();
static int setMenuTitleFont();
static int setMenuTextFont();
static int setIconTitleFont();
static int setIconTitleColor();
static int setIconTitleBack();
static int setDisplayFont();
static int setWTitleColor();
static int setFTitleBack();
static int setPTitleBack();
static int setUTitleBack();
static int setWorkspaceBack();
static int setWorkspaceSpecificBack();
static int setMenuTitleColor();
static int setMenuTextColor();
static int setMenuDisabledColor();
static int setMenuTitleBack();
static int setMenuTextBack();
static int setHightlight();
static int setHightlightText();
static int setKeyGrab();
static int setDoubleClick();
static int setIconPosition();

static int setClipTitleFont();
static int setClipTitleColor();

static int updateUsableArea();



/*
 * Tables to convert strings to enumeration values.
 * Values stored are char
 */


/* WARNING: sum of length of all value strings must not exceed
 * this value */
#define TOTAL_VALUES_LENGTH	80



static WOptionEnumeration seFocusModes[] = {
    {"Manual", WKF_CLICK, 0}, {"ClickToFocus", WKF_CLICK, 1},
    {"Auto", WKF_POINTER, 0}, {"FocusFollowMouse", WKF_POINTER, 1},
    {"Sloppy", WKF_SLOPPY, 0}, {"SemiAuto", WKF_SLOPPY, 1},
    {NULL, 0, 0}
};

static WOptionEnumeration seColormapModes[] = {
    {"Manual", WKF_CLICK, 0}, {"ClickToFocus", WKF_CLICK, 1},
    {"Auto", WKF_POINTER, 0}, {"FocusFollowMouse", WKF_POINTER, 1},
    {NULL, 0, 0}
};

static WOptionEnumeration sePlacements[] = {
    {"Auto", WPM_SMART, 0}, {"Smart", WPM_SMART, 1},
    {"Cascade", WPM_CASCADE, 0},
    {"Random", WPM_RANDOM, 0},
    {"Manual", WPM_MANUAL, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seGeomDisplays[] = {
    {"Center", WDIS_CENTER, 0},
    {"Corner", WDIS_TOPLEFT, 0},
    {"Floating", WDIS_FRAME_CENTER, 0},
    {"Line", WDIS_NEW, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seSpeeds[] = {
    {"UltraFast", SPEED_ULTRAFAST, 0},
    {"Fast", SPEED_FAST, 0},
    {"Medium", SPEED_MEDIUM, 0},
    {"Slow", SPEED_SLOW, 0},
    {"UltraSlow", SPEED_ULTRASLOW, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seMouseButtons[] = {
    {"Left", Button1, 0}, {"Button1", Button1, 1},
    {"Middle", Button2, 0}, {"Button2", Button2, 1},
    {"Right", Button3, 0}, {"Button3", Button3, 1},
    {"Button4", Button4, 0},
    {"Button5", Button5, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seIconificationStyles[] = {
    {"Zoom", WIS_ZOOM, 0},
    {"Twist", WIS_TWIST, 0},
    {"Flip", WIS_FLIP, 0},
    {"None", WIS_NONE, 0},
    {"random", WIS_RANDOM, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seJustifications[] = {
    {"Left", WTJ_LEFT, 0},
    {"Center", WTJ_CENTER, 0},
    {"Right", WTJ_RIGHT, 0},
    {NULL, 0, 0}
};

static WOptionEnumeration seIconPositions[] = {
    {"blv", IY_BOTTOM|IY_LEFT|IY_VERT, 0},
    {"blh", IY_BOTTOM|IY_LEFT|IY_HORIZ, 0},
    {"brv", IY_BOTTOM|IY_RIGHT|IY_VERT, 0},
    {"brh", IY_BOTTOM|IY_RIGHT|IY_HORIZ, 0},
    {"tlv", IY_TOP|IY_LEFT|IY_VERT, 0},
    {"tlh", IY_TOP|IY_LEFT|IY_HORIZ, 0},
    {"trv", IY_TOP|IY_RIGHT|IY_VERT, 0},
    {"trh", IY_TOP|IY_RIGHT|IY_HORIZ, 0},
    {NULL, 0, 0}
};



/*
 * All entries in the tables bellow, NEED to have a default value
 * defined, and this value needs to be correct.
 */

/* these options will only affect the window manager on startup 
 *
 * static defaults can't access the screen data, because it is
 * created after these defaults are read
 */
WDefaultEntry staticOptionList[] = {    
    
    {"DisableDithering",	"NO",			NULL,
	  &wPreferences.no_dithering,	getBool,	NULL
    },
    {"ColormapSize",	"4",			NULL,
	  &wPreferences.cmap_size,	getInt,		NULL
    },
    /* static by laziness */
    {"IconSize",	"64",			NULL,
	  &wPreferences.icon_size,	getInt,		NULL
    },
    {"ModifierKey",	"Mod1",			NULL,
	  &wPreferences.modifier_mask,	getModMask,	NULL
    },
    {"DisableWSMouseActions", "NO",		NULL,
	  &wPreferences.disable_root_mouse, getBool,	NULL
    },
    {"FocusMode",	"manual",		seFocusModes,
	  &wPreferences.focus_mode,	getEnum,	NULL
    }, /* have a problem when switching from manual to sloppy without restart */
    {"NewStyle",	"NO",			NULL,
	  &wPreferences.new_style, 	getBool, 	NULL
    },
    {"DisableDock",	"NO",		(void*) WM_DOCK,
	  NULL, 	getBool,        setIfDockPresent
    },
    {"DisableClip",	"NO",		(void*) WM_CLIP,
	  NULL, 	getBool,        setIfDockPresent
    }
};



WDefaultEntry optionList[] = {
      /* dynamic options */
    {"IconPosition",	"blh",			seIconPositions,
	  &wPreferences.icon_yard,	getEnum,	setIconPosition
    },
    {"IconificationStyle", "Zoom",             seIconificationStyles,
         &wPreferences.iconification_style,    getEnum, NULL
    },
    {"SelectWindowsMouseButton", "Left",	seMouseButtons,
	  &wPreferences.select_button, 	getEnum,	NULL
    },
    {"WindowListMouseButton", "Middle",		seMouseButtons,
	  &wPreferences.windowl_button, getEnum,	NULL
    },
    {"ApplicationMenuMouseButton", "Right",	seMouseButtons,
	  &wPreferences.menu_button,	getEnum,	NULL
    },
    {"PixmapPath",	DEF_PIXMAP_PATHS,	NULL,
	  &wPreferences.pixmap_path,	getPathList,	NULL
    },
    {"IconPath",	DEF_ICON_PATHS,		NULL,
	  &wPreferences.icon_path,	getPathList,	NULL
    },
    {"ColormapMode",	"auto",			seColormapModes,
	  &wPreferences.colormap_mode,	getEnum,	NULL
    },
    {"AutoFocus", 	"NO",			NULL,
	  &wPreferences.auto_focus,	getBool,	NULL
    },
    {"RaiseDelay",	"0",			NULL,
	  &wPreferences.raise_delay,	getInt,		NULL
    },
    {"CirculateRaise",	"NO",			NULL,
	  &wPreferences.circ_raise, 	getBool, 	NULL
    },
    {"Superfluous",	"NO",			NULL,
	  &wPreferences.superfluous, 	getBool, 	NULL
    },
    {"AdvanceToNewWorkspace", "NO",		NULL,
	  &wPreferences.ws_advance,      getBool,       NULL
    },
    {"CycleWorkspaces", "NO",			NULL,
	  &wPreferences.ws_cycle,        getBool,	NULL
    },
    {"StickyIcons", "NO",			NULL,
	  &wPreferences.sticky_icons, getBool, setStickyIcons
    },
    {"SaveSessionOnExit", "NO",			NULL,
	  &wPreferences.save_session_on_exit, getBool,	NULL
    },
    {"WrapMenus", "NO",				NULL,
	  &wPreferences.wrap_menus,	getBool,	NULL
    },
    {"ScrollableMenus",	"NO",			NULL,
	  &wPreferences.scrollable_menus, getBool,	NULL
    },
    {"MenuScrollSpeed",	"medium",    		seSpeeds,
	  &wPreferences.menu_scroll_speed, getEnum,    	NULL
    },
    {"IconSlideSpeed",	"medium",		seSpeeds,
	  &wPreferences.icon_slide_speed, getEnum,     	NULL
    },
    {"ShadeSpeed",	"medium",    		seSpeeds,
	  &wPreferences.shade_speed,	 getEnum,	NULL
    },
    {"DoubleClickTime",	"250",   (void*) &wPreferences.dblclick_time,
	  &wPreferences.dblclick_time, getInt,        setDoubleClick,
    },
    {"AlignSubmenus",	"NO",			NULL,
	  &wPreferences.align_menus,	getBool,	NULL
    },
    {"OpenTransientOnOwnerWorkspace",	"NO",			NULL,
	  &wPreferences.open_transients_with_parent, getBool,	NULL
    },
    {"WindowPlacement",	"auto",		sePlacements,
	  &wPreferences.window_placement, getEnum,	NULL
    },
    {"IgnoreFocusClick","NO",			NULL,
	  &wPreferences.ignore_focus_click, getBool,	NULL
    },
    {"UseSaveUnders",	"NO",			NULL,
	  &wPreferences.use_saveunders,	getBool,	NULL
    },
    {"OpaqueMove",	"NO",			NULL,
	  &wPreferences.opaque_move,	getBool,	NULL
    },
    {"DisableSound",	"NO",			NULL,
	  &wPreferences.no_sound,	getBool,	NULL
    },
    {"DisableAnimations",	"NO",			NULL,
	  &wPreferences.no_animations,	getBool,	NULL
    },
    {"DontLinkWorkspaces","NO",			NULL,
	  &wPreferences.no_autowrap,	getBool,	NULL
    },
    {"AutoArrangeIcons", "NO",			NULL,
	  &wPreferences.auto_arrange_icons, getBool,	NULL
    },
    {"NoWindowOverDock", "NO",			NULL,
	  &wPreferences.no_window_over_dock, getBool,   updateUsableArea
    },
    {"NoWindowOverIcons", "NO",			NULL,
	  &wPreferences.no_window_over_icons, getBool,  updateUsableArea
    },
    {"WindowPlaceOrigin", "(0, 0)",		NULL,
	  &wPreferences.window_place_origin, getCoord,  NULL
    },
    {"ResizeDisplay",	"corner",	       	seGeomDisplays,
	  &wPreferences.size_display,	getEnum,	NULL
    },
    {"MoveDisplay",	"corner",      		seGeomDisplays,
	  &wPreferences.move_display,	getEnum,	NULL
    },
    {"DontConfirmKill",	"NO",			NULL,
	  &wPreferences.dont_confirm_kill,	getBool,NULL
    },
    {"WindowTitleBalloons", "NO",		NULL,
	  &wPreferences.window_balloon,	getBool,	NULL
    },
    {"MiniwindowTitleBalloons", "NO",		NULL,
	  &wPreferences.miniwin_balloon,getBool,	NULL
    },
    {"AppIconBalloons", "NO",		NULL,
	  &wPreferences.appicon_balloon,getBool,	NULL
    },
    {"EdgeResistance", 	"30",			NULL,
	  &wPreferences.edge_resistance,getInt,		NULL
    },
    {"DisableBlinking",	"NO",		NULL,
	   &wPreferences.dont_blink,	getBool,	NULL
    },
#ifdef WEENDOZE_CYCLE
    {"WindozeCycling","NO", NULL,
	    &wPreferences.windoze_cycling,	getBool, NULL
    },
    {"PopupSwitchMenu","YES",NULL, 
	    &wPreferences.popup_switchmenu,	getBool, NULL
    },
#endif /* WEENDOZE_CYCLE */
      /* style options */
    {"WidgetColor",	"(solid, gray)",	NULL,
	  NULL,				getTexture,	setWidgetColor,
    },
    {"WorkspaceSpecificBack","()",	NULL,
	  NULL,		getWSSpecificBackground, setWorkspaceSpecificBack
    },
	/* WorkspaceBack must come after WorkspaceSpecificBack or
	 * WorkspaceBack wont know WorkspaceSpecificBack was also
	 * specified and 2 copies of wmsetbg will be launched */
    {"WorkspaceBack",	"(solid, black)",	NULL,
	  NULL,				getWSBackground,setWorkspaceBack
    },
    {"IconBack",	"(solid, gray)",       	NULL,
	  NULL,				getTexture,	setIconTile
    },
    {"TitleJustify",	"center",		seJustifications,
	  &wPreferences.title_justification, getEnum,	setJustify
    },
    {"WindowTitleFont",	DEF_TITLE_FONT,	       	NULL,
	  NULL,				getFont,	setWinTitleFont
    },
    {"MenuTitleFont",	DEF_MENU_TITLE_FONT,	NULL,
	  NULL,				getFont,	setMenuTitleFont
    },
    {"MenuTextFont",	DEF_MENU_ENTRY_FONT,	NULL,
	  NULL,				getFont,	setMenuTextFont
    },
    {"IconTitleFont",	DEF_ICON_TITLE_FONT,	NULL,
	  NULL,				getFont,	setIconTitleFont
    },
    {"ClipTitleFont",	DEF_CLIP_TITLE_FONT,	NULL,
	  NULL,				getFont,	setClipTitleFont
    },
    {"DisplayFont",	DEF_INFO_TEXT_FONT,	NULL,
	  NULL,				getFont,	setDisplayFont
    },
    {"HighlightColor",	"white",		NULL,
	  NULL,				getColor,	setHightlight
    },
    {"HighlightTextColor",	"black",	NULL,
	  NULL,				getColor,	setHightlightText
    },
    {"ClipTitleColor",	"black",		(void*)CLIP_NORMAL,
	  NULL,				getColor,	setClipTitleColor
    },
    {"CClipTitleColor", "\"#454045\"",		(void*)CLIP_COLLAPSED,
	  NULL,				getColor,	setClipTitleColor
    },
    {"FTitleColor",	"white",		(void*)WS_FOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"PTitleColor",	"white",		(void*)WS_PFOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"UTitleColor",	"black",		(void*)WS_UNFOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"FTitleBack",	"(solid, black)",      	NULL,
	  NULL,				getTexture,	setFTitleBack
    },
    {"PTitleBack",	"(solid, \"#616161\")",	NULL,
	  NULL,				getTexture,	setPTitleBack
    },
    {"UTitleBack",	"(solid, gray)",	NULL,
	  NULL,				getTexture,	setUTitleBack
    },
    {"MenuTitleColor",	"white",       		NULL,
	  NULL,				getColor,	setMenuTitleColor
    },
    {"MenuTextColor",	"black",       		NULL,
	  NULL,				getColor,	setMenuTextColor
    },
    {"MenuDisabledColor", "\"#616161\"",       	NULL,
	  NULL,				getColor,	setMenuDisabledColor
    },
    {"MenuTitleBack",	"(solid, black)",      	NULL,
	  NULL,				getTexture,	setMenuTitleBack
    },
    {"MenuTextBack",	"(solid, gray)",       	NULL,
	  NULL,				getTexture,	setMenuTextBack
    },
    {"IconTitleColor",	"white",       		NULL,
	  NULL,				getColor,	setIconTitleColor
    },
    {"IconTitleBack",	"black",       		NULL,
	  NULL,				getColor,	setIconTitleBack
    },

	/* keybindings */
#ifndef LITE
    {"RootMenuKey",	"None",			(void*)WKBD_ROOTMENU,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"WindowListKey",	"None",			(void*)WKBD_WINDOWLIST,
	  NULL,				getKeybind,	setKeyGrab
    },
#endif /* LITE */
    {"WindowMenuKey",	"None",		       	(void*)WKBD_WINDOWMENU,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipLowerKey",   "None",		        (void*)WKBD_CLIPLOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipRaiseKey",   "None",		        (void*)WKBD_CLIPRAISE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipRaiseLowerKey", "None",		 (void*)WKBD_CLIPRAISELOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"MiniaturizeKey",	"None",	       		(void*)WKBD_MINIATURIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"HideKey", "None",	       			(void*)WKBD_HIDE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"MoveResizeKey", "None",	       		(void*)WKBD_MOVERESIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"CloseKey", "None",       			(void*)WKBD_CLOSE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"MaximizeKey", "None",			(void*)WKBD_MAXIMIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"VMaximizeKey", "None",			(void*)WKBD_VMAXIMIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"RaiseKey", "Meta+Up",			(void*)WKBD_RAISE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"LowerKey", "Meta+Down",			(void*)WKBD_LOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"RaiseLowerKey", "None",			(void*)WKBD_RAISELOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ShadeKey", "None",			(void*)WKBD_SHADE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"SelectKey", "None",			(void*)WKBD_SELECT,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"FocusNextKey", "None",		       	(void*)WKBD_FOCUSNEXT,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"FocusPrevKey", "None",			(void*)WKBD_FOCUSPREV,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"NextWorkspaceKey", "None",		(void*)WKBD_NEXTWORKSPACE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"PrevWorkspaceKey", "None",       		(void*)WKBD_PREVWORKSPACE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"NextWorkspaceLayerKey", "None",		(void*)WKBD_NEXTWSLAYER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"PrevWorkspaceLayerKey", "None", 		(void*)WKBD_PREVWSLAYER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace1Key", "None",			(void*)WKBD_WORKSPACE1,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace2Key", "None",			(void*)WKBD_WORKSPACE2,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace3Key", "None",			(void*)WKBD_WORKSPACE3,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace4Key", "None",			(void*)WKBD_WORKSPACE4,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace5Key", "None",			(void*)WKBD_WORKSPACE5,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace6Key", "None",			(void*)WKBD_WORKSPACE6,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace7Key", "None",			(void*)WKBD_WORKSPACE7,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace8Key", "None",			(void*)WKBD_WORKSPACE8,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace9Key", "None",			(void*)WKBD_WORKSPACE9,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace10Key", "None",			(void*)WKBD_WORKSPACE10,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"WindowShortcut1Key","None",		(void*)WKBD_WINDOW1,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut2Key","None",		(void*)WKBD_WINDOW2,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut3Key","None",		(void*)WKBD_WINDOW3,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut4Key","None",		(void*)WKBD_WINDOW4,
	    NULL, 			getKeybind,	setKeyGrab
    }
#ifdef EXTEND_WINDOWSHORTCUT
    ,{"WindowShortcut5Key","None",		(void*)WKBD_WINDOW5,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut6Key","None",		(void*)WKBD_WINDOW6,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut7Key","None",		(void*)WKBD_WINDOW7,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut8Key","None",		(void*)WKBD_WINDOW8,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut9Key","None",		(void*)WKBD_WINDOW9,
	    NULL, 			getKeybind,	setKeyGrab
    },
    {"WindowShortcut10Key","None",		(void*)WKBD_WINDOW10,
	    NULL, 			getKeybind,	setKeyGrab
    }
#endif /* EXTEND_WINDOWSHORTCUT */

#ifdef KEEP_XKB_LOCK_STATUS
    ,{"ToggleKbdModeKey", "None",                      (void*)WKBD_TOGGLE,
	    NULL,                       getKeybind,     setKeyGrab
    },
    {"KbdModeLock", "NO",  	                  NULL,
	    &wPreferences.modelock, 	getBool,	NULL
    }
#endif /* KEEP_XKB_LOCK_STATUS */
#ifdef TITLE_TEXT_SHADOW
    ,{"FShadowColor",     "black",                (void*)WS_SFOCUSED,
          NULL,                         getColor,       setWTitleColor
    },
    {"PShadowColor",     "black",                (void*)WS_SPFOCUSED,
          NULL,                         getColor,       setWTitleColor
    },
    {"UShadowColor",     "grey50",               (void*)WS_SUNFOCUSED,
         NULL,                         getColor,       setWTitleColor
    },
    {"MShadowColor",     "black",               (void*)WS_SMENU,
          NULL,                         getColor,       setMenuTitleColor
    },
    {"Shadow", "Yes",                    NULL,
         &wPreferences.title_shadow, getBool,  setJustify
    }
#endif /* TITLE_TEXT_SHADOW */
};


#if 0
static void rereadDefaults(void);
#endif

#if 0
static void
rereadDefaults(void)
{
    /* must defer the update because accessing X data from a
     * signal handler can mess up Xlib */
    
}
#endif

static void
initDefaults()
{
    int i;
    WDefaultEntry *entry;

    PLSetStringCmpHook(StringCompareHook);

    for (i=0; i<sizeof(optionList)/sizeof(WDefaultEntry); i++) {
	entry = &optionList[i];

	entry->plkey = PLMakeString(entry->key);
	if (entry->default_value)
	    entry->plvalue = PLGetProplistWithDescription(entry->default_value);
	else
	    entry->plvalue = NULL;
    }
    
    for (i=0; i<sizeof(staticOptionList)/sizeof(WDefaultEntry); i++) {
	entry = &staticOptionList[i];
	
	entry->plkey = PLMakeString(entry->key);
	if (entry->default_value)
	    entry->plvalue = PLGetProplistWithDescription(entry->default_value);
	else
	    entry->plvalue = NULL;
    }
        
/*
    wDomainName = PLMakeString(WMDOMAIN_NAME);
    wAttributeDomainName = PLMakeString(WMATTRIBUTE_DOMAIN_NAME);

    PLRegister(wDomainName, rereadDefaults);
    PLRegister(wAttributeDomainName, rereadDefaults);
 */
}




#if 0
proplist_t
wDefaultsInit(int screen_number)
{
    static int defaults_inited = 0;
    proplist_t dict;
    
    if (!defaults_inited) {
	initDefaults();
    }
    
    dict = PLGetDomain(wDomainName);
    if (!dict) {
	wwarning(_("could not read domain \"%s\" from defaults database"),
		 PLGetString(wDomainName));
    }

    return dict;
}
#endif


void
wDefaultsDestroyDomain(WDDomain *domain)
{
    if (domain->dictionary)
	PLRelease(domain->dictionary);
    free(domain->path);
    free(domain);
}


WDDomain*
wDefaultsInitDomain(char *domain, Bool requireDictionary)
{
    WDDomain *db;
    struct stat stbuf;
    static int inited = 0;
    char path[PATH_MAX];
    char *the_path;
    proplist_t shared_dict=NULL;

    if (!inited) {
	inited = 1;
	initDefaults();
    }

    db = wmalloc(sizeof(WDDomain));
    memset(db, 0, sizeof(WDDomain));
    db->domain_name = domain;
    db->path = wdefaultspathfordomain(domain);
    the_path = db->path;
    
    if (the_path && stat(the_path, &stbuf)>=0) {
	db->dictionary = ReadProplistFromFile(the_path);
	if (db->dictionary) {
	    if (requireDictionary && !PLIsDictionary(db->dictionary)) {
		PLRelease(db->dictionary);
		db->dictionary = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 domain, the_path);
	    }
	    db->timestamp = stbuf.st_mtime;
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     domain);
	}
    }

    /* global system dictionary */
    sprintf(path, "%s/%s", SYSCONFDIR, domain);
    if (stat(path, &stbuf)>=0) {
        shared_dict = ReadProplistFromFile(path);
        if (shared_dict) {
	    if (requireDictionary && !PLIsDictionary(shared_dict)) {
		wwarning(_("Domain %s (%s) of global defaults database is corrupted!"),
			 domain, path);
		PLRelease(shared_dict);
		shared_dict = NULL;
	    } else {
		if (db->dictionary && PLIsDictionary(shared_dict) &&
		    PLIsDictionary(db->dictionary)) {
		    PLMergeDictionaries(shared_dict, db->dictionary);
		    PLRelease(db->dictionary);
		    db->dictionary = shared_dict;
		    if (stbuf.st_mtime > db->timestamp)
			db->timestamp = stbuf.st_mtime;
		} else if (!db->dictionary) {
		    db->dictionary = shared_dict;
		    if (stbuf.st_mtime > db->timestamp)
			db->timestamp = stbuf.st_mtime;
		}
            }
        } else {
	    wwarning(_("could not load domain %s from global defaults database"),
		     domain);
	}
    }

    /* set to save it in user's directory, no matter from where it was read */
    if (db->dictionary) {
        proplist_t tmp = PLMakeString(db->path);

        PLSetFilename(db->dictionary, tmp);
        PLRelease(tmp);
    }

    return db;
}


void
wReadStaticDefaults(proplist_t dict)
{
    proplist_t plvalue;
    WDefaultEntry *entry;
    int i;
    void *tdata;

    
    for (i=0; i<sizeof(staticOptionList)/sizeof(WDefaultEntry); i++) {
	entry = &staticOptionList[i];

	if (dict)
	  plvalue = PLGetDictionaryEntry(dict, entry->plkey);
	else
	  plvalue = NULL;
	
	if (!plvalue) {
	    /* no default in the DB. Use builtin default */
	    plvalue = entry->plvalue;
	}
	
	if (plvalue) {
	    /* convert data */
	    (*entry->convert)(NULL, entry, plvalue, entry->addr, &tdata);
            if (entry->update) {
                (*entry->update)(NULL, entry, tdata, entry->extra_data);
            }
	}
    }
}

void
wDefaultsCheckDomains(void *foo)
{
    WScreen *scr;
    struct stat stbuf;
    proplist_t dict;
    int i;
    char path[PATH_MAX];

#ifdef HEARTBEAT
    puts("Checking domains...");
#endif
    if (stat(WDWindowMaker->path, &stbuf)>=0
	&& WDWindowMaker->timestamp < stbuf.st_mtime) {
	proplist_t shared_dict = NULL;
#ifdef HEARTBEAT
	puts("Checking WindowMaker domain");
#endif
	WDWindowMaker->timestamp = stbuf.st_mtime;	

	/* global dictionary */
	sprintf(path, "%s/WindowMaker", SYSCONFDIR);
	if (stat(path, &stbuf)>=0) {
	    shared_dict = ReadProplistFromFile(path);
	    if (shared_dict && !PLIsDictionary(shared_dict)) {
		wwarning(_("Domain %s (%s) of global defaults database is corrupted!"),
			 "WindowMaker", path);
		PLRelease(shared_dict);
		shared_dict = NULL;
	    } else if (!shared_dict) {
		wwarning(_("could not load domain %s from global defaults database"),
			 "WindowMaker");
	    }
	}
	/* user dictionary */
	dict = ReadProplistFromFile(WDWindowMaker->path);
	if (dict) {
	    if (!PLIsDictionary(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WindowMaker", WDWindowMaker->path);
	    } else {
		if (shared_dict) {
		    PLSetFilename(shared_dict, PLGetFilename(dict));
		    PLMergeDictionaries(shared_dict, dict);
		    PLRelease(dict);
		    dict = shared_dict;
		    shared_dict = NULL;
		}
		for (i=0; i<wScreenCount; i++) {
		    scr = wScreenWithNumber(i);
		    if (scr)
			wReadDefaults(scr, dict);
		}
		if (WDWindowMaker->dictionary) {
		    PLRelease(WDWindowMaker->dictionary);
		}
		WDWindowMaker->dictionary = dict;
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WindowMaker");
	}
	if (shared_dict) {
	    PLRelease(shared_dict);
	}
    }

    if (stat(WDWindowAttributes->path, &stbuf)>=0
	&& WDWindowAttributes->timestamp < stbuf.st_mtime) {
#ifdef HEARTBEAT
	puts("Checking WMWindowAttributes domain");
#endif
	dict = ReadProplistFromFile(WDWindowAttributes->path);
	if (dict) {
	    if (!PLIsDictionary(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WMWindowAttributes", WDWindowAttributes->path);
	    } else {
		if (WDWindowAttributes->dictionary)
		    PLRelease(WDWindowAttributes->dictionary);
		WDWindowAttributes->dictionary = dict;
		for (i=0; i<wScreenCount; i++) {
		    scr = wScreenWithNumber(i);
		    if (scr)
			wDefaultUpdateIcons(scr);
		}
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WMWindowAttributes");
	}
	WDWindowAttributes->timestamp = stbuf.st_mtime;
    }

#ifndef LITE
    if (stat(WDRootMenu->path, &stbuf)>=0
	&& WDRootMenu->timestamp < stbuf.st_mtime) {
	dict = ReadProplistFromFile(WDRootMenu->path);
#ifdef HEARTBEAT
	puts("Checking WMRootMenu domain");
#endif
	if (dict) {
	    if (!PLIsArray(dict) && !PLIsString(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WMRootMenu", WDRootMenu->path);
	    } else {
		if (WDRootMenu->dictionary) {
		    PLRelease(WDRootMenu->dictionary);
		}
		WDRootMenu->dictionary = dict;
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WMRootMenu");
	}
	WDRootMenu->timestamp = stbuf.st_mtime;
    }
#endif /* !LITE */

    WMAddTimerHandler(DEFAULTS_CHECK_INTERVAL, wDefaultsCheckDomains, foo);
}



#define REFRESH_WINDOW_TEXTURES	(1<<0)
#define REFRESH_MENU_TEXTURES	(1<<1)
#define REFRESH_WINDOW_FONT	(1<<2)
#define REFRESH_MENU_TITLE_FONT	(1<<3)
#define REFRESH_MENU_FONT	(1<<4)
#define REFRESH_FORE_COLOR	(1<<5)
#define REFRESH_ICON_TILE	(1<<6)
#define REFRESH_ICON_FONT	(1<<7)
#define REFRESH_WORKSPACE_BACK	(1<<8)

static void 
refreshMenus(WScreen *scr, int flags)
{
    WMenu *menu;

#ifndef LITE
    menu = scr->root_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->switch_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

#endif /* !LITE */

    menu = scr->workspace_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->window_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->icon_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    if (scr->dock) {
	menu = scr->dock->menu;
	if (menu)
	    wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    }
    menu = scr->clip_menu;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    
    menu = scr->clip_submenu;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    
    menu = scr->clip_options;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
}


static void
refreshAppIcons(WScreen *scr, int flags)
{
    WAppIcon *aicon = scr->app_icon_list;

    while (aicon) {
        if (aicon->icon) {
            aicon->icon->force_paint = 1;
        }
        aicon = aicon->next;
    }
}


static void
refreshWindows(WScreen *scr, int flags)
{
    WWindow *wwin;

    wwin = scr->focused_window;
    while (wwin) {
        if (flags & REFRESH_WINDOW_FONT) {
            wWindowConfigureBorders(wwin);
        }
        if ((flags & (REFRESH_ICON_TILE|REFRESH_WINDOW_TEXTURES)) &&
            wwin->flags.miniaturized && wwin->icon) {
            wwin->icon->force_paint = 1;
        }
        if (flags & REFRESH_WINDOW_TEXTURES) {
            wwin->frame->flags.need_texture_remake = 1;
        }
        wwin = wwin->prev;
    }
}


void
wReadDefaults(WScreen *scr, proplist_t new_dict)
{
    proplist_t plvalue, old_value;
    WDefaultEntry *entry;
    int i, must_update;
    int needs_refresh;
    void *tdata;
    proplist_t old_dict = (WDWindowMaker->dictionary!=new_dict 
			   ? WDWindowMaker->dictionary : NULL);
    
    must_update = 0;
    
    needs_refresh = 0;
    
    for (i=0; i<sizeof(optionList)/sizeof(WDefaultEntry); i++) {
	entry = &optionList[i];
	
	if (new_dict)
	    plvalue = PLGetDictionaryEntry(new_dict, entry->plkey);
	else
	    plvalue = NULL;

	if (!old_dict)
	    old_value = NULL;
	else
	    old_value = PLGetDictionaryEntry(old_dict, entry->plkey);

	
	if (!plvalue && !old_value) {
	    /* no default in  the DB. Use builtin default */
	    plvalue = entry->plvalue;
	    if (plvalue && new_dict) {
		PLInsertDictionaryEntry(new_dict, entry->plkey, plvalue);
		must_update = 1;
	    }
	} else if (!plvalue) {
	    /* value was deleted from DB. Keep current value */
	    continue;
	} else if (!old_value) {
	    /* set value for the 1st time */
	} else if (!PLIsEqual(plvalue, old_value)) {	    
	    /* value has changed */
	} else {
	    /* value was not changed since last time */
	    continue;
	}
	
	if (plvalue) {
#ifdef DEBUG
	    printf("Updating %s to %s\n", entry->key, PLGetDescription(plvalue));
#endif
	    /* convert data */
	    if ((*entry->convert)(scr, entry, plvalue, entry->addr, &tdata)) {
		if (entry->update) {
		    needs_refresh |= 
			(*entry->update)(scr, entry, tdata, entry->extra_data);
		}
	    }
	}
    }

    if (needs_refresh!=0) {
	int foo;

	foo = 0;
	if (needs_refresh & REFRESH_MENU_TEXTURES)
	    foo |= MR_TEXT_BACK;
	if (needs_refresh & REFRESH_MENU_FONT)
	    foo |= MR_RESIZED;
	if (needs_refresh & REFRESH_MENU_TITLE_FONT)
	    foo |= MR_TITLE_TEXT;

        if (foo)
            refreshMenus(scr, foo);

        if (needs_refresh & (REFRESH_WINDOW_TEXTURES|REFRESH_WINDOW_FONT|
                             REFRESH_ICON_TILE))
            refreshWindows(scr, needs_refresh);

        if (needs_refresh & REFRESH_ICON_TILE)
            refreshAppIcons(scr, needs_refresh);

        wRefreshDesktop(scr);
    }
}


void
wDefaultUpdateIcons(WScreen *scr)
{
    WAppIcon *aicon = scr->app_icon_list;
    WWindow *wwin = scr->focused_window;
    char *file;

    while(aicon) {
        file = wDefaultGetIconFile(scr, aicon->wm_instance, aicon->wm_class,
				   False);
        if ((file && aicon->icon->file && strcmp(file, aicon->icon->file)!=0)
            || (file && !aicon->icon->file)) {
            RImage *new_image;

            if (aicon->icon->file)
                free(aicon->icon->file);
            aicon->icon->file = wstrdup(file);

            new_image = wDefaultGetImage(scr, aicon->wm_instance,
					 aicon->wm_class);
            if (new_image) {
                wIconChangeImage(aicon->icon, new_image);
                wAppIconPaint(aicon);
            }
        }
        aicon = aicon->next;
    }

    if (!wPreferences.flags.noclip)
        wClipIconPaint(scr->clip_icon);

    while (wwin) {
        if (wwin->icon && wwin->flags.miniaturized) {
            file = wDefaultGetIconFile(scr, wwin->wm_instance, wwin->wm_class,
					False);
            if ((file && wwin->icon->file && strcmp(file, wwin->icon->file)!=0)
                || (file && !wwin->icon->file)) {
                RImage *new_image;

                if (wwin->icon->file)
                    free(wwin->icon->file);
                wwin->icon->file = wstrdup(file);

                new_image = wDefaultGetImage(scr, wwin->wm_instance,
					     wwin->wm_class);
                if (new_image)
                    wIconChangeImage(wwin->icon, new_image);
            }
        }
        wwin = wwin->prev;
    }
}


/* --------------------------- Local ----------------------- */

#define STRINGP(x) if (!PLIsString(value)) { \
	wwarning(_("Wrong option format for key \"%s\". Should be %s."), \
		entry->key, x); \
	return False; }



static int
string2index(proplist_t key, proplist_t val, proplist_t def,
	     WOptionEnumeration *values)
{
    char *str;
    WOptionEnumeration *v;
    char buffer[TOTAL_VALUES_LENGTH];

    if (PLIsString(val) && (str = PLGetString(val))) {
	for (v=values; v->string!=NULL; v++) {
	    if (strcasecmp(v->string, str)==0)
		return v->value;
	}
    }

    buffer[0] = 0;
    for (v=values; v->string!=NULL; v++) {
	if (!v->is_alias) {
	    if (buffer[0]!=0)
		strcat(buffer, ", ");
	    strcat(buffer, v->string);
	}
    }
    wwarning(_("wrong option value for key \"%s\". Should be one of %s"),
	     PLGetString(key), buffer);

    if (def) {
	return string2index(key, val, NULL, values);
    }

    return -1;
}




/*
 * value - is the value in the defaults DB
 * addr - is the address to store the data
 * ret - is the address to store a pointer to a temporary buffer. ret
 * 	must not be freed and is used by the set functions
 */
static int 
getBool(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
	void **ret)
{
    static char data;
    char *val;
    int second_pass=0;

    STRINGP("Boolean");

    val = PLGetString(value);

again:
    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y'))
	|| strcasecmp(val, "YES")==0) {

	data = 1;
    } else if ((val[1]=='\0' && (val[0]=='n' || val[0]=='N'))
		|| strcasecmp(val, "NO")==0) {
      data = 0;
    } else {
	int i;
	if (sscanf(val, "%i", &i)==1) {
	    if (i!=0)
		data = 1;
	    else
		data = 0;
	} else {
	    wwarning(_("can't convert \"%s\" to boolean for key \"%s\""),
		     val, entry->key);
	    if (second_pass==0) {
		val = PLGetString(entry->plvalue);
		second_pass = 1;
		wwarning(_("using default \"%s\" instead"), val);
		goto again;
	    }
	    return False;
	}
    }
    
    if (ret)
      *ret = &data;
    
    if (addr) {
	*(char*)addr = data;
    }
    
    return True;
}


static int 
getInt(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
       void **ret)
{
    static int data;
    char *val;
    
    
    STRINGP("Integer");
    
    val = PLGetString(value);
    
    if (sscanf(val, "%i", &data)!=1) {
        wwarning(_("can't convert \"%s\" to integer for key \"%s\""),
                 val, entry->key);
        val = PLGetString(entry->plvalue);
        wwarning(_("using default \"%s\" instead"), val);
        if (sscanf(val, "%i", &data)!=1) {
            return False;
        }
    }
    
    if (ret)
      *ret = &data;
    
    if (addr) {
	*(int*)addr = data;
    }
    return True;
}


static int 
getCoord(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
         void **ret)
{
    static WCoord data;
    char *val_x, *val_y;
    int nelem, changed=0;
    proplist_t elem_x, elem_y;

again:
    if (!PLIsArray(value)) {
        wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 entry->key, "Coordinate");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    nelem = PLGetNumberOfElements(value);
    if (nelem != 2) {
        wwarning(_("Incorrect number of elements in array for key \"%s\"."),
                 entry->key);
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    elem_x = PLGetArrayElement(value, 0);
    elem_y = PLGetArrayElement(value, 1);
    
    if (!elem_x || !elem_y || !PLIsString(elem_x) || !PLIsString(elem_y)) {
        wwarning(_("Wrong value for key \"%s\". Should be Coordinate."),
                 entry->key);
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    val_x = PLGetString(elem_x);
    val_y = PLGetString(elem_y);

    if (sscanf(val_x, "%i", &data.x)!=1 || sscanf(val_y, "%i", &data.y)!=1) {
        wwarning(_("can't convert array to integers for \"%s\"."), entry->key);
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    if (data.x < 0)
        data.x = 0;
    else if (data.x > scr->scr_width/3)
        data.x = scr->scr_width/3;
    if (data.y < 0)
        data.y = 0;
    else if (data.y > scr->scr_height/3)
        data.y = scr->scr_height/3;

    if (ret)
        *ret = &data;

    if (addr) {
        *(WCoord*)addr = data;
    }

    return True;
}


#if 0
/* This function is not used at the moment. */
static int 
getString(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	  void **ret)
{
    static char *data;
    
    STRINGP("String");
    
    data = PLGetString(value);
    
    if (!data) {
        data = PLGetString(entry->plvalue);
        if (!data)
            return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(char**)addr = wstrdup(data);
    
    return True;
}
#endif


static int 
getPathList(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
	    void **ret)
{
    static char *data;
    int i, count, len;
    char *ptr;
    proplist_t d;
    int changed=0;

again:
    if (!PLIsArray(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 entry->key, "an array of paths");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
	return False; 
    }

    i = 0;
    count = PLGetNumberOfElements(value);
    if (count < 1) {
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    len = 0;
    for (i=0; i<count; i++) {
	d = PLGetArrayElement(value, i);
	if (!d || !PLIsString(d)) {
	    count = i;
	    break;
	}
	len += strlen(PLGetString(d))+1;
    }

    ptr = data = wmalloc(len+1);

    for (i=0; i<count; i++) {
	d = PLGetArrayElement(value, i);
	if (!d || !PLIsString(d)) {
	    break;
	}
	strcpy(ptr, PLGetString(d));
	ptr += strlen(PLGetString(d));
	*ptr = ':';
	ptr++;
    }
    ptr--; *(ptr--) = 0;

    if (*(char**)addr!=NULL) {
	free(*(char**)addr);
    }
    *(char**)addr = data;

    return True;
}


static int 
getEnum(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	void **ret)
{
    static signed char data;

    data = string2index(entry->plkey, value, entry->default_value,
			(WOptionEnumeration*)entry->extra_data);
    if (data < 0)
	return False;

    if (ret)
      *ret = &data;

    if (addr)
      *(signed char*)addr = data;
    
    return True;    
}



/*
 * (solid <color>)
 * (hgradient <color> <color>)
 * (vgradient <color> <color>)
 * (dgradient <color> <color>)
 * (mhgradient <color> <color> ...)
 * (mvgradient <color> <color> ...)
 * (tpixmap <file> <color>)
 * (spixmap <file> <color>)
 * (cpixmap <file> <color>)
 * (thgradient <file> <opaqueness> <color> <color>)
 * (tvgradient <file> <opaqueness> <color> <color>)
 * (tdgradient <file> <opaqueness> <color> <color>)
 */

static WTexture*
parse_texture(WScreen *scr, proplist_t pl)
{
    proplist_t elem;
    char *val;
    int nelem;
    WTexture *texture=NULL;
    
    nelem = PLGetNumberOfElements(pl);
    if (nelem < 1)
      return NULL;
    
    
    elem = PLGetArrayElement(pl, 0);
    if (!elem || !PLIsString(elem))
      return NULL;
    val = PLGetString(elem);

    
    if (strcasecmp(val, "solid")==0) {
	XColor color;

	if (nelem != 2)
	    return NULL;

	/* get color */
	
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &color)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	texture = (WTexture*)wTextureMakeSolid(scr, &color);
    } else if (strcasecmp(val, "dgradient")==0
	       || strcasecmp(val, "vgradient")==0
	       || strcasecmp(val, "hgradient")==0) {
	RColor color1, color2;
	XColor xcolor;
	int type;

	if (nelem != 3) {
	    wwarning(_("bad number of arguments in gradient specification"));
	    return NULL;
	}

	if (val[0]=='d' || val[0]=='D')
	  type = WTEX_DGRADIENT;
	else if (val[0]=='h' || val[0]=='H')
	  type = WTEX_HGRADIENT;
	else
	  type = WTEX_VGRADIENT;

	
	/* get from color */
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &xcolor)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	color1.alpha = 255;
	color1.red = xcolor.red >> 8;
	color1.green = xcolor.green >> 8;
	color1.blue = xcolor.blue >> 8;
	
	/* get to color */
	elem = PLGetArrayElement(pl, 2);
	if (!elem || !PLIsString(elem)) {
	    return NULL;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &xcolor)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	color2.alpha = 255;
	color2.red = xcolor.red >> 8;
	color2.green = xcolor.green >> 8;
	color2.blue = xcolor.blue >> 8;

	texture = (WTexture*)wTextureMakeGradient(scr, type, &color1, &color2);
	
    } else if (strcasecmp(val, "mhgradient")==0
	       || strcasecmp(val, "mvgradient")==0
	       || strcasecmp(val, "mdgradient")==0) {
	XColor color;
	RColor **colors;
	int i, count;
	int type;

	if (nelem < 3) {
	    wwarning(_("too few arguments in multicolor gradient specification"));
	    return NULL;
	}

	if (val[1]=='h' || val[1]=='H')
	    type = WTEX_MHGRADIENT;
	else if (val[1]=='v' || val[1]=='V')
	    type = WTEX_MVGRADIENT;
	else
	    type = WTEX_MDGRADIENT;

	count = nelem-1;

	colors = wmalloc(sizeof(RColor*)*(count+1));

	for (i=0; i<count; i++) {
	    elem = PLGetArrayElement(pl, i+1);
	    if (!elem || !PLIsString(elem)) {
		for (--i; i>=0; --i) {
		    free(colors[i]);
		}
		free(colors);
		return NULL;
	    }
	    val = PLGetString(elem);

	    if (!XParseColor(dpy, scr->w_colormap, val, &color)) {
		wwarning(_("\"%s\" is not a valid color name"), val);
		for (--i; i>=0; --i) {
		    free(colors[i]);
		}
		free(colors);
		return NULL;
	    } else {
		colors[i] = wmalloc(sizeof(RColor));
		colors[i]->red = color.red >> 8;
		colors[i]->green = color.green >> 8;
		colors[i]->blue = color.blue >> 8;
	    }
	}
	colors[i] = NULL;
	
	texture = (WTexture*)wTextureMakeMGradient(scr, type, colors);
    } else if (strcasecmp(val, "spixmap")==0 ||
	       strcasecmp(val, "cpixmap")==0 ||
               strcasecmp(val, "tpixmap")==0) {
	XColor color;
        int type;

	if (nelem != 3) 
	    return NULL;

        if (val[0] == 's' || val[0] == 'S')
            type = WTP_SCALE;
        else if (val[0] == 'c' || val[0] == 'C')
	    type = WTP_CENTER;
	else
            type = WTP_TILE;

	/* get color */
	elem = PLGetArrayElement(pl, 2);
	if (!elem || !PLIsString(elem)) {
	    return NULL;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &color)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	/* file name */
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	texture = (WTexture*)wTextureMakePixmap(scr, type, val, &color);
    } else if (strcasecmp(val, "thgradient")==0
	       || strcasecmp(val, "tvgradient")==0
	       || strcasecmp(val, "tdgradient")==0) {
	RColor color1, color2;
	XColor xcolor;
	int opacity;
	int style;

	if (val[1]=='h' || val[1]=='H')
	    style = WTEX_THGRADIENT;
	else if (val[1]=='v' || val[1]=='V')
	    style = WTEX_TVGRADIENT;
	else
	    style = WTEX_TDGRADIENT;

	if (nelem != 5) {
	    wwarning(_("bad number of arguments in textured gradient specification"));
	    return NULL;
	}
	
	/* get from color */
	elem = PLGetArrayElement(pl, 3);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &xcolor)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	color1.alpha = 255;
	color1.red = xcolor.red >> 8;
	color1.green = xcolor.green >> 8;
	color1.blue = xcolor.blue >> 8;

	/* get to color */
	elem = PLGetArrayElement(pl, 4);
	if (!elem || !PLIsString(elem)) {
	    return NULL;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->w_colormap, val, &xcolor)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	color2.alpha = 255;
	color2.red = xcolor.red >> 8;
	color2.green = xcolor.green >> 8;
	color2.blue = xcolor.blue >> 8;

	/* get opacity */
	elem = PLGetArrayElement(pl, 2);
	if (!elem || !PLIsString(elem))
	    opacity = 128;
	else
	    val = PLGetString(elem);
	
	if (!val || (opacity = atoi(val)) < 0 || opacity > 255) {
	    wwarning(_("bad opacity value for tgradient texture \"%s\". Should be [0..255]"), val);
	    opacity = 128;
	}

	/* get file name */
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem))
	  return NULL;
	val = PLGetString(elem);

	texture = (WTexture*)wTextureMakeTGradient(scr, style, &color1, &color2,
						   val, opacity);
    } else {
	wwarning(_("invalid texture type %s"), val);
	return NULL;
    }
    return texture;
}



static int 
getTexture(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
	   void **ret)
{
    static WTexture *texture;
    int changed=0;

again:
    if (!PLIsArray(value)) { 
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
		 entry->key, "Texture");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }
    
    if (strcmp(entry->key, "WidgetColor")==0 && !changed) {
	proplist_t pl;
	
	pl = PLGetArrayElement(value, 0);
	if (!pl || !PLIsString(pl) || !PLGetString(pl)
	    || strcasecmp(PLGetString(pl), "solid")!=0) {
	    wwarning(_("Wrong option format for key \"%s\". Should be %s."),
		     entry->key, "Solid Texture");

	    value = entry->plvalue;
	    changed = 1;
	    wwarning(_("using default \"%s\" instead"), entry->default_value);
	    goto again;
	}
    }

    texture = parse_texture(scr, value);

    if (!texture) {
	wwarning(_("Error in texture specification for key \"%s\""),
		 entry->key);
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }
    
    if (ret) 
      *ret = &texture;

    if (addr)
      *(WTexture**)addr = texture;
    
    return True;
}



static int
getWSBackground(WScreen *scr, WDefaultEntry *entry, proplist_t value,
		void *addr, void **ret)
{
    proplist_t elem;
    int changed = 0;
    char *val;
    int nelem;

again:
    if (!PLIsArray(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 "WorkspaceBack", "Texture or None");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    /* only do basic error checking and verify for None texture */

    nelem = PLGetNumberOfElements(value);
    if (nelem > 0) {
	elem = PLGetArrayElement(value, 0);
	if (!elem || !PLIsString(elem)) {
	    wwarning(_("Wrong type for workspace background. Should be a texture type."));
	    if (changed==0) {
		value = entry->plvalue;
		changed = 1;
		wwarning(_("using default \"%s\" instead"), entry->default_value);
		goto again;
	    }
	    return False;
	}
	val = PLGetString(elem);

	if (strcasecmp(val, "None")==0)
	    return True;
    }
    *ret = PLRetain(value);

    return True;
}


static int
getWSSpecificBackground(WScreen *scr, WDefaultEntry *entry, proplist_t value,
			void *addr, void **ret)
{
    proplist_t elem;
    int nelem;
    int changed = 0;

again:
    if (!PLIsArray(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 "WorkspaceSpecificBack", "an array of textures");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    /* only do basic error checking and verify for None texture */

    nelem = PLGetNumberOfElements(value);
    if (nelem > 0) {
	while (nelem--) {
	    elem = PLGetArrayElement(value, nelem);
	    if (!elem || !PLIsArray(elem)) {
		wwarning(_("Wrong type for background of workspace %i. Should be a texture."),
			 nelem);
	    }
	}
    }

    *ret = PLRetain(value);

#ifdef notworking
    /* 
     * Kluge to force wmsetbg helper to set the default background.
     * If the WorkspaceSpecificBack is changed once wmaker has started,
     * the WorkspaceBack won't be sent to the helper, unless the user
     * changes it's value too. So, we must force this by removing the
     * value from the defaults DB.
     */
    if (!scr->flags.backimage_helper_launched && !scr->flags.startup) {
	proplist_t key = PLMakeString("WorkspaceBack");

	PLRemoveDictionaryEntry(WDWindowMaker->dictionary, key);

	PLRelease(key);
    }
#endif
    return True;
}


static int 
getFont(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	void **ret)
{
    static WFont *font;
    char *val;
    
    STRINGP("Font");
    
    val = PLGetString(value);
    
    font = wLoadFont(val);
    if (!font) {
	wfatal(_("could not load any usable font!!!"));
	exit(1);
    }

    if (ret)
      *ret = font;
    
    if (addr) {
	wwarning("BUG:can't assign font value outside update function");
    }
    
    return True;
}


static int 
getColor(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	 void **ret)
{
    static unsigned long pixel;    
    XColor color;
    char *val;
    int second_pass=0;

    STRINGP("Color");
    
    val = PLGetString(value);

again:
    if (!wGetColor(scr, val, &color)) {
        wwarning(_("could not get color for key \"%s\""),
                 entry->key);
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
        return False;
    }
    
    pixel = color.pixel;
    
    if (ret)
      *ret = &pixel;
    
    if (addr)
      *(unsigned long*)addr = pixel;
    
    return True;
}



static int
getKeybind(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	 void **ret)
{
    static WShortKey shortcut;
    KeySym ksym;
    char *val;
    char *k;
    char buf[128], *b;


    STRINGP("Key spec");

    val = PLGetString(value);

    if (!val || strcasecmp(val, "NONE")==0) {
	shortcut.keycode = 0;
	shortcut.modifier = 0;
	if (ret)
	    *ret = &shortcut;
	return True;
    }

    strcpy(buf, val);
    
    b = (char*)buf;

    /* get modifiers */
    shortcut.modifier = 0;
    while ((k = strchr(b, '+'))!=NULL) {
	int mod;
	
	*k = 0;
	mod = wXModifierFromKey(b);
	if (mod<0) {
	    wwarning(_("%s:invalid key modifier \"%s\""), entry->key, b);
	    return False;
	}
	shortcut.modifier |= mod;
	
	b = k+1;
    }
    
    /* get key */
    ksym = XStringToKeysym(b);
    
    if (ksym==NoSymbol) {
	wwarning(_("%s:invalid kbd shortcut specification \"%s\""), entry->key,
		 val);
	return False;
    }
    
    shortcut.keycode = XKeysymToKeycode(dpy, ksym);
    if (shortcut.keycode==0) {
	wwarning(_("%s:invalid key in shortcut \"%s\""), entry->key, val);
	return False;
    }

    if (ret)
      *ret = &shortcut;

    return True;
}


static int
getModMask(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	   void **ret)
{
    unsigned int mask;
    char *str;

    STRINGP("Modifier Key");

    str = PLGetString(value);
    if (!str)
	return False;
    
    mask = wXModifierFromKey(str);
    if (mask < 0) {
	wwarning(_("%s: modifier key %s is not valid"), entry->key, str);
	mask = 0;
	return False;
    }

    if (addr)
	*(unsigned int*)addr = mask;
    
    if (ret)
	*ret = &mask;
    
    return True;
}



/* ---------------- value setting functions --------------- */
static int
setJustify(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    return REFRESH_FORE_COLOR;
}


static int
setIfDockPresent(WScreen *scr, WDefaultEntry *entry, int *flag, long which)
{
    switch (which) {
    case WM_DOCK:
        wPreferences.flags.nodock = wPreferences.flags.nodock || *flag;
        break;
    case WM_CLIP:
        wPreferences.flags.noclip = wPreferences.flags.noclip || *flag;
        break;
    default:
        break;
    }
    return 0;
}


static int
setStickyIcons(WScreen *scr, WDefaultEntry *entry, void *bar, void *foo)
{
    if (scr->workspaces) {
        wWorkspaceForceChange(scr, scr->current_workspace);
        wArrangeIcons(scr, False);
    }
    return 0;
}

#if not_used
static int
setPositive(WScreen *scr, WDefaultEntry *entry, int *value, void *foo)
{
    if (*value <= 0)
        *(int*)foo = 1;

    return 0;
}
#endif



static int
setIconTile(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    Pixmap pixmap;
    RImage *img;
    int reset = 0;
   
    img = wTextureRenderImage(*texture, wPreferences.icon_size,
			      wPreferences.icon_size, 
			      ((*texture)->any.type & WREL_BORDER_MASK) 
			      ? WREL_ICON : WREL_FLAT);
    if (!img) {
	wwarning(_("could not render texture for icon background"));
	if (!entry->addr)
	  wTextureDestroy(scr, *texture);
	return 0;
    }
    RConvertImage(scr->rcontext, img, &pixmap);
    
    if (scr->icon_tile) {
	reset = 1;
	RDestroyImage(scr->icon_tile);
	XFreePixmap(dpy, scr->icon_tile_pixmap);
    }

    scr->icon_tile = img;

    if (!wPreferences.flags.noclip) {
	if (scr->clip_tile) {
	    RDestroyImage(scr->clip_tile);
	}
	scr->clip_tile = wClipMakeTile(scr, img);
    }

    scr->icon_tile_pixmap = pixmap;

    if (scr->def_icon_pixmap) {
        XFreePixmap(dpy, scr->def_icon_pixmap);
        scr->def_icon_pixmap = None;
    }
    if (scr->def_ticon_pixmap) {
        XFreePixmap(dpy, scr->def_ticon_pixmap);
        scr->def_ticon_pixmap = None;
    }

    if (scr->icon_back_texture) {
	wTextureDestroy(scr, (WTexture*)scr->icon_back_texture);
    }
    scr->icon_back_texture = wTextureMakeSolid(scr, &((*texture)->any.color));

    if (scr->clip_balloon)
	XSetWindowBackground(dpy, scr->clip_balloon, 
			     (*texture)->any.color.pixel);
    
    /*
     * Free the texture as nobody else will use it, nor refer to it.
     */
    if (!entry->addr)
	wTextureDestroy(scr, *texture);

    return (reset ? REFRESH_ICON_TILE : 0);
}



static int 
setWinTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->title_font) {
	wFreeFont(scr->title_font);
    }
    
    scr->title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->window_title_gc, font->font->fid);
#endif
    
    return REFRESH_WINDOW_FONT;
}


static int 
setMenuTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->menu_title_font) {
	wFreeFont(scr->menu_title_font);
    }
    
    scr->menu_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->menu_title_gc, font->font->fid);
#endif

    return REFRESH_MENU_TITLE_FONT;
}


static int 
setMenuTextFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->menu_entry_font) {
	wFreeFont(scr->menu_entry_font);
    }
    
    scr->menu_entry_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->menu_entry_gc, font->font->fid);
    XSetFont(dpy, scr->disabled_menu_entry_gc, font->font->fid);
    XSetFont(dpy, scr->select_menu_gc, font->font->fid);
#endif
    
    return REFRESH_MENU_FONT;
}



static int 
setIconTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->icon_title_font) {
	wFreeFont(scr->icon_title_font);
    }
    
    scr->icon_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->icon_title_gc, font->font->fid);
#endif
		 
    return REFRESH_ICON_FONT;
}


static int 
setClipTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->clip_title_font) {
	wFreeFont(scr->clip_title_font);
    }
    
    scr->clip_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->clip_title_gc, font->font->fid);
#endif
		 
    return REFRESH_ICON_FONT;
}


static int
setDisplayFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->info_text_font) {
	wFreeFont(scr->info_text_font);
    }
    
    scr->info_text_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->info_text_gc, font->font->fid);
    XSetFont(dpy, scr->line_gc, font->font->fid);
#endif

    /* This test works because the scr structure is initially zeroed out
       and None = 0. Any other time, the window should be valid. */
    if (scr->geometry_display != None) {
        wGetGeometryWindowSize(scr, &scr->geometry_display_width,
			       &scr->geometry_display_height);
	XResizeWindow(dpy, scr->geometry_display,
	    scr->geometry_display_width, scr->geometry_display_height);
    }

    return 0;
}


static int
setHightlight(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
   if (scr->select_pixel!=scr->white_pixel &&
	scr->select_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->select_pixel);
    }
    
    scr->select_pixel = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setHightlightText(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
   if (scr->select_text_pixel!=scr->white_pixel &&
	scr->select_text_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->select_text_pixel);
    }
    
    scr->select_text_pixel = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setClipTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
    if (scr->clip_title_pixel[index]!=scr->white_pixel &&
	scr->clip_title_pixel[index]!=scr->black_pixel) {
	wFreeColor(scr, scr->clip_title_pixel[index]);
    }
    
    scr->clip_title_pixel[index] = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setWTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
    if (scr->window_title_pixel[index]!=scr->white_pixel &&
	scr->window_title_pixel[index]!=scr->black_pixel) {
	wFreeColor(scr, scr->window_title_pixel[index]);
    }
    
    scr->window_title_pixel[index] = color->pixel;

    if (index == WS_UNFOCUSED)
	XSetForeground(dpy, scr->info_text_gc, color->pixel);
    
    return REFRESH_FORE_COLOR;
}


static int 
setMenuTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
#ifdef TITLE_TEXT_SHADOW
    if (index == WS_SMENU){
        if (scr->menu_title_pixel[WS_SMENU]!=scr->white_pixel &&
             scr->menu_title_pixel[WS_SMENU]!=scr->black_pixel) {
             wFreeColor(scr, scr->menu_title_pixel[WS_SMENU]);
        }
        scr->menu_title_pixel[WS_SMENU] = color->pixel;
    }
    else {
        if (scr->menu_title_pixel[0]!=scr->white_pixel &&
             scr->menu_title_pixel[0]!=scr->black_pixel) {
             wFreeColor(scr, scr->menu_title_pixel[0]);
        }
        scr->menu_title_pixel[0] = color->pixel;
    }
#else /* !TITLE_TEXT_SHADOW */
    if (scr->menu_title_pixel[0]!=scr->white_pixel &&
	scr->menu_title_pixel[0]!=scr->black_pixel) {
	wFreeColor(scr, scr->menu_title_pixel[0]);
    }
    
    scr->menu_title_pixel[0] = color->pixel;
#endif /* !TITLE_TEXT_SHADOW */
    XSetForeground(dpy, scr->menu_title_gc, color->pixel);
    
    return REFRESH_FORE_COLOR;
}


static int 
setMenuTextColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    XGCValues gcv;
#define gcm (GCForeground|GCBackground|GCFillStyle)

    if (scr->mtext_pixel!=scr->white_pixel &&
	scr->mtext_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->mtext_pixel);
    }
    
    scr->mtext_pixel = color->pixel;
    
    XSetForeground(dpy, scr->menu_entry_gc, color->pixel);


    if (scr->dtext_pixel == scr->mtext_pixel) {
	gcv.foreground = scr->white_pixel;
	gcv.background = scr->black_pixel;
	gcv.fill_style = FillStippled;
    } else {
	gcv.foreground = scr->dtext_pixel;
	gcv.fill_style = FillSolid;
    }
    XChangeGC(dpy, scr->disabled_menu_entry_gc, gcm, &gcv);
    
    return REFRESH_FORE_COLOR;
#undef gcm
}


static int 
setMenuDisabledColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    XGCValues gcv;
#define gcm (GCForeground|GCBackground|GCFillStyle)

    if (scr->dtext_pixel!=scr->white_pixel &&
	scr->dtext_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->dtext_pixel);
    }
    
    scr->dtext_pixel = color->pixel;

    if (scr->dtext_pixel == scr->mtext_pixel) {
	gcv.foreground = scr->white_pixel;
	gcv.background = scr->black_pixel;
	gcv.fill_style = FillStippled;
    } else {
	gcv.foreground = scr->dtext_pixel;
	gcv.fill_style = FillSolid;
    }
    XChangeGC(dpy, scr->disabled_menu_entry_gc, gcm, &gcv);

    return REFRESH_FORE_COLOR;
#undef gcm
}

static int 
setIconTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    XSetForeground(dpy, scr->icon_title_gc, color->pixel); 
    
    return REFRESH_FORE_COLOR;
}

static int 
setIconTitleBack(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    if (scr->icon_title_texture) {
	wTextureDestroy(scr, (WTexture*)scr->icon_title_texture);
    }
    XQueryColor (dpy, scr->w_colormap, color);
    scr->icon_title_texture = wTextureMakeSolid(scr, color);
    
    return REFRESH_WINDOW_TEXTURES;
}


static void
trackDeadProcess(pid_t pid, unsigned char status, WScreen *scr)
{
    close(scr->helper_fd);
    scr->helper_fd = 0;
    scr->helper_pid = 0;
    scr->flags.backimage_helper_launched = 0;
}    


static int
setWorkspaceSpecificBack(WScreen *scr, WDefaultEntry *entry, proplist_t value, 
			 void *bar)
{
    int i;
    proplist_t val;
    char *str;

    if (scr->flags.backimage_helper_launched) {
	if (PLGetNumberOfElements(value)==0) {
	    SendHelperMessage(scr, 'C', 0, NULL);
	    SendHelperMessage(scr, 'K', 0, NULL);

	    PLRelease(value);
	    return 0;
	}
    } else {
	pid_t pid;
	int filedes[2];

	if (PLGetNumberOfElements(value) == 0)
	    return 0;
	
	if (pipe(filedes) < 0) {
	    wsyserror("pipe() failed:can't set workspace specific background image");
	    
	    PLRelease(value);
	    return 0;
	}

	pid = fork();
	if (pid < 0) {
	    wsyserror("fork() failed:can't set workspace specific background image");
	    if (close(filedes[0]) < 0)
		wsyserror("could not close pipe");
	    if (close(filedes[1]) < 0)
		wsyserror("could not close pipe");

	} else if (pid == 0) {
	    SetupEnvironment(scr);

	    if (close(0) < 0)
		wsyserror("could not close pipe");
	    if (dup(filedes[0]) < 0) {
		wsyserror("dup() failed:can't set workspace specific background image");
	    }
	    execlp("wmsetbg", "wmsetbg", "-helper", "-d", NULL);
	    wsyserror("could not execute wmsetbg");
	    exit(1);
	} else {

	    if (fcntl(filedes[0], F_SETFD, FD_CLOEXEC) < 0) {
		wsyserror("error setting close-on-exec flag");
	    }
	    if (fcntl(filedes[1], F_SETFD, FD_CLOEXEC) < 0) {
		wsyserror("error setting close-on-exec flag");
	    }

	    scr->helper_fd = filedes[1];
	    scr->helper_pid = pid;
	    scr->flags.backimage_helper_launched = 1;

	    wAddDeathHandler(pid, (WDeathHandler*)trackDeadProcess, scr);

	    SendHelperMessage(scr, 'P', -1, wPreferences.pixmap_path);
	}
    }

    for (i = 0; i < PLGetNumberOfElements(value); i++) {
	val = PLGetArrayElement(value, i);
	if (val && PLIsArray(val) && PLGetNumberOfElements(val)>0) {
	    str = PLGetDescription(val);

	    SendHelperMessage(scr, 'S', i+1, str);

	    free(str);
	} else {
	    SendHelperMessage(scr, 'U', i+1, NULL);
	}
    }
    sleep(1);

    PLRelease(value);
    return 0;
}


static int
setWorkspaceBack(WScreen *scr, WDefaultEntry *entry, proplist_t value, 
		 void *bar)
{
    if (scr->flags.backimage_helper_launched) {
	char *str;

	if (PLGetNumberOfElements(value)==0) {
	    SendHelperMessage(scr, 'U', 0, NULL);
	} else {
	    /* set the default workspace background to this one */
	    str = PLGetDescription(value);
	    if (str) {
		SendHelperMessage(scr, 'S', 0, str);
		free(str);
		SendHelperMessage(scr, 'C', scr->current_workspace+1, NULL);
	    } else {
		SendHelperMessage(scr, 'U', 0, NULL);
	    }
	}
    } else {
	char *command;
	char *text;

	SetupEnvironment(scr);
	text = PLGetDescription(value);
	command = wmalloc(strlen(text)+40);
	sprintf(command, "wmsetbg -d -p '%s' &", text);
	free(text);
	system(command);
	free(command);
    }
    PLRelease(value);

    return 0;
}


static int 
setWidgetColor(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->widget_texture) {
	wTextureDestroy(scr, (WTexture*)scr->widget_texture);
    }
    scr->widget_texture = *(WTexSolid**)texture;

    return 0;
}


static int 
setFTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{		       
    if (scr->window_title_texture[WS_FOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_FOCUSED]);
    }
    scr->window_title_texture[WS_FOCUSED] = *texture;

    return REFRESH_WINDOW_TEXTURES;
}


static int 
setPTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->window_title_texture[WS_PFOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_PFOCUSED]);
    }
    scr->window_title_texture[WS_PFOCUSED] = *texture;
    
    return REFRESH_WINDOW_TEXTURES;
}


static int 
setUTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->window_title_texture[WS_UNFOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_UNFOCUSED]);
    }
    scr->window_title_texture[WS_UNFOCUSED] = *texture;
    
    if (scr->resizebar_texture[0]) {
	wTextureDestroy(scr, (WTexture*)scr->resizebar_texture[0]);
    }
    scr->resizebar_texture[0]
      = wTextureMakeSolid(scr, &scr->window_title_texture[WS_UNFOCUSED]->any.color);
    
    if (scr->geometry_display != None)
    	XSetWindowBackground(dpy, scr->geometry_display, 
			 scr->resizebar_texture[0]->normal.pixel);
			 
    return REFRESH_WINDOW_TEXTURES;
}


static int 
setMenuTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->menu_title_texture[0]) {
	wTextureDestroy(scr, scr->menu_title_texture[0]);
    }
    scr->menu_title_texture[0] = *texture;

    return REFRESH_MENU_TEXTURES;
}


static int 
setMenuTextBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->menu_item_texture) {
	wTextureDestroy(scr, scr->menu_item_texture);
	wTextureDestroy(scr, (WTexture*)scr->menu_item_auxtexture);
    }
    scr->menu_item_texture = *texture;

    scr->menu_item_auxtexture 
      = wTextureMakeSolid(scr, &scr->menu_item_texture->any.color);
    
    return REFRESH_MENU_TEXTURES;
}


static int
setKeyGrab(WScreen *scr, WDefaultEntry *entry, WShortKey *shortcut, long index)
{
    WWindow *wwin;
    wKeyBindings[index] = *shortcut;
    
    wwin = scr->focused_window;
    
    while (wwin!=NULL) {
	XUngrabKey(dpy, AnyKey, AnyModifier, wwin->frame->core->window);

	if (!WFLAGP(wwin, no_bind_keys)) {
	    wWindowSetKeyGrabs(wwin);
	}
	wwin = wwin->prev;
    }
    
    return 0;
}


static int
setIconPosition(WScreen *scr, WDefaultEntry *entry, void *bar, void *foo)
{
    wArrangeIcons(scr, True);

    return 0;
}


static int
updateUsableArea(WScreen *scr, WDefaultEntry *entry, void *bar, void *foo)
{
    wScreenUpdateUsableArea(scr);

    return 0;
}



/*
 * Very ugly kluge.
 * Need access to the double click variables, so that all widgets in
 * wmaker panels will have the same dbl-click values. 
 * TODO: figure a better way of dealing with it.
 */
#include "WINGsP.h"

static int
setDoubleClick(WScreen *scr, WDefaultEntry *entry, int *value, void *foo)
{
    extern _WINGsConfiguration WINGsConfiguration;

    if (*value <= 0)
        *(int*)foo = 1;

    WINGsConfiguration.doubleClickDelay = *value;
    
    return 0;
}



