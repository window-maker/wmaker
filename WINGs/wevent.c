

/*
 * This event handling stuff was inspired on Tk.
 */

#include "WINGsP.h"


/* table to map event types to event masks */
static unsigned long eventMasks[] = {
    0,
    0,
    KeyPressMask,                       /* KeyPress */
    KeyReleaseMask,                     /* KeyRelease */
    ButtonPressMask,                    /* ButtonPress */
    ButtonReleaseMask,                  /* ButtonRelease */
    PointerMotionMask|PointerMotionHintMask|ButtonMotionMask
            |Button1MotionMask|Button2MotionMask|Button3MotionMask
            |Button4MotionMask|Button5MotionMask,
                                        /* MotionNotify */
    EnterWindowMask,                    /* EnterNotify */
    LeaveWindowMask,                    /* LeaveNotify */
    FocusChangeMask,                    /* FocusIn */
    FocusChangeMask,                    /* FocusOut */
    KeymapStateMask,                    /* KeymapNotify */
    ExposureMask,                       /* Expose */
    ExposureMask,                       /* GraphicsExpose */
    ExposureMask,                       /* NoExpose */
    VisibilityChangeMask,               /* VisibilityNotify */
    SubstructureNotifyMask,             /* CreateNotify */
    StructureNotifyMask,                /* DestroyNotify */
    StructureNotifyMask,                /* UnmapNotify */
    StructureNotifyMask,                /* MapNotify */
    SubstructureRedirectMask,           /* MapRequest */
    StructureNotifyMask,                /* ReparentNotify */
    StructureNotifyMask,                /* ConfigureNotify */
    SubstructureRedirectMask,           /* ConfigureRequest */
    StructureNotifyMask,                /* GravityNotify */
    ResizeRedirectMask,                 /* ResizeRequest */
    StructureNotifyMask,                /* CirculateNotify */
    SubstructureRedirectMask,           /* CirculateRequest */
    PropertyChangeMask,                 /* PropertyNotify */
    0,                                  /* SelectionClear */
    0,                                  /* SelectionRequest */
    0,                                  /* SelectionNotify */
    ColormapChangeMask,                 /* ColormapNotify */
    ClientMessageMask,		        /* ClientMessage */
    0,			                /* Mapping Notify */
};



/* hook for other toolkits or wmaker process their events */
static WMEventHook *extraEventHandler=NULL;





/*
 * WMCreateEventHandler--
 * 	Create an event handler and put it in the event handler list for the
 * view. If the same callback and clientdata are already used in another
 * handler, the masks are swapped.
 * 
 */
void
WMCreateEventHandler(WMView *view, unsigned long mask, WMEventProc *eventProc,
		     void *clientData)
{
    W_EventHandler *handler, *ptr;
    unsigned long eventMask;
    WMBagIterator iter;

    
    handler = NULL;
    eventMask = mask;

    WM_ITERATE_BAG(view->eventHandlers, ptr, iter) {
	if (ptr->clientData == clientData && ptr->proc == eventProc) {
	    handler = ptr;
	    eventMask |= ptr->eventMask;
	}
    }
    if (!handler) {
	handler = wmalloc(sizeof(W_EventHandler));

	WMPutInBag(view->eventHandlers, handler);
    }
    /* select events for window */
    handler->eventMask = eventMask;
    handler->proc = eventProc;
    handler->clientData = clientData;
}


/*
 * WMDeleteEventHandler--
 * 	Delete event handler matching arguments from windows
 * event handler list.
 * 
 */
void
WMDeleteEventHandler(WMView *view, unsigned long mask, WMEventProc *eventProc, 
		     void *clientData)
{
    W_EventHandler *handler, *ptr;
    WMBagIterator iter;
        
    handler = NULL;
    
    WM_ITERATE_BAG(view->eventHandlers, ptr, iter) {
	if (ptr->eventMask == mask && ptr->proc == eventProc 
	    && ptr->clientData == clientData) {
	    handler = ptr;
	    break;
	}
    }
    
    if (!handler)
	return;
    
    WMRemoveFromBag(view->eventHandlers, handler);

    wfree(handler);
}



void
W_CleanUpEvents(WMView *view)
{
    W_EventHandler *ptr;
    WMBagIterator iter;
    
    WM_ITERATE_BAG(view->eventHandlers, ptr, iter) {
	wfree(ptr);
    }
}



static Time
getEventTime(WMScreen *screen, XEvent *event)
{
    switch (event->type) {
     case ButtonPress:
     case ButtonRelease:
	return event->xbutton.time;
     case KeyPress:
     case KeyRelease:
	return event->xkey.time;
     case MotionNotify:
	return event->xmotion.time;
     case EnterNotify:
     case LeaveNotify:
	return event->xcrossing.time;
     case PropertyNotify:
	return event->xproperty.time;
     case SelectionClear:
	return event->xselectionclear.time;
     case SelectionRequest:
	return event->xselectionrequest.time;
     case SelectionNotify:
	return event->xselection.time;
     default:
	return screen->lastEventTime;	
    }
}


void
W_CallDestroyHandlers(W_View *view)
{
    XEvent event;
    WMBagIterator iter;
    W_EventHandler *hPtr;
    
    event.type = DestroyNotify;
    event.xdestroywindow.window = view->window;
    event.xdestroywindow.event = view->window;

    WM_ITERATE_BAG(view->eventHandlers, hPtr, iter) {
	if (hPtr->eventMask & StructureNotifyMask) {
	    (*hPtr->proc)(&event, hPtr->clientData);
	}
    }
}



void
WMSetViewNextResponder(WMView *view, WMView *responder)
{
    /* set the widget to receive keyboard events that aren't handled
     * by this widget */
    
    view->nextResponder = responder;
}


void
WMRelayToNextResponder(WMView *view, XEvent *event)
{
    unsigned long mask = eventMasks[event->xany.type];

    if (view->nextResponder) {
	WMView *next = view->nextResponder;
	W_EventHandler *hPtr;
	WMBagIterator iter;

	WM_ITERATE_BAG(next->eventHandlers, hPtr, iter) {
	    if ((hPtr->eventMask & mask)) {
		(*hPtr->proc)(event, hPtr->clientData);
	    }
	}
    }
}


int
WMHandleEvent(XEvent *event)
{
    W_EventHandler *hPtr;
    W_View *view, *vPtr, *toplevel;
    unsigned long mask;
    Window window;
    WMBagIterator iter;

    if (event->type == MappingNotify) {
	XRefreshKeyboardMapping(&event->xmapping);
	return True;
    }

    mask = eventMasks[event->xany.type];
    
    window = event->xany.window;

    /* diferentiate SubstructureNotify with StructureNotify */
    if (mask == StructureNotifyMask) {
	if (event->xmap.event != event->xmap.window) {
	    mask = SubstructureNotifyMask;
	    window = event->xmap.event;
	}
    }
    view = W_GetViewForXWindow(event->xany.display, window);

    if (!view) {
	if (extraEventHandler)
	    (extraEventHandler)(event);

	return False;
    }

    view->screen->lastEventTime = getEventTime(view->screen, event);

    toplevel = W_TopLevelOfView(view);

    if (event->type == SelectionNotify || event->type == SelectionClear
	|| event->type == SelectionRequest) {
	/* handle selection related events */
	W_HandleSelectionEvent(event);
	
    } else if (event->type == ClientMessage) {
	
	W_HandleDNDClientMessage(toplevel, &event->xclient);
    }

    /* if it's a key event, redispatch it to the focused control */
    if (mask & (KeyPressMask|KeyReleaseMask)) {
	W_View *focused = W_FocusedViewOfToplevel(toplevel);
	
	if (focused) {
	    view = focused;
	}
    }

    /* compress Motion events */
    if (event->type == MotionNotify && !view->flags.dontCompressMotion) {
        while (XPending(event->xmotion.display)) {
            XEvent ev;
            XPeekEvent(event->xmotion.display, &ev);
            if (ev.type == MotionNotify 
		&& event->xmotion.window == ev.xmotion.window 
		&& event->xmotion.subwindow == ev.xmotion.subwindow) {
		/* replace events */
                XNextEvent(event->xmotion.display, event);
            } else break;
        }
    }
    
    /* compress expose events */
    if (event->type == Expose && !view->flags.dontCompressExpose) {
        while (XCheckTypedWindowEvent(event->xexpose.display, view->window,
				      Expose, event));
    }


    if (view->screen->modalLoop && toplevel!=view->screen->modalView
	&& !toplevel->flags.worksWhenModal) {
	if (event->type == KeyPress || event->type == KeyRelease
	    || event->type == MotionNotify || event->type == ButtonPress
	    || event->type == ButtonRelease
	    || event->type == FocusIn || event->type == FocusOut) {
	    return True;
	}
    }

    /* do balloon stuffs */
    if (event->type == EnterNotify)
	W_BalloonHandleEnterView(view);
    else if (event->type == LeaveNotify)
	W_BalloonHandleLeaveView(view);

    /* This is a hack. It will make the panel be secure while
     * the event handlers are handled, as some event handler
     * might destroy the widget. */
    W_RetainView(toplevel);

    WM_ITERATE_BAG(view->eventHandlers, hPtr, iter) {
	if ((hPtr->eventMask & mask)) {
	    (*hPtr->proc)(event, hPtr->clientData);
	}
    }
#if 0
    /* pass the event to the top level window of the widget */
    /* TODO: change this to a responder chain */
    if (view->parent != NULL) {
	vPtr = view;
	while (vPtr->parent != NULL)
	    vPtr = vPtr->parent;

	WM_ITERATE_BAG(vPtr->eventHandlers, hPtr, iter) {
	    if (hPtr->eventMask & mask) {
		(*hPtr->proc)(event, hPtr->clientData);
	    }
	}
    }
#endif
    /* save button click info to track double-clicks */
    if (view->screen->ignoreNextDoubleClick) {
	view->screen->ignoreNextDoubleClick = 0;
    } else {
	if (event->type == ButtonPress) {
	    view->screen->lastClickWindow = event->xbutton.window;
	    view->screen->lastClickTime = event->xbutton.time;
	}
    }

    W_ReleaseView(toplevel);

    return True;
}


int
WMIsDoubleClick(XEvent *event)
{
    W_View *view;
    
    if (event->type != ButtonPress)
	return False;
    
    view = W_GetViewForXWindow(event->xany.display, event->xbutton.window);

    if (!view)
	return False;
    
    if (view->screen->lastClickWindow != event->xbutton.window)
	return False;
    
    if (event->xbutton.time - view->screen->lastClickTime 
	< WINGsConfiguration.doubleClickDelay) {
	view->screen->lastClickTime = 0;
	view->screen->lastClickWindow = None;
	view->screen->ignoreNextDoubleClick = 1;
	return True;
    } else
	return False;
}


Bool
W_WaitForEvent(Display *dpy, unsigned long xeventmask)
{
    XSync(dpy, False);
    if (xeventmask==0) {
	if (XPending(dpy))
	    return True;
    } else {
	XEvent ev;
	if (XCheckMaskEvent(dpy, xeventmask, &ev)) {
	    XPutBackEvent(dpy, &ev);
	    return True;
	}
    }

    return W_HandleInputEvents(True, ConnectionNumber(dpy));
}


void
WMNextEvent(Display *dpy, XEvent *event)
{ 
    /* Check any expired timers */
    W_CheckTimerHandlers();

    while (XPending(dpy) == 0) {
	/* Do idle stuff */
	/* Do idle and timer stuff while there are no timer or X events */
	while (XPending(dpy) == 0 && W_CheckIdleHandlers()) {
	    /* dispatch timer events */
            W_CheckTimerHandlers();
	}

	/*
	 * Make sure that new events did not arrive while we were doing
	 * timer/idle stuff. Or we might block forever waiting for
	 * an event that already arrived. 
	 */
	/* wait for something to happen or a timer to expire */
	W_WaitForEvent(dpy, 0);
	
        /* Check any expired timers */
        W_CheckTimerHandlers();
    }

    XNextEvent(dpy, event);
}

/*
 * Cant use this because XPending() will make W_WaitForEvent
 * return even if the event in the queue is not what we want,
 * and if we block until some new event arrives from the
 * server, other events already in the queue (like Expose)
 * will be deferred.
 */
void
WMMaskEvent(Display *dpy, long mask, XEvent *event)
{ 
    while (!XCheckMaskEvent(dpy, mask, event)) {
	/* Do idle stuff while there are no timer or X events */
	while (W_CheckIdleHandlers()) {
	    if (XCheckMaskEvent(dpy, mask, event))
		return;
	}

        /* Wait for input on the X connection socket */
	W_WaitForEvent(dpy, mask);

        /* Check any expired timers */
        W_CheckTimerHandlers();
    }
}

Bool 
WMScreenPending(WMScreen *scr)
{
    if (XPending(scr->display))
	return True;
    else
	return False;
}


WMEventHook*
WMHookEventHandler(WMEventHook *handler)
{
    WMEventHook *oldHandler = extraEventHandler;
    
    extraEventHandler = handler;
    
    return oldHandler;
}


