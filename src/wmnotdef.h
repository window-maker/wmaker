/*
 *  WindowMaker external notification definitions
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

#ifndef WMNOTDEF_H_
#define WMNOTDEF_H_

/* Make a notification ID from a category and an 'event' */
#define WMN_MAKE_ID(cat, ev)		( ((cat)<<4) | (ev) )

/* Split an ID into category and 'event' */
#define WMN_CATEGORY_OF( id )		( ((id) & 0x00f0) >> 4 )
#define WMN_EVENT_OF( id )		(  (id) & 0x000f )

/* Make notification mask from categories or ids */
#define WMN_CATEGORY_TO_MASK( cat )	( 0x0001 << ( cat ))
#define WMN_ID_TO_MASK( id )	WMN_CATEGORY_TO_MASK( WMN_CATEGORY_OF( id ))

/* Constant and mask for "all events in all categories" */
#define WMN_NOTIFY_ALL		0x0000
#define WMN_MASK_ALL		0x00ff

/* Extract the notify type from an int or long which may have other data */
#define WMN_GET_VALUE( val )	( (val) & 0x00ff )

/* Notification categories */
#define WMN_CAT_SYSTEM		0x0001
#define WMN_CAT_WORKSPACE    	0x0002
#define WMN_CAT_APPLICATION    	0x0003
#define WMN_CAT_WINOP		0x0004

/* System notifications */
#define WMN_SYS_START		WMN_MAKE_ID( WMN_CAT_SYSTEM, 0x0001 )
#define WMN_SYS_QUIT		WMN_MAKE_ID( WMN_CAT_SYSTEM, 0x0002 )
#define WMN_SYS_RESTART      	WMN_MAKE_ID( WMN_CAT_SYSTEM, 0x0003 )
#define WMN_SYS_DEFLOAD		WMN_MAKE_ID( WMN_CAT_SYSTEM, 0x0004 )

/* Workspace notifications */
#define WMN_WS_CHANGE		WMN_MAKE_ID( WMN_CAT_WORKSPACE, 0x0001 )
#define WMN_WS_CREATE		WMN_MAKE_ID( WMN_CAT_WORKSPACE, 0x0002 )
#define WMN_WS_DESTROY		WMN_MAKE_ID( WMN_CAT_WORKSPACE, 0x0003 )
#define WMN_WS_RENAME		WMN_MAKE_ID( WMN_CAT_WORKSPACE, 0x0004 )

/* Application notifications */
#define WMN_APP_START		WMN_MAKE_ID( WMN_CAT_APPLICATION, 0x0001 )
#define WMN_APP_EXIT		WMN_MAKE_ID( WMN_CAT_APPLICATION, 0x0002 )

/* Window operation notifications */
#define WMN_WIN_MAXIMIZE	WMN_MAKE_ID( WMN_CAT_WINOP, 0x0001 )
#define WMN_WIN_UNMAXIMIZE	WMN_MAKE_ID( WMN_CAT_WINOP, 0x0002 )
#define WMN_WIN_SHADE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0003 )
#define WMN_WIN_UNSHADE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0004 )
#define WMN_WIN_ICONIFY		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0005 )
#define WMN_WIN_DEICONIFY	WMN_MAKE_ID( WMN_CAT_WINOP, 0x0006 )
#define WMN_WIN_MOVE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0007 )
#define WMN_WIN_RESIZE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0008 )
#define WMN_WIN_HIDE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x0009 )
#define WMN_WIN_UNHIDE		WMN_MAKE_ID( WMN_CAT_WINOP, 0x000a )
#define WMN_WIN_FOCUS		WMN_MAKE_ID( WMN_CAT_WINOP, 0x000a )
#define WMN_WIN_UNFOCUS		WMN_MAKE_ID( WMN_CAT_WINOP, 0x000b )


#endif /*WMNOTDEF_H_*/
