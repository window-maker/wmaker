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

#include "getopt.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "generic.h"

RImage *waves (int argc, char **argv, int width, int height, int relief) {

	int i, j, k, done, sine [256];
	int from[3] = { 0x00, 0x00, 0x00 };
	int to[3] = { 0xff, 0xff, 0xff };
	int layers, range, dx[1000], dy[1000];
	RImage *image;
	unsigned char *cptr;

	int option_index = 0;
	int c;

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

	if (!start_image ("waves", argc, 2, 3, width, height, &image)) {
		return (RImage *)0;
	}

	layers = atoi (argv[0]);
	range = atoi (argv[1]);

	if (layers <= 0) {
		layers = 1;
	}
	if (layers > 256) {
		layers = 256;
	}

	if (range <= 0) {
		range = 1;
	}

	for (i=0; i<256; i++) {
		sine[i] = (int) (127.0 * sin (2.0 * M_PI * i / 256)) + 128;
	}

	srand (time (0));

	for (i=0; i<layers; i++) {
		dx[i] = random_int (range) - range / 2;
		dy[i] = random_int (range) - range / 2;
	}

	cptr = image->data;
	for (i=0; i<height; i++) {
		for (j=0; j<width; j++) {
			int output[3], value = 0;
			for (k=0; k<layers; k++) {
				value += sine[(j*dx[k]+i*dy[k]) & 255];
			}
			interpolate_color (output, from, to, value / layers);
			*cptr++ = output[0];
			*cptr++ = output[1];
			*cptr++ = output[2];
			if ( RRGBAFormat==image->format )
				cptr++;
		}
	}

	return image;
}

