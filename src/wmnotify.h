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

#ifndef WMNOTIFY_H_
#define WMNOTIFY_H_

#ifndef WMNOTIFY

#define wNotify( id, a1, a2 )
#define wNotifyWin( id, wwin )

#else  /* WMNOTIFY */
#include "wmnotdef.h"

void wNotify( int id, int a1, int a2 );
void wNotifyWin( int id, WWindow *wwin );
int wNotifySet(Window window);
int wNotifyClear(Window window);

#endif /* WMNOTIFY */

#endif /*WMNOTIFY_H_*/
