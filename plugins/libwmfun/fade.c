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
 * Revision 1.2  2001/04/21 07:12:30  dan
 * For libwraster:
 * ---------------
 *
 * - Added retain/release mechanism to RImage by adding RRetainImage() and
 *   RReleaseImage(). RDestroyImage() is an alias to RReleaseImage() now, but
 *   will be removed in a future release because it no longer fits with the
 *   semantics. Will be kept for a while to allow a smoother transition.
 *   More about in wrlib/NEWS
 *
 *
 * For WINGs:
 * ----------
 *
 * - Small API change:
 *   1. Renamed WMSetApplicationIconImage(), WMGetApplicationIconImage() and
 *      WMSetWindowMiniwindowImage() to respectively WMSetApplicationIconPixmap(),
 *      WMGetApplicationIconPixmap() and WMSetWindowMiniwindowPixmap()
 *      They operate on a WMPixmap which is practically an X Pixmap with no alpha
 *      channel information and the new name is more suggestive and also leaves
 *      room for the new functions added for operating on images with alpha info.
 *   2. Added WMSetApplicationIconImage() and WMGetApplicationIconImage() which
 *      operate on an RImage and store alpha information too.
 *   3. Added WMGetApplicationIconBlendedPixmap() which will take the image with
 *      alpha set by WMSetApplicationIconImage() and will blend it with a color.
 *      If color is NULL it will blend using the default panel color (#aeaaae)
 *   All these changes will allow WINGs to handle images with alpha blending
 *   correctly in panels and wherever else needed. More about in WINGs/NEWS.
 * - updated panels to use the newly available RImages if present and fallback
 *   to old WMPixmaps if not, to properly show alpha blended images.
 * - replaced some still left malloc's with wmalloc's.
 *
 *
 * For Window Maker:
 * -----------------
 * - Fixed wrong mapping position of the "Docked Applications Panel" for some
 *   icons.
 * - Smoother animation for the smiley =)
 * - Made images with alpha blending be shown correctly in the panels and the
 *   icon chooser.
 * - The icon image set to be shown in panels ("Logo.WMPanel") will be
 *   automatically updated if its entry in WMWindowAttributes changes (without
 *   a need to restart as until now).
 *
 *
 * *** Note!!! ***
 *
 * If you are developing applications with one of libwraster or libWINGs
 * then you should look to wrlib/NEWS and WINGs/NEWS to see what changed
 * and how should you update your code.
 *
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

