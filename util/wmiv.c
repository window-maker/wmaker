/*
 *  Window Maker window manager
 *
 *  Copyright (c) 2014-2026 Window Maker Team - David Maciejak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <WINGs/WINGsP.h>
#include <wraster.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <ftw.h>
#include <libgen.h>
#include "config.h"
#include "xdnd.h"

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#ifdef USE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

#ifdef USE_XPM
extern int XpmCreatePixmapFromData(Display *, Drawable, char **, Pixmap *, Pixmap *, void *);
/*	This is the icon from the eog project git.gnome.org/browse/eog */
#include "wmiv.h"
#endif

#ifdef DEBUG
#define WMIV_DEBUG 1
#else
#define WMIV_DEBUG 0
#endif

#define FILE_SEPARATOR '/'

/* CHUNK size for INCR (bytes). Tune if desired. */
#define CHUNK_SIZE 65536

Display *dpy;
Window win;
RContext *ctx;
RImage *img;
Pixmap pix;

/* DND support for sending drops */
static DndClass send_dnd;
static Bool dnd_initialized = False;
static Bool button1_pressed = False;
static int drag_start_x, drag_start_y;
static int drag_threshold = 5; /* pixels */

const char *APPNAME = "wmiv";
int NEXT = 0;
int PREV = 1;
float zoom_factor = 0;
int max_width = 0;
int max_height = 0;

/* Archive support */
static char *temp_dir = NULL;
static Bool is_archive = False;

Bool fullscreen_flag = False;
Bool focus = True;
Bool back_from_fullscreen = False;
Bool ignore_unknown_file_format = False;

#ifdef HAVE_PTHREAD
Bool slideshow_flag = False;
int slideshow_delay = 5;
pthread_t tid = 0;
#endif
XTextProperty title_property;
XTextProperty icon_property;
unsigned current_index = 1;
unsigned max_index = 1;

RColor lightGray;
RColor darkGray;
RColor black;
RColor red;

/* Structure to hold the frame extents */
typedef struct {
	unsigned long left;
	unsigned long right;
	unsigned long top;
	unsigned long bottom;
} FrameExtents;

FrameExtents extents = {0, 0, 0, 0};

typedef struct link link_t;
struct link {
	const void *data;
	link_t *prev;
	link_t *next;
};

typedef struct linked_list {
	int count;
	link_t *first;
	link_t *last;
} linked_list_t;

linked_list_t list;
link_t *current_link;

/* --- Clipboard/PNG + JPEG + INCR globals --- */
static Atom clipboard_atom = None;
static Atom targets_atom = None;
static Atom png_atom = None;
static Atom jpeg_atom = None;
static Atom property_atom = None;
static Atom incr_atom = None;

static unsigned char *clipboard_png_data = NULL;
static size_t clipboard_png_size = 0;
static unsigned char *clipboard_jpeg_data = NULL;
static size_t clipboard_jpeg_size = 0;
static Bool we_own_clipboard = False;

/* INCR transfer state */
typedef struct {
	Bool active;
	Window requestor;
	Atom property;      /* property name the requestor asked for (req->property) */
	Atom target;        /* requested target (png_atom or jpeg_atom) */
	size_t total_size;
	size_t offset;
	size_t chunk_size;
	long saved_event_mask; /* to restore previous event mask on requestor window */
	unsigned char *data; /* pointer to either PNG or JPEG data */
} incr_transfer_t;

static incr_transfer_t incr = { .active = False };

/* End clipboard globals */

/*
	Wait for the window manager to set the _NET_FRAME_EXTENTS property
	by listening for PropertyNotify events
	Returns 1 on success, 0 on timeout
*/
int wait_for_frame_extents(Display *disp, Window win, int timeout_ms) {
	Atom net_frame_extents = XInternAtom(disp, "_NET_FRAME_EXTENTS", False);
	if (net_frame_extents == None)
		return 0;

	XEvent event;
	int elapsed = 0;
	const int poll_interval = 10; /* milliseconds */

	while (elapsed < timeout_ms) {
		/* Check if property is already available */
		Atom actual_type;
		int actual_format;
		unsigned long num_items;
		unsigned long bytes_after;
		unsigned char *prop_return = NULL;

		int status = XGetWindowProperty(disp, win, net_frame_extents, 0L, 1L, False, XA_CARDINAL,
				&actual_type, &actual_format, &num_items, &bytes_after, &prop_return);

		if (status == Success && actual_type == XA_CARDINAL && num_items >= 1) {
			if (prop_return)
				XFree(prop_return);
			return 1;
		}
		if (prop_return) XFree(prop_return);

		/* Wait for events or timeout */
		if (XPending(disp)) {
			XNextEvent(disp, &event);
			if (event.type == PropertyNotify &&
				event.xproperty.window == win &&
				event.xproperty.atom == net_frame_extents &&
				event.xproperty.state == PropertyNewValue) {
				return 1;
			}
		}

		usleep(poll_interval * 1000);
		elapsed += poll_interval;
		XFlush(disp);
	}

	return 0;
}

/*
	Get the frame extents (decorations) of a window
	Returns 1 on success, 0 on failure
*/
int get_window_decor_size(Display *disp, Window win, FrameExtents *extents) {
	Atom actual_type;
	int actual_format;
	unsigned long num_items;
	unsigned long bytes_after;
	unsigned char *prop_return = NULL;
	Atom net_frame_extents;
	int status;

	/* Get the atom for the _NET_FRAME_EXTENTS property */
	net_frame_extents = XInternAtom(disp, "_NET_FRAME_EXTENTS", False);
	if (net_frame_extents == None) {
		fprintf(stderr, "Warning: _NET_FRAME_EXTENTS atom not supported\n");
		return 0;
	}

	/* Retrieve the property value */
	status = XGetWindowProperty(disp, win, net_frame_extents, 0L, 4L, False, XA_CARDINAL,
				&actual_type, &actual_format, &num_items, &bytes_after, &prop_return);

	if (status == Success && actual_type == XA_CARDINAL && actual_format == 32 && num_items == 4) {
		unsigned long *extents_data = (unsigned long *)prop_return;
		extents->left = extents_data[0];
		extents->right = extents_data[1];
		extents->top = extents_data[2];
		extents->bottom = extents_data[3];

		if (WMIV_DEBUG) {
			fprintf(stderr, "Successfully got frame extents: left=%lu, right=%lu, top=%lu, bottom=%lu\n",
				extents->left, extents->right, extents->top, extents->bottom);
		}

		XFree(prop_return);
		return 1;
	}

	if (prop_return)
		XFree(prop_return);

	if (status != Success) {
		if (WMIV_DEBUG) {
			fprintf(stderr, "Error: XGetWindowProperty failed with status: %d\n", status);
		}
	} else if (actual_type != XA_CARDINAL) {
		if (WMIV_DEBUG) {
			fprintf(stderr, "Error: wrong property type: got %lu, expected %lu (XA_CARDINAL)\n",
				(unsigned long)actual_type, (unsigned long)XA_CARDINAL);
		}
	} else if (actual_format != 32) {
		if (WMIV_DEBUG) {
			fprintf(stderr, "Error: wrong property format: got %d, expected 32\n", actual_format);
		}
	} else if (num_items != 4) {
		if (WMIV_DEBUG) {
			fprintf(stderr, "Error: wrong number of items: got %lu, expected 4\n", num_items);
		}
	}

	return 0;
}

#ifdef HAVE_LIBARCHIVE
/*
	Create directory path recursively
	Returns 0 on success, -1 on failure
*/
static int create_directories(const char *path)
{
	char *path_copy = strdup(path);
	char *p;
	int result = 0;

	if (!path_copy)
		return -1;

	/* Skip leading slash if present */
	p = path_copy;
	if (*p == '/')
		p++;

	/* Create each directory component */
	while ((p = strchr(p, '/')) != NULL) {
		*p = '\0';
		if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
			result = -1;
			break;
		}
		*p = '/';
		p++;
	}

	/* Create final directory component */
	if (result == 0 && mkdir(path_copy, 0755) != 0 && errno != EEXIST)
		result = -1;

	free(path_copy);
	return result;
}
#endif

/*
	Callback function for nftw to remove files and directories
*/
static int remove_temp_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	(void)sb;
	(void)typeflag;
	(void)ftwbuf;

	return remove(fpath);
}

/*
	Remove the temporary directory and its contents
*/
static void cleanup_temp_dir(void)
{
	if (temp_dir) {
		if (WMIV_DEBUG)
			fprintf(stderr, "Cleaning up temporary directory: %s\n", temp_dir);
		nftw(temp_dir, remove_temp_file, 64, FTW_DEPTH | FTW_PHYS);
		free(temp_dir);
		temp_dir = NULL;
	}
	is_archive = False;
}

/*
	Check if file has archive extension
	Returns True if it is an archive, False otherwise
*/
static Bool is_archive_file(const char *filename)
{
	const char *ext = strrchr(filename, '.');

	return (ext && (strcasecmp(ext, ".cbz") == 0 ||
					strcasecmp(ext, ".zip") == 0 ||
					strcasecmp(ext, ".cbr") == 0 ||
					strcasecmp(ext, ".rar") == 0 ||
					strcasecmp(ext, ".cbt") == 0 ||
					strcasecmp(ext, ".cb7") == 0 ||
					strcasecmp(ext, ".tar") == 0 ||
					strcasecmp(ext, ".7z") == 0 ||
					strcasecmp(ext, ".tar.gz") == 0 ||
					strcasecmp(ext, ".tar.bz2") == 0));
}

#ifdef HAVE_LIBARCHIVE

/*
	Recursively find directory containing image files
	Returns path to directory with images, or NULL if not found
*/
static char *find_image_directory(const char *start_dir)
{
	DIR *dir;
	struct dirent *entry;
	struct stat st;
	char full_path[PATH_MAX];
	int image_count = 0;
	char *result = NULL;

	dir = opendir(start_dir);
	if (!dir)
		return NULL;


	/* First pass: count image files in current directory */
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		snprintf(full_path, sizeof(full_path), "%s/%s", start_dir, entry->d_name);
		if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
			image_count++;
	}
	closedir(dir);

	/* If current directory has images, return it */
	if (image_count > 0) {
		return strdup(start_dir);
	}

	/* Second pass: recursively check subdirectories */
	dir = opendir(start_dir);
	if (!dir)
		return NULL;

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		snprintf(full_path, sizeof(full_path), "%s/%s", start_dir, entry->d_name);
		if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
			result = find_image_directory(full_path);
			if (result)
				break;
		}
	}
	closedir(dir);
	return result;
}

/*
	Extract archive using libarchive
	Returns 0 on success, -1 on failure
*/
static int extract_archive_libarchive(const char *filename, const char *dest_dir)
{
	struct archive *a;
	struct archive_entry *entry;
	char dest_path[PATH_MAX];
	char *buffer = NULL;
	const size_t buffer_size = 8192;
	int r;

	/* Create archive reader */
	a = archive_read_new();
	if (!a) {
		fprintf(stderr, "Error: failed to create archive reader\n");
		return -1;
	}

	/* Enable support for all formats and filters */
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	/* Open the archive file */
	r = archive_read_open_filename(a, filename, 10240);
	if (r != ARCHIVE_OK) {
		fprintf(stderr, "Error: failed to open archive: %s\n", filename);
		archive_read_free(a);
		return -1;
	}

	buffer = malloc(buffer_size);
	if (!buffer) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		archive_read_free(a);
		return -1;
	}

	/* Extract each entry */
	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
		const char *entry_name = archive_entry_pathname(entry);
		la_int64_t entry_size = archive_entry_size(entry);
		mode_t entry_mode = archive_entry_mode(entry);
		FILE *output_file;
		la_ssize_t bytes_read;

		if (!entry_name)
			continue;

		/* Skip directories and special files */
		if (!S_ISREG(entry_mode) || entry_size <= 0)
			continue;

		/* Create destination path */
		snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry_name);

		/* Create directory structure if needed */
		char *dir_end = strrchr(dest_path, '/');
		if (dir_end) {
			*dir_end = '\0';
			if (create_directories(dest_path) != 0) {
				if (WMIV_DEBUG)
					fprintf(stderr, "Warning: failed to create directory: %s\n", dest_path);
			}
			*dir_end = '/';
		}

		/* Create output file */
		output_file = fopen(dest_path, "wb");
		if (!output_file) {
			if (WMIV_DEBUG)
				fprintf(stderr, "Warning: failed to create file: %s\n", dest_path);
			continue;
		}

		/* Extract file content */
		while ((bytes_read = archive_read_data(a, buffer, buffer_size)) > 0) {
			fwrite(buffer, 1, bytes_read, output_file);
		}

		if (bytes_read < 0) {
			if (WMIV_DEBUG)
				fprintf(stderr, "Warning: failed to read data for: %s\n", entry_name);
		}

		fclose(output_file);

		//if (WMIV_DEBUG)
		//	fprintf(stderr, "Extracted %s\n", entry_name);
	}

	if (r != ARCHIVE_EOF)
		fprintf(stderr, "Warning: archive reading ended with error\n");

	free(buffer);
	archive_read_free(a);

	return 0;
}
#endif

/*
	Extract archive to temporary directory
	Returns path to extracted directory on success, NULL on failure
*/
static char *extract_archive(const char *filename)
{
#ifdef HAVE_LIBARCHIVE
	char *temp_template = NULL;
	int result;

	/* Create temporary directory template */
	if (asprintf(&temp_template, "/tmp/wmiv_XXXXXX") == -1) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		return NULL;
	}

	/* Create temporary directory */
	temp_dir = mkdtemp(temp_template);
	if (!temp_dir) {
		perror("mkdtemp");
		free(temp_template);
		return NULL;
	}

	if (WMIV_DEBUG)
		fprintf(stderr, "Created temporary directory: %s\n", temp_dir);

	/* Extract the archive */
	if (is_archive_file(filename)) {
		result = extract_archive_libarchive(filename, temp_dir);
	} else {
		fprintf(stderr, "Error: unknown archive type\n");
		cleanup_temp_dir();
		return NULL;
	}

	if (result != 0) {
		cleanup_temp_dir();
		return NULL;
	}

	/* Check if extraction resulted in nested directories - find the one with actual images */
	char *image_dir = find_image_directory(temp_dir);
	if (image_dir && strcmp(image_dir, temp_dir) != 0) {
		if (WMIV_DEBUG)
			fprintf(stderr, "Archive contains nested directories, using image directory: %s\n", image_dir);

		char *result_dir = strdup(image_dir);
		free(image_dir);
		is_archive = True;
		return result_dir;
	}

	if (image_dir)
		free(image_dir);

	is_archive = True;
	return strdup(temp_dir);
#else
	(void) filename;
	fprintf(stderr, "Error: archive support not available\n");

	return NULL;
#endif
}

/*
	Convert current RImage to specified format using RSaveRawImage
	Returns 0 on success (buffer & size filled), -1 on failure
*/
static int convert_image_to_format(RImage *image, const char *format, unsigned char **out_buf, size_t *out_size)
{
	if (!image || !format || !out_buf || !out_size) {
		if (WMIV_DEBUG)
			fprintf(stderr, "Error: convert_image_to_format invalid parameters\n");
		return -1;
	}

	if (!RSaveRawImage(image, format, out_buf, out_size)) {
		if (WMIV_DEBUG)
			fprintf(stderr, "Error: RSaveRawImage failed for %s, RErrorCode=%d\n", format, RErrorCode);
		return -1;
	}

	return 0;
}

/*
	Free current clipboard data (both PNG and JPEG)
*/
static void free_clipboard_data(void)
{
	if (clipboard_png_data) {
		free(clipboard_png_data);
		clipboard_png_data = NULL;
	}
	clipboard_png_size = 0;
	if (clipboard_jpeg_data) {
		free(clipboard_jpeg_data);
		clipboard_jpeg_data = NULL;
	}
	clipboard_jpeg_size = 0;
}

/*
	Send the next chunk for INCR transfer (called when requestor deletes the property)
*/
static void send_next_incr_chunk(void)
{
	size_t remaining, to_send;

	if (!incr.active)
		return;

	if (!incr.data || incr.total_size == 0) {
		/* Nothing to send: finish with zero-length property */
		XChangeProperty(dpy, incr.requestor, incr.property, incr.target, 8, PropModeReplace, NULL, 0);
		/* Clean up: remove PropertyChangeMask from requestor window */
		XSelectInput(dpy, incr.requestor, NoEventMask);
		incr.active = False;
		we_own_clipboard = False;
		return;
	}

	remaining = incr.total_size - incr.offset;
	to_send = remaining;

	if (to_send > incr.chunk_size)
		to_send = incr.chunk_size;

	XChangeProperty(dpy, incr.requestor, incr.property, incr.target, 8, PropModeReplace,
				incr.data + incr.offset, (int)to_send);
	XFlush(dpy);

	incr.offset += to_send;

	if (incr.offset >= incr.total_size) {
		/* finished: next deletion by client -> we'll send zero-length and finish */
		if (WMIV_DEBUG)
			fprintf(stderr, "INCR: all data sent (%zu bytes), awaiting final property deletion\n", incr.total_size);
	} else {
		if (WMIV_DEBUG)
			fprintf(stderr, "INCR: sent chunk %zu/%zu\n", incr.offset, incr.total_size);
	}
}

/*
	Called when Ctrl+C pressed: encode current image to PNG and JPEG and claim CLIPBOARD
*/
static void copy_current_image_to_clipboard(void)
{
	if (!dpy || !win || !img) return;

	/* free previous */
	free_clipboard_data();

	if (WMIV_DEBUG)
		fprintf(stderr, "Attempting to copy image to clipboard (size: %dx%d)\n", img->width, img->height);

	/* Convert image to both formats */
#ifdef USE_PNG
	if (convert_image_to_format(img, "PNG", &clipboard_png_data, &clipboard_png_size) != 0) {
		fprintf(stderr, "Error: failed to convert image to PNG\n");
		return;
	}
#endif
#ifdef USE_JPEG
	if (convert_image_to_format(img, "JPEG", &clipboard_jpeg_data, &clipboard_jpeg_size) != 0) {
		fprintf(stderr, "Error: failed to convert image to JPEG\n");
		free_clipboard_data();
		return;
	}
#endif
	if (clipboard_png_data == NULL && clipboard_jpeg_data == NULL) {
		fprintf(stderr, "Error: no image formats available to copy to the clipboard\n");
		return;
	}

	/* claim CLIPBOARD */
	XSetSelectionOwner(dpy, clipboard_atom, win, CurrentTime);
	if (XGetSelectionOwner(dpy, clipboard_atom) != win) {
		fprintf(stderr, "Error: failed to own clipboard selection\n");
		free_clipboard_data();
		we_own_clipboard = False;
		return;
	}
	we_own_clipboard = True;
	XFlush(dpy);
	if (WMIV_DEBUG)
		fprintf(stderr, "Copied image to clipboard (PNG: %zu bytes, JPEG: %zu bytes)\n", clipboard_png_size, clipboard_jpeg_size);
}

/*
	Handle SelectionRequest events: serve TARGETS and image formats (with INCR when needed)
*/
static void handle_selection_request(XSelectionRequestEvent *req)
{
	if (WMIV_DEBUG) {
		char *target_name = XGetAtomName(req->display, req->target);
		char *selection_name = XGetAtomName(req->display, req->selection);

		fprintf(stderr, "SelectionRequest received: target=%s, selection=%s\n",
				target_name ? target_name : "unknown",
				selection_name ? selection_name : "unknown");
		if (target_name)
			XFree(target_name);
		if (selection_name)
			XFree(selection_name);
	}

	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.xselection.type = SelectionNotify;
	ev.xselection.display = req->display;
	ev.xselection.requestor = req->requestor;
	ev.xselection.selection = req->selection;
	ev.xselection.target = req->target;
	ev.xselection.time = req->time;
	ev.xselection.property = None;

	if (req->target == targets_atom) {
		/* advertise supported targets (image/png and image/jpeg) */
#if defined(USE_PNG) && defined(USE_JPEG)
		Atom types[2];
		types[0] = png_atom;
		types[1] = jpeg_atom;
#else
#if defined(USE_JPEG)
		Atom types[1];
		types[0] = jpeg_atom;
#else
		Atom types[1];
		types[0] = png_atom;
#endif
#endif

		XChangeProperty(dpy, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
				(unsigned char *)types, 2);
		ev.xselection.property = req->property;
	}
	else if (req->target == png_atom) {
		if (!clipboard_png_data || clipboard_png_size == 0) {
			ev.xselection.property = None;
		} else {
			/* Decide whether to use INCR */
			if (clipboard_png_size > CHUNK_SIZE) {
				/* Start INCR transfer */
				if (WMIV_DEBUG)
					fprintf(stderr, "Starting INCR for PNG %zu bytes\n", clipboard_png_size);

				/* Prepare incr state */
				incr.active = True;
				incr.requestor = req->requestor;
				incr.property = req->property;
				incr.target = req->target;
				incr.total_size = clipboard_png_size;
				incr.offset = 0;
				incr.chunk_size = CHUNK_SIZE;
				incr.data = clipboard_png_data;

				/* Announce INCR by writing a 32-bit size to the property of type INCR */
				unsigned long size32 = (unsigned long)incr.total_size;
				XChangeProperty(dpy, req->requestor, req->property, incr_atom, 32, PropModeReplace,
				(unsigned char *)&size32, 1);
				ev.xselection.property = req->property;

				/* Send SelectionNotify now to tell client INCR was used */
				XSendEvent(dpy, req->requestor, False, 0, (XEvent *)&ev);
				XFlush(dpy);

				/* Now select PropertyChangeMask on the requestor window to see property deletions.
				The client should delete the property when ready for the first chunk. */
				XSelectInput(dpy, req->requestor, PropertyChangeMask);
				return; /* we already sent SelectionNotify above */
			} else {
				/* Small transfer: send whole PNG in one property set */
				XChangeProperty(dpy, req->requestor, req->property, png_atom, 8, PropModeReplace,
				clipboard_png_data, (int)clipboard_png_size);
				ev.xselection.property = req->property;
			}
		}
	}
	else if (req->target == jpeg_atom) {
		if (!clipboard_jpeg_data || clipboard_jpeg_size == 0) {
			ev.xselection.property = None;
		} else {
			/* Decide whether to use INCR */
			if (clipboard_jpeg_size > CHUNK_SIZE) {
				/* Start INCR transfer */
				if (WMIV_DEBUG)
					fprintf(stderr, "Starting INCR for JPEG %zu bytes\n", clipboard_jpeg_size);

				/* Prepare incr state */
				incr.active = True;
				incr.requestor = req->requestor;
				incr.property = req->property;
				incr.target = req->target;
				incr.total_size = clipboard_jpeg_size;
				incr.offset = 0;
				incr.chunk_size = CHUNK_SIZE;
				incr.data = clipboard_jpeg_data;

				/* Announce INCR by writing a 32-bit size to the property of type INCR */
				unsigned long size32 = (unsigned long)incr.total_size;
				XChangeProperty(dpy, req->requestor, req->property, incr_atom, 32, PropModeReplace,
				(unsigned char *)&size32, 1);
				ev.xselection.property = req->property;

				/* Send SelectionNotify now to tell client INCR was used */
				XSendEvent(dpy, req->requestor, False, 0, (XEvent *)&ev);
				XFlush(dpy);

				/* Now select PropertyChangeMask on the requestor window to see property deletions.
				The client should delete the property when ready for the first chunk. */
				XSelectInput(dpy, req->requestor, PropertyChangeMask);
				return; /* we already sent SelectionNotify above */
			} else {
				/* Small transfer: send whole JPEG in one property set */
				XChangeProperty(dpy, req->requestor, req->property, jpeg_atom, 8, PropModeReplace,
				clipboard_jpeg_data, (int)clipboard_jpeg_size);
				ev.xselection.property = req->property;
			}
		}
	} else {
		ev.xselection.property = None;
	}

	XSendEvent(dpy, req->requestor, False, 0, &ev);
	XFlush(dpy);
}

/*
	Called when the owner loses the selection
*/
static void handle_selection_clear(void)
{
	if (we_own_clipboard) {
		we_own_clipboard = False;
		free_clipboard_data();
	}
	if (incr.active) {
		/* Cancel any active INCR transfer and clean up */
		if (incr.requestor != None) {
			/* Remove PropertyChangeMask from requestor window */
			XSelectInput(dpy, incr.requestor, NoEventMask);
		}
		incr.active = False;
		incr.offset = 0;
		incr.total_size = 0;
		incr.requestor = None;
		incr.property = None;
		incr.target = None;
	}
}

/*
	Load an image and optionally get its orientation if libexif is available
	Returns the image on success, NULL on failure
*/
RImage *load_oriented_image(RContext *context, const char *file, int index)
{
	RImage *image;
#ifdef HAVE_EXIF
	int orientation = 0;
#endif
	image = RLoadImage(context, file, index);
	if (!image)
		return NULL;
#ifdef HAVE_EXIF
	ExifData *exifData = exif_data_new_from_file(file);
	if (exifData) {
		ExifByteOrder byteOrder = exif_data_get_byte_order(exifData);
		ExifEntry *exifEntry = exif_data_get_entry(exifData, EXIF_TAG_ORIENTATION);
		if (exifEntry)
			orientation = exif_get_short(exifEntry->data, byteOrder);

		exif_data_free(exifData);
	}

/*
	0th Row      0th Column
	1  top          left side
	2  top          right side
	3  bottom     right side
	4  bottom     left side
	5  left side    top
	6  right side  top
	7  right side  bottom
	8  left side    bottom
*/

	if (image && (orientation > 1)) {
		RImage *tmp = NULL;
		switch (orientation) {
		case 2:
			tmp = RFlipImage(image, RHorizontalFlip);
			break;
		case 3:
			tmp = RRotateImage(image, 180);
			break;
		case 4:
			tmp = RFlipImage(image, RVerticalFlip);
			break;
		case 5: {
				RImage *tmp2;
				tmp2 = RFlipImage(image, RVerticalFlip);
				if (tmp2) {
					tmp = RRotateImage(tmp2, 90);
					RReleaseImage(tmp2);
				}
			}
			break;
		case 6:
			tmp = RRotateImage(image, 90);
			break;
		case 7: {
				RImage *tmp2;
				tmp2 = RFlipImage(image, RVerticalFlip);
				if (tmp2) {
					tmp = RRotateImage(tmp2, 270);
					RReleaseImage(tmp2);
				}
			}
			break;
		case 8:
			tmp = RRotateImage(image, 270);
			break;
		}
		if (tmp) {
			RReleaseImage(image);
			image = tmp;
		}
	}
#endif
	return image;
}

/*
	Change window title
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int change_title(XTextProperty *prop, char *filename)
{
	char *combined_title = NULL;
	if (ignore_unknown_file_format) {
		if (!asprintf(&combined_title, "%s - %s", APPNAME, filename))
			return EXIT_FAILURE;
	}
	else {
		if (!asprintf(&combined_title, "%s - %u/%u - %s", APPNAME, current_index, max_index, filename))
			if (!asprintf(&combined_title, "%s - %u/%u", APPNAME, current_index, max_index))
				return EXIT_FAILURE;
	}
	Xutf8TextListToTextProperty(dpy, &combined_title, 1, XUTF8StringStyle, prop);
	XSetWMName(dpy, win, prop);
	if (prop->value)
		XFree(prop->value);
	free(combined_title);
	return EXIT_SUCCESS;
}

/*
	Rescale the current image based on the screen size
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int rescale_image(void)
{
	long final_width = img->width;
	long final_height = img->height;
	int max_width_tmp = max_width;
	int max_height_tmp = max_height;

	if (!fullscreen_flag) {
		max_width_tmp -= extents.left + extents.right;
		max_height_tmp -= extents.top + extents.bottom;
	}

	/* check if there is already a zoom factor applied */
	if (fabsf(zoom_factor) <= 0.0f) {
		final_width = img->width + (int)(img->width * zoom_factor);
		final_height = img->height + (int)(img->height * zoom_factor);
	}
	if ((max_width_tmp < final_width) || (max_height_tmp < final_height)) {
		double val = 0;
		if (final_width > final_height) {
			val = final_height * max_width_tmp / final_width;
			final_width = ceil(final_width * val / final_height);
			final_height = ceil(val);
			if (val > max_height_tmp) {
				val = final_width * max_height_tmp / final_height;
				final_height = ceil(final_height * val / final_width);
				final_width = ceil(val);
			}
		} else {
			val = final_width * max_height_tmp / final_height;
			final_height = ceil(final_height * val / final_width);
			final_width = ceil(val);
			if (val > max_width_tmp) {
				val = final_height * max_width_tmp / final_width;
				final_width = ceil(final_width * val / final_height);
				final_height = ceil(val);
			}
		}
	}
	if ((final_width != img->width) || (final_height != img->height)) {
		RImage *old_img = img;
		img = RScaleImage(img, final_width, final_height);
		if (!img) {
			img = old_img;
			return EXIT_FAILURE;
		}
		RReleaseImage(old_img);
	}
	if (!RConvertImage(ctx, img, &pix)) {
		fprintf(stderr, "Error: %s\n", RMessageForError(RErrorCode));
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
	Find the best image size for the current display
	Returns EXIT_SUCCESS on success
*/
int maximize_image(void)
{
	rescale_image();
	XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0,
		img->width, img->height, max_width/2-img->width/2, max_height/2-img->height/2);
	return EXIT_SUCCESS;
}

/*
	Merge the current image with a checkerboard background
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int merge_with_background(RImage *i)
{
	if (i) {
		RImage *back;
		back = RCreateImage(i->width, i->height, True);
		if (back) {
			int opaq = 255;
			int x = 0, y = 0;

			RFillImage(back, &lightGray);
			for (x = 0; x <= i->width; x += 8) {
				if (x/8 % 2)
					y = 8;
				else
					y = 0;
				for (; y <= i->height; y += 16)
					ROperateRectangle(back, RAddOperation, x, y, x+8, y+8, &darkGray);
			}

			RCombineImagesWithOpaqueness(i, back, opaq);
			RReleaseImage(back);
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}

/*
	Rotate the image by the angle passed
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int rotate_image(float angle)
{
	RImage *tmp;

	if (!img)
		return EXIT_FAILURE;

	tmp = RRotateImage(img, angle);
	if (!tmp)
		return EXIT_FAILURE;

	if (!fullscreen_flag) {
		if (img->width != tmp->width || img->height != tmp->height)
			XResizeWindow(dpy, win, tmp->width, tmp->height);
	}

	RReleaseImage(img);
	img = tmp;

	rescale_image();
	if (!fullscreen_flag) {
		XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 0, 0);
	} else {
		XClearWindow(dpy, win);
		XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0,
			img->width, img->height, max_width/2-img->width/2, max_height/2-img->height/2);
	}

	return EXIT_SUCCESS;
}

/*
	Rotate the image by 90 degrees to the right
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int rotate_image_right(void)
{
	return rotate_image(90.0);
}

/*
	Rotate the image by -90 degrees to the left
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int rotate_image_left(void)
{
	return rotate_image(-90.0);
}

/*
	Create a red crossed image to indicate an error loading file
	Returns the image on success, NULL on failure
*/
RImage *draw_failed_image(void)
{
	RImage *failed_image = NULL;
	XWindowAttributes attr;

	if (win && (XGetWindowAttributes(dpy, win, &attr) >= 0))
		failed_image = RCreateImage(attr.width, attr.height, False);
	else
		failed_image = RCreateImage(50, 50, False);
	if (!failed_image)
		return NULL;

	RFillImage(failed_image, &black);

	/* Calculate 80% size cross centered in the image */
	int cross_width = (int)(failed_image->width * 0.6);
	int cross_height = (int)(failed_image->height * 0.6);
	int offset_x = (failed_image->width - cross_width) / 2;
	int offset_y = (failed_image->height - cross_height) / 2;

	/* Calculate line thickness based on image size (minimum 3, maximum 8 pixels) */
	int thickness = (failed_image->width + failed_image->height) / 100;
	if (thickness < 3) thickness = 3;
	if (thickness > 8) thickness = 8;

	/* Draw thick diagonal lines for the cross using 80% of the image size */
	for (int i = -thickness/2; i <= thickness/2; i++) {
		/* First diagonal (top-left to bottom-right) */
		ROperateLine(failed_image, RAddOperation,
			offset_x + i, offset_y,
			offset_x + cross_width + i, offset_y + cross_height, &red);
		ROperateLine(failed_image, RAddOperation,
			offset_x, offset_y + i,
			offset_x + cross_width, offset_y + cross_height + i, &red);

		/* Second diagonal (top-right to bottom-left) */
		ROperateLine(failed_image, RAddOperation,
			offset_x + i, offset_y + cross_height,
			offset_x + cross_width + i, offset_y, &red);
		ROperateLine(failed_image, RAddOperation,
			offset_x, offset_y + cross_height - i,
			offset_x + cross_width, offset_y - i, &red);
	}

	return failed_image;
}

/*
	Send event to the window manager to switch from/to full screen mode
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int full_screen(void)
{
	XEvent xev;

	Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", True);
	Atom fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True);
	long mask = SubstructureNotifyMask;

	if (fullscreen_flag) {
		fullscreen_flag = False;
		zoom_factor = 0;
		back_from_fullscreen = True;
	} else {
		fullscreen_flag = True;
		zoom_factor = 1000;
		RReleaseImage(img);
		img = load_oriented_image(ctx, current_link->data, 0);
		if (!img)
			img = draw_failed_image();
		else
			merge_with_background(img);
	}

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.display = dpy;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = fullscreen_flag;
	xev.xclient.data.l[1] = fullscreen;

	if (!XSendEvent(dpy, DefaultRootWindow(dpy), False, mask, &xev)) {
		fprintf(stderr, "Error: sending fullscreen event to xserver\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
	Init the linked list
*/
void linked_list_init(linked_list_t *list)
{
	list->first = list->last = 0;
	list->count = 0;
}

/*
	Add an element to the linked list
	Returns EXIT_SUCCESS on success, EXIT_FAILURE otherwise
*/
int linked_list_add(linked_list_t *list, const void *data)
{
	link_t *link;

	/* calloc sets the "next" field to zero. */
	link = calloc(1, sizeof(link_t));
	if (!link) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		return EXIT_FAILURE;
	}
	link->data = data;
	if (list->last) {
		/* Join the two final links together. */
		list->last->next = link;
		link->prev = list->last;
		list->last = link;
	} else {
		list->first = link;
		list->last = link;
	}
	list->count++;
	return EXIT_SUCCESS;
}

/*
	Delete an element from the linked list
	Returns EXIT_SUCCESS on success, EXIT_FAILURE otherwise
*/
int linked_list_del(linked_list_t *list, link_t *link)
{
	if (!list || !link) {
		return EXIT_FAILURE;
	}

	/* Update previous link's next pointer */
	if (link->prev) {
		link->prev->next = link->next;
	} else {
		/* This was the first link */
		list->first = link->next;
	}

	/* Update next link's previous pointer */
	if (link->next) {
		link->next->prev = link->prev;
	} else {
		/* This was the last link */
		list->last = link->prev;
	}

	if (link->data)
		free((char *)link->data);
	free(link);

	list->count--;

	return EXIT_SUCCESS;
}

/*
	Deallocate the whole linked list
*/
void linked_list_free(linked_list_t *list)
{
	link_t *link;
	link_t *next;
	for (link = list->first; link; link = next) {
		/* Store the next value so that we don't access freed memory. */
		next = link->next;
		if (link->data)
			free((char *)link->data);
		free(link);
	}
	current_link = NULL;
}

/*
	Apply a zoom factor on the current image
	Arg: 1 to zoom in, 0 to zoom out
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int zoom_in_out(int z)
{
	RImage *old_img = img;
	RImage *tmp = load_oriented_image(ctx, current_link->data, 0);
	if (!tmp)
		return EXIT_FAILURE;

	if (z) {
		zoom_factor += 0.2f;
		img = RScaleImage(tmp, tmp->width + (int)(tmp->width * zoom_factor),
				tmp->height + (int)(tmp->height * zoom_factor));
		if (!img) {
			img = old_img;
			RReleaseImage(tmp);
			return EXIT_FAILURE;
		}
	} else {
		zoom_factor -= 0.2f;
		int new_width = tmp->width + (int) (tmp->width * zoom_factor);
		int new_height = tmp->height + (int)(tmp->height * zoom_factor);
		if ((new_width <= 0) || (new_height <= 0)) {
			zoom_factor += 0.2f;
			RReleaseImage(tmp);
			return EXIT_FAILURE;
		}
		img = RScaleImage(tmp, new_width, new_height);
		if (!img) {
			img = old_img;
			RReleaseImage(tmp);
			return EXIT_FAILURE;
		}
	}
	RReleaseImage(old_img);
	RReleaseImage(tmp);
	XFreePixmap(dpy, pix);

	merge_with_background(img);
	if (!RConvertImage(ctx, img, &pix)) {
		fprintf(stderr, "Error: %s\n", RMessageForError(RErrorCode));
		return EXIT_FAILURE;
	}
	XResizeWindow(dpy, win, img->width, img->height);
	return EXIT_SUCCESS;
}

/*
	Transitional function used to call zoom_in_out with zoom in flag
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int zoom_in(void)
{
	return zoom_in_out(1);
}

/*
	Transitional function used to call zoom_in_out with zoom out flag
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int zoom_out(void)
{
	return zoom_in_out(0);
}

/*
	Load previous or next image
	Arg: way which could be PREV or NEXT constant
	Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
*/
int change_image(int way)
{
	if (max_index == 0)
		return EXIT_FAILURE;

	if (img && current_link) {
		int old_img_width = img->width;
		int old_img_height = img->height;

		RReleaseImage(img);

		if (way == NEXT) {
			current_link = current_link->next;
			current_index++;
		} else {
			current_link = current_link->prev;
			current_index--;
		}
		if (current_link == NULL) {
			if (way == NEXT) {
				current_link = list.first;
				current_index = 1;
			} else {
				current_link = list.last;
				current_index = max_index;
			}
		}
		if (WMIV_DEBUG)
			fprintf(stderr, "Current file is> %s\n", (char *)current_link->data);
		img = load_oriented_image(ctx, current_link->data, 0);
		if (!img) {
			if (strlen((char *)current_link->data) == 0)
				fprintf(stderr, "Error: %s\n", RMessageForError(RErrorCode));
			else
				fprintf(stderr, "Error: %s %s\n", (char *)current_link->data, RMessageForError(RErrorCode));
			img = draw_failed_image();
			if (ignore_unknown_file_format) {
				link_t *tmp_link;
				if (WMIV_DEBUG)
					fprintf(stderr, "Skipping file...\n");
				if (way == NEXT)
					if (!current_link->prev)
						tmp_link = list.last;
					else
						tmp_link = current_link->prev;
				else
					if (!current_link->next)
						tmp_link = list.first;
					else
						tmp_link = current_link->next;
				linked_list_del(&list, current_link);
				current_link = tmp_link;
				max_index = list.count;
				return change_image(way);
			}
		} else {
			merge_with_background(img);
		}
		rescale_image();
		if (win) {
			if (!fullscreen_flag) {
				if ((old_img_width != img->width) || (old_img_height != img->height))
					XResizeWindow(dpy, win, img->width, img->height);
				else
					XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 0, 0);
				change_title(&title_property, (char *)current_link->data);
			} else {
				XClearWindow(dpy, win);
				XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0,
					img->width, img->height, max_width/2-img->width/2, max_height/2-img->height/2);
			}
		}
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

#ifdef HAVE_PTHREAD
/*
	Send an X event to display the next image at every delay set to slideshow_delay
*/
void *slideshow(void *arg)
{
	(void) arg;

	XKeyEvent event;
	event.display = dpy;
	event.window = win;
	event.root = DefaultRootWindow(dpy);
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = XKeysymToKeycode(dpy, XK_Right);
	event.state = 0;
	event.type = KeyPress;

	while (slideshow_flag) {
		int r;
		r = XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
		if (!r)
			fprintf(stderr, "Error: can't send event\n");
		XFlush(dpy);
		/* default sleep time between moving to next image */
		sleep(slideshow_delay);
	}
	tid = 0;
	return arg;
}
#endif

/*
	List and sort by name all files from a given directory
	Arg: the directory path that contains images, the linked list where to add the new file refs
	Returns: the first argument of the list or NULL on failure
*/
link_t *connect_dir(char *dirpath, linked_list_t *list)
{
	struct dirent **dir;
	int dv, idx;
	char path[PATH_MAX] = "";

	if (!dirpath)
		return NULL;

	dv = scandir(dirpath, &dir, 0, alphasort);
	if (dv < 0) {
		/* maybe it's a file */
		struct stat stDirInfo;
		if (lstat(dirpath, &stDirInfo) == 0)
			linked_list_add(list, strdup(dirpath));
		return list->first;
	}
	for (idx = 0; idx < dv; idx++) {
			struct stat stDirInfo;
			if (dirpath[strlen(dirpath)-1] == FILE_SEPARATOR)
				snprintf(path, PATH_MAX, "%s%s", dirpath, dir[idx]->d_name);
			else
				snprintf(path, PATH_MAX, "%s%c%s", dirpath, FILE_SEPARATOR, dir[idx]->d_name);

			free(dir[idx]);
			if ((lstat(path, &stDirInfo) == 0) && !S_ISDIR(stDirInfo.st_mode))
				linked_list_add(list, strdup(path));
	}
	free(dir);
	return list->first;
}

#ifdef USE_RANDR
/*
	Retrieve the monitor resolution where the window is located
*/
void get_monitor_dimensions(Display *dpy, int screen_num, Window win, int *width, int *height)
{
	int monitor_count, abs_x, abs_y;
	Window child;
	Window root_window = RootWindow(dpy, screen_num);
	XWindowAttributes attrs;
	XGetWindowAttributes(dpy, win, &attrs);
	XTranslateCoordinates(dpy, win, attrs.root, 0, 0, &abs_x, &abs_y, &child);

	XRRMonitorInfo *monitors = XRRGetMonitors(dpy, root_window, True, &monitor_count);
	if (monitors && monitor_count > 0) {
		for (int i = 0; i < monitor_count; i++) {
			if (abs_x >= monitors[i].x && abs_x < (monitors[i].x + monitors[i].width) &&
				abs_y >= monitors[i].y && abs_y < (monitors[i].y + monitors[i].height)) {
					*width = monitors[i].width;
					*height = monitors[i].height;
					break;
				}
		}
		XRRFreeMonitors(monitors);
	}
}
#endif

/*
	Transform the given URI to UTF-8 string
*/
void decode_uri(char *uri)
{
	char *last = uri + strlen(uri);

	while (uri < last-2) {
			if (*uri == '%') {
					int h;
					if (sscanf(uri+1, "%2X", &h) != 1)
							break;
					*uri = h;
					memmove(uri+1, uri+3, last - (uri+2));
					last -= 2;
			}
			uri++;
	}
}

/*
	Provide data for drag operations
*/
static void widget_get_data_callback(DndClass *dnd, Window window, unsigned char **data, int *length, Atom type) {
	/* Mark unused parameters to suppress compiler warnings */
	(void)dnd;
	(void)window;
	(void)type;
	char *filename, *full_path, *uri_data;
	int uri_length = 0;
	static char *last_provided_data = NULL;

	if (!current_link || !current_link->data) {
		*data = NULL;
		*length = 0;
		return;
	}

	/* Get the full absolute path for the current file */
	filename = (char *)current_link->data;
	full_path = realpath(filename, NULL);

	if (!full_path) {
		*data = NULL;
		*length = 0;
		return;
	}

	/* Create file:// URI for the current file */
	uri_length = strlen("file://") + strlen(full_path) + 1;
	uri_data = malloc(uri_length);
	if (!uri_data) {
		free(full_path);
		*data = NULL;
		*length = 0;
		return;
	}

	snprintf(uri_data, uri_length, "file://%s", full_path);
	*data = (unsigned char *)uri_data;
	*length = strlen(uri_data);

	/* Only log if this is different data than last time */
	if (WMIV_DEBUG && (!last_provided_data || strcmp(uri_data, last_provided_data) != 0)) {
		fprintf(stderr, "Providing drag data: %s\n", uri_data);
		if (last_provided_data)
			free(last_provided_data);
		last_provided_data = strdup(uri_data);
	}
	free(full_path);
}

/*
	Initialize DND for sending
*/
static void init_dnd_send(void) {
	if (!dnd_initialized) {
		Atom typelist[] = {
			XInternAtom(dpy, "text/uri-list", False),
			XInternAtom(dpy, "text/plain", False),
			None
		};

		xdnd_init(&send_dnd, dpy);

		send_dnd.widget_get_data = widget_get_data_callback;
		dnd_initialized = True;
		xdnd_set_dnd_aware(&send_dnd, win, typelist);
	}
}

/*
	Initiate a file drag operation
*/
static void initiate_file_drag(void) {
	Atom action = XInternAtom(dpy, "XdndActionCopy", False);
	Atom typelist[] = {
		XInternAtom(dpy, "text/uri-list", False),
		None
	};

	if (!current_link || !current_link->data) {
		return;
	}

	init_dnd_send();
	xdnd_drag(&send_dnd, win, action, typelist);
}

/*
	main
*/
int main(int argc, char **argv)
{
	int option = -1;
	RContextAttributes attr = {};
	XEvent e;
	KeySym keysym;
	char *reading_filename = "";
	int screen_num, file_i;
	int quit = 0;
	XClassHint *class_hints;
	XSizeHints *size_hints;
	XWMHints *win_hints;
	Atom delWindow;
	Atom xdnd_aware;
	Atom xdnd_version = XDND_VERSION;

#ifdef USE_XPM
	Pixmap icon_pixmap, icon_shape;
#endif

	class_hints = XAllocClassHint();
	if (!class_hints) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		return EXIT_FAILURE;
	}
	class_hints->res_name = (char *)APPNAME;
	class_hints->res_class = "wmiv";

	/* Init colors */
	lightGray.red = lightGray.green = lightGray.blue = 211;
	darkGray.red = darkGray.green = darkGray.blue = 169;
	lightGray.alpha = darkGray.alpha = 1;
	black.red = black.green = black.blue = 0;
	red.red = 255;
	red.green = red.blue = 0;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"ignore-unknown", no_argument, 0, 'i'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	int option_index = 0;

	option = getopt_long (argc, argv, "hiv", long_options, &option_index);
	if (option != -1) {
		switch (option) {
		case 'h':
			printf("Usage: %s [image(s)|directory|archive]\n"
			"Options:\n"
			"  -h, --help            display this help and exit\n"
			"  -i, --ignore-unknown  ignore unknown image format\n"
			"  -v, --version         print version\n"
			"\nKeys:\n\n"
			"  [+]            zoom in\n"
			"  [-]            zoom out\n"
			"  [▸]            next image\n"
			"  [◂]            previous image\n"
			"  [▴]            first image\n"
			"  [▾]            last image\n"
			"  [Ctrl+C]       copy image to clipboard\n"
#ifdef HAVE_PTHREAD
			"  [D]            start slideshow\n"
#endif
			"  [Esc]          actual size\n"
			"  [F]            toggle full-screen mode\n"
			"  [L]            rotate image on the left\n"
			"  [Q]            quit\n"
			"  [R]            rotate image on the right\n",
			argv[0]);
			return EXIT_SUCCESS;
		case 'v':
			printf("%s version %s\n", APPNAME, VERSION);
			return EXIT_SUCCESS;
		case 'i':
			ignore_unknown_file_format = True;
			break;
		case '?':
			return EXIT_FAILURE;
		}
	}

	linked_list_init(&list);

#ifdef HAVE_LIBARCHIVE
	/* Register cleanup function for temporary directory */
	atexit(cleanup_temp_dir);
#endif
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "Error: can't open display\n");
		linked_list_free(&list);
		return EXIT_FAILURE;
	}

	/* Initialize clipboard atoms */
	clipboard_atom = XInternAtom(dpy, "CLIPBOARD", False);
	targets_atom = XInternAtom(dpy, "TARGETS", False);
	png_atom = XInternAtom(dpy, "image/png", False);
	jpeg_atom = XInternAtom(dpy, "image/jpeg", False);
	property_atom = XInternAtom(dpy, "WMIV_CLIPBOARD_PROP", False);
	incr_atom = XInternAtom(dpy, "INCR", False);

	screen_num = DefaultScreen(dpy);
	max_width = DisplayWidth(dpy, screen_num);
	max_height = DisplayHeight(dpy, screen_num);

	attr.flags = RC_RenderMode | RC_ColorsPerChannel;
	attr.render_mode = RDitheredRendering;
	attr.colors_per_channel = 4;
	ctx = RCreateContext(dpy, screen_num, &attr);

	if (argc < 2) {
		argv[1] = ".";
		argc = 2;
	}

	for (file_i = 1; file_i < argc; file_i++) {
		/* Check if this is an archive file */
		if (is_archive_file(argv[file_i])) {
			char *extracted_dir = extract_archive(argv[file_i]);
			if (extracted_dir) {
				current_link = connect_dir(extracted_dir, &list);
				free(extracted_dir);
				if (current_link) {
				reading_filename = (char *)current_link->data;
				max_index = list.count;
				}
			}
		} else {
			current_link = connect_dir(argv[file_i], &list);
			if (current_link) {
				reading_filename = (char *)current_link->data;
				max_index = list.count;
			}
		}
	}
	img = load_oriented_image(ctx, reading_filename, 0);

	if (!img) {
		if (strlen(reading_filename) == 0)
			fprintf(stderr, "Error: %s\n", RMessageForError(RErrorCode));
		else
			fprintf(stderr, "Error: %s %s\n", reading_filename, RMessageForError(RErrorCode));
		img = draw_failed_image();
		if (!current_link)
			return EXIT_FAILURE;
		if (ignore_unknown_file_format) {
			if (WMIV_DEBUG)
				fprintf(stderr, "Skipping file...\n");
			if (change_image(NEXT) != EXIT_SUCCESS || !img)
				return EXIT_FAILURE;
			if (current_link)
				reading_filename = (char *)current_link->data;
		}
	}

	merge_with_background(img);

	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, BlackPixel(dpy, screen_num));
	XSelectInput(dpy, win, KeyPressMask|StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|FocusChangeMask|PropertyChangeMask);

	size_hints = XAllocSizeHints();
	if (!size_hints) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		return EXIT_FAILURE;
	}
	size_hints->width = img->width;
	size_hints->height = img->height;

	delWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &delWindow, 1);
	change_title(&title_property, reading_filename);

	xdnd_aware = XInternAtom(dpy, "XdndAware", False);
	XChangeProperty(dpy, win, xdnd_aware, XA_ATOM, 32, PropModeReplace, (unsigned char *) &xdnd_version, 1);
	win_hints = XAllocWMHints();

	if (win_hints) {
		win_hints->flags = StateHint|InputHint|WindowGroupHint;

#ifdef USE_XPM
		if ((XpmCreatePixmapFromData(dpy, win, wmiv_xpm, &icon_pixmap, &icon_shape, NULL)) == 0) {
			win_hints->flags |= IconPixmapHint|IconMaskHint;
			win_hints->icon_pixmap = icon_pixmap;
			win_hints->icon_mask = icon_shape;

			Atom net_wm_icon = XInternAtom(dpy, "_NET_WM_ICON", False);
			if (net_wm_icon != None) {
				int icon_width, icon_height;
				if (sscanf(wmiv_xpm[0], "%d %d", &icon_width, &icon_height) == 2) {
					/* Convert XPM pixmap to ARGB32 format */
					XImage *icon_image = XGetImage(dpy, icon_pixmap, 0, 0, icon_width, icon_height, AllPlanes, ZPixmap);
					if (icon_image) {
						XImage *mask_image = NULL;
						if (icon_shape != None) {
							mask_image = XGetImage(dpy, icon_shape, 0, 0, icon_width, icon_height, AllPlanes, ZPixmap);
						}

						/* Create ARGB32 data: width, height, followed by ARGB pixel data */
						unsigned long *icon_data = malloc(sizeof(unsigned long) * (2 + icon_width * icon_height));
						if (icon_data) {
							icon_data[0] = icon_width;
							icon_data[1] = icon_height;

							for (int y = 0; y < icon_height; y++) {
								for (int x = 0; x < icon_width; x++) {
									unsigned long pixel = XGetPixel(icon_image, x, y);
									unsigned long alpha = 0xFF000000; /* default opaque */

									/* Check mask for transparency */
									if (mask_image) {
										unsigned long mask_pixel = XGetPixel(mask_image, x, y);
										if (mask_pixel == 0) {
											alpha = 0x00000000; /* transparent */
										}
									}

									icon_data[2 + y * icon_width + x] = alpha | (pixel & 0x00FFFFFF);
								}
							}

							XChangeProperty(dpy, win, net_wm_icon, XA_CARDINAL, 32, PropModeReplace,
								(unsigned char *)icon_data, 2 + icon_width * icon_height);
							free(icon_data);
						}

						XDestroyImage(icon_image);
						if (mask_image) {
							XDestroyImage(mask_image);
						}
					}
				}
			}
		}
#endif
		win_hints->initial_state = NormalState;
		win_hints->input = True;
		win_hints->window_group = win;
		XStringListToTextProperty((char **)&APPNAME, 1, &icon_property);
		XSetWMProperties(dpy, win, NULL, &icon_property, argv, argc, size_hints, win_hints, class_hints);
		if (icon_property.value)
			XFree(icon_property.value);
		XFree(win_hints);
		XFree(class_hints);
		XFree(size_hints);
	}
	XMapWindow(dpy, win);
	XFlush(dpy);

	if (wait_for_frame_extents(dpy, win, 1000)) { /* Wait up to 1 second */
		if (!get_window_decor_size(dpy, win, &extents) && WMIV_DEBUG)
			fprintf(stderr, "Warning: Property found but could not parse frame extents\n");
	} else {
		if (WMIV_DEBUG)
			fprintf(stderr, "Warning: Window manager did not set _NET_FRAME_EXTENTS property within timeout\n");
	}

#ifdef USE_RANDR
	get_monitor_dimensions(dpy, screen_num, win, &max_width, &max_height);
#endif

	if (WMIV_DEBUG)
		fprintf(stderr, "Display size: %dx%d\n", max_width, max_height);

	rescale_image();
	XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 0, 0);
	XResizeWindow(dpy, win, img->width, img->height);
	XSync(dpy, True);

	/* Main event loop */
	while (!quit) {
		XNextEvent(dpy, &e);
		if (e.type == ClientMessage) {
			unsigned char *dropBuffer;
			Atom typelist[] = {
				XInternAtom(dpy, "text/uri-list", False),
				XInternAtom(dpy, "text/plain", False),
				None
			};
			Atom actionlist[] = {
				XInternAtom(dpy, "XdndActionCopy", False),
				XInternAtom(dpy, "XdndActionMove", False),
				None
			};
			int length;
			Atom type;
			int x, y;

			if (xdnd_get_drop(dpy, &e, typelist, actionlist, &dropBuffer, &length, &type, &x, &y)) {
				char *handled_uri_header = "file://";
				char *filename_separator = "\n";
				char *ptr = strtok((char *)dropBuffer, filename_separator);

				if (WMIV_DEBUG)
					fprintf(stderr, "Dropped data: %s\n", (char *)dropBuffer);

				linked_list_free(&list);
				linked_list_init(&list);

				Bool find_one_file = False;
				while (ptr != NULL) {
					if (strstr(ptr, handled_uri_header) == ptr) {
						char *tmp_file;
						if (ptr[strlen(ptr) - 1] == '\r')
							ptr[strlen(ptr) - 1] = '\0';
						tmp_file = ptr + strlen(handled_uri_header);

						/* drag-and-drop file name format as per the spec is encoded as an URI */
						decode_uri(tmp_file);
						if (connect_dir(tmp_file, &list))
							find_one_file = True;
					}
					ptr = strtok(NULL, filename_separator);
				}
				free(dropBuffer);

				if (find_one_file) {
					max_index = list.count;
					current_link = list.last;
					change_image(NEXT);
				} else {
					if (WMIV_DEBUG)
						fprintf(stderr, "Error: no valid file found in dropped data\n");
					XClearArea(dpy, win, 0, 0, 0, 0, True);
				}
			}
			if (e.xclient.data.l[0] == delWindow)
				quit = 1;
			continue;
		}
		if (e.type == SelectionRequest) {
			/* Serve clipboard requests */
			handle_selection_request(&e.xselectionrequest);
			continue;
		}
		if (e.type == SelectionClear) {
			/* we lost ownership */
			handle_selection_clear();
			continue;
		}
		if (e.type == PropertyNotify) {
			XPropertyEvent *pe = &e.xproperty;
			if (incr.active &&
				(pe->window == incr.requestor) &&
				(pe->atom == incr.property) &&
				(pe->state == PropertyDelete)) {

				if (WMIV_DEBUG)
					fprintf(stderr, "INCR: property delete observed, sending next chunk\n");

				/* Check if we've already sent all data */
				if (incr.offset >= incr.total_size) {
					/* Send zero-length termination to signal end of transfer */
					XChangeProperty(dpy, incr.requestor, incr.property, incr.target, 8, PropModeReplace, NULL, 0);
					XFlush(dpy);
					/* Remove PropertyChangeMask from requestor window to clean up */
					XSelectInput(dpy, incr.requestor, NoEventMask);
					incr.active = False;
					if (WMIV_DEBUG)
						fprintf(stderr, "INCR: transfer complete\n");
				} else {
					/* Send next data chunk */
					send_next_incr_chunk();
				}
			} else if (pe->window == win) {
				if (WMIV_DEBUG && incr.active) {
					fprintf(stderr, "Ignoring PropertyNotify on main window during INCR\n");
				}
			}
			continue;
		}
		if (e.type == FocusIn) {
			focus = True;
			continue;
		}
		if (e.type == FocusOut) {
			focus = False;
			continue;
		}
		if (!fullscreen_flag && (e.type == Expose)) {
			XExposeEvent xev = e.xexpose;
			if (xev.count == 0)
				XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 0, 0);
			continue;
		}
		if (!fullscreen_flag && e.type == ConfigureNotify) {
			XConfigureEvent xce = e.xconfigure;

			/* there is no file loaded and the window is resized */
			if (!current_link) {
				XResizeWindow(dpy, win, img->width, img->height);
				continue;
			}

			if (xce.width != img->width || xce.height != img->height) {
				RImage *old_img = img;
				img = load_oriented_image(ctx, current_link->data, 0);
				if (!img) {
					/* keep the old img and window size */
					img = old_img;
					XResizeWindow(dpy, win, img->width, img->height);
				} else {
					RImage *tmp2;
					if (!back_from_fullscreen) {
						/* manually resized window */
						tmp2 = RScaleImage(img, xce.width, xce.height);
					}
					else {
						/* back from fullscreen mode, maybe img was rotated */
						tmp2 = img;
						back_from_fullscreen = False;
						XClearWindow(dpy, win);
					}
					merge_with_background(tmp2);
					if (RConvertImage(ctx, tmp2, &pix)) {
						RReleaseImage(old_img);
						img = RCloneImage(tmp2);
						RReleaseImage(tmp2);
						change_title(&title_property, (char *)current_link->data);
						XSync(dpy, True);
						rescale_image();
						XResizeWindow(dpy, win, img->width, img->height);
						XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0,
							img->width, img->height, 0, 0);

					}
				}
			}
			continue;
		}
		if (fullscreen_flag && e.type == ConfigureNotify) {
			int new_width = e.xconfigure.width;
			int new_height = e.xconfigure.height;
			if (new_width != img->width || new_height != img->height)
				maximize_image();
			continue;
		}
		if (e.type == ButtonPress) {
			switch (e.xbutton.button) {
			case Button1: {
				/* Store button press for potential drag operation */
				button1_pressed = True;
				drag_start_x = e.xbutton.x;
				drag_start_y = e.xbutton.y;
			}
			break;
			case Button4:
				zoom_in();
				break;
			case Button5:
				zoom_out();
				break;
			case 8:
				change_image(PREV);
				break;
			case 9:
				change_image(NEXT);
				break;
			}
			continue;
		}
		if (e.type == ButtonRelease) {
			if (e.xbutton.button == Button1 && button1_pressed) {
				/* Button released without dragging - treat as click for image navigation */
				button1_pressed = False;
				if (focus) {
					if (img && (drag_start_x > img->width/2))
						change_image(NEXT);
					else
						change_image(PREV);
				}
			}
			continue;
		}
		if (e.type == MotionNotify && button1_pressed) {
			/* Check if we've moved enough to start a drag */
			int dx = e.xmotion.x - drag_start_x;
			int dy = e.xmotion.y - drag_start_y;
			if (abs(dx) > drag_threshold || abs(dy) > drag_threshold) {
				/* Start drag operation */
				button1_pressed = False;
				if (current_link && current_link->data) {
					initiate_file_drag();
				}
			}
			continue;
		}
		if (e.type == KeyPress) {
			keysym = W_KeycodeToKeysym(dpy, e.xkey.keycode, e.xkey.state & ShiftMask?1:0);
#ifdef HAVE_PTHREAD
			if (keysym != XK_Right)
				slideshow_flag = False;
#endif
			/* Detect Ctrl+C: state contains ControlMask */
			if ((e.xkey.state & ControlMask) && (keysym == XK_c || keysym == XK_C)) {
				copy_current_image_to_clipboard();
				continue;
			}

			switch (keysym) {
			case XK_Right:
				change_image(NEXT);
				break;
			case XK_Left:
				change_image(PREV);
				break;
			case XK_Up:
				if (current_link) {
				current_link = list.last;
				change_image(NEXT);
				}
				break;
			case XK_Down:
				if (current_link) {
				current_link = list.first;
				change_image(PREV);
				}
				break;
#ifdef HAVE_PTHREAD
			case XK_F5:
			case XK_d:
				if (!tid) {
					if (current_link && !slideshow_flag) {
						slideshow_flag = True;
						pthread_create(&tid, NULL, &slideshow, NULL);
					} else {
						fprintf(stderr, "Error: can't start slideshow\n");
					}
				}
				break;
#endif
			case XK_q:
				quit = 1;
				break;
			case XK_Escape:
				if (!fullscreen_flag) {
					zoom_factor = -0.2f;
					zoom_in();
				} else {
					full_screen();
				}
				break;
			case XK_plus:
				zoom_in();
				break;
			case XK_minus:
				zoom_out();
				break;
			case XK_F11:
			case XK_f:
				full_screen();
				break;
			case XK_r:
				rotate_image_right();
				break;
			case XK_l:
				rotate_image_left();
				break;
			}

		}
	}

	if (img)
		RReleaseImage(img);
	if (pix)
		XFreePixmap(dpy, pix);
#ifdef USE_XPM
	if (icon_pixmap)
		XFreePixmap(dpy, icon_pixmap);
	if (icon_shape)
		XFreePixmap(dpy, icon_shape);
#endif
	linked_list_free(&list);
	RDestroyContext(ctx);
	RShutdown();

	free_clipboard_data();
	cleanup_temp_dir();

	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
