

/*
 * This event handling stuff was based on Tk.
 * adapted from wevent.c
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
        wfree(tmp);
    } else {
        while (tmp->next) {
            if (tmp->next->clientData==cdata) {
                handler = tmp->next;
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
	wfree(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next == handler) {
		tmp->next = handler->next;
		wfree(handler);
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
	wfree(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next == handler) {
		tmp->next = handler->next;
		wfree(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}


static Bool
checkIdleHandlers()
{
    IdleHandler *handler, *tmp;

    if (!idleHandler) {
	W_FlushIdleNotificationQueue();
        /* make sure an observer in queue didn't added an idle handler */
        return (idleHandler!=NULL);
    }

    handler = idleHandler;

    /* we will process all idleHandlers so, empty the handler list */
    idleHandler = NULL;

    while (handler) {
	tmp = handler->next;
	(*handler->callback)(handler->clientData);
	/* remove the handler */
	wfree(handler);

	handler = tmp;
    }

    W_FlushIdleNotificationQueue();

    /* this is not necesarrily False, because one handler can re-add itself */
    return (idleHandler!=NULL);
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

    while (timerHandler && IS_AFTER(now, timerHandler->when)) {
	handler = timerHandler;
	timerHandler = timerHandler->next;
	handler->next = NULL;
	(*handler->callback)(handler->clientData);
	wfree(handler);
    }

    W_FlushASAPNotificationQueue();
}



static void
delayUntilNextTimerEvent(struct timeval *delay)
{
    struct timeval now;

    if (!timerHandler) {
        /* The return value of this function is only valid if there _are_
         * active timers. */
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


/*
 * This functions will handle input events on all registered file descriptors.
 * Input:
 *    - waitForInput - True if we want the function to wait until an event
 *                     appears on a file descriptor we watch, False if we
 *                     want the function to immediately return if there is
 *                     no data available on the file descriptors we watch.
 * Output:
 *    if waitForInput is False, the function will return False if there are no
 *                     input handlers registered, or if there is no data
 *                     available on the registered ones, and will return True
 *                     if there is at least one input handler that has data
 *                     available.
 *    if waitForInput is True, the function will return False if there are no
 *                     input handlers registered, else it will block until an
 *                     event appears on one of the file descriptors it watches
 *                     and then it will return True.
 *
 * If the retured value is True, the input handlers for the corresponding file
 * descriptors are also called.
 *
 */
static Bool
handleInputEvents(Bool waitForInput)
{
#if defined(HAVE_POLL) && defined(HAVE_POLL_H) && !defined(HAVE_SELECT)
    struct pollfd *fds;
    InputHandler *handler;
    int count, timeout, nfds, k;

    if (!inputHandler) {
        W_FlushASAPNotificationQueue();
        return False;
    }

    for (nfds = 0, handler = inputHandler;
         handler != 0; handler = handler->next) nfds++;

    fds = wmalloc(nfds * sizeof(struct pollfd));

    for (k = 0, handler = inputHandler;
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
     * If we don't wait for input, set timeout to return immediately,
     * else setup the timeout to the estimated time until the
     * next timer expires or if no timer is pending to infinite.
     */
    if (!waitForInput) {
        timeout = 0;
    } else if (timerPending()) {
        struct timeval tv;
	delayUntilNextTimerEvent(&tv);
        timeout = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    } else {
        timeout = -1;
    }

    count = poll(fds, nfds, timeout);

    if (count > 0) {
	handler = inputHandler;
        k = 0;
	while (handler) {
	    int mask;
            InputHandler *next;

	    mask = 0;

            if ((handler->mask & WIReadMask) &&
                (fds[k].revents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI)))
                mask |= WIReadMask;

            if ((handler->mask & WIWriteMask) &&
                (fds[k].revents & (POLLOUT | POLLWRBAND)))
                mask |= WIWriteMask;

            if ((handler->mask & WIExceptMask) &&
                (fds[k].revents & (POLLHUP | POLLNVAL | POLLERR)))
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

    wfree(fds);

    W_FlushASAPNotificationQueue();

    return (count > 0);
#else /* not HAVE_POLL */
#ifdef HAVE_SELECT
    struct timeval timeout;
    struct timeval *timeoutPtr;
    fd_set rset, wset, eset;
    int maxfd;
    int count;
    InputHandler *handler = inputHandler;

    if (!inputHandler) {
        W_FlushASAPNotificationQueue();
        return False;
    }

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    maxfd = 0;

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
     * If we don't wait for input, set timeout to return immediately,
     * else setup the timeout to the estimated time until the
     * next timer expires or if no timer is pending to infinite.
     */
    if (!waitForInput) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        timeoutPtr = &timeout;
    } else if (timerPending()) {
	delayUntilNextTimerEvent(&timeout);
	timeoutPtr = &timeout;
    } else {
	timeoutPtr = (struct timeval*)0;
    }

    count = select(1 + maxfd, &rset, &wset, &eset, timeoutPtr);

    if (count > 0) {
	handler = inputHandler;

	while (handler) {
	    int mask;
            InputHandler *next;

	    mask = 0;

	    if ((handler->mask & WIReadMask) && FD_ISSET(handler->fd, &rset))
		mask |= WIReadMask;

	    if ((handler->mask & WIWriteMask) && FD_ISSET(handler->fd, &wset))
		mask |= WIWriteMask;

	    if ((handler->mask & WIExceptMask) && FD_ISSET(handler->fd, &eset))
		mask |= WIExceptMask;

            /* save it because the handler may remove itself! */
            next = handler->next;

	    if (mask!=0 && handler->callback) {
		(*handler->callback)(handler->fd, mask,
				     handler->clientData);
	    }

	    handler = next;
	}
    }

    W_FlushASAPNotificationQueue();

    return (count > 0);
#else /* not HAVE_SELECT, not HAVE_POLL */
Neither select nor poll. You lose.
#endif /* HAVE_SELECT */
#endif /* HAVE_POLL */
}


void
WHandleEvents()
{
    /* Check any expired timers */
    checkTimerHandlers();

    /* We need to make sure that we have some input handler before calling
     * checkIdleHandlers() in a while loop, because else the while loop
     * can run forever (if some idle handler reinitiates itself).
     */
    if (inputHandler) {
        /* Do idle and timer stuff while there are no input events */
        /* We check InputHandler again because some idle handler could have
         * removed them */
        while (checkIdleHandlers() && inputHandler && !handleInputEvents(False)) {
            /* dispatch timer events */
            checkTimerHandlers();
        }
    } else {
        checkIdleHandlers();
        /* dispatch timer events */
        checkTimerHandlers();
    }

    handleInputEvents(True);

    /* Check any expired timers */
    checkTimerHandlers();
}


