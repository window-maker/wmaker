

/*
 * This event handling stuff was based on Tk.
 */

#include "WINGsP.h"

#include "../src/config.h"

#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif


//#include <X11/Xos.h>

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


/* queue of timer event handlers */
static TimerHandler *timerHandler=NULL;

static IdleHandler *idleHandler=NULL;

static InputHandler *inputHandler=NULL;



#define timerPending()	(timerHandler)

#define idlePending()	(idleHandler)


static void
rightNow(struct timeval *tv) {
    X_GETTIMEOFDAY(tv);
}

/* is t1 after t2 ? */
#define IS_AFTER(t1, t2)	(((t1).tv_sec > (t2).tv_sec) || \
				 (((t1).tv_sec == (t2).tv_sec) \
				  && ((t1).tv_usec > (t2).tv_usec)))


static void
addmillisecs(struct timeval *tv, int milliseconds)
{
    tv->tv_usec += milliseconds*1000;

    tv->tv_sec += tv->tv_usec/1000000;
    tv->tv_usec = tv->tv_usec%1000000;
}


WMHandlerID
WMAddTimerHandler(int milliseconds, WMCallback *callback, void *cdata)
{
    TimerHandler *handler, *tmp;
    
    handler = malloc(sizeof(TimerHandler));
    if (!handler)
      return NULL;
    
    rightNow(&handler->when);
    addmillisecs(&handler->when, milliseconds);
    handler->callback = callback;
    handler->clientData = cdata;
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

    if (!idleHandler) {
	W_FlushIdleNotificationQueue();
	return;
    }

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
    W_FlushIdleNotificationQueue();
}



static void
checkTimerHandlers()
{
    TimerHandler *handler;
    struct timeval now;

    rightNow(&now);

    while (timerHandler && IS_AFTER(now, timerHandler->when)) {
	handler = timerHandler;
	timerHandler = timerHandler->next;
	handler->next = NULL;
	(*handler->callback)(handler->clientData);
	free(handler);
    }

    W_FlushASAPNotificationQueue();
}



static void
delayUntilNextTimerEvent(struct timeval *delay)
{
    struct timeval now;

    if (!timerHandler) {
        /* The return value of this function is only valid if there _are_
         timers active. */
	delay->tv_sec = 0;
	delay->tv_usec = 0;
	return;
    }

    rightNow(&now);
    if (IS_AFTER(now, timerHandler->when)) {
	delay->tv_sec = 0;
	delay->tv_usec = 0;
    } else {
	delay->tv_sec = timerHandler->when.tv_sec - now.tv_sec;
	delay->tv_usec = timerHandler->when.tv_usec - now.tv_usec;
	if (delay->tv_usec < 0) {
	    delay->tv_usec += 1000000;
	    delay->tv_sec--;
	}
    }
}


Bool
W_WaitForEvent(Display *dpy, unsigned long xeventmask)
{
#if defined(HAVE_POLL) && defined(HAVE_POLL_H) && !defined(HAVE_SELECT)
    struct pollfd *fds;
    InputHandler *handler;
    int count, timeout, nfds, k, retval;

    for (nfds = 1, handler = inputHandler; 
         handler != 0; handler = handler->next) nfds++;
    
    fds = wmalloc(nfds * sizeof(struct pollfd));
    fds[0].fd = ConnectionNumber(dpy);
    fds[0].events = POLLIN;

    for (k = 1, handler = inputHandler; 
         handler; 
         handler = handler->next, k++) {
        fds[k].fd = handler->fd;
        fds[k].events = 0;
	if (handler->mask & WIReadMask)
            fds[k].events |= POLLIN;

	if (handler->mask & WIWriteMask)
            fds[k].events |= POLLOUT;

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

    if (count > 0) {
	handler = inputHandler;
        k = 1;
	while (handler) {
	    int mask;
            InputHandler *next;

	    mask = 0;

	    if (fds[k].revents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI))
		mask |= WIReadMask;
	    
	    if (fds[k].revents & (POLLOUT | POLLWRBAND))
		mask |= WIWriteMask;
	    
	    if (fds[k].revents & (POLLHUP | POLLNVAL | POLLERR))
		mask |= WIExceptMask;

            next = handler->next;

	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask, 
				     handler->clientData);
	    }

	    handler = next;
            k++;
	}
    }

    retval = fds[0].revents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI);
    free(fds);

    W_FlushASAPNotificationQueue();

    return retval;
#else /* not HAVE_POLL */
#ifdef HAVE_SELECT
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

    if (count > 0) {
	handler = inputHandler;

	while (handler) {
	    int mask;
            InputHandler *next;

	    mask = 0;

	    if (FD_ISSET(handler->fd, &rset))
		mask |= WIReadMask;
	    
	    if (FD_ISSET(handler->fd, &wset))
		mask |= WIWriteMask;
	    
	    if (FD_ISSET(handler->fd, &eset))
		mask |= WIExceptMask;

            next = handler->next;

	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask, 
				     handler->clientData);
	    }

	    handler = next;
	}
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


