/*
 *  Window Maker Sound Server
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

#ifndef WSOUND_H_
#define WSOUND_H_

#define WSOUND_SHADE       1001
#define WSOUND_UNSHADE     1002
#define WSOUND_MAXIMIZE    1003
#define WSOUND_UNMAXIMIZE  1004
#define WSOUND_ICONIFY     1005
#define WSOUND_DEICONIFY   1006
#define WSOUND_HIDE        1007
#define WSOUND_UNHIDE      1008
#define WSOUND_APPSTART    1009
#define WSOUND_APPEXIT     1010
#define WSOUND_DOCK        1011
#define WSOUND_UNDOCK      1012
#define WSOUND_KABOOM      1013


void wSoundPlay(long event_sound);
void wSoundServerGrab(Window wm_win);

#endif /*WSOUND_H_*/
