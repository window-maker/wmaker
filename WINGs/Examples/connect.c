/*
 *  WINGs connect.c: example how to create a network client using WMConnection
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


#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <WINGs/WINGs.h>



static int initialized = 0;



static void didReceiveInput(ConnectionDelegate *self, WMConnection *cPtr);

static void connectionDidDie(ConnectionDelegate *self, WMConnection *cPtr);

static void didInitialize(ConnectionDelegate *self, WMConnection *cPtr);



static ConnectionDelegate socketDelegate = {
    NULL,              /* data */
    NULL,              /* didCatchException */
    connectionDidDie,  /* didDie */
    didInitialize,     /* didInitialize */
    didReceiveInput,   /* didReceiveInput */
    NULL               /* didTimeout */
};



void
wAbort(Bool foo) /*FOLD00*/
{
    exit(1);
}


static char*
getMessage(WMConnection *cPtr)
{
    char *buffer;
    WMData *aData;
    int length;

    aData = WMGetConnectionAvailableData(cPtr);
    if (!aData)
        return NULL;
    if ((length=WMGetDataLength(aData))==0) {
        WMReleaseData(aData);
        return NULL;
    }

    buffer = (char*)wmalloc(length+1);
    WMGetDataBytes(aData, buffer);
    buffer[length]= '\0';
    WMReleaseData(aData);

    return buffer;
}


static void
inputHandler(int fd, int mask, void *clientData)
{
    WMConnection *cPtr = (WMConnection*)clientData;
    WMData *aData;
    char buf[4096];
    int n;

    if (!initialized)
        return;

    n = read(fd, buf, 4096);
    if (n>0) {
        aData = WMCreateDataWithBytes(buf, n);
        WMSendConnectionData(cPtr, aData);
        WMReleaseData(aData);
    }
}


static void
didReceiveInput(ConnectionDelegate *self, WMConnection *cPtr) /*FOLD00*/
{
    char *buffer;

    buffer = getMessage(cPtr);
    if (!buffer) {
        fprintf(stderr, "Connection closed by peer.\n");
        exit(0);
    }

    printf("%s", buffer);

    wfree(buffer);
}


static void
connectionDidDie(ConnectionDelegate *self, WMConnection *cPtr) /*FOLD00*/
{
    WMCloseConnection(cPtr);

    fprintf(stderr, "Connection closed by peer.\n");
    exit(0);
}


static void
didInitialize(ConnectionDelegate *self, WMConnection *cPtr)
{
    int state = WMGetConnectionState(cPtr);
    WMHost *host;

    if (state == WCConnected) {
        host = WMGetHostWithAddress(WMGetConnectionAddress(cPtr));
        fprintf(stderr, "connected to '%s:%s'\n",
                host?WMGetHostName(host):WMGetConnectionAddress(cPtr),
                WMGetConnectionService(cPtr));
        initialized = 1;
        if (host)
            WMReleaseHost(host);
        return;
    } else {
        wsyserrorwithcode(WCErrorCode, "Unable to connect");
        exit(1);
    }
}


int
main(int argc, char **argv) /*FOLD00*/
{
    char *ProgName, *host, *port;
    int i;
    WMConnection *sPtr;

    wsetabort(wAbort);

    WMInitializeApplication("connect", &argc, argv);

    ProgName = strrchr(argv[0],'/');
    if (!ProgName)
        ProgName = argv[0];
    else
        ProgName++;

    host = NULL;
    port = "7000";

    if (argc>1) {
        for (i=1; i<argc; i++) {
            if (strcmp(argv[i], "--help")==0 || strcmp(argv[i], "-h")==0) {
                printf("usage: %s [host [port]]\n\n", ProgName);
                exit(0);
            } else {
                if (!host)
                    host = argv[i];
                else
                    port = argv[i];
            }
        }
    }

    printf("Trying to make connection to '%s:%s'\n",
           host?host:"localhost", port);

    sPtr = WMCreateConnectionToAddressAndNotify(host, port, NULL);
    if (!sPtr) {
        wfatal("could not create connection. exiting");
        exit(1);
    }

    WMSetConnectionDelegate(sPtr, &socketDelegate);

    /* watch what user types and send it over the connection */
    WMAddInputHandler(0, WIReadMask, inputHandler, sPtr);

    while (1) {
        WHandleEvents();
    }

    return 0;

}


