/* flip.c - image flip
 *
 * Raster graphics library
 *
 * Copyright (c) 2014 Window Maker Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>

#include "wraster.h"

RImage *RVerticalFlipImage(RImage *source)
{
	RImage *target;
	int nwidth, nheight;
	int x, y;

	nwidth = source->width;
	nheight = source->height;

	target = RCreateImage(nwidth, nheight, (source->format != RRGBFormat));
	if (!target)
		return NULL;

	if (source->format == RRGBFormat) {
		unsigned char *optr, *nptr;

		optr = source->data;
		nptr = target->data + 3 * (nwidth * nheight - nwidth);

		for (y = 0; y < nheight; y++) {
			for (x = 0; x < nwidth; x++) {
				nptr[0] = optr[0];
				nptr[1] = optr[1];
				nptr[2] = optr[2];

				optr += 3;
				nptr += 3;
			}
			nptr -= (nwidth * 3) * 2;
		}
	} else {
		unsigned char *optr, *nptr;

		optr = source->data;
		nptr = target->data + 4 * (nwidth * nheight - nwidth);

		for (y = 0; y < nheight; y++) {
			for (x = 0; x < nwidth; x++) {
				nptr[0] = optr[0];
				nptr[1] = optr[1];
				nptr[2] = optr[2];
				nptr[3] = optr[3];

				optr += 4;
				nptr += 4;
			}
			nptr -= (nwidth * 4) * 2;
		}
	}
	return target;
}

RImage *RHorizontalFlipImage(RImage *source)
{
	RImage *target;
	int nwidth, nheight;
	int x, y;

	nwidth = source->width;
	nheight = source->height;

	target = RCreateImage(nwidth, nheight, (source->format != RRGBFormat));
	if (!target)
		return NULL;

	if (source->format == RRGBFormat) {
		unsigned char *optr, *nptr;

		optr = source->data;
		nptr = target->data + 3 * (nwidth - 1);

		for (y = nheight; y; y--) {
			for (x = 0; x < nwidth; x++) {
				nptr[0] = optr[0];
				nptr[1] = optr[1];
				nptr[2] = optr[2];

				optr += 3;
				nptr -= 3;
			}
			nptr += (nwidth * 3) * 2;
		}
	} else {
		unsigned char *optr, *nptr;

		optr = source->data;
		nptr = target->data + 4 * (nwidth - 1);

		for (y = nheight; y; y--) {
			for (x = 0; x < nwidth; x++) {
				nptr[0] = optr[0];
				nptr[1] = optr[1];
				nptr[2] = optr[2];
				nptr[3] = optr[3];

				optr += 4;
				nptr -= 4;
			}
			nptr += (nwidth * 4) * 2;
		}
	}
	return target;
}
