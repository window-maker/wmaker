/* save_png.c - save PNG image
 *
 * Raster graphics library
 *
 * Copyright (c) 2023-2025 Window Maker Team
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
#include <assert.h>
#include <errno.h>
#include <png.h>

#include "wraster.h"
#include "imgformat.h"
#include "wr_i18n.h"

/* Structure to hold PNG data in memory */
struct png_mem_data {
	unsigned char *buffer;
	size_t size;
	size_t capacity;
};

/* Callback function to write PNG data to memory buffer */
static void png_write_to_memory(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct png_mem_data *p = (struct png_mem_data *)png_get_io_ptr(png_ptr);
	size_t new_size = p->size + length;

	/* Expand buffer if necessary */
	if (new_size > p->capacity) {
		size_t new_capacity = p->capacity ? p->capacity * 2 : 8192;
		while (new_capacity < new_size)
			new_capacity *= 2;

		unsigned char *new_buffer = realloc(p->buffer, new_capacity);
		if (!new_buffer) {
			png_error(png_ptr, "Out of memory");
			return;
		}
		p->buffer = new_buffer;
		p->capacity = new_capacity;
	}

	/* Copy data to buffer */
	memcpy(p->buffer + p->size, data, length);
	p->size += length;
}

/* Dummy flush function for memory I/O */
static void png_flush_memory(png_structp png_ptr)
{
	/* No-op for memory I/O */
	(void)png_ptr;
}

/*
 * Save RImage to PNG data in memory
 */
Bool RSaveRawPNG(RImage *img, char *title, unsigned char **out_buf, size_t *out_size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_bytep png_row;
	RColor pixel;
	int x, y;
	int width = img->width;
	int height = img->height;
	struct png_mem_data png_data = {NULL, 0, 0};

	*out_buf = NULL;
	*out_size = 0;

	/* Initialize write structure */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		RErrorCode = RERR_NOMEMORY;
		return False;
	}

	/* Initialize info structure */
	png_info_ptr = png_create_info_struct(png_ptr);
	if (png_info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, NULL);
		RErrorCode = RERR_NOMEMORY;
		return False;
	}

	/* Setup Exception handling */
	if (setjmp(png_jmpbuf(png_ptr))) {
		if (png_data.buffer)
			free(png_data.buffer);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		RErrorCode = RERR_INTERNAL;
		return False;
	}

	/* Set up memory I/O */
	png_set_write_fn(png_ptr, &png_data, png_write_to_memory, png_flush_memory);

	/* Write header (8 bit colour depth) */
	png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

	/* Set title if any */
	if (title) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = title;
		png_set_text(png_ptr, png_info_ptr, &title_text, 1);
	}

	png_write_info(png_ptr, png_info_ptr);

	/* Allocate memory for one row (3 bytes per pixel - RGB) */
	png_row = (png_bytep) malloc(3 * width * sizeof(png_byte));
	if (!png_row) {
		if (png_data.buffer)
			free(png_data.buffer);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		RErrorCode = RERR_NOMEMORY;
		return False;
	}

	/* Write image data */
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			png_byte *ptr;
			
			RGetPixel(img, x, y, &pixel);
			ptr = &(png_row[x * 3]);
			ptr[0] = pixel.red;
			ptr[1] = pixel.green;
			ptr[2] = pixel.blue;
		}
		png_write_row(png_ptr, png_row);
	}

	/* End write */
	png_write_end(png_ptr, NULL);

	/* Clean up structures */
	if (png_info_ptr != NULL)
		png_free_data(png_ptr, png_info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL)
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	if (png_row != NULL)
		free(png_row);

	/* Return the buffer */
	*out_buf = png_data.buffer;
	*out_size = png_data.size;

	return True;
}

/*
 * Save RImage to PNG image
 */
Bool RSavePNG(RImage *img, const char *filename, char *title)
{
	FILE *file;
	unsigned char *png_data = NULL;
	size_t png_size = 0;
	size_t written;

	if (!img || !filename) {
		RErrorCode = RERR_BADIMAGEFILE;
		return False;
	}

	/* Use RSaveRawPNG to generate PNG data in memory */
	if (!RSaveRawPNG(img, title, &png_data, &png_size)) {
		/* Error code already set by RSaveRawPNG */
		return False;
	}

	/* Open file for writing */
	file = fopen(filename, "wb");
	if (file == NULL) {
		free(png_data);
		RErrorCode = RERR_OPEN;
		return False;
	}

	/* Write PNG data to file */
	written = fwrite(png_data, 1, png_size, file);
	fclose(file);

	/* Check if all data was written */
	if (written != png_size) {
		free(png_data);
		RErrorCode = RERR_WRITE;
		return False;
	}

	/* Clean up */
	free(png_data);
	return True;
}
