

#include <X11/Xatom.h>
#include <math.h>

#include "WINGsP.h"




#define SPIT(a) puts(a)


#define IS_DROPPABLE(view) (view!=NULL && view->droppableTypes!=NULL && \
				view->dragDestinationProcs!=NULL)


static Bool _XErrorOccured = False;




void
WMSetViewDragSourceProcs(WMView *view, WMDragSourceProcs *procs)
{
    if (view->dragSourceProcs)
	free(view->dragSourceProcs);
    view->dragSourceProcs = wmalloc(sizeof(WMDragSourceProcs));
    
    *view->dragSourceProcs = *procs;
    
    /* XXX fill in non-implemented stuffs */
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
	XShapeCombineMask(dpy, scr->balloon->window, ShapeBounding, 0, 0, mask,
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
	    Window child;
	    
	    child = findChildInWindow(dpy, children[i], 
				      x - attr.x, y - attr.y);

	    XFree(children);
	    
	    if (child == None)
		return toplevel;
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
    
    if (child != None)
	return W_GetViewForXWindow(dpy, child);
    else
	return NULL;
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
    unsigned operation;
	
    switch (info->sourceOperation) {
     default:
	operation = None;
	break;
    }

    sendClientMessage(scr->display, info->destinationWindow, 
		      scr->xdndPositionAtom,
		      info->sourceWindow, 
		      0, /* reserved */
		      info->location.x<<16|info->location.y,
		      info->timestamp, 
		      operation/* operation */);
    
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
updateDraggingInfo(WMScreen *scr, WMDraggingInfo *info,
		   XEvent *event, Window iconWindow)
{
    Window toplevel;
    WMSize size;
    
    size = WMGetPixmapSize(info->image);

    if (event->type == MotionNotify) {
	info->imageLocation.x = event->xmotion.x_root-(int)size.width/2;
	info->imageLocation.y = event->xmotion.y_root-(int)size.height/2;
        
	info->location.x = event->xmotion.x_root;
	info->location.y = event->xmotion.y_root;
	info->timestamp = event->xmotion.time;
	
    } else if (event->type == ButtonRelease) {
	info->imageLocation.x = event->xbutton.x_root-(int)size.width/2;
	info->imageLocation.y = event->xbutton.y_root-(int)size.height/2;
        
	info->location.x = event->xbutton.x_root;
	info->location.y = event->xbutton.y_root;
	info->timestamp = event->xbutton.time;
    }

    toplevel = findToplevelUnderDragPointer(scr,
					    info->location.x,
					    info->location.y,
					    iconWindow);
    info->destinationWindow = toplevel;
    /*
    if (toplevel == None) {
	info->destinationWindow = None;
    } else if (toplevel == scr->rootWin) {
	info->destinationWindow = scr->rootWin;
    } else {
	Window child;
	int x, y;
	
	XTranslateCoordinates(scr->display, scr->rootWin, toplevel,
			      info->location.x, info->location.y,
			      &x, &y, &child);

	child = findChildInWindow(scr->display, toplevel, x, y);

	if (child != None) {
	    info->destination = W_GetViewForXWindow(scr->display, child);
	    if (info->destination->droppableTypes == NULL) {
		info->destination = NULL;
	    } else if (info->destination->dragDestinationProcs == NULL) {
		info->destination = NULL;
	    }
	} else {
	    info->destination = NULL;
	}
	info->destinationWindow = toplevel;
    }
    */
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
    
    /* little trick to simulate XdndStatus for local dnd */
    /*
    if (bla && action != currentAction) {
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.display = scr->display;
	ev.xclient.message_type = scr->xdndStatusAtom;
	ev.xclient.format = 32;
	ev.xclient.window = info->destinationWindow;
	ev.xclient.data.l[0] = info->sourceWindow;
	ev.xclient.data.l[1] = (action ? 1 : 0);
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = action;
	
	XPutBackEvent(scr->display, &ev);
    }*/
}




static void
timeoutCallback(void *data)
{
    wwarning("drag & drop timed out while waiting for response from 0x%x\n",
	     (unsigned)data);
    _XErrorOccured = 1;
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
    Window icon;
    XEvent ev;
    WMSize size;
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
    WMData *draggedData = NULL;

    
    wassertr(view->dragSourceProcs != NULL);

    
    /* prepare icon to be dragged */
    if (image == NULL)
	image = scr->defaultObjectIcon;

    size = WMGetPixmapSize(image);

    icon = makeDragIcon(scr, image);

    XMoveWindow(dpy, icon, event->xmotion.x_root-(int)size.width/2,
		event->xmotion.y_root-(int)size.height/2);
    XMapRaised(dpy, icon);
    

    /* init dragging info */
    memset(&dragInfo, 0, sizeof(WMDraggingInfo));
    memset(&oldDragInfo, 0, sizeof(WMDraggingInfo));
    dragInfo.image = image;
    dragInfo.sourceWindow = W_VIEW_DRAWABLE(W_TopLevelOfView(view));

    dragInfo.destinationWindow = dragInfo.sourceWindow;

    dragInfo.location.x = event->xmotion.x_root;
    dragInfo.location.y = event->xmotion.y_root;
    dragInfo.imageLocation = atLocation;
    
    
    /* start pointer grab */
    XGrabPointer(dpy, scr->rootWin, False,
		 ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
		 GrabModeSync, GrabModeAsync, None, scr->defaultCursor,
		 CurrentTime);

    XFlush(dpy);

    _XErrorOccured = False;
    
    /* XXX: take ownership of XdndSelection */

    
    if (view->dragSourceProcs->beganDragImage != NULL) {
	view->dragSourceProcs->beganDragImage(view, image, atLocation);
    }

    processMotion(scr, &dragInfo, &oldDragInfo, &rect, action);

    state = 1;

    while (state != 6 && state != 0 && !_XErrorOccured) {
	XAllowEvents(dpy, SyncPointer, CurrentTime);
	WMNextEvent(dpy, &ev);

	switch (ev.type) {
	 case MotionNotify:
	    if (state >= 1 && state <= 4) {
		while (XCheckTypedEvent(dpy, MotionNotify, &ev)) ;

		protectBlock(True);

		oldDragInfo = dragInfo;

		updateDraggingInfo(scr, &dragInfo, &ev, icon);

		XMoveWindow(dpy, icon, dragInfo.imageLocation.x,
			    dragInfo.imageLocation.y);

		if (state != 2) {
		    processMotion(scr, &dragInfo, &oldDragInfo, &rect, action);
		}
		protectBlock(False);

		/* XXXif entered a different destination, check the operation */

		switch (state) {
		 case 1:
		    if (oldDragInfo.destinationWindow != None
			 && (dragInfo.destinationWindow == None
			     || dragInfo.destinationWindow == scr->rootWin)) {
			/* left the droppable window */
			state = 2;
			action = -1;
		    }
		    break;
		    
		 case 2:
		    if (dragInfo.destinationWindow != None) {
			state = 1;
			action = -1;
		    }
		    break;

		 case 3:
		 case 4:
		    if (oldDragInfo.destinationWindow != None
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

		updateDraggingInfo(scr, &dragInfo, &ev, icon);
	    
		XMoveWindow(dpy, icon, dragInfo.imageLocation.x,
			    dragInfo.imageLocation.y);

		processMotion(scr, &dragInfo, &oldDragInfo, &rect,
			      action);

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


	 case SelectionRequest:
	    
	    draggedData = NULL;
	    
	    break;

	 case ClientMessage:
	    if ((state == 1 || state == 3 || state == 4 || state == 5)
		&& ev.xclient.message_type == scr->xdndStatusAtom
		&& ev.xclient.window == dragInfo.destinationWindow) {

		if (ev.xclient.data.l[1] & 1) {
		    puts("got accept msg");
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
		    if (ev.xclient.data.l[4] == None) {
			action = 0;
		    } else {
			action = ev.xclient.data.l[4];/*XXX*/
		    }
		} else {
		    puts("got reject msg");
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
	    printf("state changed to %i\n", state);
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
    /* wait for Finished message and SelectionRequest if not a local drop */
    
    

    XDestroyWindow(dpy, icon);
    if (view->dragSourceProcs->endedDragImage != NULL) {
	view->dragSourceProcs->endedDragImage(view, image, 
					      dragInfo.imageLocation,
					      True);
    }
    return;

cancelled:
    if (draggedData) {
	WMReleaseData(draggedData);
    }
    
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


void
W_HandleDNDClientMessage(WMView *toplevel, XClientMessageEvent *event)
{
#if 0
    WMScreen *scr = W_VIEW_SCREEN(toplevel);
    WMView *oldView = NULL, *newView = NULL;
    unsigned operation = 0;
    int x, y;

    if (event->message_type == scr->xdndEnterAtom) {
	Window foo, bar;
	int bla;
	unsigned ble;
	puts("entered");
	
	if (scr->dragInfo.sourceWindow != None) {
	    puts("received Enter event in bad order");
	}
	
	memset(&scr->dragInfo, 0, sizeof(WMDraggingInfo));
	
	
	if ((event->data.l[0] >> 24) > XDND_VERSION) {
	    wwarning("received drag & drop request with unsupported version %i",
		    (event->data.l[0] >> 24));
	    return;
	}
	
	scr->dragInfo.protocolVersion = event->data.l[1] >> 24;
	scr->dragInfo.sourceWindow = event->data.l[0];
	scr->dragInfo.destinationWindow = event->window;
	
	/* XXX */
	scr->dragInfo.image = NULL;
	
	XQueryPointer(scr->display, scr->rootWin, &foo, &bar,
		      &scr->dragInfo.location.x, &scr->dragInfo.location.y,
		      &bla, &bla, &ble);

	translateCoordinates(scr, scr->dragInfo.destinationWindow,
			     scr->dragInfo.location.x,
			     scr->dragInfo.location.y, &x, &y);


	newView = findViewInToplevel(scr->display,
		 		  scr->dragInfo.destinationWindow, 
				  x, y);
	
	if (IS_DROPPABLE(view)) {
	    
	    scr->dragInfo.destinationView = view;
	    
	    operation =
		view->dragDestinationProcs->draggingEntered(view, 
							    &scr->dragInfo);
	}
	if (operation > 0) {
	    Atom action;
		
	    switch (operation) {
	     default:
		action = 0;
		break;
	    }

	    sendClientMessage(scr->display, scr->dragInfo.sourceWindow,
			      scr->xdndStatusAtom,
			      scr->dragInfo.destinationWindow,
			      1, 0, 0, action);
	}

    } else if (event->message_type == scr->xdndPositionAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {
	
	scr->dragInfo.location.x = event->data.l[2] >> 16;
	scr->dragInfo.location.y = event->data.l[2] & 0xffff;
	
	if (scr->dragInfo.protocolVersion >= 1) {
	    scr->dragInfo.timestamp = event->data.l[3];
	    scr->dragInfo.sourceOperation = event->data.l[4];
	} else {
	    scr->dragInfo.timestamp = CurrentTime;
	    scr->dragInfo.sourceOperation = 0; /*XXX*/
	}

	translateCoordinates(scr, scr->dragInfo.destinationWindow,
			     scr->dragInfo.location.x,
			     scr->dragInfo.location.y, &x, &y);

	view = findViewInToplevel(scr->display, 
				  scr->dragInfo.destinationWindow,
				  x, y);
	
	if (scr->dragInfo.destinationView != view) {
	    WMView *oldVIew = scr->dragInfo.destinationView;
	    
	    oldView->dragDestinationProcs->draggingExited(oldView,
							  &scr->dragInfo);
	    
	    scr->dragInfo.destinationView = NULL;
	}
	

	if (IS_DROPPABLE(view)) {
	    
	    
	    operation =
		view->dragDestinationProcs->draggingUpdated(view,
							    &scr->dragInfo);
	}
	
	if (operation == 0) {
	    sendClientMessage(scr->display, scr->dragInfo.sourceWindow,
			      scr->xdndStatusAtom,
			      scr->dragInfo.destinationWindow,
			      0, 0, 0, None);
	} else {
	    Atom action;
		
	    switch (operation) {
	     default:
		action = 0;
		break;
	    }

	    sendClientMessage(scr->display, scr->dragInfo.sourceWindow,
			      scr->xdndStatusAtom,
			      scr->dragInfo.destinationWindow,
			      1, 0, 0, action);
	}
	
    } else if (event->message_type == scr->xdndLeaveAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {

	void (*draggingExited)(WMView *self, WMDraggingInfo *info);
	
	puts("leave");
    } else if (event->message_type == scr->xdndDropAtom
	       && scr->dragInfo.sourceWindow == event->data.l[0]) {
	
	
	
	puts("drop");
    }
#endif
}

