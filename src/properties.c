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

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>
#include <stdlib.h>

#include "WindowMaker.h"
#include "window.h"
#include "GNUstep.h"
#ifdef MWM_HINTS
#  include "motif.h"
#endif


/* atoms */
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_SAVE_YOURSELF;

extern Atom _XA_GNUSTEP_WM_ATTR;
extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;

extern Atom _XA_WINDOWMAKER_WM_FUNCTION;
extern Atom _XA_WINDOWMAKER_MENU;
extern Atom _XA_WINDOWMAKER_WM_PROTOCOLS;


int
PropGetNormalHints(Window window, XSizeHints *size_hints, int *pre_iccm)
{
    long supplied_hints;

    if (!XGetWMNormalHints(dpy, window, size_hints, &supplied_hints)) {
	return False;
    }    
    if (supplied_hints==(USPosition|USSize|PPosition|PSize|PMinSize|PMaxSize
	|PResizeInc|PAspect)) {
	*pre_iccm = 1;
    } else {
	*pre_iccm = 0;
    }
    return True;
}


int
PropGetWMClass(Window window, char **wm_class, char **wm_instance)
{
    XClassHint	*class_hint;

    class_hint = XAllocClassHint();
    if (XGetClassHint(dpy,window,class_hint) == 0) {
	*wm_class = NULL;
	*wm_instance = NULL;
	XFree(class_hint);
	return False;
    }
    *wm_instance = class_hint->res_name;
    *wm_class = class_hint->res_class;

    XFree(class_hint);    
    return True;
}

void
PropGetProtocols(Window window, WProtocols *prots)
{
    Atom *protocols;
    int count, i;
    
    memset(prots, 0, sizeof(WProtocols));
    if (!XGetWMProtocols(dpy, window, &protocols, &count)) {
	return;
    }
    for (i=0; i<count; i++) {
	if (protocols[i]==_XA_WM_TAKE_FOCUS)
	  prots->TAKE_FOCUS=1;
	else if (protocols[i]==_XA_WM_DELETE_WINDOW)
	  prots->DELETE_WINDOW=1;
	else if (protocols[i]==_XA_WM_SAVE_YOURSELF)
	  prots->SAVE_YOURSELF=1;
	else if (protocols[i]==_XA_GNUSTEP_WM_MINIATURIZE_WINDOW)
	  prots->MINIATURIZE_WINDOW=1;
    }
    XFree(protocols);
}


int
PropGetGNUstepWMAttr(Window window, GNUstepWMAttributes **attr)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    CARD32 *data;

    if (XGetWindowProperty(dpy, window, _XA_GNUSTEP_WM_ATTR, 0, 9,
			   False, _XA_GNUSTEP_WM_ATTR,
			   &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
			   (unsigned char **)&data)!=Success || !data)
      return False;
    if (type_ret!=_XA_GNUSTEP_WM_ATTR || !data || fmt_ret!=32)
      return False;
    
    *attr = malloc(sizeof(GNUstepWMAttributes));
    if (!*attr) {
	XFree(data);
	return False;
    }
    (*attr)->flags = data[0];
    (*attr)->window_style = data[1];
    (*attr)->window_level = data[2];
    (*attr)->reserved = data[3];
    (*attr)->miniaturize_pixmap = data[4];
    (*attr)->close_pixmap = data[5];
    (*attr)->miniaturize_mask = data[6];
    (*attr)->close_mask = data[7];
    (*attr)->extra_flags = data[8];

    XFree(data);

    return True;
}


 
 
#ifdef MWM_HINTS
int
PropGetMotifWMHints(Window window, MWMHints **mwmhints)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    CARD32 *data;
    
    if (XGetWindowProperty(dpy, window, _XA_MOTIF_WM_HINTS, 0,
			   PROP_MWM_HINTS_ELEMENTS,
			   False, _XA_MOTIF_WM_HINTS,
			   &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
			   (unsigned char **)&data)!=Success)
      return 0;
    
    if (type_ret!=_XA_MOTIF_WM_HINTS || fmt_ret!=32 
	|| nitems_ret!=PROP_MWM_HINTS_ELEMENTS || !data)
	return 0;
    
    *mwmhints = malloc(sizeof(MWMHints));
    if (!*mwmhints) {
	XFree(data);
	return 0;
    }

    (*mwmhints)->flags = data[0];
    (*mwmhints)->functions = data[1];
    (*mwmhints)->decorations = data[2];
    (*mwmhints)->inputMode = data[3];

    XFree(data);

    if (type_ret==_XA_MOTIF_WM_HINTS)
      return 1;
    else 
      return 0;
}
#endif /* MWM_HINTS */


void
PropSetWMakerProtocols(Window root)
{
    Atom protocols[2];
    int count=0;
    
    protocols[count++] = _XA_WINDOWMAKER_MENU;
    protocols[count++] = _XA_WINDOWMAKER_WM_FUNCTION;

    XChangeProperty(dpy, root, _XA_WINDOWMAKER_WM_PROTOCOLS, XA_ATOM,
		    32, PropModeReplace,  (unsigned char *)protocols, count);
}


int
PropGetClientLeader(Window window, Window *leader)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    Window *win;

    if (XGetWindowProperty(dpy, window, _XA_WM_CLIENT_LEADER, 0, 4,
			   False, AnyPropertyType,
			   &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
			   (unsigned char**)&win)!=Success)
      return 0;

    if (!win) return 0;
    *leader = (Window)*win;
    XFree(win);
    return 1;
}



void
PropWriteGNUstepWMAttr(Window window, GNUstepWMAttributes *attr)
{
    CARD32 data[9];
        
    /* handle idiot compilers where array of CARD32 != struct of CARD32 */
    data[0] = attr->flags;
    data[1] = attr->window_style;
    data[2] = attr->window_level;
    data[3] = 0;		       /* reserved */
    /* The X protocol says XIDs are 32bit */
    data[4] = attr->miniaturize_pixmap;
    data[5] = attr->close_pixmap;
    data[6] = attr->miniaturize_mask;
    data[7] = attr->close_mask;
    data[8] = attr->extra_flags;
    XChangeProperty(dpy, window, _XA_GNUSTEP_WM_ATTR, _XA_GNUSTEP_WM_ATTR, 
		    32, PropModeReplace,  (unsigned char *)data, 9);
}


void
PropCleanUp(Window root)
{
    XDeleteProperty(dpy, root, _XA_WINDOWMAKER_WM_PROTOCOLS);
    
    XDeleteProperty(dpy, root, XA_WM_ICON_SIZE);
}
