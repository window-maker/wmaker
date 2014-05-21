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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "wraster.h"

RImage *RVerticalFlipImage(RImage *image)
{
	RImage *img;
	int nwidth, nheight;
	int x, y;
	int bpp = image->format == RRGBAFormat ? 4 : 3;

	nwidth = image->width;
	nheight = image->height;

	img = RCreateImage(nwidth, nheight, True);
	if (!img)
		return NULL;

	if (bpp == 3) {
			unsigned char *optr, *nptr;

			optr = image->data;
			nptr = img->data + 4 * (nwidth * nheight - nwidth - 1);

			for (y = 0; y < nheight; y++) {
				for (x = 0; x < nwidth; x++) {
					nptr[0] = optr[0];
					nptr[1] = optr[1];
					nptr[2] = optr[2];
					nptr[3] = 255;

					optr += 3;
					nptr += 4;
				}
				nptr -= nwidth*8;
			}
		} else {
			unsigned char *optr, *nptr;

			optr = image->data;
			nptr = img->data + 4 * (nwidth * nheight - nwidth - 1);

			for (y = 0; y < nheight; y++) {
				for (x = 0; x < nwidth; x++) {
					nptr[0] = optr[0];
					nptr[1] = optr[1];
					nptr[2] = optr[2];
					nptr[3] = optr[3];

					optr += 4;
					nptr += 4;
				}
				nptr -= nwidth*8;
			}
		}
	return img;
}

RImage *RHorizontalFlipImage(RImage *image)
{
	RImage *img;
	int nwidth, nheight;
	int x, y;
	int bpp = image->format == RRGBAFormat ? 4 : 3;

	nwidth = image->width;
	nheight = image->height;

	img = RCreateImage(nwidth, nheight, True);
	if (!img)
		return NULL;

	if (bpp == 3) {
			unsigned char *optr, *nptr;

			nptr = img->data + nwidth * nheight * 4 - 4;
			for (y = nheight; y; y--) {
				for (x = 0; x < nwidth; x++) {
					optr = image->data + (y*nwidth + x) * 3;

					nptr[0] = optr[0];
					nptr[1] = optr[1];
					nptr[2] = optr[2];
					nptr[3] = 255;

					nptr -= 4;
				}
			}
		} else {
			unsigned char *optr, *nptr;

			nptr = img->data + nwidth * nheight * 4 - 4;
			for (y = nheight; y; y--) {
				for (x = 0; x < nwidth; x++) {
					optr = image->data + (y*nwidth + x) * 4;

					nptr[0] = optr[0];
					nptr[1] = optr[1];
					nptr[2] = optr[2];
					nptr[3] = optr[3];

					nptr -= 4;
				}
			}
		}
	return img;
}
