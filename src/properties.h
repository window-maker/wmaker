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


#ifndef WMPROPERTIES_H_
#define WMPROPERTIES_H_

#include "GNUstep.h"

#ifdef MWM_HINTS
# include "motif.h"
#endif


int PropGetNormalHints(Window window, XSizeHints *size_hints, int *pre_iccm);
void PropGetProtocols(Window window, WProtocols *prots);
int PropGetWMClass(Window window, char **wm_class, char **wm_instance);
int PropGetGNUstepWMAttr(Window window, GNUstepWMAttributes **attr);
void PropWriteGNUstepWMAttr(Window window, GNUstepWMAttributes *attr);

void PropSetWMakerProtocols(Window root);
void PropCleanUp(Window root);

#ifdef MWM_HINTS
int PropGetMotifWMHints(Window window, MWMHints **mwmhints);
#endif

Window PropGetClientLeader(Window window);

#ifdef R6SM
char *PropGetClientID(Window window);
#endif

#endif
