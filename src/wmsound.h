/*
 *  WMSound Server
 * 
 *  Copyright (c) 1997 Anthony P. Quinn
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

#ifndef WMSOUND_H_
#define WMSOUND_H_

#define WMSOUND_SHADE       1001
#define WMSOUND_UNSHADE     1002
#define WMSOUND_MAXIMIZE    1003
#define WMSOUND_UNMAXIMIZE  1004
#define WMSOUND_ICONIFY     1005
#define WMSOUND_DEICONIFY   1006
#define WMSOUND_HIDE        1007
#define WMSOUND_UNHIDE      1008
#define WMSOUND_APPSTART    1009
#define WMSOUND_APPEXIT     1010
#define WMSOUND_DOCK        1011
#define WMSOUND_UNDOCK      1012
#define WMSOUND_KABOOM      1013

#if 0
/* don't delete this */
extern WWindow *wSoundServer;
extern Atom WSStartup, WSShade, WSUnshade, WSShutdown;
extern Atom WSMaximize, WSUnmaximize, WSIconify, WSUniconify, WSAppStart;
extern Atom WSHide, WSUnhide, WSAppExit, WSDock, WSUnDock, WSKaboom;
extern WApplication *wSoundApp;
#endif


void wSoundPlay(long event_sound);
void wSoundServerGrab(Window wm_win);

#endif /*WMSOUND_H_*/
