/*
 *  libwmfun - WindowMaker texture function library
 *  Copyright (C) 1999 Tobias Gloth
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

/*
 * $Id$
 *
 * $Log$
 * Revision 1.3  2000/12/11 03:10:26  id
 * changes related to plugin system & drawstring
 *
 * Revision 1.2  2000/12/03 22:18:20  id
 * initiate
 *
 * Revision 1.1  2000/12/03 18:58:41  id
 * initiate plugins branch
 *
 * Revision 1.1.1.1  1999/02/21 17:16:47  gloth
 * initial revision
 *
 */
#include "config.h"

#ifndef _GENERIC_H
#define _GENERIC_H

#include <X11/Xlib.h>

#include <wraster.h>

typedef struct _WPluginData {
        int size;
            void **array;
} WPluginData;

extern void initWindowMaker (Display *d, Colormap c);
extern int start_image (const char *name, int argc, int argc_min, int argc_max,
	int width, int height, RImage **image);
extern void error (const char *format, ...);

extern int parse_color (const char *string, int *result);
extern void interpolate_color (int *t, const int *s0, const int *s1, int mix);

extern int random_int (int range);
extern double random_double (double range);


#endif
