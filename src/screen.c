/* screen.c - screen management
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
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <wraster.h>

#include "WindowMaker.h"
#include "def_pixmaps.h"
#include "screen.h"
#include "texture.h"
#include "pixmap.h"
#include "menu.h"
#include "funcs.h"
#include "actions.h"
#include "properties.h"
#include "dock.h"
#include "resources.h"
#include "workspace.h"
#include "session.h"
#include "balloon.h"

#include <proplist.h>

#include "defaults.h"


#define EVENT_MASK (LeaveWindowMask|EnterWindowMask|PropertyChangeMask\
		|SubstructureNotifyMask|PointerMotionMask \
		|SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask\
		|KeyPressMask|KeyReleaseMask)


/**** Global variables ****/

extern Cursor wCursor[WCUR_LAST];
extern WPreferences wPreferences;
extern Atom _XA_WINDOWMAKER_STATE;


extern int wScreenCount;

extern WDDomain *WDWindowMaker;


/**** Local ****/

#define STIPPLE_WIDTH 2
#define STIPPLE_HEIGHT 2
static char STIPPLE_DATA[] = {0x02, 0x01};

static int CantManageScreen = 0;

static proplist_t dApplications = NULL;
static proplist_t dWorkspace;
static proplist_t dDock;
static proplist_t dClip;


static void
make_keys()
{
    if (dApplications!=NULL)
        return;

    dApplications = PLMakeString("Applications");
    dWorkspace = PLMakeString("Workspace");
    dDock = PLMakeString("Dock");
    dClip = PLMakeString("Clip");
}


/*
 *----------------------------------------------------------------------
 * alreadyRunningError--
 * 	X error handler used to catch errors when trying to do
 * XSelectInput() on the root window. These errors probably mean that
 * there already is some other window manager running.
 * 
 * Returns:
 * 	Nothing, unless something really evil happens...
 * 
 * Side effects:
 * 	CantManageScreen is set to 1;
 *---------------------------------------------------------------------- 
 */
static int 
alreadyRunningError(Display *dpy, XErrorEvent *error)
{
    CantManageScreen = 1;
    return -1;
}


/*
 *---------------------------------------------------------------------- 
 * allocButtonPixmaps--
 * 	Allocate pixmaps used on window operation buttons (those in the
 * titlebar). The pixmaps are linked to the program. If XPM is supported
 * XPM pixmaps are used otherwise, equivalent bitmaps are used.
 * 
 * Returns:
 * 	Nothing
 * 
 * Side effects:
 * 	Allocates shared pixmaps for the screen. These pixmaps should 
 * not be freed by anybody.
 *---------------------------------------------------------------------- 
 */
static void
allocButtonPixmaps(WScreen *scr)
{
    WPixmap *pix;

    /* create predefined pixmaps */
    pix = wPixmapCreateFromXPMData(scr, PRED_CLOSE_XPM);
    if (pix)
      pix->shared = 1;
    scr->b_pixmaps[WBUT_CLOSE] = pix;

    pix = wPixmapCreateFromXPMData(scr, PRED_BROKEN_CLOSE_XPM);
    if (pix)
      pix->shared = 1;
    scr->b_pixmaps[WBUT_BROKENCLOSE] = pix;

    pix = wPixmapCreateFromXPMData(scr, PRED_ICONIFY_XPM);
    if (pix)
      pix->shared = 1;
    scr->b_pixmaps[WBUT_ICONIFY] = pix;
    
    pix = wPixmapCreateFromXPMData(scr, PRED_KILL_XPM);
    if (pix)
      pix->shared = 1;
    scr->b_pixmaps[WBUT_KILL] = pix;
}



static void
draw_dot(WScreen *scr, Drawable d, int x, int y, GC gc)
{
    XSetForeground(dpy, gc, scr->black_pixel);
    XDrawLine(dpy, d, gc, x, y, x+1, y);
    XDrawPoint(dpy, d, gc, x, y+1);
    XSetForeground(dpy, gc, scr->white_pixel);
    XDrawLine(dpy, d, gc, x+2, y, x+2, y+1);
    XDrawPoint(dpy, d, gc, x+1, y+1);
}


static WPixmap*
make3Dots(WScreen *scr)
{
    WPixmap *wpix;
    GC gc2, gc;
    XGCValues gcv;
    Pixmap pix, mask;
    
    gc = scr->copy_gc;
    pix = XCreatePixmap(dpy, scr->w_win, wPreferences.icon_size, 
			wPreferences.icon_size,	scr->w_depth);
    XSetForeground(dpy, gc, scr->black_pixel);
    XFillRectangle(dpy, pix, gc, 0, 0, wPreferences.icon_size, 
		   wPreferences.icon_size);
    XSetForeground(dpy, gc, scr->white_pixel);
    draw_dot(scr, pix, 4, wPreferences.icon_size-6, gc);
    draw_dot(scr, pix, 9, wPreferences.icon_size-6, gc);
    draw_dot(scr, pix, 14, wPreferences.icon_size-6, gc);

    mask = XCreatePixmap(dpy, scr->w_win, wPreferences.icon_size, 
			 wPreferences.icon_size, 1);
    gcv.foreground = 0;
    gcv.graphics_exposures = False;
    gc2 = XCreateGC(dpy, mask, GCForeground|GCGraphicsExposures, &gcv);
    XFillRectangle(dpy, mask, gc2, 0, 0, wPreferences.icon_size, 
		   wPreferences.icon_size);
    XSetForeground(dpy, gc2, 1);
    XFillRectangle(dpy, mask, gc2, 4, wPreferences.icon_size-6, 3, 2);
    XFillRectangle(dpy, mask, gc2, 9, wPreferences.icon_size-6, 3, 2);
    XFillRectangle(dpy, mask, gc2, 14, wPreferences.icon_size-6, 3, 2);
    
    XFreeGC(dpy, gc2);
    
    wpix = wPixmapCreate(scr, pix, mask);
    wpix->shared = 1;

    return wpix;
}


static void
allocGCs(WScreen *scr)
{
    XGCValues gcv;
    XColor color;
    unsigned long mtextcolor;
    int gcm;

    scr->stipple_bitmap = 
      XCreateBitmapFromData(dpy, scr->w_win, STIPPLE_DATA, STIPPLE_WIDTH,
			    STIPPLE_HEIGHT);

    gcv.stipple = scr->stipple_bitmap;
    gcv.foreground = scr->white_pixel;
    gcv.fill_style = FillStippled;
    gcv.graphics_exposures = False;
    gcm = GCForeground|GCStipple|GCFillStyle|GCGraphicsExposures;
    scr->stipple_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

   
    /* selected icon border GCs */
    gcv.function    = GXcopy;
    gcv.foreground  = scr->white_pixel;
    gcv.background  = scr->black_pixel;
    gcv.line_width  = 1;
    gcv.line_style  = LineDoubleDash;
    gcv.fill_style  = FillSolid;
    gcv.dash_offset = 0;
    gcv.dashes      = 4;
    gcv.graphics_exposures = False;
   
    gcm = GCFunction | GCGraphicsExposures;
    gcm |= GCForeground | GCBackground;
    gcm |= GCLineWidth | GCLineStyle;
    gcm |= GCFillStyle;
    gcm |= GCDashOffset | GCDashList;

    scr->icon_select_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    gcm = GCForeground|GCGraphicsExposures;
    
    scr->menu_title_pixel[0] = scr->white_pixel;
    gcv.foreground = scr->white_pixel;
    gcv.graphics_exposures = False;
    scr->menu_title_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    scr->mtext_pixel = scr->black_pixel;
    mtextcolor = gcv.foreground = scr->black_pixel;
    scr->menu_entry_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    /* selected menu entry GC */
    gcm = GCForeground|GCBackground|GCGraphicsExposures;
    gcv.foreground = scr->white_pixel;
    gcv.background = scr->white_pixel;
    gcv.graphics_exposures = False;
    scr->select_menu_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    /* disabled menu entry GC */
    scr->dtext_pixel = scr->black_pixel;
    gcm = GCForeground|GCBackground|GCStipple|GCGraphicsExposures;
    gcv.stipple = scr->stipple_bitmap;
    gcv.graphics_exposures = False;
    scr->disabled_menu_entry_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    /* frame GC */
    wGetColor(scr, DEF_FRAME_COLOR, &color);
    gcv.function = GXxor;
    /* this will raise the probability of the XORed color being different
     * of the original color in PseudoColor when not all color cells are
     * initialized */
    if (DefaultVisual(dpy, scr->screen)->class==PseudoColor)
    	gcv.plane_mask = (1<<(scr->depth-1))|1;
    else
	gcv.plane_mask = AllPlanes;
    gcv.foreground = color.pixel;
    if (gcv.foreground == 0)
	gcv.foreground = 1;
    gcv.line_width = DEF_FRAME_THICKNESS;
    gcv.subwindow_mode = IncludeInferiors;
    gcv.graphics_exposures = False;
    scr->frame_gc = XCreateGC(dpy, scr->root_win, GCForeground|GCGraphicsExposures
			      |GCFunction|GCSubwindowMode|GCLineWidth
			      |GCPlaneMask, &gcv);
    
    /* line GC */
    gcv.foreground = color.pixel;

    if (gcv.foreground == 0)
        /* XOR:ing with a zero is not going to be of much use, so
           in that case, we somewhat arbitrarily xor with 17 instead. */
        gcv.foreground = 17;

    gcv.function = GXxor;
    gcv.subwindow_mode = IncludeInferiors;
    gcv.line_width = 1;
    gcv.cap_style = CapRound;
    gcv.graphics_exposures = False;
    gcm = GCForeground|GCFunction|GCSubwindowMode|GCLineWidth|GCCapStyle
	|GCGraphicsExposures;
    scr->line_gc = XCreateGC(dpy, scr->root_win, gcm, &gcv);

    scr->line_pixel = gcv.foreground;
    
    /* copy GC */
    gcv.foreground = scr->white_pixel;
    gcv.background = scr->black_pixel;
    gcv.graphics_exposures = False;
    scr->copy_gc = XCreateGC(dpy, scr->w_win, GCForeground|GCBackground
			     |GCGraphicsExposures, &gcv);

    /* window title text GC */
    gcv.graphics_exposures = False;
    scr->window_title_gc = XCreateGC(dpy, scr->w_win,GCGraphicsExposures,&gcv);

    /* icon title GC */
    scr->icon_title_gc = XCreateGC(dpy, scr->w_win, GCGraphicsExposures, &gcv);

    /* clip title GC */
    scr->clip_title_gc = XCreateGC(dpy, scr->w_win, GCGraphicsExposures, &gcv);

    /* move/size display GC */
    gcv.graphics_exposures = False;
    gcm = GCGraphicsExposures;
    scr->info_text_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);
    
    /* misc drawing GC */
    gcv.graphics_exposures = False;
    gcm = GCGraphicsExposures;
    scr->draw_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);    
}



static void
createPixmaps(WScreen *scr)
{
    WPixmap *pix;
    WMPixmap *wmpix;
    RImage *image;
    Pixmap p, m;

    /* load pixmaps */
    pix = wPixmapCreateFromXBMData(scr, (char*)MENU_RADIO_INDICATOR_XBM_DATA,
				   (char*)MENU_RADIO_INDICATOR_XBM_DATA,
				   MENU_RADIO_INDICATOR_XBM_SIZE,
				   MENU_RADIO_INDICATOR_XBM_SIZE,
				   scr->black_pixel, scr->white_pixel);
    if (pix!=NULL)
      pix->shared = 1;
    scr->menu_radio_indicator = pix;


    pix = wPixmapCreateFromXBMData(scr, (char*)MENU_CHECK_INDICATOR_XBM_DATA,
				   (char*)MENU_CHECK_INDICATOR_XBM_DATA,
				   MENU_CHECK_INDICATOR_XBM_SIZE,
				   MENU_CHECK_INDICATOR_XBM_SIZE,
				   scr->black_pixel, scr->white_pixel);
    if (pix!=NULL)
      pix->shared = 1;
    scr->menu_check_indicator = pix;

    pix = wPixmapCreateFromXBMData(scr, (char*)MENU_MINI_INDICATOR_XBM_DATA,
				   (char*)MENU_MINI_INDICATOR_XBM_DATA,
				   MENU_MINI_INDICATOR_XBM_SIZE,
				   MENU_MINI_INDICATOR_XBM_SIZE,
				   scr->black_pixel, scr->white_pixel);
    if (pix!=NULL)
      pix->shared = 1;
    scr->menu_mini_indicator = pix;

    pix = wPixmapCreateFromXBMData(scr, (char*)MENU_HIDE_INDICATOR_XBM_DATA,
				   (char*)MENU_HIDE_INDICATOR_XBM_DATA,
				   MENU_HIDE_INDICATOR_XBM_SIZE,
				   MENU_HIDE_INDICATOR_XBM_SIZE,
				   scr->black_pixel, scr->white_pixel);
    if (pix!=NULL)
      pix->shared = 1;
    scr->menu_hide_indicator = pix;

    pix = wPixmapCreateFromXBMData(scr, (char*)MENU_SHADE_INDICATOR_XBM_DATA,
                                  (char*)MENU_SHADE_INDICATOR_XBM_DATA,
                                  MENU_SHADE_INDICATOR_XBM_SIZE,
                                  MENU_SHADE_INDICATOR_XBM_SIZE,
                                  scr->black_pixel, scr->white_pixel);
    if (pix!=NULL)
      pix->shared = 1;
    scr->menu_shade_indicator = pix;


    image = wDefaultGetImage(scr, "Logo", "WMPanel");

    if (!image) {
	wwarning(_("could not load logo image for panels"));
    } else {
	if (!RConvertImageMask(scr->rcontext, image, &p, &m, 128)) {
	    wwarning(_("error making logo image for panel:%s"), RMessageForError(RErrorCode));
	} else {
	    wmpix = WMCreatePixmapFromXPixmaps(scr->wmscreen, p, m,
					       image->width, image->height, 
					       scr->depth);
	    WMSetApplicationIconImage(scr->wmscreen, wmpix); 
	    WMReleasePixmap(wmpix);
	}
	RDestroyImage(image);
    }

    if (!wPreferences.flags.nodock || !wPreferences.flags.noclip) {
	scr->dock_dots = make3Dots(scr);
    }

    /* titlebar button pixmaps */
    allocButtonPixmaps(scr);
}


/*
 *----------------------------------------------------------------------
 * createInternalWindows--
 * 	Creates some windows used internally by the program. One to
 * receive input focus when no other window can get it and another
 * to display window geometry information during window resize/move.
 * 
 * Returns:
 * 	Nothing
 * 
 * Side effects:
 * 	Windows are created and some colors are allocated for the
 * window background.
 *---------------------------------------------------------------------- 
 */
static void
createInternalWindows(WScreen *scr)
{
    int vmask;
    XSetWindowAttributes attribs;

    /* window for displaying geometry information during resizes and moves */
    vmask = CWBorderPixel|CWBackPixmap|CWBackPixel|CWCursor|CWSaveUnder|CWOverrideRedirect;
    attribs.border_pixel = 0;
    attribs.save_under = True;
    attribs.override_redirect = True;    
    attribs.cursor = wCursor[WCUR_DEFAULT];
    attribs.background_pixmap = None;
    if (scr->resizebar_texture[0])
	attribs.background_pixel = scr->resizebar_texture[0]->normal.pixel;
    else
	attribs.background_pixel = scr->light_pixel;
    vmask |= CWColormap;
    attribs.colormap = scr->w_colormap;

    wGetGeometryWindowSize(scr, &scr->geometry_display_width,
			   &scr->geometry_display_height);
    scr->geometry_display =
      XCreateWindow(dpy, scr->root_win, 1, 1,
		    scr->geometry_display_width,
		    scr->geometry_display_height,
		    1, scr->w_depth, CopyFromParent, scr->w_visual,
		    vmask, &attribs);
    
    /* InputOnly window to get the focus when no other window can get it */
    vmask = CWEventMask|CWOverrideRedirect;
    attribs.event_mask = KeyPressMask|FocusChangeMask;
    attribs.override_redirect = True;
    scr->no_focus_win=XCreateWindow(dpy,scr->root_win,-10, -10, 4, 4, 0, 0,
				    InputOnly,CopyFromParent, vmask, &attribs);
    XSelectInput(dpy, scr->no_focus_win, KeyPressMask|KeyReleaseMask);
    XMapWindow(dpy, scr->no_focus_win);

    XSetInputFocus (dpy, scr->no_focus_win, RevertToParent, CurrentTime);
 
    /* shadow window for dock buttons */
    vmask = CWBorderPixel|CWBackPixmap|CWBackPixel|CWCursor|CWSaveUnder|CWOverrideRedirect;
    attribs.border_pixel = 0;
    attribs.save_under = True;
    attribs.override_redirect = True;
    attribs.background_pixmap = None;
    attribs.background_pixel = scr->white_pixel;
    vmask |= CWColormap;
    attribs.colormap = scr->w_colormap;
    scr->dock_shadow =
      XCreateWindow(dpy, scr->root_win, 0, 0, wPreferences.icon_size, 
		    wPreferences.icon_size, 0, scr->w_depth, CopyFromParent, 
		    scr->w_visual, vmask, &attribs);
   
    /* workspace name balloon for clip */
    vmask = CWBackPixel|CWSaveUnder|CWOverrideRedirect|CWColormap;
    attribs.save_under = True;
    attribs.override_redirect = True;
    attribs.colormap = scr->w_colormap;
    attribs.background_pixel = scr->icon_back_texture->normal.pixel;
    scr->clip_balloon =
      XCreateWindow(dpy, scr->root_win, 0, 0, 10, 10, 0, scr->w_depth, 
		    CopyFromParent, scr->w_visual, vmask, &attribs);
    /*
     * If the window is clicked without having ButtonPress selected, the
     * resulting event will have event.xbutton.window == root.
     */
    XSelectInput(dpy, scr->clip_balloon, ButtonPressMask);
}


/*
 *----------------------------------------------------------------------
 * wScreenInit--
 * 	Initializes the window manager for the given screen and 
 * allocates a WScreen descriptor for it. Many resources are allocated
 * for the screen and the root window is setup appropriately.
 *
 * Returns:
 * 	The WScreen descriptor for the screen.
 * 
 * Side effects:
 * 	Many resources are allocated and the IconSize property is
 * set on the root window.
 *	The program can be aborted if some fatal error occurs.
 * 
 * TODO: User specifiable visual.
 *---------------------------------------------------------------------- 
 */
WScreen*
wScreenInit(int screen_number)
{
    WScreen *scr;
    XIconSize icon_size[1];
    RContextAttributes rattr;
    extern int wVisualID;
    long event_mask;
    WMColor *color;
    XErrorHandler oldHandler;

    scr = wmalloc(sizeof(WScreen));
    memset(scr, 0, sizeof(WScreen));

    /* initialize globals */
    scr->screen = screen_number;
    scr->root_win = RootWindow(dpy, screen_number);
    scr->depth = DefaultDepth(dpy, screen_number);
    scr->colormap = DefaultColormap(dpy, screen_number);

    scr->scr_width = WidthOfScreen(ScreenOfDisplay(dpy, screen_number));
    scr->scr_height = HeightOfScreen(ScreenOfDisplay(dpy, screen_number));

    CantManageScreen = 0;
    oldHandler = XSetErrorHandler((XErrorHandler)alreadyRunningError);

    event_mask = EVENT_MASK;

    if (wPreferences.disable_root_mouse)
	event_mask &= ~(ButtonPressMask|ButtonReleaseMask);
    XSelectInput(dpy, scr->root_win, event_mask);

    XSync(dpy, False);
    XSetErrorHandler(oldHandler);
    
    if (CantManageScreen) {
	free(scr);
	return NULL;
    }

    XDefineCursor(dpy, scr->root_win, wCursor[WCUR_DEFAULT]);

    /* screen descriptor for raster graphic library */
    rattr.flags = RC_RenderMode | RC_ColorsPerChannel;
    rattr.render_mode = wPreferences.no_dithering?RM_MATCH:RM_DITHER;
    rattr.colors_per_channel = wPreferences.cmap_size;
    if (rattr.colors_per_channel<2)
	rattr.colors_per_channel = 2;

    if (wVisualID>=0) {
	rattr.flags |= RC_VisualID;
	rattr.visualid = wVisualID;
    }
    scr->rcontext = RCreateContext(dpy, screen_number, &rattr);
    if (!scr->rcontext) {
	wwarning(_("could not initialize graphics library context: %s"),
		 RMessageForError(RErrorCode));
	wAbort(False);
    } else {
	char **formats;
	int i = 0;

	formats = RSupportedFileFormats();
	if (formats) {
	    for (i=0; formats[i]!=NULL; i++) {
		if (strcmp(formats[i], "TIFF")==0) {
		    scr->flags.supports_tiff = 1;
		    break;
		}
	    }
	    RFreeStringList(formats);
	}
    }
    
    scr->w_win = scr->rcontext->drawable;
    scr->w_visual = scr->rcontext->visual;
    scr->w_depth = scr->rcontext->depth;
    scr->w_colormap = scr->rcontext->cmap;

    scr->black_pixel = scr->rcontext->black;
    scr->white_pixel = scr->rcontext->white;    

    /* create screen descriptor for WINGs */
    scr->wmscreen = WMCreateScreenWithRContext(dpy, screen_number, 
					       scr->rcontext);

    color = WMGrayColor(scr->wmscreen);
    scr->light_pixel = WMColorPixel(color);
    WMReleaseColor(color);

    color = WMDarkGrayColor(scr->wmscreen);
    scr->dark_pixel = WMColorPixel(color);
    WMReleaseColor(color);

    {
	XColor xcol;
	/* frame boder color */
	wGetColor(scr, FRAME_BORDER_COLOR, &xcol);
	scr->frame_border_pixel = xcol.pixel;
    }

    /* create GCs with default values */
    allocGCs(scr);

    /* read defaults for this screen */
    wReadDefaults(scr, WDWindowMaker->dictionary);

    /* create initial workspace */
    wWorkspaceNew(scr);

    createInternalWindows(scr);

    /* create shared pixmaps */
    createPixmaps(scr);

    /* set icon sizes we can accept from clients */
    icon_size[0].min_width = 8;
    icon_size[0].min_height = 8;
    icon_size[0].max_width = wPreferences.icon_size-4;
    icon_size[0].max_height = wPreferences.icon_size-4;
    icon_size[0].width_inc = 1;
    icon_size[0].height_inc = 1;
    XSetIconSizes(dpy, scr->root_win, icon_size, 1);

    /* setup WindowMaker protocols property in the root window*/
    PropSetWMakerProtocols(scr->root_win);

#ifdef BALLOON_TEXT
    /* initialize balloon text stuff */
    wBalloonInitialize(scr);
#endif

    /* kluge to load menu configurations at startup */
    OpenRootMenu(scr, -10000, -10000, False);
    wMenuUnmap(scr->root_menu);

    return scr;
}


void
wScreenRestoreState(WScreen *scr)
{
    proplist_t state;
    char *path;

    make_keys();

    if (wScreenCount == 1)
	path = wdefaultspathfordomain("WMState");
    else {
	char buf[16];
	sprintf(buf, "WMState.%i", scr->screen);
	path = wdefaultspathfordomain(buf);
    }
    scr->session_state = PLGetProplistWithPath(path);
    free(path);
    if (!scr->session_state && wScreenCount>1) {
	char buf[16];
	sprintf(buf, "WMState.%i", scr->screen);
	path = wdefaultspathfordomain(buf);
	scr->session_state = PLGetProplistWithPath(path);
	free(path);
    }

    if (!wPreferences.flags.noclip) {
        state = PLGetDictionaryEntry(scr->session_state, dClip);
        scr->clip_icon = wClipRestoreState(scr, state);
    }

    wWorkspaceRestoreState(scr);

    if (!wPreferences.flags.nodock) {
        state = PLGetDictionaryEntry(scr->session_state, dDock);
        scr->dock = wDockRestoreState(scr, state, WM_DOCK);
    }

}


void
wScreenSaveState(WScreen *scr)
{
    WWorkspaceState wstate;
    WWindow *wwin;
    char *str;
    proplist_t path, old_state, foo;
    CARD32 data[2];


    make_keys();

/*
 * Save current workspace, so can go back to it upon restart.
 */
    wstate.workspace = scr->current_workspace;

    data[0] = wstate.flags;
    data[1] = wstate.workspace;

    XChangeProperty(dpy, scr->root_win, _XA_WINDOWMAKER_STATE,
                    _XA_WINDOWMAKER_STATE, 32, PropModeReplace,
                    (unsigned char *) data, 2);
    
    /* save state of windows */
    wwin = scr->focused_window;
    while (wwin) {
	wWindowSaveState(wwin);
	wwin = wwin->prev;
    }


    if (wPreferences.flags.noupdates)
	return;


    old_state = scr->session_state;
    scr->session_state = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    PLSetStringCmpHook(NULL);

    /* save dock state to file */
    if (!wPreferences.flags.nodock) {
        wDockSaveState(scr);
    } else {
        if ((foo = PLGetDictionaryEntry(old_state, dDock))!=NULL) {
            PLInsertDictionaryEntry(scr->session_state, dDock, foo);
        }
    }
    if (!wPreferences.flags.noclip) {
        wClipSaveState(scr);
    } else {
        if ((foo = PLGetDictionaryEntry(old_state, dClip))!=NULL) {
            PLInsertDictionaryEntry(scr->session_state, dClip, foo);
        }
    }

    wWorkspaceSaveState(scr, old_state);

    if (wPreferences.save_session_on_exit) {
        wSessionSaveState(scr);
    } else {
        if ((foo = PLGetDictionaryEntry(old_state, dApplications))!=NULL) {
            PLInsertDictionaryEntry(scr->session_state, dApplications, foo);
        }
        if ((foo = PLGetDictionaryEntry(old_state, dWorkspace))!=NULL) {
            PLInsertDictionaryEntry(scr->session_state, dWorkspace, foo);
        }
    }

    /* clean up */
    PLSetStringCmpHook(StringCompareHook);

    wMenuSaveState(scr);

    if (wScreenCount == 1)
	str = wdefaultspathfordomain("WMState");
    else {
	char buf[16];
	sprintf(buf, "WMState.%i", scr->screen);
	str = wdefaultspathfordomain(buf);
    }
    path = PLMakeString(str);
    free(str);
    PLSetFilename(scr->session_state, path);
    if (!PLSave(scr->session_state, YES)) {
	wwarning(_("could not save session state in %s"), PLGetString(path));
    }
    PLRelease(path);
    PLRelease(old_state);
}



int
wScreenBringInside(WScreen *scr, int *x, int *y, int width, int height)
{
    int moved = 0;
    int tol_w, tol_h;

    if (width > 20)
	tol_w = width/2;
    else
	tol_w = 20;

    if (height > 20)
	tol_h = height/2;
    else
	tol_h = 20;

    if (*x+width < 10)
	*x = -tol_w, moved = 1;
    else if (*x >= scr->scr_width - 10)
	*x = scr->scr_width - tol_w - 1, moved = 1;

    if (*y < -height + 10)
	*y = -tol_h, moved = 1;
    else if (*y >= scr->scr_height - 10)
	*y = scr->scr_height - tol_h - 1, moved = 1;

    return moved;
}
