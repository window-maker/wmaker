/*
 *  WINGs WMConnection function library
 * 
 *  Copyright (c) 1999 Dan Pascu
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

/* for SunOS */
#ifndef SA_RESTART
# define SA_RESTART             0
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

    WMBag *outputQueue;
    unsigned bufPos;

    TimeoutData sendTimeout;

    WMConnectionState state;

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
    int i;

    cPtr->bufPos = 0;

    for (i=0; i<WMGetBagItemCount(cPtr->outputQueue); i++)
        WMReleaseData(WMGetFromBag(cPtr->outputQueue, i));

    WMEmptyBag(cPtr->outputQueue);
}


static void
sendTimeout(void *cdata) /*FOLD00*/
{
    WMConnection *cPtr = (WMConnection*) cdata;
    TimeoutData *tPtr = &cPtr->sendTimeout;

    tPtr->handler = NULL;
    if (cPtr->handler.write) {
        WMDeleteInputHandler(cPtr->handler.write);
        cPtr->handler.write = NULL;
    }
    if (WMGetBagItemCount(cPtr->outputQueue)>0) {
        clearOutputQueue(cPtr);
        if (cPtr->delegate && cPtr->delegate->didTimeout)
            (*cPtr->delegate->didTimeout)(cPtr->delegate, cPtr);
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
            int	result;
            int	len = sizeof(result);

            if (getsockopt(cPtr->sock, SOL_SOCKET, SO_ERROR,
                           (void*)&result, &len) == 0 && result != 0) {
                cPtr->state = WCFailed;
                WCErrorCode = result;
                /* should call wsyserrorwithcode(result, ...) here? */
            } else {
                cPtr->state = WCConnected;
            }

            if (cPtr->handler.write) {
                WMDeleteInputHandler(cPtr->handler.write);
                cPtr->handler.write = NULL;
            }

            if (cPtr->delegate && cPtr->delegate->didInitialize)
                (*cPtr->delegate->didInitialize)(cPtr->delegate, cPtr);

            if (cPtr->state == WCFailed)
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
        wsyserror("Failed to get socket flags with fcntl.");
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
        wsyserror("Failed to set socket flags with fcntl.");
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

    if (!protocol || protocol[0]=='\0')
        protocol = "tcp";

    memset(&socketaddr, 0, sizeof(struct sockaddr_in));
    socketaddr.sin_family = AF_INET;

    /*
     * If we were given a hostname, we use any address for that host.
     * Otherwise we expect the given name to be an address unless it is
     * NULL (any address).
     */
    if (name && name[0]!='\0') {
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

    if (!service || service[0]=='\0') {
        socketaddr.sin_port = 0;
    } else if ((sp = getservbyname(service, protocol))==0) {
        char *endptr;
        unsigned portNumber;

        portNumber = strtoul(service, &endptr, 10);

        if (service[0]!='\0' && *endptr=='\0' && portNumber<65536) {
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

    cPtr->sock = sock;
    cPtr->sendTimeout.timeout = DEF_TIMEOUT;
    cPtr->sendTimeout.handler = NULL;
    cPtr->closeOnRelease = closeOnRelease;
    cPtr->outputQueue = WMCreateBag(16);
    cPtr->state = WCNotConnected;

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

    // some way to find out if it is connected, and binded. can't find
    // if it listens though!!!

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

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        WCErrorCode = 0;
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        wsyserror("Unable to create socket");
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
        wsyserror("Unable to bind to address '%s:%hu'",
                  inet_ntoa(socketaddr->sin_addr),
                  ntohs(socketaddr->sin_port));
        close(sock);
        return NULL;
    }

    if (listen(sock, 10) < 0) {
        WCErrorCode = errno;
        wsyserror("Unable to listen on port '%hu'",
                  ntohs(socketaddr->sin_port));
        close(sock);
        return NULL;
    }

    /* Find out what is the address/service/protocol we get */
    /* In case some of address/service/protocol were NULL */
    size = sizeof(*socketaddr);
    if (getsockname(sock, (struct sockaddr*)socketaddr, &size) < 0) {
        WCErrorCode = errno;
        wsyserror("Unable to get socket address");
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

    if (service==NULL || service[0]=='\0') {
        WCErrorCode = 0;
        wwarning("Bad argument - service is not specified");
        return NULL;
    }

    if (host==NULL || host[0]=='\0')
        host = "localhost";

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        WCErrorCode = 0;
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        wsyserror("Unable to create socket");
        return NULL;
    }
    /* make socket blocking while we connect. */
    setSocketNonBlocking(sock, False);
    if (connect(sock, (struct sockaddr*)socketaddr, sizeof(*socketaddr)) < 0) {
        WCErrorCode = errno;
        wsyserror("Unable to make connection to address '%s:%hu'",
                  inet_ntoa(socketaddr->sin_addr),
                  ntohs(socketaddr->sin_port));
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
    /*TimeoutData *tPtr;*/
    struct sockaddr_in *socketaddr;
    int sock;
    Bool isNonBlocking;

    if (service==NULL || service[0]=='\0') {
        WCErrorCode = 0;
        wwarning("Bad argument - service is not specified");
        return NULL;
    }

    if (host==NULL || host[0]=='\0')
        host = "localhost";

    if ((socketaddr = getSocketAddress(host, service, protocol)) == NULL) {
        WCErrorCode = 0;
        wwarning("Bad address-service-protocol combination");
        return NULL;
    }

    /* Create the actual socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock<0) {
        WCErrorCode = errno;
        wsyserror("Unable to create socket");
        return NULL;
    }
    isNonBlocking = setSocketNonBlocking(sock, True);
    if (connect(sock, (struct sockaddr*)socketaddr, sizeof(*socketaddr)) < 0) {
        if (errno!=EINPROGRESS) {
            WCErrorCode = errno;
            wsyserror("Unable to make connection to address '%s:%hu'",
                      inet_ntoa(socketaddr->sin_addr),
                      ntohs(socketaddr->sin_port));
            close(sock);
            return NULL;
        }
    }

    cPtr = createConnectionWithSocket(sock, True);
    cPtr->state = WCInProgress;
    cPtr->isNonBlocking = isNonBlocking;

    /*tPtr = &cPtr->sendTimeout;
     tPtr->handler = WMAddTimerHandler(tPtr->timeout*1000, connectTimeout, cPtr);
    */
    cPtr->handler.write = WMAddInputHandler(cPtr->sock, WIWriteMask,
                                            inputHandler, cPtr);

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
    if (cPtr->sendTimeout.handler)
        WMDeleteTimerHandler(cPtr->sendTimeout.handler);

    cPtr->handler.read = NULL;
    cPtr->handler.write = NULL;
    cPtr->handler.exception = NULL;
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
    clearOutputQueue(cPtr);
    WMFreeBag(cPtr->outputQueue);

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

    if (listener->state!=WCListening) {
        wwarning("Called 'WMAcceptConnection()' on a non-listening connection");
        WCErrorCode = 0;
        return NULL;
    }

    size = sizeof(clientname);
    newSock = accept(listener->sock, (struct sockaddr*) &clientname, &size);
    if (newSock<0) {
        if (errno!=EAGAIN && errno!=EWOULDBLOCK) {
            WCErrorCode = errno;
            wsyserror("Could not accept connection");
        } else {
            WCErrorCode = 0;
        }
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
WMGetConnectionProtocol(WMConnection *cPtr)
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


Bool
WMEnqueueConnectionData(WMConnection *cPtr, WMData *data) /*FOLD00*/
{
    wassertrv(cPtr->state!=WCNotConnected && cPtr->state!=WCListening, False);
    wassertrv(cPtr->state!=WCInProgress && cPtr->state!=WCFailed, False);

    if (cPtr->state!=WCConnected)
        return False;

    WMPutInBag(cPtr->outputQueue, WMRetainData(data));
    return True;
}


int
WMSendConnectionData(WMConnection *cPtr, WMData *data) /*FOLD00*/
{
    int bytes, pos, len, totalTransfer;
    TimeoutData *tPtr = &cPtr->sendTimeout;
    const void *dataBytes;

    wassertrv(cPtr->state!=WCNotConnected && cPtr->state!=WCListening, -1);
    wassertrv(cPtr->state!=WCInProgress && cPtr->state!=WCFailed, -1);

    if (cPtr->state!=WCConnected)
        return -1;

    /* If we have no data just flush the queue, else try to send data */
    if (data && WMGetDataLength(data)>0) {
        WMPutInBag(cPtr->outputQueue, WMRetainData(data));
        /* If there already was something in queue, and also a write input
         * handler is established, it means we were unable to send, so
         * return and let the write handler notify us when we can send.
         */
        if (WMGetBagItemCount(cPtr->outputQueue)>1 && cPtr->handler.write)
            return 0;
    }

    totalTransfer = 0;

    while (WMGetBagItemCount(cPtr->outputQueue) > 0) {
        data = WMGetFromBag(cPtr->outputQueue, 0);
        dataBytes = WMDataBytes(data);
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
                    /*clearOutputQueue(cPtr);*/
                    removeAllHandlers(cPtr);
                    if (cPtr->delegate && cPtr->delegate->didDie)
                        (*cPtr->delegate->didDie)(cPtr->delegate, cPtr);
                    return -1;
                }
            }
            pos += bytes;
            totalTransfer += bytes;
        }
        WMReleaseData(data);
        WMDeleteFromBag(cPtr->outputQueue, 0);
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
        wsyserror("Failed to get socket flags with fcntl.");
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


void
WMSetConnectionNonBlocking(WMConnection *cPtr, Bool flag) /*FOLD00*/
{
    if (cPtr->sock < 0)
        return;

    if (cPtr->isNonBlocking == flag)
        return;

    if (setSocketNonBlocking(cPtr->sock, flag)==True)
        cPtr->isNonBlocking = flag;
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


void
WMSetConnectionSendTimeout(WMConnection *cPtr, unsigned int timeout) /*FOLD00*/
{
    if (timeout == 0)
        timeout = DEF_TIMEOUT;

    cPtr->sendTimeout.timeout = timeout;
}


