/*
 *  WindowMaker window manager
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

#ifndef WMACTIONS_H_
#define WMACTIONS_H_

#include "window.h"

#define MAX_HORIZONTAL 	1
#define MAX_VERTICAL 	2


void wSetFocusTo(WScreen *scr, WWindow *wwin);

int wMouseMoveWindow(WWindow *wwin, XEvent *ev);

void wMouseResizeWindow(WWindow *wwin, XEvent *ev);

void wShadeWindow(WWindow *wwin);
void wUnshadeWindow(WWindow *wwin);

void wIconifyWindow(WWindow *wwin);
void wDeiconifyWindow(WWindow *wwin);

void wSelectWindows(WScreen *scr, XEvent *ev);
void wSelectWindow(WWindow *wwin);
void wUnselectWindows();

void wMaximizeWindow(WWindow *wwin, int directions);
void wUnmaximizeWindow(WWindow *wwin);

void wHideOtherApplications(WWindow *wwin);
void wShowAllWindows(WScreen *scr);

void wHideApplication(WApplication *wapp);
void wUnhideApplication(WApplication *wapp, Bool miniwindows, 
			Bool bringToCurrentWS);

void wRefreshDesktop(WScreen *scr);

void wArrangeIcons(WScreen *scr, Bool arrangeAll);



#endif
