
#include "../src/config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <math.h>


#include "WINGsP.h"




#define SPIT(a)


#define IS_DROPPABLE(view) (view!=NULL && view->droppableTypes!=NULL && \
				view->dragDestinationProcs!=NULL)


static Atom operationToAction(WMScreen *scr, WMDragOperationType operation);
static WMDragOperationType actionToOperation(WMScreen *scr, Atom action);

static Bool _XErrorOccured = False;



static unsigned
defDraggingSourceOperation(WMView *self, Bool local)
{
    return WDOperationCopy;
}


static void
defBeganDragImage(WMView *self, WMPixmap *image, WMPoint point)
{
}


static void
defEndedDragImage(WMView *self, WMPixmap *image, WMPoint point, Bool deposited)
{
}


static WMData*
defFetchDragData(WMView *self, char *type)
{
    return NULL;
}


void
WMSetViewDragSourceProcs(WMView *view, WMDragSourceProcs *procs)
{
    if (view->dragSourceProcs)
	wfree(view->dragSourceProcs);
    view->dragSourceProcs = wmalloc(sizeof(WMDragSourceProcs));
    
    *view->dragSourceProcs = *procs;

    if (procs->draggingSourceOperation == NULL) {
	view->dragSourceProcs->draggingSourceOperation = defDraggingSourceOperation;
    }
    if (procs->beganDragImage == NULL) {
	view->dragSourceProcs->beganDragImage = defBeganDragImage;
    }
    if (procs->endedDragImage == NULL) {
	view->dragSourceProcs->endedDragImage = defEndedDragImage;
    }
    if (procs->fetchDragData == NULL) {
	view->dragSourceProcs->fetchDragData = defFetchDragData;
    }
}


/***********************************************************************/


static int
handleXError(Display *dpy, XErrorEvent *ev)
{
    _XErrorOccured = True;
    
    return 1;
}


static void
protectBlock(Bool begin)
{
    static void *oldHandler = NULL;
	
    if (begin) {
	oldHandler = XSetErrorHandler(handleXError);
    } else {
	XSetErrorHandler(oldHandler);
    }
}




static Window
makeDragIcon(WMScreen *scr, WMPixmap *pixmap)
{
    Window window;
    WMSize size;
    unsigned long flags;
    XSetWindowAttributes attribs;
    Pixmap pix, mask;
    
    if (!pixmap) {
	pixmap = scr->defaultObjectIcon;
    }
    size = WMGetPixmapSize(pixmap);
    pix = pixmap->pixmap;
    mask = pixmap->mask;

    flags = CWSaveUnder|CWBackPixmap|CWOverrideRedirect|CWColormap;
    attribs.save_under = True;
    attribs.background_pixmap = pix;
    attribs.override_redirect = True;
    attribs.colormap = scr->colormap;
    window = XCreateWindow(scr->display, scr->rootWin, 0, 0, size.width,
			   size.height, 0, scr->depth, InputOutput,
			   scr->visual, flags, &attribs);

#ifdef SHAPE
    if (mask) {
	XShapeCombineMask(scr->display, window, ShapeBounding, 0, 0, mask,
			  ShapeSet);
    }
#endif

    return window;
}


static void
slideWindow(Display *dpy, Window win, int srcX, int srcY, int dstX, int dstY)
{
    double x, y, dx, dy;
    int i;
    int iterations;

    iterations = WMIN(25, WMAX(abs(dstX-srcX), abs(dstY-srcY)));
    
    x = srcX;
    y = srcY;
    
    dx = (double)(dstX-srcX)/iterations;
    dy = (double)(dstY-srcY)/iterations;

    for (i = 0; i <= iterations; i++) {
	XMoveWindow(dpy, win, x, y);
        XFlush(dpy);
        
	wusleep(800);
	
        x += dx;
        y += dy;
    }
}


static Window
findChildInWindow(Display *dpy, Window toplevel, int x, int y)
{
    Window foo, bar;
    Window *children;
    unsigned nchildren;
    int i;
        
    if (!XQueryTree(dpy, toplevel, &foo, &bar,
		    &children, &nchildren) || children == NULL) {
	return None;
    }
    
    /* first window that contains the point is the one */
    for (i = nchildren-1; i >= 0; i--) {
	XWindowAttributes attr;

	if (XGetWindowAttributes(dpy, children[i], &attr)
	    && attr.map_state == IsViewable
	    && x >= attr.x && y >= attr.y
	    && x < attr.x + attr.width && y < attr.y + attr.height) {
	    Window child, tmp;
	    
	    tmp = children[i];
	    
	    child = findChildInWindow(dpy, tmp, x - attr.x, y - attr.y);

	    XFree(children);
	    
	    if (child == None)
		return tmp;
	    else
		return child;
	}
    }

    XFree(children);
    return None;
}


static WMView*
findViewInToplevel(Display *dpy, Window toplevel, int x, int y)
{
    Window child;

    child = findChildInWindow(dpy, toplevel, x, y);

    if (child != None) {
	return W_GetViewForXWindow(dpy, child);
    } else {
	return NULL;
    }
}



static Window
lookForToplevel(WMScreen *scr, Window window, Bool *isAware)
{
    Window toplevel = None;
    Atom *atoms;
    int j, count;
    
    *isAware = False;
    
    atoms = XListProperties(scr->display, window, &count);
    for (j = 0; j < count; j++) {
	if (atoms[j] == scr->wmStateAtom) {
	    toplevel = window;
	} else if (atoms[j] == scr->xdndAwareAtom) {
	    *isAware = True;
	}
    }
    if (atoms)
	XFree(atoms);

    if (toplevel == None) {
	Window *children;
	Window foo, bar;
	unsigned nchildren;
	
	if (!XQueryTree(scr->display, window, &foo, &bar,
			&children, &nchildren) || children == NULL) {
	    return None;
	}
	
	for (j = 0; j < nchildren; j++) {
	    toplevel = lookForToplevel(scr, children[j], isAware);
	    if (toplevel != None)
		break;
	}
	
	XFree(children);
    }
    
    return toplevel;
}


    
static Window
findToplevelUnderDragPointer(WMScreen *scr, int x, int y, Window iconWindow)
{
    Window foo, bar;
    Window *children;
    unsigned nchildren;
    Bool overSomething = False;
    int i;

    if (!XQueryTree(scr->display, scr->rootWin, &foo, &bar,
		    &children, &nchildren) || children == NULL) {
	SPIT("couldnt query tree!");
	return None;
    }
    
    /* try to find the window below the iconWindow by traversing
     * the whole window list */

    /* first find the position of the iconWindow */
    for (i = nchildren-1; i >= 0; i--) {
	if (children[i] == iconWindow) {
	    i--;
	    break;
	}
    }
    if (i <= 0) {
	XFree(children);
	return scr->rootWin;
    }

    /* first window that contains the point is the one */
    for (; i >= 0; i--) {
	XWindowAttributes attr;

	if (XGetWindowAttributes(scr->display, children[i], &attr)
	    && attr.map_state == IsViewable
	    && x >= attr.x && y >= attr.y
	    && x < attr.x + attr.width && y < attr.y + attr.height) {
	    Window toplevel;
	    Bool isaware;
	    
	    overSomething = True;
	    
	    toplevel = lookForToplevel(scr, children[i], &isaware);
	    
	    XFree(children);
	    
	    if (isaware)
		return toplevel;
	    else
		return None;
	}
    }

    XFree(children);
    if (!overSomething)
	return scr->rootWin;
    else
	return None;
}






static void
sendClientMessage(Display *dpy, Window win, Atom message,
		  unsigned data1, unsigned data2, unsigned data3,
		  unsigned data4, unsigned data5)
{
    XEvent ev;
    
    ev.type = ClientMessage;
    ev.xclient.message_type = message;
    ev.xclient.format = 32;
    ev.xclient.window = win;
    ev.xclient.data.l[0] = data1;
    ev.xclient.data.l[1] = data2;
    ev.xclient.data.l[2] = data3;
    ev.xclient.data.l[3] = data4;
    ev.xclient.data.l[4] = data5;

    XSendEvent(dpy, win, False, 0, &ev);
    XFlush(dpy);
}




static unsigned
notifyPosition(WMScreen *scr, WMDraggingInfo *info)
{
    Atom action = operationToAction(scr, info->sourceOperation);

    sendClientMessage(scr->display, info->destinationWindow, 
		      scr->xdndPositionAtom,
		      info->sourceWindow, 
		      0, /* reserved */
		      info->location.x<<16|info->location.y,
		      info->timestamp, 
		      action/* operation */);
    
    return 0;
}



static void
notifyDrop(WMScreen *scr, WMDraggingInfo *info)
{
    sendClientMessage(scr->display, info->destinationWindow,
		      scr->xdndDropAtom,
		      info->sourceWindow, 
		      0, /* reserved */
		      info->timestamp,
		      0, 0);
}



static void
notifyDragLeave(WMScreen *scr, WMDraggingInfo *info)
{	
    sendClientMessage(scr->display, info->destinationWindow,
		      scr->xdndLeaveAtom,
		      info->sourceWindow, 0, 0, 0, 0);
}



static unsigned
notifyDragEnter(WMScreen *scr, WMDraggingInfo *info)
{
    unsigned d;
	
    d = XDND_VERSION << 24;
	
    sendClientMessage(scr->display, info->destinationWindow,
		      scr->xdndEnterAtom,
		      info->sourceWindow, d, 0, 0, 0);
    
    return 0;
}


static void
translateCoordinates(WMScreen *scr, Window target, int fromX, int fromY,
		     int *x, int *y)
{
    Window child;
	
    XTranslateCoordinates(scr->display, scr->rootWin, target,
			  fromX, fromY, x, y, &child);
}


static void
updateDraggingInfo(WMScreen *scr, WMDraggingInfo *info, WMSize offset,
		   XEvent *event, Window iconWindow)
{
    Window toplevel;

    if (event->type == MotionNotify) {
	info->imageLocation.x = event->xmotion.x_root-offset.width;
	info->imageLocation.y = event->xmotion.y_root-offset.height;
        
	info->location.x = event->xmotion.x_root;
	info->location.y = event->xmotion.y_root;
/*	info->timestamp = event->xmotion.time;*/
	
    } else if (event->type == ButtonRelease) {
	info->imageLocation.x = event->xbutton.x_root-offset.width;
	info->imageLocation.y = event->xbutton.y_root-offset.height;
        
	info->location.x = event->xbutton.x_root;
	info->location.y = event->xbutton.y_root;
/*	info->timestamp = event->xbutton.time;*/
    }

    toplevel = findToplevelUnderDragPointer(scr,
					    info->location.x,
					    info->location.y,
					    iconWindow);
    info->destinationWindow = toplevel;
}




static void
processMotion(WMScreen *scr, WMDraggingInfo *info, WMDraggingInfo *oldInfo,
	      WMRect *rect, unsigned currentAction)
{
    unsigned action;
    
    if (info->destinationWindow == None) { /* entered an unsupporeted window */

	if (oldInfo->destinationWindow != None
	    && oldInfo->destinationWindow != scr->rootWin) {
	    SPIT("left window");
	    
	    notifyDragLeave(scr, oldInfo);
	}

    } else if (info->destinationWindow == scr->rootWin) {

	if (oldInfo->destinationWindow != None
	    && oldInfo->destinationWindow != scr->rootWin) {
	    SPIT("left window to root");
	    
	    notifyDragLeave(scr, oldInfo);
	} else {
	    /* nothing */
	}

    } else if (oldInfo->destinationWindow != info->destinationWindow) {

	if (oldInfo->destinationWindow != None
	    && oldInfo->destinationWindow != scr->rootWin) {
	    notifyDragLeave(scr, oldInfo);
	    SPIT("crossed");
	} else {
	    SPIT("entered window");
	}

	action = notifyDragEnter(scr, info);

    } else {

#define LEFT_RECT(r, X, Y) (X < r->pos.x || Y < r->pos.y \
				|| X >= r->pos.x + r->size.width \
				|| Y >= r->pos.y + r->size.height)

	if (rect->size.width == 0 ||
	    (LEFT_RECT(rect, info->location.x, info->location.y))) {

	    action = notifyPosition(scr, info);

	    rect->size.width = 0;
	}
    }
}



static WMData*
convertSelection(WMView *view, Atom selection, Atom target,
		 void *cdata, Atom *type)
{
    WMScreen *scr = W_VIEW_SCREEN(view);
    WMData *data;
    char *typeName = XGetAtomName(scr->display, target);
    
    *type = target;
    
    data = view->dragSourceProcs->fetchDragData(view, typeName);

    if (typeName != NULL)
	XFree(typeName);

    return data;
}


static void 
selectionLost(WMView *view, Atom selection, void *cdata)
{
    if (W_VIEW_SCREEN(view)->dragSourceView == view) {
	wwarning("DND selection lost during drag operation...");
	W_VIEW_SCREEN(view)->dragSourceView = NULL;
    }
}


static void 
selectionDone(WMView *view, Atom selection, Atom target, void *cdata)
{
    
}





static void
setMouseOffsetHint(WMView *view, WMSize mouseOffset)
{
    WMScreen *scr = W_VIEW_SCREEN(view);
    long hint[2];

    /*
     * Tell the offset from the topleft corner of the icon being
     * dragged. Not from XDND, but it's backwards compatible.
     */
    
    hint[0] = mouseOffset.width;
    hint[1] = mouseOffset.height;
    
    XChangeProperty(scr->display, W_VIEW_DRAWABLE(view), 
		    scr->wmIconDragOffsetAtom, XA_INTEGER, 32,
		    PropModeReplace, (unsigned char*)hint, 2);
}


static Bool
getMouseOffsetHint(WMScreen *scr, Window source, WMSize *mouseOffset)
{
    long *hint;
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret, bytes_after_ret;
    Bool ok = False;
    

    hint = NULL;
    
    XGetWindowProperty(scr->display, source, 
		       scr->wmIconDragOffsetAtom, 0, 2, False, XA_INTEGER,
		       &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
		       (unsigned char **)&hint);
    
    if (hint && nitems_ret == 2) {
	mouseOffset->width = hint[0];
	mouseOffset->height = hint[1];
	ok = True;
    }
    if (hint)
	XFree(hint);
    
    return ok;
}



static void
timeoutCallback(void *data)
{
    wwarning("drag & drop timed out while waiting for response from 0x%x\n",
	     (unsigned)data);
    _XErrorOccured = 2;
}

/*
 * State Machine For Drag Source:
 * ------------------------------
 * 			        	     Events
 * State                Call  Mtn   Ent   Lea   Crs   BUp   StA   StR   Fin   TO
 * 0) idle		1bu   -     -     -     -     -     -     -     -     -
 * 1) drag over target  -     1au   -     2cu   1cbu  5fu   3     4     1w    -
 * 2) drag over nothing -     2     1bu   -     -     0     -     -     2w    -
 * 3) drag targ+accept  -     3u    -     2cu   1cbu  6f    3     4w    0z    -
 * 4) drag targ+reject  -     4u    -     2cu   1cbu  0     3w    4     0z    -
 * 5) waiting status    -     5X    5X    5X    5X    -     6f    0     0z    0w
 * 6) dropped		-     -     -     -     -     -     -     -     0     0w
 * 
 * Events:
 * Call - called WMDragImageFromView()
 * Mtn - Motion
 * Ent - Enter droppable window
 * Lea - Leave droppable window (or rectangle)
 * Crs - Leave droppable window (or rectangle) and enter another
 * BUp - Button Released
 * StA - XdndStatus client msg with Accept drop
 * StR - XdndStatus client msg with Reject drop
 * Fin - XdndFinish client msg
 * TO  - timeout
 * 
 * Actions:
 * a - send update message
 * b - send enter message
 * c - send leave message
 * d - release drag section info
 * e - send drop message
 * f - setup timeout
 * u - update dragInfo
 * 
 * X - ignore
 * w - warn about unexpected reply
 * z - abort operation.. unexpected reply
 * -   shouldnt happen
 */
void
WMDragImageFromView(WMView *view, WMPixmap *image, char *dataTypes[],
		    WMPoint atLocation, WMSize mouseOffset, XEvent *event,
		    Bool slideBack)
{
    WMScreen *scr = view->screen;
    Display *dpy = scr->display;
    WMView *toplevel = W_TopLevelOfView(view);
    Window icon;
    XEvent ev;
    WMRect rect = {{0,0},{0,0}};
    int ostate = -1;
    int state;
    int action = -1;
    XColor black = {0, 0,0,0, DoRed|DoGreen|DoBlue};
    XColor green = {0x0045b045, 0x4500,0xb000,0x4500, DoRed|DoGreen|DoBlue};
    XColor back = {0, 0xffff,0xffff,0xffff, DoRed|DoGreen|DoBlue};
    WMDraggingInfo dragInfo;
    WMDraggingInfo oldDragInfo;
    WMHandlerID timer = NULL;
    static WMSelectionProcs handler = {
	convertSelection,
	    selectionLost,
	    selectionDone
    };

    
    if (scr->dragSourceView != NULL)
	return;
    
    wassertr(view->dragSourceProcs != NULL);

    
    /* prepare icon to be dragged */
    if (image == NULL)
	image = scr->defaultObjectIcon;

    icon = makeDragIcon(scr, image);

    XMoveWindow(dpy, icon, atLocation.x, atLocation.y);
    XMapRaised(dpy, icon);
    

    /* init dragging info */
    
    scr->dragSourceView = view;

    memset(&dragInfo, 0, sizeof(WMDraggingInfo));
    memset(&oldDragInfo, 0, sizeof(WMDraggingInfo));
    dragInfo.image = image;
    dragInfo.sourceView = view;
    dragInfo.sourceWindow = W_VIEW_DRAWABLE(toplevel);

    dragInfo.destinationWindow = dragInfo.sourceWindow;

    dragInfo.location.x = atLocation.x + mouseOffset.width;
    dragInfo.location.y = atLocation.y + mouseOffset.height;
    dragInfo.imageLocation = atLocation;
    
    
    /* start pointer grab */
    XGrabPointer(dpy, scr->rootWin, False,
		 ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
		 GrabModeAsync, GrabModeAsync, None, scr->defaultCursor,
		 CurrentTime);

    XFlush(dpy);

    _XErrorOccured = False;
    
    /* take ownership of XdndSelection */
    if (!WMCreateSelectionHandler(view, scr->xdndSelectionAtom,
				  event->xmotion.time,
				  &handler, NULL)) {
	wwarning("could not get ownership or DND selection");
	return;
    }
    
    setMouseOffsetHint(toplevel, mouseOffset);

    if (view->dragSourceProcs->beganDragImage != NULL) {
	view->dragSourceProcs->beganDragImage(view, image, atLocation);
    }

    processMotion(scr, &dragInfo, &oldDragInfo, &rect, action);

    state = 1;

    while (state != 6 && state != 0 && !_XErrorOccured) {
	WMNextEvent(dpy, &ev);

	switch (ev.type) {
	 case MotionNotify:
	    if (state >= 1 && state <= 4) {
		while (XCheckTypedEvent(dpy, MotionNotify, &ev)) ;

		protectBlock(True);

		oldDragInfo = dragInfo;

		updateDraggingInfo(scr, &dragInfo, mouseOffset, &ev, icon);

		XMoveWindow(dpy, icon, dragInfo.imageLocation.x,
			    dragInfo.imageLocation.y);

		processMotion(scr, &dragInfo, &oldDragInfo, &rect, action);

		protectBlock(False);

		/* XXXif entered a different destination, check the operation */

		switch (state) {
		 case 1:
		    if (oldDragInfo.destinationWindow != None
			 && oldDragInfo.destinationWindow != scr->rootWin
			 && (dragInfo.destinationWindow == None
			     || dragInfo.destinationWindow == scr->rootWin)) {
			/* left the droppable window */
			state = 2;
			action = -1;
		    }
		    break;
		    
		 case 2:
		    if (dragInfo.destinationWindow != None
			&& dragInfo.destinationWindow != scr->rootWin) {

			state = 1;
			action = -1;
		    }
		    break;

		 case 3:
		 case 4:
		    if (oldDragInfo.destinationWindow != None
			 && oldDragInfo.destinationWindow != scr->rootWin
			 && (dragInfo.destinationWindow == None
			     || dragInfo.destinationWindow == scr->rootWin)) {
			/* left the droppable window */
			state = 2;
			action = -1;
		    }
		    break;
		}
	    }
	    break;
	    

	 case ButtonRelease:
	    /* if (state >= 1 && state <= 4) */ {
		
		protectBlock(True);
	    
		oldDragInfo = dragInfo;

		updateDraggingInfo(scr, &dragInfo, mouseOffset, &ev, icon);

		XMoveWindow(dpy, icon, dragInfo.imageLocation.x,
			    dragInfo.imageLocation.y);

		if (state == 4 || state == 1) {
		    dragInfo.destinationWindow = None;
		    dragInfo.destView = NULL;
		}
		processMotion(scr, &dragInfo, &oldDragInfo, &rect, action);
		
		dragInfo.timestamp = ev.xbutton.time;

		protectBlock(False);
		
		switch (state) {
		 case 1:
		    state = 5;
		    timer = WMAddTimerHandler(3000, timeoutCallback,
					      (void*)dragInfo.destinationWindow);
		    break;
		 case 2:
		    state = 0;
		    break;
		 case 3:
		    state = 6;
		    break;
		 case 4:
		    state = 0;
		    break;
		}
	    }
	    break;

	 case ClientMessage:
	    if ((state == 1 || state == 3 || state == 4 || state == 5)
		&& ev.xclient.message_type == scr->xdndStatusAtom
		&& ev.xclient.data.l[0] == dragInfo.destinationWindow) {

		if (ev.xclient.data.l[1] & 1) {
		    SPIT("got accept msg");
		    /* will accept drop */
		    switch (state) {
		     case 1:
		     case 3:
		     case 4:
			state = 3;
			break;
		     case 5:
			if (timer) {
			    WMDeleteTimerHandler(timer);
			    timer = NULL;
			}
			state = 6;
			break;
		    }
		    action = actionToOperation(scr, ev.xclient.data.l[4]);
		} else {
		    SPIT("got reject msg");
		    switch (state) {
		     case 1:
		     case 3:
		     case 4:
			state = 4;
			break;
		     case 5:
			state = 0;
			if (timer) {
			    WMDeleteTimerHandler(timer);
			    timer = NULL;
			}
			break;
		    }
		    action = 0;
		}

		if (ev.xclient.data.l[1] & (1<<1)) {
		    rect.pos.x = ev.xclient.data.l[2] >> 16;
		    rect.pos.y = ev.xclient.data.l[2] & 0xffff;
		    rect.size.width = ev.xclient.data.l[3] >> 16;
		    rect.size.height = ev.xclient.data.l[3] & 0xffff;
		} else {
		    rect.size.width = 0;
		}
				
	    } else if ((state >= 1 && state <= 5)
		       && ev.xclient.message_type == scr->xdndFinishedAtom
		       && ev.xclient.window == dragInfo.destinationWindow) {

		wwarning("drag source received unexpected XdndFinished message from %x",
			 (unsigned)dragInfo.destinationWindow);

		if (state == 3 || state == 4 || state == 5) {
		    state = 0;
		    if (timer) {
			WMDeleteTimerHandler(timer);
			timer = NULL;
		    }
		}
	    }
	    
	 default:
	    WMHandleEvent(&ev);
	    break;
	}
	
	if (ostate != state) {
	    if (state == 3) {
		XRecolorCursor(dpy, scr->defaultCursor, &green, &back);
	    } else if (ostate == 3) {
		XRecolorCursor(dpy, scr->defaultCursor, &black, &back);
	    }
	    ostate = state;
	}
    }
    
    if (timer) {
	WMDeleteTimerHandler(timer);
	timer = NULL;
    } else if (_XErrorOccured) {
	/* got a timeout, send leave */
	notifyDragLeave(scr, &dragInfo);
    }

    XUngrabPointer(dpy, CurrentTime);
    
    SPIT("exited main loop");

    if (_XErrorOccured || state != 6) {
	goto cancelled;
    }

    assert(dragInfo.destinationWindow != None);
    
    protectBlock(True);
    notifyDrop(scr, &dragInfo);
    protectBlock(False);
    
    if (_XErrorOccured)
	goto cancelled;
    
    
    SPIT("dropped");
    

    XDestroyWindow(dpy, icon);
    
    return;

cancelled:
    scr->dragSourceView = NULL;
    
    WMDeleteSelectionHandler(view, scr->xdndSelectionAtom, 
			     event->xmotion.time);

    if (slideBack) {
	slideWindow(dpy, icon,
		    dragInfo.imageLocation.x, dragInfo.imageLocation.y,
		    atLocation.x, atLocation.y);
    }
    XDestroyWindow(dpy, icon);
    if (view->dragSourceProcs->endedDragImage != NULL) {
	view->dragSourceProcs->endedDragImage(view, image, 
					      dragInfo.imageLocation,
					      False);
    }
}












static Atom
operationToAction(WMScreen *scr, WMDragOperationType operation)
{
    switch (operation) {
     case WDOperationNone:
	return None;
	    
     case WDOperationCopy:
	return scr->xdndActionCopy;
	    
     case WDOperationMove:
	return scr->xdndActionMove;
	    
     case WDOperationLink:
	return scr->xdndActionLink;
	
     case WDOperationAsk:
	return scr->xdndActionAsk;
	    
     case WDOperationPrivate:
	return scr->xdndActionPrivate;
	    
     default:
	return None;
    }
}


static WMDragOperationType
actionToOperation(WMScreen *scr, Atom action)
{
    if (action == scr->xdndActionCopy) {
	return WDOperationCopy;
	
    } else if (action == scr->xdndActionMove) {
	return WDOperationMove;
	
    } else if (action == scr->xdndActionLink) {
	return WDOperationLink;
	
    } else if (action == scr->xdndActionAsk) {
	return WDOperationAsk;
	
    } else if (action == scr->xdndActionPrivate) {
	return WDOperationPrivate;
	
    } else if (action == None) {
	
	return WDOperationCopy;
    } else {
	char *tmp = XGetAtomName(scr->display, action);
	
	wwarning("unknown XDND action %s from 0x%x", tmp,
		 (unsigned)scr->dragInfo.sourceWindow);
	XFree(tmp);
	
	return WDOperationCopy;
    }
}
    





static Atom*
getTypeList(Window window, XClientMessageEvent *event)
{
    int i = 0;
    Atom *types = NULL;
    
    if (event->data.l[1] & 1) { /* > 3 types */
	    
    } else {
	types = wmalloc(4 * sizeof(Atom));
	if (event->data.l[2] != None)
	    types[i++] = event->data.l[2];
	if (event->data.l[3] != None)
	    types[i++] = event->data.l[3];
	if (event->data.l[4] != None)
	    types[i++] = event->data.l[4];
	types[i] = 0;
    }
    
    if (types[0] == 0) {
	wwarning("received invalid drag & drop type list");
	/*XXX	    return;*/
    }

    return types;
}



#define DISPATCH(view, func, info) (view)->dragDestinationProcs->func(view, info)



static void
receivedData(WMView *view, Atom selection, Atom target,
	     Time timestamp, void *cdata, WMData *data)
{
    WMScreen *scr = W_VIEW_SCREEN(view);
    WMDraggingInfo *info = (WMDraggingInfo*)cdata;
    Bool res;

    res = view->dragDestinationProcs->performDragOperation(view, info);

    if (res) {
	DISPATCH(view, concludeDragOperation, info);
    }

    /* send finished message */
    sendClientMessage(scr->display, info->sourceWindow,
		      scr->xdndFinishedAtom,
		      info->destinationWindow,
		      0, 0, 0, 0);

    memset(&scr->dragInfo, 0, sizeof(WMDraggingInfo));
}



void
W_HandleDNDClientMessage(WMView *toplevel, XClientMessageEvent *event)
{
    WMScreen *scr = W_VIEW_SCREEN(toplevel);
    WMView *oldView = NULL;
    WMView *newView = NULL;
    unsigned operation = 0;
    int x, y;
    enum {
	WNothing,
	WEnter,
	WLeave,
	WCross, /* leave one and enter another */
	WUpdate,
	WDrop
    };
    Window source;
    int what = WNothing;
    Bool sendStatus = False;
    

    source = scr->dragInfo.sourceWindow;
    oldView = scr->dragInfo.destView;

    if (event->message_type == scr->xdndFinishedAtom) {
	WMView *view = scr->dragSourceView;
	
	WMDeleteSelectionHandler(view, scr->xdndSelectionAtom,
				 scr->dragInfo.timestamp);
	
	if (view->dragSourceProcs->endedDragImage != NULL) {
	    view->dragSourceProcs->endedDragImage(view,
						  scr->dragInfo.image,
						  scr->dragInfo.imageLocation,
						  True);
	}
	
	scr->dragSourceView = NULL;
	
	return;
    }
    
    

    if (event->message_type == scr->xdndEnterAtom) {
	Window foo, bar;
	int bla;
	unsigned ble;

	if (scr->dragInfo.sourceWindow != None) {
	    puts("received Enter event in bad order");
	}
	
	memset(&scr->dragInfo, 0, sizeof(WMDraggingInfo));
	
	
	if ((event->data.l[1] >> 24) > XDND_VERSION) {
	    wwarning("received drag & drop request with unsupported version %i",
		    (event->data.l[1] >> 24));
	    return;
	}
	
	scr->dragInfo.protocolVersion = event->data.l[1] >> 24;
	scr->dragInfo.sourceWindow = source = event->data.l[0];
	scr->dragInfo.destinationWindow = event->window;
	
	getMouseOffsetHint(scr, source, &scr->dragInfo.mouseOffset);
	
	/* XXX */
	scr->dragInfo.image = NULL;
	
	XQueryPointer(scr->display, scr->rootWin, &foo, &bar,
		      &scr->dragInfo.location.x, &scr->dragInfo.location.y,
		      &bla, &bla, &ble);
	
	scr->dragInfo.imageLocation = scr->dragInfo.location;
	scr->dragInfo.imageLocation.x -= scr->dragInfo.mouseOffset.width;
	scr->dragInfo.imageLocation.y -= scr->dragInfo.mouseOffset.height;

	translateCoordinates(scr, scr->dragInfo.destinationWindow,
			     scr->dragInfo.location.x,
			     scr->dragInfo.location.y, &x, &y);


	newView = findViewInToplevel(scr->display,
				     scr->dragInfo.destinationWindow, 
				     x, y);

    } else if (event->message_type == scr->xdndPositionAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {

	scr->dragInfo.location.x = event->data.l[2] >> 16;
	scr->dragInfo.location.y = event->data.l[2] & 0xffff;

	scr->dragInfo.imageLocation = scr->dragInfo.location;
	scr->dragInfo.imageLocation.x -= scr->dragInfo.mouseOffset.width;
	scr->dragInfo.imageLocation.y -= scr->dragInfo.mouseOffset.height;

	if (scr->dragInfo.protocolVersion >= 1) {
	    scr->dragInfo.timestamp = event->data.l[3];
	    scr->dragInfo.sourceOperation = actionToOperation(scr,
							    event->data.l[4]);

	} else {
	    scr->dragInfo.timestamp = CurrentTime;
	    scr->dragInfo.sourceOperation = WDOperationCopy;
	}

	translateCoordinates(scr, scr->dragInfo.destinationWindow,
			     scr->dragInfo.location.x,
			     scr->dragInfo.location.y, &x, &y);

	newView = findViewInToplevel(scr->display, 
				     scr->dragInfo.destinationWindow,
				     x, y);
	
    } else if (event->message_type == scr->xdndLeaveAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {

	memset(&scr->dragInfo, 0, sizeof(WMDraggingInfo));

    } else if (event->message_type == scr->xdndDropAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {

	/* drop */
	if (oldView != NULL)
	    what = WDrop;

    } else {
	return;
    }


    /*
     * Now map the XDND events to WINGs events.
     */

    if (what == WNothing) {
	if (IS_DROPPABLE(newView)) {
	    if (!IS_DROPPABLE(oldView)) { /* entered */
		what = WEnter;
	    } else if (oldView == newView) { /* updated */
		what = WUpdate;
	    } else {
		what = WCross;
	    }
	} else {
	    if (IS_DROPPABLE(oldView)) {
		what = WLeave;
	    } else {
		/* just send rejection msg */
		sendStatus = True;
	    }
	}
    }
    
    
    
    switch (what) {

     case WEnter:
	scr->dragInfo.destView = newView;
	operation = DISPATCH(newView, draggingEntered, &scr->dragInfo);
	sendStatus = True;
	break;
	
     case WLeave:
	scr->dragInfo.destView = NULL;
	DISPATCH(oldView, draggingExited, &scr->dragInfo);
	sendStatus = True;
	operation = WDOperationNone;
	break;
	
     case WCross:
	DISPATCH(oldView, draggingExited, &scr->dragInfo);
	scr->dragInfo.destView = newView;
	operation = DISPATCH(newView, draggingEntered, &scr->dragInfo);
	sendStatus = True;
	break;
	
     case WUpdate:
	operation = DISPATCH(oldView, draggingUpdated, &scr->dragInfo);
	sendStatus = True;
	break;
	
     case WDrop:
	{
	    Bool res;
	    
	    res = DISPATCH(oldView, prepareForDragOperation, &scr->dragInfo);

	    if (res) {
		res = DISPATCH(oldView, performDragOperation, &scr->dragInfo);
	    }
	    
	    if (res) {
		
	    }
	}
	break;
	
     default:
	break;
    }
    
    
    
    if (sendStatus) {
	Atom action;

	action = operationToAction(scr, operation);

	sendClientMessage(scr->display, source,
			  scr->xdndStatusAtom,
			  scr->dragInfo.destinationWindow,
			  action != None ? 1 : 0, 0, 0, action);
    }
}




