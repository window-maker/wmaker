/* ppm.c - load PPM image from file
 *
 * Raster graphics library
 *
 * Copyright (c) 1997-2003 Alfredo K. Kojima
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

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wraster.h"
#include "imgformat.h"

static RImage *load_graymap(FILE *file, int w, int h, int max, int raw)
{
	RImage *image;
	unsigned char *ptr;
	char *buf;
	int x, y;

	image = RCreateImage(w, h, 0);
	if (!image)
		return NULL;

	if (!raw)
		return image;

	if (max < 256) {
		buf = malloc(w + 1);
		if (!buf)
			return NULL;

		ptr = image->data;
		for (y = 0; y < h; y++) {
			if (!fread(buf, w, 1, file)) {
				free(buf);
				RErrorCode = RERR_BADIMAGEFILE;
				return NULL;
			}

			for (x = 0; x < w; x++) {
				*(ptr++) = buf[x];
				*(ptr++) = buf[x];
				*(ptr++) = buf[x];
			}
		}
		free(buf);
	}

	return image;
}

static RImage *load_pixmap(FILE *file, int w, int h, int max, int raw)
{
	RImage *image;
	int i;
	char buf[3];
	unsigned char *ptr;

	image = RCreateImage(w, h, 0);
	if (!image)
		return NULL;

	if (!raw)
		return image;

	ptr = image->data;
	if (max < 256) {
		i = 0;
		while (i < w * h) {
			if (fread(buf, 1, 3, file) != 3) {
				RErrorCode = RERR_BADIMAGEFILE;
				return NULL;
			}

			*(ptr++) = buf[0];
			*(ptr++) = buf[1];
			*(ptr++) = buf[2];
			i++;
		}
	}

	return image;
}

RImage *RLoadPPM(const char *file_name)
{
	FILE *file;
	RImage *image = NULL;
	char buffer[256];
	int w, h, m;
	int type;

	file = fopen(file_name, "rb");
	if (!file) {
		RErrorCode = RERR_OPEN;
		return NULL;
	}

	/* get signature */
	if (!fgets(buffer, 255, file)) {
		RErrorCode = RERR_BADIMAGEFILE;
		fclose(file);
		return NULL;
	}

	/* only accept raw pixmaps or graymaps */
	if (buffer[0] != 'P' || (buffer[1] != '5' && buffer[1] != '6')) {
		RErrorCode = RERR_BADFORMAT;
		fclose(file);
		return NULL;
	}

	type = buffer[1];

	/* skip comments */
	while (1) {
		if (!fgets(buffer, 255, file)) {
			RErrorCode = RERR_BADIMAGEFILE;
			fclose(file);
			return NULL;
		}

		if (buffer[0] != '#')
			break;
	}

	/* get size */
	if (sscanf(buffer, "%i %i", &w, &h) != 2 || w < 1 || h < 1) {
		/* Short file */
		RErrorCode = RERR_BADIMAGEFILE;
		fclose(file);
		return NULL;
	}

	if (!fgets(buffer, 255, file)) {
		RErrorCode = RERR_BADIMAGEFILE;
		fclose(file);
		return NULL;
	}

	if (sscanf(buffer, "%i", &m) != 1 || m < 1) {
		/* Short file */
		RErrorCode = RERR_BADIMAGEFILE;
		fclose(file);
		return NULL;
	}

	if (type == '5')
		image = load_graymap(file, w, h, m, type == '5');
	else if (type == '6')
		image = load_pixmap(file, w, h, m, type == '6');

	fclose(file);
	return image;
}
