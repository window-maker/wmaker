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



/* anywhere */
#define WKBD_ROOTMENU		0
#define WKBD_WINDOWMENU		1
#define WKBD_WINDOWLIST		2
/* window */
#define WKBD_MINIATURIZE	3
#define WKBD_HIDE		4
#define WKBD_HIDE_OTHERS        5
#define WKBD_MAXIMIZE           6
#define WKBD_VMAXIMIZE          7
#define WKBD_HMAXIMIZE          8
#define WKBD_LHMAXIMIZE	        9
#define WKBD_RHMAXIMIZE	        10
#define WKBD_SELECT             11
/* Clip */
#define WKBD_CLIPLOWER          12
#define WKBD_CLIPRAISE		13
#define WKBD_CLIPRAISELOWER	14
/* window */
#define WKBD_RAISE		15
#define WKBD_LOWER		16
#define WKBD_RAISELOWER		17
#define WKBD_MOVERESIZE		18
#define WKBD_SHADE		19
/* window, menu */
#define WKBD_CLOSE		20
/* window */
#define WKBD_FOCUSNEXT		21
#define WKBD_FOCUSPREV          22

#define WKBD_WORKSPACE1		23
#define WKBD_WORKSPACE2		24
#define WKBD_WORKSPACE3		25
#define WKBD_WORKSPACE4		26
#define WKBD_WORKSPACE5		27
#define WKBD_WORKSPACE6		28
#define WKBD_WORKSPACE7		29
#define WKBD_WORKSPACE8		30
#define WKBD_WORKSPACE9		31
#define WKBD_WORKSPACE10	32
#define WKBD_NEXTWORKSPACE	33
#define WKBD_PREVWORKSPACE	34
#define WKBD_NEXTWSLAYER	35
#define WKBD_PREVWSLAYER	36

/* window shortcuts */
#define WKBD_WINDOW1		37
#define WKBD_WINDOW2		38
#define WKBD_WINDOW3		39
#define WKBD_WINDOW4		40
#define WKBD_WINDOW5		41
#define WKBD_WINDOW6		42
#define WKBD_WINDOW7		43
#define WKBD_WINDOW8		44
#define WKBD_WINDOW9		45
#define WKBD_WINDOW10		46

#define WKBD_SWITCH_SCREEN      47

#ifdef KEEP_XKB_LOCK_STATUS
# define WKBD_TOGGLE		48
# define WKBD_TMP		49
#else
# define WKBD_TMP		48
#endif

#ifdef VIRTUAL_DESKTOP
# define WKBD_VDESK_LEFT	WKBD_TMP
# define WKBD_VDESK_RIGHT	(WKBD_TMP+1)
# define WKBD_VDESK_UP		(WKBD_TMP+2)
# define WKBD_VDESK_DOWN	(WKBD_TMP+3)
# define WKBD_LAST		(WKBD_TMP+4)
#else
# define WKBD_LAST		WKBD_TMP
#endif /* VIRTUAL_DESKTOP */


typedef struct WShortKey {
    unsigned int modifier;
    KeyCode keycode;
} WShortKey;

void wKeyboardInitialize();
