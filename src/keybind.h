/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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


enum {
	/* anywhere */
	WKBD_ROOTMENU,
	WKBD_WINDOWMENU,
	WKBD_WINDOWLIST,

	/* window */
	WKBD_MINIATURIZE,
	WKBD_HIDE,
	WKBD_HIDE_OTHERS,
	WKBD_MAXIMIZE,
	WKBD_VMAXIMIZE,
	WKBD_HMAXIMIZE,
        WKBD_LHMAXIMIZE,
	WKBD_RHMAXIMIZE,
	WKBD_MAXIMUS,
	WKBD_SELECT,
	WKBD_RAISE,
	WKBD_LOWER,
	WKBD_RAISELOWER,
	WKBD_MOVERESIZE,
	WKBD_SHADE,
	WKBD_FOCUSNEXT,
	WKBD_FOCUSPREV,
	WKBD_GROUPNEXT,
	WKBD_GROUPPREV,

	/* window, menu */
	WKBD_CLOSE,

	/* Dock */
	WKBD_DOCKRAISELOWER,

	/* Clip */
	WKBD_CLIPLOWER,
	WKBD_CLIPRAISE,
	WKBD_CLIPRAISELOWER,

	/* workspace */
	WKBD_WORKSPACE1,
	WKBD_WORKSPACE2,
	WKBD_WORKSPACE3,
	WKBD_WORKSPACE4,
	WKBD_WORKSPACE5,
	WKBD_WORKSPACE6,
	WKBD_WORKSPACE7,
	WKBD_WORKSPACE8,
	WKBD_WORKSPACE9,
	WKBD_WORKSPACE10,
	WKBD_NEXTWORKSPACE,
	WKBD_PREVWORKSPACE,
	WKBD_NEXTWSLAYER,
	WKBD_PREVWSLAYER,

	/* window shortcuts */
	WKBD_WINDOW1,
	WKBD_WINDOW2,
	WKBD_WINDOW3,
	WKBD_WINDOW4,
	WKBD_WINDOW5,
	WKBD_WINDOW6,
	WKBD_WINDOW7,
	WKBD_WINDOW8,
	WKBD_WINDOW9,
	WKBD_WINDOW10,

	/* screen */
	WKBD_SWITCH_SCREEN,

#ifdef KEEP_XKB_LOCK_STATUS
	WKBD_TOGGLE,
#endif

#ifdef VIRTUAL_DESKTOP
	WKBD_VDESK_LEFT,
	WKBD_VDESK_RIGHT,
	WKBD_VDESK_UP,
	WKBD_VDESK_DOWN,
#endif
	/* keep this last */
	WKBD_LAST
};

typedef struct WShortKey {
    unsigned int modifier;
    KeyCode keycode;
} WShortKey;

void wKeyboardInitialize();
