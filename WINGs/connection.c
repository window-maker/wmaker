/*
 *  WINGs WMConnection function library
 * 
 *  Copyright (c) 1999-2000 Dan Pascu
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * TODO:
 * - decide if we want to support connections with external sockets, else
 *   clean up the structure of the unneeded members.
 * - decide what to do with all wwarning() calls that are still there.
 *
 */


#include "../src/config.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#ifdef __FreeBSD__
#include <sys/signal.h>
#endif

#include "WINGs.h"


/* Some older systems does not define this (linux libc5, maybe others too) */
#ifndef SHUT_RDWR
# define SHUT_RDWR              2
#endif

/* For SunOS */
#ifndef SA_RESTART
# define SA_RESTART             0
#endif

/* For Solaris */
#ifndef INADDR_NONE
# define INADDR_NONE           -1
#endif

/* Stuff for setting the sockets into non-blocking mode. */
/*#ifdef	__POSIX_SOURCE
# define NONBLOCK_OPT           O_NONBLOCK
#else
# define NONBLOCK_OPT           FNDELAY
#endif*/

#define NONBLOCK_OPT            O_NONBLOCK

#define NETBUF_SIZE             4096

#define DEF_TIMEOUT             600   /* 600 seconds == 10 minutes */



int WCErrorCode = 0;

static Bool SigInitialized = False;

static unsigned int DefaultTimeout = DEF_TIMEOUT;
static unsigned int OpenTimeout = DEF_TIMEOUT;



typedef struct TimeoutData {
    unsigned timeout;
    WMHandlerID *handler;
} TimeoutData;


typedef struct W_Connection {
    int sock;                     /* the socket we speak through */

    struct {
        WMHandlerID *read;        /* the input read handler */
        WMHandlerID *write;       /* the input write handler */
        WMHandlerID *exception;   /* the input exception handler */
    } handler;

    ConnectionDelegate *delegate; /* client delegates */
    void *clientData;             /* client data */
    unsigned int uflags;          /* flags for the client */

    WMArray *outputQueue;
    unsigned bufPos;

    TimeoutData sendTimeout;
    TimeoutData openTimeout;

    WMConnectionState state;
    WMConnectionTimeoutState timeoutState;

    char *address;
    char *service;
    char *protocol;

    Bool closeOnRelease;
    Bool wasNonBlocking;
    Bool isNonBlocking;

} W_Connection;



static void
clearOutputQueue(WMConnection *cPtr) /*FOLD00*/
{
    cPtr->bufPos = 0;
    WMEmptyArray(cPtr->outputQueue);
}


static void
openTimeout(void *cdata) /*FOLD00*/
{
    WMConnection *cPtr = (WMConnection*) cdata;

    cPtr->openTimeout.handler = NULL;
    if (cPtr->handler.write) {
        WMDeleteInputHandler(cPtr->handler.write);
        cPtr->handler.write = NULL;
    }
    if (cPtr->state != WCConnected) {
        cPtr->state = WCTimedOut;
        cPtr->timeoutState = WCTWhileOpening;
        if (cPtr->delegate && cPtr->delegate->didTimeout) {
            (*cPtr->delegate->didTimeout)(cPtr->delegate, cPtr);
        } else {
            WMCloseConnection(cPtr);
            cPtr->state = WCTimedOut; /* the above set state to WCClosed */
        }
    }
}


static void
sendTimeout(void *cdata) /*FOLD00*/
{
    WMConnection *cPtr = (WMConnection*) cdata;

    cPtr->sendTimeout.handler = NULL;
    if (cPtr->handler.write) {
        WMDeleteInputHandler(cPtr->handler.write);
        cPtr->handler.write = NULL;
    }
    if (WMGetArrayItemCount(cPtr->outputQueue)>0) {
        clearOutputQueue(cPtr);
        cPtr->state = WCTimedOut;
        cPtr->timeoutState = WCTWhileSending;
        if (cPtr->delegate && cPtr->delegate->didTimeout) {
            (*cPtr->delegate->didTimeout)(cPtr->delegate, cPtr);
        } else {
            WMCloseConnection(cPtr);
            cPtr->state = WCTimedOut; /* the above set state to WCClosed */
        }
    }
}


static void
inputHandler(int fd, int mask, void *clientData) /*FOLD00*/
{
    WMConnection *cPtr = (WMConnection*)clientData;

    if (cPtr->state==WCClosed || cPtr->state==WCDied)
        return;

    if ((mask & WIWriteMask)) {
        if (cPtr->state == WCInProgress) {
            Bool failed;
            int	result;
            int	len = sizeof(result);

            WCErrorCode = 0;
            if (getsockopt(cPtr->sock, SOL_SOCKET, SO_ERROR,
                           (void*)&result, &len) == 0 && result != 0) {
                cPtr->state = WCFailed;
                WCErrorCode = result;
                failed = True;
                /* should call wsyserrorwithcode(result, ...) here? */
            } else {
                cPtr->state = WCConnected;
                failed = False;
            }

            if (cPtr->handler.write) {
                WMDeleteInputHandler(cPtr->handler.write);
                cPtr->handler.write = NULL;
            }

            if (cPtr->openTimeout.handler) {
                WMDeleteTimerHandler(cPtr->openTimeout.handler);
                cPtr->openTimeout.handler = NULL;
            }

            if (cPtr->delegate && cPtr->delegate->didInitialize)
                (*cPtr->delegate->didInitialize)(cPtr->delegate, cPtr);

            /* we use failed and not cPtr->state here, because cPtr may be
             * destroyed by the delegate called above if the connection failed
             */
            if (failed)
                return;
        } else if (cPtr->state == WCConnected) {
            WMFlushConnection(cPtr);
        }
    }

    if (!cPtr->delegate)
        return;

    /* if the connection died, may get destroyed in the delegate, so retain */
    wretain(cPtr);

    if ((mask & WIReadMask) && cPtr->delegate->didReceiveInput)
        (*cPtr->delegate->didReceiveInput)(cPtr->delegate, cPtr);

    if ((mask & WIExceptMask) && cPtr->delegate->didCatchException)
        (*cPtr->delegate->didCatchException)(cPtr->delegate, cPtr);

    wrelease(cPtr);
}


static Bool
setSocketNonBlocking(int sock, Bool flag) /*FOLD00*/
{
    int state;
    Bool isNonBlock;

    state = fcntl(sock, F_GETFL, 0);

    if (state < 0) {
        /* set WCErrorCode here? -Dan*/
        return False;
    }

    isNonBlock = (state & NONBLOCK_OPT) != 0;

    if (flag) {
        if (isNonBlock)
            return True;
        state |= NONBLOCK_OPT;
    } else {
        if (!isNonBlock)
            return True;
        state &= ~NONBLOCK_OPT;
    }

    if (fcntl(sock, F_SETFL, state) < 0) {
        /* set WCErrorCode here? -Dan*/
        return False;
    }

    return True;
}


static void
setConnectionAddress(WMConnection *cPtr, struct sockaddr_in *socketaddr) /*FOLD00*/
{
    wassertr(cPtr->address==NULL);

    cPtr->address = wstrdup(inet_ntoa(socketaddr->sin_addr));
    cPtr->service = wmalloc(16);
    sprintf(cPtr->service, "%hu", ntohs(socketaddr->sin_port));
    cPtr->protocol = wstrdup("tcp");
}


static struct sockaddr_in*
getSocketAddress(char* name, char* service, char* protocol) /*FOLD00*/
{
    static struct sockaddr_in socketaddr;
    struct servent *sp;

    if (!protocol || protocol[0]==0)
        protocol = "tcp";

    memset(&socketaddr, 0, sizeof(struct sockaddr_in));
    socketaddr.sin_family = AF_INET;

    /*
     * If we were given a hostname, we use any address for that host.
     * Otherwise we expect the given name to be an address unless it is
     * NULL (any address).
     */
    if (name && name[0]!=0) {
        WMHost *host = WMGetHostWithName(name);

        if (!host)
            return NULL; /* name is not a hostname nor a number and dot adr */

        name = WMGetHostAddress(host);
#ifndef	HAVE_INET_ATON
        if ((socketaddr.sin_addr.s_addr = inet_addr(name)) == INADDR_NONE) {
#else
        if (inet_aton(name, &socketaddr.sin_addr) == 0) {
#endif
            WMReleaseHost(host);
            return NULL;
        }
        WMReleaseHost(host);
    } else {
        socketaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (!service || service[0]==0) {
        socketaddr.sin_port = 0;
    } else if ((sp = getservbyname(service, protocol))==0) {
        char *endptr;
        unsigned portNumber;

        portNumber = strtoul(service, &endptr, 10);

        if (service[0]!=0 && *endptr==0 && portNumber<65536) {
            socketaddr.sin_port = htons(portNumber);
        } else {
            return NULL;
        }
    } else {
        socketaddr.sin_port = sp->s_port;
    }

    return &socketaddr;
}


static WMConnection*
createConnectionWithSocket(int sock, Bool closeOnRelease) /*FOLD00*/
{
    WMConnection *cPtr;
    struct sigaction sig_action;

    cPtr = wmalloc(sizeof(WMConnection));
    wretain(cPtr);
    memset(cPtr, 0, sizeof(WMConnection));

    fcntl(sock, F_SETFD, FD_CLOEXEC); /* by default close on exec */

    cPtr->sock = sock;
    cPtr->openTimeout.timeout = OpenTimeout;
    cPtr->openTimeout.handler = NULL;
    cPtr->sendTimeout.timeout = DefaultTimeout;
    cPtr->sendTimeout.handler = NULL;
    cPtr->closeOnRelease = closeOnRelease;
    cPtr->outputQueue =
        WMCreateArrayWithDestructor(16, (WMFreeDataProc*)WMReleaseData);
    cPtr->state = WCNotConnected;
    cPtr->timeoutState = WCTNone;

    /* ignore dead pipe */
    if (!SigInitialized) {
        sig_action.sa_handler = SIG_IGN;
        sig_action.sa_flags = SA_RESTART;
        sigaction(SIGPIPE, &sig_action, NULL);
        SigInitialized = True;
    }

    return cPtr;
}


#if 0
WMConnection*
WMCreateConnectionWithSocket(int sock, Bool closeOnRelease) /*FOLD00*/
{
    WMConnection *cPtr;
    struct sockaddr_in clientname;
    int size, n;

    cPtr = createConnectionWithSocket(sock, closeOnRelease);
    cPtr->wasNonBlocking = WMIsConnectionNonBlocking(cPtr);
    cPtr->isNonBlocking = cPtr->wasNonBlocking;

    /* some way to find out if it is connected, and binded. can't find
       if it listens though!!!
    */  

    size = sizeof(clientname);
    n = getpeername(sock, (struct sockaddr*) &clientname, &size);
    if (n==0) {
        /* Since we have a peer, it means we are connected */
        cPtr->state = WCConnected;
    } else {
        size = sizeof(clientname);
        n = getsockname(sock, (struct sockaddr*) &clientname, &size);
        if (n==0) {
            /* We don't have a peer, but we are binded to an address.
             * Assume we are listening on it (we don't know that for sure!)
             */
            cPtr->state = WCListening;
        } else {
            cPtr->state = WCNotConnected;
        }
    }

    return cPtr;
}
#endif


/*
 * host     is the name on which we want to listen for incoming connections,
 *          and it must be a name of this host, or NULL if we want to listen
 *          on any incoming address.
 * service  is either a service name as present in /etc/services, or the port
 *          number we want to listen on. If NULL, a random port between
 *          1024 and 65535 will be assigned to us.
 * protocol is one of "tcp" or "udp". If NULL, "tcp" will be used by default.
 *          currently only "tcp" is supported.
 */
WMConnection*
WMCreateConnectionAsServerAtAddress(char *host, char *service, char *protocol) /*FOLD00*/
{
    WMConnection *cPtr;
    struct sockaddr_in *socketaddr;
    int sock, size, on;

    WCErrorCode = 0;

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        return NULL;
    }

    /*
     * Set socket options. We try to make the port reusable and have it
     * close as fast as possible without waiting in unnecessary wait states
     * on close.
     */
    on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

    if (bind(sock, (struct sockaddr *)socketaddr, sizeof(*socketaddr)) < 0) {
        WCErrorCode = errno;
        close(sock);
        return NULL;
    }

    if (listen(sock, 10) < 0) {
        WCErrorCode = errno;
        close(sock);
        return NULL;
    }

    /* Find out what is the address/service/protocol we get */
    /* In case some of address/service/protocol were NULL */
    size = sizeof(*socketaddr);
    if (getsockname(sock, (struct sockaddr*)socketaddr, &size) < 0) {
        WCErrorCode = errno;
        close(sock);
        return NULL;
    }

    cPtr = createConnectionWithSocket(sock, True);
    cPtr->state = WCListening;
    WMSetConnectionNonBlocking(cPtr, True);

    setConnectionAddress(cPtr, socketaddr);

    return cPtr;
}


WMConnection*
WMCreateConnectionToAddress(char *host, char *service, char *protocol) /*FOLD00*/
{
    WMConnection *cPtr;
    struct sockaddr_in *socketaddr;
    int sock;

    WCErrorCode = 0;

    wassertrv(service!=NULL && service[0]!=0, NULL);

    if (host==NULL || host[0]==0)
        host = "localhost";

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        return NULL;
    }
    /* make socket blocking while we connect. */
    setSocketNonBlocking(sock, False);
    if (connect(sock, (struct sockaddr*)socketaddr, sizeof(*socketaddr)) < 0) {
        WCErrorCode = errno;
        close(sock);
        return NULL;
    }

    cPtr = createConnectionWithSocket(sock, True);
    cPtr->state = WCConnected;
    WMSetConnectionNonBlocking(cPtr, True);
    setConnectionAddress(cPtr, socketaddr);

    return cPtr;
}


WMConnection*
WMCreateConnectionToAddressAndNotify(char *host, char *service, char *protocol) /*FOLD00*/
{
    WMConnection *cPtr;
    struct sockaddr_in *socketaddr;
    int sock;
    Bool isNonBlocking;

    WCErrorCode = 0;

    wassertrv(service!=NULL && service[0]!=0, NULL);

    if (host==NULL || host[0]==0)
        host = "localhost";

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        return NULL;
    }
    isNonBlocking = setSocketNonBlocking(sock, True);
    if (connect(sock, (struct sockaddr*)socketaddr, sizeof(*socketaddr)) < 0) {
        if (errno!=EINPROGRESS) {
            WCErrorCode = errno;
            close(sock);
            return NULL;
        }
    }

    cPtr = createConnectionWithSocket(sock, True);
    cPtr->state = WCInProgress;
    cPtr->isNonBlocking = isNonBlocking;

    cPtr->handler.write = WMAddInputHandler(cPtr->sock, WIWriteMask,
                                            inputHandler, cPtr);

    cPtr->openTimeout.handler =
        WMAddTimerHandler(cPtr->openTimeout.timeout*1000, openTimeout, cPtr);

    setConnectionAddress(cPtr, socketaddr);

    return cPtr;
}


static void
removeAllHandlers(WMConnection *cPtr) /*FOLD00*/
{
    if (cPtr->handler.read)
        WMDeleteInputHandler(cPtr->handler.read);
    if (cPtr->handler.write)
        WMDeleteInputHandler(cPtr->handler.write);
    if (cPtr->handler.exception)
        WMDeleteInputHandler(cPtr->handler.exception);
    if (cPtr->openTimeout.handler)
        WMDeleteTimerHandler(cPtr->openTimeout.handler);
    if (cPtr->sendTimeout.handler)
        WMDeleteTimerHandler(cPtr->sendTimeout.handler);

    cPtr->handler.read = NULL;
    cPtr->handler.write = NULL;
    cPtr->handler.exception = NULL;
    cPtr->openTimeout.handler = NULL;
    cPtr->sendTimeout.handler = NULL;
}


void
WMDestroyConnection(WMConnection *cPtr) /*FOLD00*/
{
    if (cPtr->closeOnRelease && cPtr->sock>=0) {
        shutdown(cPtr->sock, SHUT_RDWR);
        close(cPtr->sock);
    }

    removeAllHandlers(cPtr);
    WMFreeArray(cPtr->outputQueue); /* will also free the items with the destructor */

    if (cPtr->address) {
        wfree(cPtr->address);
        wfree(cPtr->service);
        wfree(cPtr->protocol);
    }

    wrelease(cPtr);
}


void
WMCloseConnection(WMConnection *cPtr) /*FOLD00*/
{
    if (cPtr->sock>=0) {
        shutdown(cPtr->sock, SHUT_RDWR);
        close(cPtr->sock);
        cPtr->sock = -1;
    }

    removeAllHandlers(cPtr);
    clearOutputQueue(cPtr);

    cPtr->state = WCClosed;
}


WMConnection*
WMAcceptConnection(WMConnection *listener) /*FOLD00*/
{
    struct sockaddr_in clientname;
    int size;
    int newSock;
    WMConnection *newConnection;

    WCErrorCode = 0;
    wassertrv(listener && listener->state==WCListening, NULL);

    size = sizeof(clientname);
    newSock = accept(listener->sock, (struct sockaddr*) &clientname, &size);
    if (newSock<0) {
        WCErrorCode = ((errno!=EAGAIN && errno!=EWOULDBLOCK) ? errno : 0);
        return NULL;
    }

    newConnection = createConnectionWithSocket(newSock, True);
    WMSetConnectionNonBlocking(newConnection, True);
    newConnection->state = WCConnected;
    setConnectionAddress(newConnection, &clientname);

    return newConnection;
}


char*
WMGetConnectionAddress(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->address;
}


char*
WMGetConnectionService(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->service;
}


char*
WMGetConnectionProtocol(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->protocol;
}


int
WMGetConnectionSocket(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->sock;
}


WMConnectionState
WMGetConnectionState(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->state;
}


WMConnectionTimeoutState
WMGetConnectionTimeoutState(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->timeoutState;
}


Bool
WMEnqueueConnectionData(WMConnection *cPtr, WMData *data) /*FOLD00*/
{
    wassertrv(cPtr->state!=WCNotConnected && cPtr->state!=WCListening, False);
    wassertrv(cPtr->state!=WCInProgress && cPtr->state!=WCFailed, False);

    if (cPtr->state!=WCConnected)
        return False;

    WMAddToArray(cPtr->outputQueue, WMRetainData(data));
    return True;
}


int
WMSendConnectionData(WMConnection *cPtr, WMData *data) /*FOLD00*/
{
    int bytes, pos, len, totalTransfer;
    TimeoutData *tPtr = &cPtr->sendTimeout;
    const unsigned char *dataBytes;

    wassertrv(cPtr->state!=WCNotConnected && cPtr->state!=WCListening, -1);
    wassertrv(cPtr->state!=WCInProgress && cPtr->state!=WCFailed, -1);

    if (cPtr->state!=WCConnected)
        return -1;

    /* If we have no data just flush the queue, else try to send data */
    if (data && WMGetDataLength(data)>0) {
        WMAddToArray(cPtr->outputQueue, WMRetainData(data));
        /* If there already was something in queue, and also a write input
         * handler is established, it means we were unable to send, so
         * return and let the write handler notify us when we can send.
         */
        if (WMGetArrayItemCount(cPtr->outputQueue)>1 && cPtr->handler.write)
            return 0;
    }

    totalTransfer = 0;

    while (WMGetArrayItemCount(cPtr->outputQueue) > 0) {
        data = WMGetFromArray(cPtr->outputQueue, 0);
        dataBytes = (const unsigned char *)WMDataBytes(data);
        len = WMGetDataLength(data);
        pos = cPtr->bufPos; /* where we're left last time */
        while(pos < len) {
        again:
            bytes = write(cPtr->sock, dataBytes+pos, len - pos);
            if(bytes<0) {
                switch (errno) {
                case EINTR:
                    goto again;
                case EWOULDBLOCK:
                    /* save the position where we're left and add a timeout */
                    cPtr->bufPos = pos;
                    if (!tPtr->handler) {
                        tPtr->handler = WMAddTimerHandler(tPtr->timeout*1000,
                                                          sendTimeout, cPtr);
                    }
                    if (!cPtr->handler.write) {
                        cPtr->handler.write =
                            WMAddInputHandler(cPtr->sock, WIWriteMask,
                                              inputHandler, cPtr);
                    }
                    return totalTransfer;
                default:
                    WCErrorCode = errno;
                    cPtr->state = WCDied;
                    removeAllHandlers(cPtr);
                    if (cPtr->delegate && cPtr->delegate->didDie)
                        (*cPtr->delegate->didDie)(cPtr->delegate, cPtr);
                    return -1;
                }
            }
            pos += bytes;
            totalTransfer += bytes;
        }
        WMDeleteFromArray(cPtr->outputQueue, 0);
        cPtr->bufPos = 0;
        if (tPtr->handler) {
            WMDeleteTimerHandler(tPtr->handler);
            tPtr->handler = NULL;
        }
        if (cPtr->handler.write) {
            WMDeleteInputHandler(cPtr->handler.write);
            cPtr->handler.write = NULL;
        }
    }

    return totalTransfer;
}


/*
 * WMGetConnectionAvailableData(connection):
 *
 * will return a WMData structure containing the available data on the
 * specified connection. If connection is non-blocking (default) and no data
 * is available when this function is called, an empty WMData is returned.
 *
 * If an error occurs while reading or the other side closed connection,
 * it will return NULL.
 * Also trying to read from an already died or closed connection is
 * considered to be an error condition, and will return NULL.
 */
WMData*
WMGetConnectionAvailableData(WMConnection *cPtr) /*FOLD00*/
{
    char buffer[NETBUF_SIZE];
    int nbytes;
    WMData *aData;

    wassertrv(cPtr->state!=WCNotConnected && cPtr->state!=WCListening, NULL);
    wassertrv(cPtr->state!=WCInProgress && cPtr->state!=WCFailed, NULL);

    if (cPtr->state!=WCConnected)
        return NULL;

    aData = NULL;

again:
    nbytes = read(cPtr->sock, buffer, NETBUF_SIZE);
    if (nbytes<0) {
        switch (errno) {
        case EINTR:
            goto again;
        case EWOULDBLOCK:
            aData = WMCreateDataWithCapacity(0);
            break;
        default:
            WCErrorCode = errno;
            cPtr->state = WCDied;
            removeAllHandlers(cPtr);
            if (cPtr->delegate && cPtr->delegate->didDie)
                (*cPtr->delegate->didDie)(cPtr->delegate, cPtr);
            break;
        }
    } else if (nbytes==0) {      /* the other side has closed connection */
        cPtr->state = WCClosed;
        removeAllHandlers(cPtr);
        if (cPtr->delegate && cPtr->delegate->didDie)
            (*cPtr->delegate->didDie)(cPtr->delegate, cPtr);
    } else {
        aData = WMCreateDataWithBytes(buffer, nbytes);
    }

    return aData;
}


void
WMSetConnectionDelegate(WMConnection *cPtr, ConnectionDelegate *delegate) /*FOLD00*/
{
    wassertr(cPtr->sock >= 0);
    /* Don't try to set the delegate multiple times */
    wassertr(cPtr->delegate == NULL);

    cPtr->delegate = delegate;
    if (delegate && delegate->didReceiveInput && !cPtr->handler.read)
        cPtr->handler.read = WMAddInputHandler(cPtr->sock, WIReadMask,
                                               inputHandler, cPtr);
    if (delegate && delegate->didCatchException && !cPtr->handler.exception)
        cPtr->handler.exception = WMAddInputHandler(cPtr->sock, WIExceptMask,
                                                    inputHandler, cPtr);
}


#if 0
Bool
WMIsConnectionNonBlocking(WMConnection *cPtr) /*FOLD00*/
{
#if 1
    int state;

    state = fcntl(cPtr->sock, F_GETFL, 0);

    if (state < 0) {
        /* If we can't use fcntl on socket, this probably also means we could
         * not use fcntl to set non-blocking mode, and since a socket defaults
         * to blocking when created, return False as the best assumption */
        return False;
    }

    return ((state & NONBLOCK_OPT)!=0);
#else
    return cPtr->isNonBlocking;
#endif
}
#endif


Bool
WMSetConnectionNonBlocking(WMConnection *cPtr, Bool flag) /*FOLD00*/
{
    wassertrv(cPtr!=NULL && cPtr->sock>=0, False);

    if (cPtr->isNonBlocking == flag)
        return True;

    if (setSocketNonBlocking(cPtr->sock, flag)==True) {
        cPtr->isNonBlocking = flag;
        return True;
    }

    return False;
}


Bool
WMSetConnectionCloseOnExec(WMConnection *cPtr, Bool flag)
{
    wassertrv(cPtr!=NULL && cPtr->sock>=0, False);

    if (fcntl(cPtr->sock, F_SETFD, (flag ? FD_CLOEXEC : 0)) < 0) {
        return False;
    }

    return True;
}


void*
WMGetConnectionClientData(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->clientData;
}


void
WMSetConnectionClientData(WMConnection *cPtr, void *data) /*FOLD00*/
{
    cPtr->clientData = data;
}


unsigned int
WMGetConnectionFlags(WMConnection *cPtr) /*FOLD00*/
{
    return cPtr->uflags;
}


void
WMSetConnectionFlags(WMConnection *cPtr, unsigned int flags) /*FOLD00*/
{
    cPtr->uflags = flags;
}


WMArray*
WMGetConnectionUnsentData(WMConnection *cPtr)
{
    return cPtr->outputQueue;
}


void
WMSetConnectionDefaultTimeout(unsigned int timeout) /*FOLD00*/
{
    if (timeout == 0) {
        DefaultTimeout = DEF_TIMEOUT;
    } else {
        DefaultTimeout = timeout;
    }
}


void
WMSetConnectionOpenTimeout(unsigned int timeout) /*FOLD00*/
{
    if (timeout == 0) {
        OpenTimeout = DefaultTimeout;
    } else {
        OpenTimeout = timeout;
    }
}


void
WMSetConnectionSendTimeout(WMConnection *cPtr, unsigned int timeout) /*FOLD00*/
{
    if (timeout == 0) {
        cPtr->sendTimeout.timeout = DefaultTimeout;
    } else {
        cPtr->sendTimeout.timeout = timeout;
    }
}


