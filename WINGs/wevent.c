

/*
 * This event handling stuff was based on Tk.
 */

#include "WINGsP.h"

#include "../src/config.h"

#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#ifdef HAVE_GETTIMEOFDAY
# include <sys/time.h>
# ifdef TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else /* ! HAVE_GETTIMEOFDAY */
# include <time.h>
#endif /* ! HAVE_GETTIMEOFDAY */


extern _WINGsConfiguration WINGsConfiguration;



typedef struct TimerHandler {
    WMCallback		*callback;	       /* procedure to call */
    unsigned long 	msec;		       /* when to call the callback */
    void 		*clientData;
    struct TimerHandler *next;
} TimerHandler;


typedef struct IdleHandler {
    WMCallback		*callback;
    void 		*clientData;
    struct IdleHandler	*next;
} IdleHandler;


typedef struct InputHandler {
    WMInputProc		*callback;
    void		*clientData;
    int			fd;
    int			mask;
    struct InputHandler *next;
} InputHandler;


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



/* queue of timer event handlers */
static TimerHandler *timerHandler=NULL;

static IdleHandler *idleHandler=NULL;

static InputHandler *inputHandler=NULL;

/* hook for other toolkits or wmaker process their events */
static WMEventHook *extraEventHandler=NULL;



#define timerPending()	(timerHandler)

#define idlePending()	(idleHandler)



/* return current time in milliseconds */
#ifdef HAVE_GETTIMEOFDAY
static unsigned long 
rightNow(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return 1000L*(unsigned long)tv.tv_sec + (unsigned long)tv.tv_usec/1000L;
}
#else /* !HAVE_GETTIMEOFDAY */
# define rightNow()	(1000*(unsigned long)time(NULL))
#endif /* !HAVE_GETTIMEOFDAY */


WMHandlerID
WMAddTimerHandler(int milliseconds, WMCallback *callback, void *cdata)
{
    TimerHandler *handler, *tmp;
    
    handler = malloc(sizeof(TimerHandler));
    if (!handler)
      return NULL;
    
    handler->msec = rightNow()+milliseconds;
    handler->callback = callback;
    handler->clientData = cdata;
    /* insert callback in queue, sorted by time left */
    if (!timerHandler || timerHandler->msec >= handler->msec) {
	/* first in the queue */
	handler->next = timerHandler;
	timerHandler = handler;
    } else {
	tmp = timerHandler;
	while (tmp->next && tmp->next->msec < handler->msec) {
	    tmp = tmp->next;
	}
	handler->next = tmp->next;
	tmp->next = handler;
    }
    return handler;
}



void
WMDeleteTimerWithClientData(void *cdata)
{
    TimerHandler *handler, *tmp;

    if (!cdata || !timerHandler)
        return;

    tmp = timerHandler;
    if (tmp->clientData==cdata) {
        timerHandler = tmp->next;
        free(tmp);
    } else {
        while (tmp->next) {
            if (tmp->next->clientData==cdata) {
                handler = tmp->next;
                tmp->next = handler->next;
                free(handler);
                break;
            }
            tmp = tmp->next;
        }
    }
}



void
WMDeleteTimerHandler(WMHandlerID handlerID)
{
    TimerHandler *tmp, *handler=(TimerHandler*)handlerID;

    if (!handler || !timerHandler) 
      return;

    tmp = timerHandler;
    if (tmp==handler) {
	timerHandler = handler->next;
	free(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next==handler) {
		tmp->next=handler->next;
		free(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}



WMHandlerID
WMAddIdleHandler(WMCallback *callback, void *cdata)
{
    IdleHandler *handler, *tmp;
    
    handler = malloc(sizeof(IdleHandler));
    if (!handler)
	return NULL;

    handler->callback = callback;
    handler->clientData = cdata;
    handler->next = NULL;
    /* add callback at end of queue */
    if (!idleHandler) {
	idleHandler = handler;
    } else {
	tmp = idleHandler;
	while (tmp->next) {
	    tmp = tmp->next;
	}
	tmp->next = handler;
    }

    return handler;
}



void
WMDeleteIdleHandler(WMHandlerID handlerID)
{
    IdleHandler *tmp, *handler = (IdleHandler*)handlerID;

    if (!handler || !idleHandler)
	return;

    tmp = idleHandler;
    if (tmp == handler) {
	idleHandler = handler->next;
	free(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next == handler) {
		tmp->next = handler->next;
		free(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}



WMHandlerID
WMAddInputHandler(int fd, int condition, WMInputProc *proc, void *clientData)
{
    InputHandler *handler;
    
    handler = wmalloc(sizeof(InputHandler));
    
    handler->fd = fd;
    handler->mask = condition;
    handler->callback = proc;
    handler->clientData = clientData;
    
    handler->next = inputHandler;
    
    inputHandler = handler;

    return handler;
}


void
WMDeleteInputHandler(WMHandlerID handlerID)
{
    InputHandler *tmp, *handler = (InputHandler*)handlerID;

    if (!handler || !inputHandler)
	return;

    tmp = inputHandler;
    if (tmp == handler) {
	inputHandler = handler->next;
	free(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next == handler) {
		tmp->next = handler->next;
		free(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }    
}


static void
checkIdleHandlers()
{
    IdleHandler *handler, *tmp;

    if (!idleHandler)
      return;

    handler = idleHandler;
    
    /* we will process all idleHandlers so, empty the handler list */
    idleHandler = NULL;

    while (handler) {
	tmp = handler->next;
	(*handler->callback)(handler->clientData);
	/* remove the handler */
	free(handler);
	
	handler = tmp;
    }
}



static void
checkTimerHandlers()
{
    TimerHandler *handler;
    unsigned long now = rightNow();

    if (!timerHandler || (timerHandler->msec > now))
      return;

    while (timerHandler && timerHandler->msec <= now) {
	handler = timerHandler;
	timerHandler = timerHandler->next;
	handler->next = NULL;
	(*handler->callback)(handler->clientData);
	free(handler);
    }
}



static unsigned long
msToNextTimerEvent()
{
    unsigned long now;

    if (!timerHandler) {
        /* The return value of this function is only valid if there _are_
         timers active. */
        return 0;
    }

    now = rightNow();
    if (timerHandler->msec < now) {
        return 0;
    } else {
        return timerHandler->msec - now;
    }
}


		     

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
    W_EventHandler *handler;
    W_EventHandler *ptr = view->handlerList;
    unsigned long eventMask;
        
    if (ptr==NULL) {
	handler = wmalloc(sizeof(W_EventHandler));

	handler->nextHandler = NULL;
	
	view->handlerList = handler;
	
	eventMask = mask;
    } else {
	handler = NULL;
	eventMask = mask;
	while (ptr != NULL) {
	    if (ptr->clientData == clientData && ptr->proc == eventProc) {
		handler = ptr;
	    }	    
	    eventMask |= ptr->eventMask;
	    
	    ptr = ptr->nextHandler;
	}
	if (!handler) {
	    handler = wmalloc(sizeof(W_EventHandler));
	    handler->nextHandler = view->handlerList;
	    view->handlerList = handler;
	}	
    }

    /* select events for window */
    handler->eventMask = mask;
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
    W_EventHandler *handler, *ptr, *pptr;
    
    ptr = view->handlerList;
    
    handler = NULL;
    pptr = NULL;
    
    while (ptr!=NULL) {
	if (ptr->eventMask == mask && ptr->proc == eventProc 
	    && ptr->clientData == clientData) {
	    handler = ptr;
	    break;
	}
	pptr = ptr;
	ptr = ptr->nextHandler;
    }
    
    if (!handler)
	return;
    
    if (!pptr) {
	view->handlerList = handler->nextHandler;
    } else {
	pptr->nextHandler = handler->nextHandler;
    }
    free(handler);
}



void
W_CleanUpEvents(WMView *view)
{
    W_EventHandler *ptr, *nptr;
    
    ptr = view->handlerList;
        
    while (ptr!=NULL) {
	nptr = ptr->nextHandler;
	free(ptr);
	ptr = nptr;
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
    W_EventHandler *hPtr;
    
    event.type = DestroyNotify;
    event.xdestroywindow.window = view->window;
    event.xdestroywindow.event = view->window;
    hPtr = view->handlerList;
    while (hPtr!=NULL) {
	if (hPtr->eventMask & StructureNotifyMask) {
	    (*hPtr->proc)(&event, hPtr->clientData);
	}
	
	hPtr = hPtr->nextHandler;
    }
}


int
WMHandleEvent(XEvent *event)
{
    W_EventHandler *hPtr;
    W_View *view, *vPtr, *toplevel;
    unsigned long mask;
    Window window;

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
    
    
    if (view->screen->modal && toplevel!=view->screen->modalView
	&& !toplevel->flags.worksWhenModal) {
	if (event->type == KeyPress || event->type == KeyRelease
	    || event->type == MotionNotify || event->type == ButtonPress
	    || event->type == ButtonRelease
	    || event->type == FocusIn || event->type == FocusOut) {
	    return True;
	}
    }

    /* This is a hack. It will make the panel be secure while
     * the event handlers are handled, as some event handler
     * might destroy the widget. */
    W_RetainView(toplevel);

    hPtr = view->handlerList;
			     
    while (hPtr!=NULL) {
	W_EventHandler *tmp;

	tmp = hPtr->nextHandler;
	
	if ((hPtr->eventMask & mask)) {
	    (*hPtr->proc)(event, hPtr->clientData);
	}
	
	hPtr = tmp;
    }
    
    /* pass the event to the top level window of the widget */
    if (view->parent!=NULL) {
	vPtr = view;
	while (vPtr->parent!=NULL)
	    vPtr = vPtr->parent;
	
	hPtr = vPtr->handlerList;

	while (hPtr!=NULL) {
	
	    if (hPtr->eventMask & mask) {
		(*hPtr->proc)(event, hPtr->clientData);
	    }
	    hPtr = hPtr->nextHandler;
	}
    }

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
#ifndef HAVE_SELECT
#error This_system_does_not_have_select(2)_and_is_not_supported
#endif
    unsigned long milliseconds;
    struct timeval timeout;
    struct timeval *timeoutPtr;
    fd_set rset, wset, eset;
    int maxfd;
    int count;
    InputHandler *handler = inputHandler;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    FD_SET(ConnectionNumber(dpy), &rset);
    maxfd = ConnectionNumber(dpy);

    while (handler) {
	if (handler->mask & WIReadMask)
	    FD_SET(handler->fd, &rset);

	if (handler->mask & WIWriteMask)
	    FD_SET(handler->fd, &wset);

	if (handler->mask & WIExceptMask)
	    FD_SET(handler->fd, &eset);

	if (maxfd < handler->fd)
	    maxfd = handler->fd;

	handler = handler->next;
    }


    /*
     * Setup the select() timeout to the estimated time until the
     * next timer expires.
     */
    if (timerPending()) {
	milliseconds = msToNextTimerEvent();
	timeout.tv_sec = milliseconds / 1000;
	timeout.tv_usec = (milliseconds % 1000) * 1000;
	timeoutPtr = &timeout;
    } else {
	timeoutPtr = (struct timeval*)0;
    }

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
    /* TODO: port to poll() */
    count = select(1 + maxfd, &rset, &wset, &eset, timeoutPtr);

    if (count > 0) {
	handler = inputHandler;

	while (handler) {
	    int mask;

	    mask = 0;

	    if (FD_ISSET(handler->fd, &rset))
		mask |= WIReadMask;
	    
	    if (FD_ISSET(handler->fd, &wset))
		mask |= WIWriteMask;
	    
	    if (FD_ISSET(handler->fd, &eset))
		mask |= WIExceptMask;
	    
	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask, 
				     handler->clientData);
	    }

	    handler = handler->next;
	}
    }
    
    return FD_ISSET(ConnectionNumber(dpy), &rset);
}



void
WMNextEvent(Display *dpy, XEvent *event)
{ 
    /* Check any expired timers */
    if (timerPending()) {
	checkTimerHandlers();
    }

    while (XPending(dpy) == 0) {
	/* Do idle stuff */
	/* Do idle and timer stuff while there are no timer or X events */
	while (!XPending(dpy) && idlePending()) {
	    if (idlePending())
		checkIdleHandlers();
	    /* dispatch timer events */
	    if (timerPending())
		checkTimerHandlers();
	}

	/*
	 * Make sure that new events did not arrive while we were doing
	 * timer/idle stuff. Or we might block forever waiting for
	 * an event that already arrived. 
	 */
	/* wait to something happen */
	W_WaitForEvent(dpy, 0);
	
        /* Check any expired timers */
        if (timerPending()) {
            checkTimerHandlers();
        }
    }

    XNextEvent(dpy, event);
}

#if 0
void
WMMaskEvent(Display *dpy, long mask, XEvent *event)
{ 
    unsigned long milliseconds;
    struct timeval timeout;
    struct timeval *timeoutOrInfty;
    fd_set readset;

    while (!XCheckMaskEvent(dpy, mask, event)) {
	/* Do idle stuff while there are no timer or X events */
	while (idlePending()) {
	    checkIdleHandlers();
	    if (XCheckMaskEvent(dpy, mask, event))
		return;
	}
		   
        /* 
         * Setup the select() timeout to the estimated time until the
         * next timer expires.
         */
        if (timerPending()) {
            milliseconds = msToNextTimerEvent();
            timeout.tv_sec = milliseconds / 1000;
            timeout.tv_usec = (milliseconds % 1000) * 1000;
            timeoutOrInfty = &timeout;
        } else {
            timeoutOrInfty = (struct timeval*)0;
        }

	if (XCheckMaskEvent(dpy, mask, event))
	    return;

        /* Wait for input on the X connection socket */
	FD_ZERO(&readset);
	FD_SET(ConnectionNumber(dpy), &readset);
	select(1 + ConnectionNumber(dpy), &readset, (fd_set*)0, (fd_set*)0,
	       timeoutOrInfty);

        /* Check any expired timers */
        if (timerPending()) {
            checkTimerHandlers();
        }
    }
}
#endif
#if 1
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
	while (idlePending()) {
	    checkIdleHandlers();
	    if (XCheckMaskEvent(dpy, mask, event))
		return;
	}

        /* Wait for input on the X connection socket */
	W_WaitForEvent(dpy, mask);

        /* Check any expired timers */
        if (timerPending()) {
            checkTimerHandlers();
        }
    }
}
#endif

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


