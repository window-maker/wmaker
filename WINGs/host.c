/*
 *  WINGs WMHost function library
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



#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "WUtil.h"


typedef struct W_Host {
    char   *name;

    WMBag  *names;
    WMBag  *addresses;

    int    refCount;
} W_Host;


static WMHashTable *hostCache = NULL;
static Bool hostCacheEnabled = True;


/* Max hostname length (RFC  1123) */
#define	W_MAXHOSTNAMELEN	255


static WMHost*
getHostFromCache(char *name)
{
    if (!hostCache)
        return NULL;

    return WMHashGet(hostCache, name);
}


static WMHost*
getHostWithHostEntry(struct hostent *host, char *name)
{
    WMHost *hPtr;
    struct in_addr in;
    int i;

    hPtr = (WMHost*)wmalloc(sizeof(WMHost));
    memset(hPtr, 0, sizeof(WMHost));

    hPtr->names = WMCreateBag(1);
    hPtr->addresses = WMCreateBag(1);

    WMPutInBag(hPtr->names, wstrdup(host->h_name));

    for (i=0; host->h_aliases[i]!=NULL; i++) {
        WMPutInBag(hPtr->names, wstrdup(host->h_aliases[i]));
    }

    for (i=0; host->h_addr_list[i]!=NULL; i++) {
        memcpy((void*)&in.s_addr, (const void*)host->h_addr_list[i],
               host->h_length);
        WMPutInBag(hPtr->addresses, wstrdup(inet_ntoa(in)));
    }

    hPtr->refCount = 1;

    if (hostCacheEnabled) {
        if (!hostCache)
            hostCache = WMCreateHashTable(WMStringPointerHashCallbacks);
        hPtr->name = wstrdup(name);
        wassertr(WMHashInsert(hostCache, hPtr->name, hPtr)==NULL);
        hPtr->refCount++;
    }

    return hPtr;
}


WMHost*
WMGetCurrentHost()
{
    char name[W_MAXHOSTNAMELEN+1];

    if (gethostname(name, W_MAXHOSTNAMELEN) < 0) {
        wsyserror("Cannot get current host name");
        return NULL;
    }

    name[W_MAXHOSTNAMELEN] = '\0';

    return WMGetHostWithName(name);
}


WMHost*
WMGetHostWithName(char* name)
{
    struct hostent *host;
    WMHost *hPtr;

    if (name == NULL) {
        wwarning("NULL host name in 'WMGetHostWithName()'");
        return NULL;
    }

    if (hostCacheEnabled) {
        if ((hPtr = getHostFromCache(name)) != NULL) {
            WMRetainHost(hPtr);
            return hPtr;
        }
    }

    host = gethostbyname(name);
    if (host == NULL) {
        return NULL;
    }

    hPtr = getHostWithHostEntry(host, name);

    return hPtr;
}


WMHost*
WMGetHostWithAddress(char* address)
{
    struct hostent *host;
    struct in_addr in;
    WMHost *hPtr;

    if (address == NULL) {
        wwarning("NULL address in 'WMGetHostWithAddress()'");
        return NULL;
    }

    if (hostCacheEnabled) {
        if ((hPtr = getHostFromCache(address)) != NULL) {
            WMRetainHost(hPtr);
            return hPtr;
        }
    }

#ifndef	HAVE_INET_ATON
    if ((in.s_addr = inet_addr(address)) == INADDR_NONE)
        return NULL;
#else
    if (inet_aton(address, &in.s_addr) == 0)
        return NULL;
#endif

    host = gethostbyaddr((char*)&in, sizeof(in), AF_INET);
    if (host == NULL) {
        return NULL;
    }

    hPtr = getHostWithHostEntry(host, address);

    return hPtr;
}


WMHost*
WMRetainHost(WMHost *hPtr)
{
    hPtr->refCount++;
    return hPtr;
}


void
WMReleaseHost(WMHost *hPtr)
{
    int i;

    hPtr->refCount--;

    if (hPtr->refCount > 0)
        return;

    for (i=0; i<WMGetBagItemCount(hPtr->names); i++)
        wfree(WMGetFromBag(hPtr->names, i));
    for (i=0; i<WMGetBagItemCount(hPtr->addresses); i++)
        wfree(WMGetFromBag(hPtr->addresses, i));

    WMFreeBag(hPtr->names);
    WMFreeBag(hPtr->addresses);

    if (hPtr->name) {
        WMHashRemove(hostCache, hPtr->name);
        wfree(hPtr->name);
    }

    wfree(hPtr);
}


void
WMSetHostCacheEnabled(Bool flag)
{
    hostCacheEnabled = flag;
}


Bool
WMIsHostCacheEnabled()
{
    return hostCacheEnabled;
}


void
WMFlushHostCache()
{
    if (hostCache && WMCountHashTable(hostCache)>0) {
        WMBag *hostBag = WMCreateBag(WMCountHashTable(hostCache));
        WMHashEnumerator enumer = WMEnumerateHashTable(hostCache);
        WMHost *hPtr;
        int i;

        while ((hPtr = WMNextHashEnumeratorItem(&enumer))) {
            /* we can't release the host here, because we can't change the
             * hash while using the enumerator functions. */
            WMPutInBag(hostBag, hPtr);
        }
        for (i=0; i<WMGetBagItemCount(hostBag); i++)
            WMReleaseHost(WMGetFromBag(hostBag, i));
        WMFreeBag(hostBag);
        WMResetHashTable(hostCache);
    }
}


Bool
WMIsHostEqualToHost(WMHost* hPtr, WMHost* aPtr)
{
    int	i, j;
    char *adr1, *adr2;

    wassertrv(hPtr!=NULL && aPtr!=NULL, False);

    if (hPtr == aPtr)
        return True;

    for (i=0; i<WMGetBagItemCount(aPtr->addresses); i++) {
        adr1 = WMGetFromBag(aPtr->addresses, i);
        for (j=0; j<WMGetBagItemCount(hPtr->addresses); j++) {
            adr2 = WMGetFromBag(hPtr->addresses, j);
            if (strcmp(adr1, adr2)==0)
                return True;
        }
    }

    return False;
}


char*
WMGetHostName(WMHost *hPtr)
{
    return (WMGetBagItemCount(hPtr->names) > 0 ?
            WMGetFromBag(hPtr->names, 0) : NULL);
    /*return WMGetFromBag(hPtr->names, 0);*/
}


WMBag*
WMGetHostNames(WMHost *hPtr)
{
    return hPtr->names;
}


char*
WMGetHostAddress(WMHost *hPtr)
{
    return (WMGetBagItemCount(hPtr->addresses) > 0 ?
            WMGetFromBag(hPtr->addresses, 0) : NULL);
}


WMBag*
WMGetHostAddresses(WMHost *hPtr)
{
    return hPtr->addresses;
}




