

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
} IdleHandler;


typedef struct InputHandler {
    WMInputProc		*callback;
    void		*clientData;
    int			fd;
    int			mask;
} InputHandler;


/* queue of timer event handlers */
static TimerHandler *timerHandler=NULL;

static WMBag *idleHandler=NULL;

static WMBag *inputHandler=NULL;



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
    if (pos >= 0) {
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
    if (pos >= 0) {
        wfree(handler);
        WMDeleteFromBag(inputHandler, pos);
    }
}



static Bool
checkIdleHandlers()
{
    IdleHandler *handler;
    WMBag *handlerCopy;
    int i, n;

    if (!idleHandler || WMGetBagItemCount(idleHandler)==0) {
	W_FlushIdleNotificationQueue();
        /* make sure an observer in queue didn't added an idle handler */
        return (idleHandler!=NULL && WMGetBagItemCount(idleHandler)>0);
    }

    n = WMGetBagItemCount(idleHandler);
    handlerCopy = WMCreateBag(n);
    for (i=0; i<n; i++)
        WMPutInBag(handlerCopy, WMGetFromBag(idleHandler, i));

    for (i=0; i<n; i++) {
        handler = WMGetFromBag(handlerCopy, i);
        /* check if the handler still exist or was removed by a callback */
        if (WMGetFirstInBag(idleHandler, handler)<0)
            continue;

	(*handler->callback)(handler->clientData);
        WMDeleteIdleHandler(handler);
        wfree(handler);
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
    int count, timeout, nfds, i;

    if (!inputHandler || (nfds=WMGetBagItemCount(inputHandler))==0) {
        W_FlushASAPNotificationQueue();
        return False;
    }

    fds = wmalloc(nfds * sizeof(struct pollfd));

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
        WMBag *handlerCopy = WMCreateBag(nfds);

        for (i=0; i<nfds, i++)
            WMPutInBag(handlerCopy, WMGetFromBag(inputHandler, i));

        for (i=0; i<nfds; i++) {
	    int mask;

            handler = WMGetFromBag(handlerCopy, i);
            /* check if the handler still exist or was removed by a callback */
            if (WMGetFirstInBag(inputHandler, handler)<0)
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

    wfree(fds);

    W_FlushASAPNotificationQueue();

    return (count > 0);
#else /* not HAVE_POLL */
#ifdef HAVE_SELECT
    struct timeval timeout;
    struct timeval *timeoutPtr;
    fd_set rset, wset, eset;
    int maxfd, nfds, i;
    int count;
    InputHandler *handler;

    if (!inputHandler || (nfds=WMGetBagItemCount(inputHandler))==0) {
        W_FlushASAPNotificationQueue();
        return False;
    }

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    maxfd = 0;

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
        WMBag *handlerCopy = WMCreateBag(nfds);

        for (i=0; i<nfds; i++)
            WMPutInBag(handlerCopy, WMGetFromBag(inputHandler, i));

        for (i=0; i<nfds; i++) {
	    int mask;

            handler = WMGetFromBag(handlerCopy, i);
            /* check if the handler still exist or was removed by a callback */
            if (WMGetFirstInBag(inputHandler, handler)<0)
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
    if (inputHandler && WMGetBagItemCount(inputHandler)>0) {
        /* Do idle and timer stuff while there are no input events */
        /* Check again if there are still input handlers, because some idle
         * handler could have removed them */
        while (checkIdleHandlers() && WMGetBagItemCount(inputHandler)>0 &&
               !handleInputEvents(False)) {
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


