/* kde.h-- stuff for support for kde hints
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1998-2002 Alfredo K. Kojima
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

#ifndef _KWM_H_
#define _KWM_H_

typedef enum {
    KWMStickyFlag = (1<<0),
	KWMMaximizedFlag = (1<<1),
	KWMIconifiedFlag = (1<<2),
	KWMAllFlags = 7
} WKWMStateFlag;

typedef enum {
    WKWMAddWindow,
	WKWMRemoveWindow,
	WKWMFocusWindow,
	WKWMRaiseWindow,
	WKWMLowerWindow,
	WKWMChangedClient,
	WKWMIconChange
} WKWMEventMessage;


void wKWMInitStuff(WScreen *scr);

Bool wKWMGetUsableArea(WScreen *scr, WArea *area);

void wKWMCheckClientHints(WWindow *wwin, int *layer, int *workspace);

Bool wKWMCheckClientHintChange(WWindow *wwin, XPropertyEvent *event);

Bool wKWMCheckRootHintChange(WScreen *scr, XPropertyEvent *event);

void wKWMUpdateWorkspaceCountHint(WScreen *scr);

void wKWMUpdateWorkspaceNameHint(WScreen *scr, int workspace);

void wKWMUpdateCurrentWorkspaceHint(WScreen *scr);

Bool wKWMProcessClientMessage(XClientMessageEvent *event);

void wKWMUpdateClientGeometryRestore(WWindow *wwin);

void wKWMUpdateClientWorkspace(WWindow *wwin);

void wKWMUpdateClientStateHint(WWindow *wwin, WKWMStateFlag flags);

Bool wKWMManageableClient(WScreen *scr, Window win, char *title);

void wKWMCheckClientInitialState(WWindow *wwin);

#ifdef not_used
void wKWMSetUsableAreaHint(WScreen *scr, int workspace);
#endif

void wKWMSetInitializedHint(WScreen *scr);

void wKWMShutdown(WScreen *scr, Bool closeModules);

void wKWMCheckModule(WScreen *scr, Window window);

void wKWMSendWindowCreateMessage(WWindow *wwin, Bool create);

void wKWMSendEventMessage(WWindow *wwin, WKWMEventMessage message);

void wKWMCheckDestroy(XDestroyWindowEvent *event);

void wKWMUpdateActiveWindowHint(WScreen *scr);

void wKWMSendStacking(WScreen *scr, Window module);

void wKWMBroadcastStacking(WScreen *scr);

char *wKWMGetWorkspaceName(WScreen *scr, int workspace);

Bool wKWMGetIconGeometry(WWindow *wwin, WArea *area);

void wKWMSelectRootRegion(WScreen *scr, int x, int y, int w, int h, 
			  Bool control);

#endif

