/*
 *  WindowMaker external notification support
 * 
 *  Copyright (c) 1998 Peter Bentley (pete@sorted.org)
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "properties.h"
#include "wmnotify.h"

#ifdef WMNOTIFY
#define DEBUG
extern Atom _XA_WINDOWMAKER_NOTIFY;

static LinkedList *wNotifyWindows=NULL;

typedef struct WNotifyClient
{
    Window	not_win;		/* Id of window to send notify events to */
    int		not_mask;	/* Mask of desired notifications	 */
} WNotifyClient;


void wNotify( int id, int a1, int a2 )
{
}


void wNotifyWin( int id, WWindow *wwin )
{
    XEvent		event;
    LinkedList		*list;
    WNotifyClient      	*clnt;
    int			count = 0, mask = WMN_ID_TO_MASK( id );

    event.xclient.type = ClientMessage;
    event.xclient.message_type = _XA_WINDOWMAKER_NOTIFY;
    event.xclient.format = 32;
    event.xclient.display = dpy;
    event.xclient.data.l[0] = id;
    if( wwin )
	event.xclient.data.l[1] = wwin->client_win; /* XXX */
    else
	event.xclient.data.l[1] = 0;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 0;

    for( list = wNotifyWindows; list; list = list->tail )
    {
	clnt = list->head;
	if( clnt->not_mask & mask )
	{
#ifdef DEBUG
	    printf( "Send event %d to 0x%x\n", id, (int) clnt->not_win );
#endif
	    event.xclient.window = clnt->not_win;
	    XSendEvent(dpy, clnt->not_win, False, NoEventMask, &event);
	    count++;
	}
    }
    if( count )
	XFlush(dpy);
}


int wNotifySet(Window window)
{
    int			mask;
    WNotifyClient	*clnt;

    if( PropGetNotifyMask( window, &mask )) {
	wNotifyClear( window );	/* Remove any current mask */
#ifdef DEBUG
	printf( "Setting notify mask for window 0x%x to 0x%02x\n",
		(int) window, mask );
#endif
	clnt = wmalloc( sizeof( WNotifyClient ));
	if( clnt )
	{
	    clnt->not_win = window;
	    clnt->not_mask = mask;
	    wNotifyWindows = list_cons( clnt, wNotifyWindows );
	    return True;
	}
    }
    return False;
}

int wNotifyClear(Window window)
{
    LinkedList		*list;
    WNotifyClient      	*clnt;

    for( list = wNotifyWindows; list; list = list->tail )
    {
	clnt = list->head;
	if( clnt->not_win == window )
	{
#ifdef DEBUG
	    printf( "Clearing notify mask for window 0x%x (was 0x%02x)\n",
		    (int) clnt->not_win, clnt->not_mask );
#endif
	    wNotifyWindows = list_remove_elem( wNotifyWindows, clnt );
	    free( clnt );
	    return True;
	}
    }
    return False;
}


#endif /* WMNOTIFY */
