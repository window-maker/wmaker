

/*
 * This event handling stuff was inspired on Tk.
 */

#include "WINGsP.h"

#include "../src/config.h"

#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif


#include <X11/Xos.h>

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#include <time.h>

#ifndef X_GETTIMEOFDAY
#define X_GETTIMEOFDAY(t) gettimeofday(t, (struct timezone*)0)
#endif






typedef struct TimerHandler {
    WMCallback		*callback;	       /* procedure to call */
    struct timeval	when;		       /* when to call the callback */
    void 		*clientData;
    struct TimerHandler *next;
    int                 nextDelay;             /* 0 if it's one-shot */
} TimerHandler;


typedef struct IdleHandler {
    WMCallback		*callback;
    void 		*clientData;
} IdleHandler;


typedef struct InputHandler {
    WMInputProc		*callback;
    void		*clientData;
    int			fd;
    int			mask;
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

static WMBag *idleHandler=NULL;

static WMBag *inputHandler=NULL;

/* hook for other toolkits or wmaker process their events */
static WMEventHook *extraEventHandler=NULL;



#define timerPending()	(timerHandler)



static void
rightNow(struct timeval *tv) {
    X_GETTIMEOFDAY(tv);
}

/* is t1 after t2 ? */
#define IS_AFTER(t1, t2)	(((t1).tv_sec > (t2).tv_sec) || \
				 (((t1).tv_sec == (t2).tv_sec) \
				  && ((t1).tv_usec > (t2).tv_usec)))

#define IS_ZERO(tv) (tv.tv_sec == 0 && tv.tv_usec == 0)

#define SET_ZERO(tv) tv.tv_sec = 0, tv.tv_usec = 0

static void
addmillisecs(struct timeval *tv, int milliseconds)
{
    tv->tv_usec += milliseconds*1000;

    tv->tv_sec += tv->tv_usec/1000000;
    tv->tv_usec = tv->tv_usec%1000000;
}


static void
enqueueTimerHandler(TimerHandler *handler)
{
    TimerHandler *tmp;
    
    /* insert callback in queue, sorted by time left */
    if (!timerHandler || !IS_AFTER(handler->when, timerHandler->when)) {
	/* first in the queue */
	handler->next = timerHandler;
	timerHandler = handler;
    } else {
	tmp = timerHandler;
	while (tmp->next && IS_AFTER(handler->when, tmp->next->when)) {
	    tmp = tmp->next;
	}
	handler->next = tmp->next;
	tmp->next = handler;
    }    
}


WMHandlerID
WMAddTimerHandler(int milliseconds, WMCallback *callback, void *cdata)
{
    TimerHandler *handler;

    handler = malloc(sizeof(TimerHandler));
    if (!handler)
      return NULL;
    
    rightNow(&handler->when);
    addmillisecs(&handler->when, milliseconds);
    handler->callback = callback;
    handler->clientData = cdata;
    handler->nextDelay = 0;
    
    enqueueTimerHandler(handler);

    return handler;
}


WMHandlerID
WMAddEternalTimerHandler(int milliseconds, WMCallback *callback, void *cdata)
{
    TimerHandler *handler = WMAddTimerHandler(milliseconds, callback, cdata);

    if (handler != NULL)
	handler->nextDelay = milliseconds;
    
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
	tmp->nextDelay = 0;
	if (!IS_ZERO(tmp->when)) {
	    timerHandler = tmp->next;
	    wfree(tmp);
	}
    } else {
        while (tmp->next) {
            if (tmp->next->clientData==cdata) {
                handler = tmp->next;
		handler->nextDelay = 0;		
		if (IS_ZERO(handler->when))
		    break;
                tmp->next = handler->next;
                wfree(handler);
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
    
    handler->nextDelay = 0;
    
    if (IS_ZERO(handler->when))
	return;
    
    if (tmp==handler) {
	timerHandler = handler->next;
	wfree(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next==handler) {
		tmp->next=handler->next;
		wfree(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}



WMHandlerID
WMAddIdleHandler(WMCallback *callback, void *cdata)
{
    IdleHandler *handler;

    handler = malloc(sizeof(IdleHandler));
    if (!handler)
        return NULL;

    handler->callback = callback;
    handler->clientData = cdata;
    /* add handler at end of queue */
    if (!idleHandler) {
        idleHandler = WMCreateBag(16);
    }
    WMPutInBag(idleHandler, handler);

    return handler;
}


void
WMDeleteIdleHandler(WMHandlerID handlerID)
{
    IdleHandler *handler = (IdleHandler*)handlerID;
    int pos;

    if (!handler || !idleHandler)
        return;

    pos = WMGetFirstInBag(idleHandler, handler);
    if (pos != WBNotFound) {
        wfree(handler);
        WMDeleteFromBag(idleHandler, pos);
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

    if (!inputHandler)
        inputHandler = WMCreateBag(16);
    WMPutInBag(inputHandler, handler);

    return handler;
}



void
WMDeleteInputHandler(WMHandlerID handlerID)
{
    InputHandler *handler = (InputHandler*)handlerID;
    int pos;

    if (!handler || !inputHandler)
	return;

    pos = WMGetFirstInBag(inputHandler, handler);
    if (pos != WBNotFound) {
        wfree(handler);
        WMDeleteFromBag(inputHandler, pos);
    }
}



static Bool
checkIdleHandlers()
{
    IdleHandler *handler;
    WMBag *handlerCopy;
    WMBagIterator iter;

    if (!idleHandler || WMGetBagItemCount(idleHandler)==0) {
	W_FlushIdleNotificationQueue();
        /* make sure an observer in queue didn't added an idle handler */
        return (idleHandler!=NULL && WMGetBagItemCount(idleHandler)>0);
    }

    handlerCopy = WMCreateBag(WMGetBagItemCount(idleHandler));
    WMAppendBag(handlerCopy, idleHandler);

    for (handler = WMBagFirst(handlerCopy, &iter);
	 iter != NULL;
	 handler = WMBagNext(handlerCopy, &iter)) {
        /* check if the handler still exist or was removed by a callback */
        if (WMGetFirstInBag(idleHandler, handler) == WBNotFound)
            continue;

	(*handler->callback)(handler->clientData);
        WMDeleteIdleHandler(handler);
    }

    WMFreeBag(handlerCopy);

    W_FlushIdleNotificationQueue();

    /* this is not necesarrily False, because one handler can re-add itself */
    return (WMGetBagItemCount(idleHandler)>0);
}



static void
checkTimerHandlers()
{
    TimerHandler *handler;
    struct timeval now;
    
    if (!timerHandler) {
        W_FlushASAPNotificationQueue();
        return;
    }

    rightNow(&now);

    handler = timerHandler;
    while (handler && IS_AFTER(now, handler->when)) {
	if (!IS_ZERO(handler->when)) {
	    SET_ZERO(handler->when);
	    (*handler->callback)(handler->clientData);
	}
	handler = handler->next;
    }

    while (timerHandler && IS_ZERO(timerHandler->when)) {
	handler = timerHandler;
	timerHandler = timerHandler->next;	
	
	if (handler->nextDelay > 0) {
	    handler->when = now;
	    addmillisecs(&handler->when, handler->nextDelay);
	    enqueueTimerHandler(handler);
	} else {
	    wfree(handler);
	}
    }
    
    W_FlushASAPNotificationQueue();
}



static void
delayUntilNextTimerEvent(struct timeval *delay)
{
    struct timeval now;
    TimerHandler *handler;

    handler = timerHandler;
    while (handler && IS_ZERO(handler->when)) handler = handler->next;
    
    if (!handler) {
        /* The return value of this function is only valid if there _are_
         timers active. */
	delay->tv_sec = 0;
	delay->tv_usec = 0;
	return;
    }
    
    rightNow(&now);
    if (IS_AFTER(now, handler->when)) {
	delay->tv_sec = 0;
	delay->tv_usec = 0;
    } else {
	delay->tv_sec = handler->when.tv_sec - now.tv_sec;
	delay->tv_usec = handler->when.tv_usec - now.tv_usec;
	if (delay->tv_usec < 0) {
	    delay->tv_usec += 1000000;
	    delay->tv_sec--;
	}
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
#if defined(HAVE_POLL) && defined(HAVE_POLL_H) && !defined(HAVE_SELECT)
    struct pollfd *fds;
    InputHandler *handler;
    int count, timeout, nfds, i, retval;

    if (inputHandler)
        nfds = WMGetBagItemCount(inputHandler);
    else
        nfds = 0;

    fds = wmalloc(nfds+1 * sizeof(struct pollfd));
    /* put this to the end of array to avoid using ranges from 1 to nfds+1 */
    fds[nfds].fd = ConnectionNumber(dpy);
    fds[nfds].events = POLLIN;

    for (i = 0; i<nfds; i++) {
        handler = WMGetFromBag(inputHandler, i);
        fds[i].fd = handler->fd;
        fds[i].events = 0;
        if (handler->mask & WIReadMask)
            fds[i].events |= POLLIN;

        if (handler->mask & WIWriteMask)
            fds[i].events |= POLLOUT;

#if 0 /* FIXME */
        if (handler->mask & WIExceptMask)
            FD_SET(handler->fd, &eset);
#endif
    }

    /*
     * Setup the select() timeout to the estimated time until the
     * next timer expires.
     */
    if (timerPending()) {
        struct timeval tv;
	delayUntilNextTimerEvent(&tv);
        timeout = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    } else {
        timeout = -1;
    }

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
    
    count = poll(fds, nfds, timeout);

    if (count>0 && nfds>0) {
        WMBag *handlerCopy = WMCreateBag(nfds);

        for (i=0; i<nfds; i++)
            WMPutInBag(handlerCopy, WMGetFromBag(inputHandler, i));

        for (i=0; i<nfds; i++) {
	    int mask;

            handler = WMGetFromBag(handlerCopy, i);
            /* check if the handler still exist or was removed by a callback */
            if (WMGetFirstInBag(inputHandler, handler) == WBNotFound)
                continue;

	    mask = 0;

            if ((handler->mask & WIReadMask) &&
                (fds[i].revents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI)))
                mask |= WIReadMask;

            if ((handler->mask & WIWriteMask) &&
                (fds[i].revents & (POLLOUT | POLLWRBAND)))
                mask |= WIWriteMask;

            if ((handler->mask & WIExceptMask) &&
                (fds[i].revents & (POLLHUP | POLLNVAL | POLLERR)))
                mask |= WIExceptMask;

	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask, 
				     handler->clientData);
	    }
        }

        WMFreeBag(handlerCopy);
    }

    retval = fds[nfds].revents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI);
    wfree(fds);

    W_FlushASAPNotificationQueue();

    return retval;
#else /* not HAVE_POLL */
#ifdef HAVE_SELECT
    struct timeval timeout;
    struct timeval *timeoutPtr;
    fd_set rset, wset, eset;
    int maxfd, nfds, i;
    int count;
    InputHandler *handler;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    FD_SET(ConnectionNumber(dpy), &rset);
    maxfd = ConnectionNumber(dpy);

    if (inputHandler)
        nfds = WMGetBagItemCount(inputHandler);
    else
        nfds = 0;

    for (i=0; i<nfds; i++) {
        handler = WMGetFromBag(inputHandler, i);
	if (handler->mask & WIReadMask)
	    FD_SET(handler->fd, &rset);

	if (handler->mask & WIWriteMask)
	    FD_SET(handler->fd, &wset);

	if (handler->mask & WIExceptMask)
	    FD_SET(handler->fd, &eset);

	if (maxfd < handler->fd)
	    maxfd = handler->fd;
    }


    /*
     * Setup the select() timeout to the estimated time until the
     * next timer expires.
     */
    if (timerPending()) {
	delayUntilNextTimerEvent(&timeout);
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

    count = select(1 + maxfd, &rset, &wset, &eset, timeoutPtr);

    if (count>0 && nfds>0) {
        WMBag *handlerCopy = WMCreateBag(nfds);

        for (i=0; i<nfds; i++)
            WMPutInBag(handlerCopy, WMGetFromBag(inputHandler, i));

        for (i=0; i<nfds; i++) {
	    int mask;

            handler = WMGetFromBag(handlerCopy, i);
            /* check if the handler still exist or was removed by a callback */
            if (WMGetFirstInBag(inputHandler, handler) == WBNotFound)
                continue;

	    mask = 0;

	    if ((handler->mask & WIReadMask) && FD_ISSET(handler->fd, &rset))
		mask |= WIReadMask;

	    if ((handler->mask & WIWriteMask) && FD_ISSET(handler->fd, &wset))
		mask |= WIWriteMask;

	    if ((handler->mask & WIExceptMask) && FD_ISSET(handler->fd, &eset))
		mask |= WIExceptMask;

	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask, 
				     handler->clientData);
	    }
        }

        WMFreeBag(handlerCopy);
    }

    W_FlushASAPNotificationQueue();

    return FD_ISSET(ConnectionNumber(dpy), &rset);
#else /* not HAVE_SELECT, not HAVE_POLL */
Neither select nor poll. You lose.
#endif /* HAVE_SELECT */
#endif /* HAVE_POLL */
}


void
WMNextEvent(Display *dpy, XEvent *event)
{ 
    /* Check any expired timers */
    checkTimerHandlers();

    while (XPending(dpy) == 0) {
	/* Do idle stuff */
	/* Do idle and timer stuff while there are no timer or X events */
	while (XPending(dpy) == 0 && checkIdleHandlers()) {
	    /* dispatch timer events */
            checkTimerHandlers();
	}

	/*
	 * Make sure that new events did not arrive while we were doing
	 * timer/idle stuff. Or we might block forever waiting for
	 * an event that already arrived. 
	 */
	/* wait for something to happen or a timer to expire */
	W_WaitForEvent(dpy, 0);
	
        /* Check any expired timers */
        checkTimerHandlers();
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
	while (checkIdleHandlers()) {
	    if (XCheckMaskEvent(dpy, mask, event))
		return;
	}
		   
        /* 
         * Setup the select() timeout to the estimated time until the
         * next timer expires.
         */
        if (timerPending()) {
	    delayUntilNextTimerEvent(&timeout);
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
        checkTimerHandlers();
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
	while (checkIdleHandlers()) {
	    if (XCheckMaskEvent(dpy, mask, event))
		return;
	}

        /* Wait for input on the X connection socket */
	W_WaitForEvent(dpy, mask);

        /* Check any expired timers */
        checkTimerHandlers();
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


