/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

/* anywhere */
#define WKBD_ROOTMENU		0
#define WKBD_WINDOWMENU		1
#define WKBD_WINDOWLIST		2
/* window */
#define WKBD_MINIATURIZE	3
#define WKBD_HIDE		4
#define WKBD_MAXIMIZE		5
#define WKBD_VMAXIMIZE		6
#define WKBD_SELECT		7
/* Clip */
#define WKBD_CLIPLOWER		8
#define WKBD_CLIPRAISE		9
#define WKBD_CLIPRAISELOWER	10
/* window, menu */
#define WKBD_RAISE		11
#define WKBD_LOWER		12
#define WKBD_RAISELOWER		13
/* window */
#define WKBD_SHADE		14
/* window, menu */
#define WKBD_CLOSE		15
/* window */
#define WKBD_FOCUSNEXT		16
#define WKBD_FOCUSPREV		17

#define WKBD_WORKSPACE1		20
#define WKBD_WORKSPACE2		21
#define WKBD_WORKSPACE3		22
#define WKBD_WORKSPACE4		23
#define WKBD_WORKSPACE5		24
#define WKBD_WORKSPACE6		25
#define WKBD_WORKSPACE7		26
#define WKBD_WORKSPACE8		27
#define WKBD_WORKSPACE9		28
#define WKBD_WORKSPACE10	29
#define WKBD_NEXTWORKSPACE	30
#define WKBD_PREVWORKSPACE	31
#define WKBD_NEXTWSLAYER	32
#define WKBD_PREVWSLAYER	33

#define WKBD_LAST               34


typedef struct WShortKey {
    unsigned int modifier;
    KeyCode keycode;
} WShortKey;

void wKeyboardInitialize();
