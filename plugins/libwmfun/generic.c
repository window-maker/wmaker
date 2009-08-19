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
 * Revision 1.1  2000/12/03 18:58:41  id
 * initiate plugins branch
 *
 * Revision 1.1.1.1  1999/02/21 17:16:47  gloth
 * initial revision
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "generic.h"

static Display *dpy = 0;
static Colormap cmap;

void initWindowMaker(Display * d, Colormap c)
{
	if (!d) {
		return;
	}
	dpy = d;
	cmap = c;
}

void error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int parse_color(const char *string, int *result)
{
	XColor color;

	if (!XParseColor(dpy, cmap, string, &color)) {
		return 0;
	}
	result[0] = color.red >> 8;
	result[1] = color.green >> 8;
	result[2] = color.blue >> 8;

	return 1;
}

int parse_xcolor(const char *string, XColor * xcolor)
{
	if (!XParseColor(dpy, cmap, string, xcolor)) {
		return 0;
	}
	if (!XAllocColor(dpy, cmap, xcolor)) {
		return 0;
	}

	return 1;
}

void interpolate_color(int *t, const int *s0, const int *s1, int mix)
{
	t[0] = (mix * s0[0] + (255 - mix) * s1[0]) >> 8;
	t[1] = (mix * s0[1] + (255 - mix) * s1[1]) >> 8;
	t[2] = (mix * s0[2] + (255 - mix) * s1[2]) >> 8;
}

int random_int(int range)
{
	return rand() / (RAND_MAX / range + 1);
}

double random_double(double range)
{
	return ((double)rand()) / (((double)RAND_MAX) / range);
}

int start_image(const char *name, int argc, int argc_min, int argc_max, int width, int height, RImage ** image)
{

	if (!dpy) {
		error("%s: library not initialized\n", name);
		return 0;
	}

	if ((argc < argc_min) || (argc >= argc_max)) {
		error("%s: invalid number of parameters: \n", name, argc);
		return 0;
	}

	*image = RCreateImage(width, height, 0);
	if (!*image) {
		error("%s: can't create image\n", name);
		return 0;
	}
	return 1;
}
