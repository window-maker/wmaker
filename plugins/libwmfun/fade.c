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

#include "getopt.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "generic.h"

RImage *fade (int argc, char **argv, int width, int height, int relief) {

	int from[3] = { 0x00, 0x00, 0x00 };
	int to[3] = { 0xff, 0xff, 0xff };
	int *this, *last;
	RImage *image;
	unsigned char *cptr;
	int i, j;
	double factor, delta;

	int c, done, option_index = 0;

	optind = 1;
	for (done=0; !done; ) {
		static struct option long_options[] = {
			{"from", 1, 0, 'f'},
			{"to", 1, 0, 't'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "f:t:",
			long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'f':
			if (!parse_color (optarg, from)) {
				error ("invalid color: %s\n", optarg);
				return 0;
			}
			break;
		case 't':
			if (!parse_color (optarg, to)) {
				error ("invalid color: %s\n", optarg);
				return 0;
			}
			break;
		default:
			done = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (!start_image ("fade", argc, 0, 1, width, height, &image)) {
		return (RImage *)0;
	}

	this = (int *) malloc (width * sizeof (int));
	last = (int *) malloc (width * sizeof (int));
	if (!this || !last) {
		RReleaseImage (image);
		free (this);
		free (last);
		return (RImage *)0;
	}

	for (i=0; i<width; i++) {
		this[i] = 255;
	}

	factor = pow (0.2, 1.0 / height);
	delta = (factor < 0.5) ? 2.0 * factor : 2.0 * (1.0 - factor);

	srand (time (0));

	cptr = image->data;
	for (j=0; j<height; j++) {
		memcpy (last, this, width * sizeof (int));
		for (i=0; i<width; i++) {
			int output[3];
			int k = i + random_int (3) - 1;
			double f = factor + random_double (delta) - delta/2.0;

			if (k < 0) {
				k = 0;
			}
			if (k >= width) {
				k = width - 1;
			}

			this[i] = (int) (f * last[k]);
			interpolate_color (output, from, to, this[i]);
			*cptr++ = output[0];
			*cptr++ = output[1];
			*cptr++ = output[2];
			if (RRGBAFormat==image->format)
				cptr++;
		}
	}

	free (this);
	free (last);

	return image;
}

