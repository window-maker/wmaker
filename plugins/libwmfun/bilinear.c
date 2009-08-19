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

#include "generic.h"

RImage *bilinear(int argc, char **argv, int width, int height, int relief)
{

	int color[4][3];
	RImage *image;
	unsigned char *cptr;
	int i, j, k;

	argc--;
	argv++;

	if (!start_image("bilinear", argc, 4, 5, width, height, &image)) {
		return (RImage *) 0;
	}

	for (i = 0; i < 4; i++) {
		if (!parse_color(argv[i], color[i])) {
			error("can't parse color: \"%s\"\n", argv[i]);
			return 0;
		}
	}

	cptr = image->data;
	for (i = 0; i < height; i++) {
		int b = 0xff * i / height;
		int t = 0xff - b;

		for (j = 0; j < width; j++) {
			int r = 0xff * j / width;
			int l = 0xff - r;
			int f[4];

			f[0] = (l * t) >> 8;
			f[1] = (r * t) >> 8;
			f[2] = (l * b) >> 8;
			f[3] = (r * b) >> 8;

			for (k = 0; k < 3; k++) {
				*cptr++ =
				    (f[0] * color[0][k] +
				     f[1] * color[1][k] + f[2] * color[2][k] + f[3] * color[3][k]) >> 8;
			}

			if (RRGBAFormat == image->format)
				cptr++;
		}
	}
	return image;
}
