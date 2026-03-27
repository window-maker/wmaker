/* randr.h - RandR multi-monitor support
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 2026 Window Maker Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _WMRANDR_H_
#define _WMRANDR_H_

#ifdef USE_RANDR

#include <X11/extensions/Xrandr.h>
#include "screen.h"

void wRandRInit(WScreen *scr);
void wRandRTeardown(WScreen *scr);
void wRandRHandleNotify(WScreen *scr, XEvent *event);

#endif /* USE_RANDR */

#endif /* _WMRANDR_H_ */
