/* wmsetbg.c- sets root window background image and also works as
 * 		workspace background setting helper for wmaker
 *
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1998, 1999 Alfredo K. Kojima
 *  Copyright (c) 1998 Dan Pascu
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
 * TODO: rewrite, too dirty
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>

#include "../src/wconfig.h"

#include "../WINGs/WINGs.h"
#include "../WINGs/WUtil.h"
#include "../wrlib/wraster.h"

#include <proplist.h>

#define PROG_VERSION	"wmsetbg (Window Maker) 2.1"


#define WORKSPACE_COUNT (MAX_WORKSPACES+1)


Display *dpy;
char *display = "";
Window root;
int scr;
int scrWidth;
int scrHeight;

Pixmap CurrentPixmap = None;
char *PixmapPath = NULL;


extern Pixmap LoadJPEG(RContext *rc, char *file_name, int *width, int *height);


typedef struct BackgroundTexture {
    int refcount;

    int solid;

    char *spec;

    XColor color;
    Pixmap pixmap;		       /* for all textures, including solid */
    int width;			       /* size of the pixmap */
    int height;
} BackgroundTexture;



RImage*
loadImage(RContext *rc, char *file)
{
    char *path;
    RImage *image;

    if (access(file, F_OK)!=0) {
	path = wfindfile(PixmapPath, file);
	if (!path) {
	    wwarning("%s:could not find image file used in texture", file);
	    return NULL;
	}
    } else {
	path = wstrdup(file);
    }

    image = RLoadImage(rc, path, 0);
    if (!image) {
	wwarning("%s:could not load image file used in texture:%s", path,
		 RMessageForError(RErrorCode));
    }
    free(path);

    return image;
}


BackgroundTexture*
parseTexture(RContext *rc, char *text)
{
    BackgroundTexture *texture = NULL;
    proplist_t texarray;
    proplist_t val;
    int count;
    char *tmp;
    char *type;

#define GETSTR(val, str, i) \
		val = PLGetArrayElement(texarray, i);\
		if (!PLIsString(val)) {\
			wwarning("could not parse texture %s", text);\
			goto error;\
		}\
		str = PLGetString(val)

    
    texarray = PLGetProplistWithDescription(text);
    if (!texarray || !PLIsArray(texarray) 
	|| (count = PLGetNumberOfElements(texarray)) < 2) {

	wwarning("could not parse texture %s", text);
	if (texarray)
	    PLRelease(texarray);
	return NULL;
    }

    texture = wmalloc(sizeof(BackgroundTexture));
    memset(texture, 0, sizeof(BackgroundTexture));

    GETSTR(val, type, 0);

    if (strcasecmp(type, "solid")==0) {
	XColor color;
	Pixmap pixmap;

	texture->solid = 1;
	
	GETSTR(val, tmp, 1);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s", tmp, text);
	    goto error;
	}
	XAllocColor(dpy, DefaultColormap(dpy, scr), &color);

	pixmap = XCreatePixmap(dpy, root, 8, 8, DefaultDepth(dpy, scr));
	XSetForeground(dpy, DefaultGC(dpy, scr), color.pixel);
	XFillRectangle(dpy, pixmap, DefaultGC(dpy, scr), 0, 0, 8, 8);

	texture->pixmap = pixmap;
	texture->color = color;
	texture->width = 8;
	texture->height = 8;
    } else if (strcasecmp(type, "vgradient")==0
	       || strcasecmp(type, "dgradient")==0
	       || strcasecmp(type, "hgradient")==0) {
	XColor color;
	RColor color1, color2;
	RImage *image;
	Pixmap pixmap;
	int gtype;
	int iwidth, iheight;

	GETSTR(val, tmp, 1);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s", tmp, text);
	    goto error;
	}

	color1.red = color.red >> 8;
	color1.green = color.green >> 8;
	color1.blue = color.blue >> 8;

	GETSTR(val, tmp, 2);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s", tmp, text);
	    goto error;
	}

	color2.red = color.red >> 8;
	color2.green = color.green >> 8;
	color2.blue = color.blue >> 8;

	switch (type[0]) {
	 case 'h':
	 case 'H':
	    gtype = RHorizontalGradient;
	    iwidth = scrWidth;
	    iheight = 1;
	    break;
	 case 'V':
	 case 'v':
	    gtype = RVerticalGradient;
	    iwidth = 1;
	    iheight = scrHeight;
	    break;
	 default:
	    gtype = RDiagonalGradient;
	    iwidth = scrWidth;
	    iheight = scrHeight;
	    break;
	}

	image = RRenderGradient(iwidth, iheight, &color1, &color2, gtype);

	if (!image) {
	    wwarning("could not render gradient texture:%s",
		     RMessageForError(RErrorCode));
	    goto error;
	}

	if (!RConvertImage(rc, image, &pixmap)) {
	    wwarning("could not convert texture:%s", 
		     RMessageForError(RErrorCode));
	    RDestroyImage(image);
	    goto error;
	}

	texture->width = image->width;
	texture->height = image->height;
	RDestroyImage(image);

	texture->pixmap = pixmap;
    } else if (strcasecmp(type, "mvgradient")==0
	       || strcasecmp(type, "mdgradient")==0
	       || strcasecmp(type, "mhgradient")==0) {
	XColor color;
	RColor **colors;
	RImage *image;
	Pixmap pixmap;
	int i, j;
	int gtype;
	int iwidth, iheight;

	colors = malloc(sizeof(RColor*)*(count-1));
	if (!colors) {
	    wwarning("out of memory while parsing texture");
	    goto error;
	}
	memset(colors, 0, sizeof(RColor*)*(count-1));

	for (i = 2; i < count; i++) {
	    val = PLGetArrayElement(texarray, i);
	    if (!PLIsString(val)) {
		wwarning("could not parse texture %s", text);

		for (j = 0; colors[j]!=NULL; j++)
		    free(colors[j]);
		free(colors);
		goto error;
	    }
	    tmp = PLGetString(val);

	    if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
		wwarning("could not parse color %s in texture %s",
			 tmp, text);

		for (j = 0; colors[j]!=NULL; j++)
		    free(colors[j]);
		free(colors);
		goto error;
	    }
	    if (!(colors[i-2] = malloc(sizeof(RColor)))) {
		wwarning("out of memory while parsing texture");

		for (j = 0; colors[j]!=NULL; j++)
		    free(colors[j]);
		free(colors);
		goto error;
	    }

	    colors[i-2]->red = color.red >> 8;
	    colors[i-2]->green = color.green >> 8;
	    colors[i-2]->blue = color.blue >> 8;
	}

	switch (type[1]) {
	 case 'h':
	 case 'H':
	    gtype = RHorizontalGradient;
	    iwidth = scrWidth;
	    iheight = 1;
	    break;
	 case 'V':
	 case 'v':
	    gtype = RVerticalGradient;
	    iwidth = 1;
	    iheight = scrHeight;
	    break;
	 default:
	    gtype = RDiagonalGradient;
	    iwidth = scrWidth;
	    iheight = scrHeight;
	    break;
	}

	image = RRenderMultiGradient(iwidth, iheight, colors, gtype);
	
	for (j = 0; colors[j]!=NULL; j++)
	    free(colors[j]);
	free(colors);

	if (!image) {
	    wwarning("could not render gradient texture:%s",
		     RMessageForError(RErrorCode));
	    goto error;
	}

	if (!RConvertImage(rc, image, &pixmap)) {
	    wwarning("could not convert texture:%s", 
		     RMessageForError(RErrorCode));
	    RDestroyImage(image);
	    goto error;
	}

	texture->width = image->width;
	texture->height = image->height;
	RDestroyImage(image);

	texture->pixmap = pixmap;
    } else if (strcasecmp(type, "cpixmap")==0
	  || strcasecmp(type, "spixmap")==0
	  || strcasecmp(type, "mpixmap")==0
	  || strcasecmp(type, "tpixmap")==0) {
	XColor color;
	Pixmap pixmap = None;
	RImage *image = NULL;
	int w, h;
	int iwidth, iheight;

	GETSTR(val, tmp, 1);
/*
	if (toupper(type[0]) == 'T' || toupper(type[0]) == 'C')
	    pixmap = LoadJPEG(rc, tmp, &iwidth, &iheight);
 */

	if (!pixmap) {
	    image = loadImage(rc, tmp);
	    if (!image) {
		goto error;
	    }
	    iwidth = image->width;
	    iheight = image->height;
	}

	GETSTR(val, tmp, 2);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s\n", tmp, text);
	    RDestroyImage(image);
	    goto error;
	}
	if (!XAllocColor(dpy, DefaultColormap(dpy, scr), &color)) {
	    RColor rcolor;

	    rcolor.red = color.red >> 8;
	    rcolor.green = color.green >> 8;
	    rcolor.blue = color.blue >> 8;
	    RGetClosestXColor(rc, &rcolor, &color);
	}
	switch (toupper(type[0])) {
	 case 'T':
	    texture->width = iwidth;
	    texture->height = iheight;
	    if (!pixmap && !RConvertImage(rc, image, &pixmap)) {
		wwarning("could not convert texture:%s", 
			 RMessageForError(RErrorCode));
		RDestroyImage(image);
		goto error;
	    }
	    if (image)
		RDestroyImage(image);
	    break;
	 case 'S':
	 case 'M':
	    if (toupper(type[0])=='S') {
		w = scrWidth;
		h = scrHeight;
	    } else {
		if (iwidth*scrHeight > iheight*scrWidth) {
		    w = scrWidth;
		    h = (scrWidth*iheight)/iwidth;
		} else {
		    h = scrHeight;
		    w = (scrHeight*iwidth)/iheight;
		}
	    }
	    {
		RImage *simage;

		simage = RScaleImage(image, w, h);
		if (!simage) {
		    wwarning("could not scale image:%s", 
			     RMessageForError(RErrorCode));
		    RDestroyImage(image);
		    goto error;
		}
		RDestroyImage(image);
		image = simage;
		iwidth = image->width;
		iheight = image->height;
	    }
	    /* fall through */
	 case 'C':
	    {
		Pixmap cpixmap;

		if (!pixmap && !RConvertImage(rc, image, &pixmap)) {
		    wwarning("could not convert texture:%s", 
			     RMessageForError(RErrorCode));
		    RDestroyImage(image);
		    goto error;
		}

		if (iwidth != scrWidth || iheight != scrHeight) {
		    int x, y, sx, sy, w, h;

		    cpixmap = XCreatePixmap(dpy, root, scrWidth, scrHeight,
					    DefaultDepth(dpy, scr));

		    XSetForeground(dpy, DefaultGC(dpy, scr), color.pixel);
		    XFillRectangle(dpy, cpixmap, DefaultGC(dpy, scr),
				   0, 0, scrWidth, scrHeight);

		    if (iheight < scrHeight) {
			h = iheight;
			y = (scrHeight - h)/2;
			sy = 0;
		    } else {
			sy = (iheight - scrHeight)/2;
			y = 0;
			h = scrHeight;
		    }
		    if (iwidth < scrWidth) {
			w = iwidth;
			x = (scrWidth - w)/2;
			sx = 0;
		    } else {
			sx = (iwidth - scrWidth)/2;
			x = 0;
			w = scrWidth;
		    }

		    XCopyArea(dpy, pixmap, cpixmap, DefaultGC(dpy, scr),
			      sx, sy, w, h, x, y);
		    XFreePixmap(dpy, pixmap);
		    pixmap = cpixmap;
		}
		if (image)
		    RDestroyImage(image);

		texture->width = scrWidth;
		texture->height = scrHeight;
	    }
	    break;
	}

	texture->pixmap = pixmap;
	texture->color = color;
    } else if (strcasecmp(type, "thgradient")==0
	       || strcasecmp(type, "tvgradient")==0
	       || strcasecmp(type, "tdgradient")==0) {
	XColor color;
	RColor color1, color2;
	RImage *image;
	RImage *gradient;
	RImage *tiled;
	Pixmap pixmap;
	int opaq;
	char *file;
	int gtype;
	int twidth, theight;

	GETSTR(val, file, 1);

	GETSTR(val, tmp, 2);

	opaq = atoi(tmp);

	GETSTR(val, tmp, 3);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s", tmp, text);
	    goto error;
	}

	color1.red = color.red >> 8;
	color1.green = color.green >> 8;
	color1.blue = color.blue >> 8;

	GETSTR(val, tmp, 4);

	if (!XParseColor(dpy, DefaultColormap(dpy, scr), tmp, &color)) {
	    wwarning("could not parse color %s in texture %s", tmp, text);
	    goto error;
	}

	color2.red = color.red >> 8;
	color2.green = color.green >> 8;
	color2.blue = color.blue >> 8;

	image = loadImage(rc, file);
	if (!image) {
	    RDestroyImage(gradient);
	    goto error;
	}

	switch (type[1]) {
	 case 'h':
	 case 'H':
	    gtype = RHorizontalGradient;
	    twidth = scrWidth;
	    theight = image->height > scrHeight ? scrHeight : image->height;
	    break;
	 case 'V':
	 case 'v':
	    gtype = RVerticalGradient;
	    twidth = image->width > scrWidth ? scrWidth : image->width;
	    theight = scrHeight;
	    break;
	 default:
	    gtype = RDiagonalGradient;
	    twidth = scrWidth;
	    theight = scrHeight;
	    break;
	}
	gradient = RRenderGradient(twidth, theight, &color1, &color2, gtype);

	if (!gradient) {
	    wwarning("could not render texture:%s",
		     RMessageForError(RErrorCode));
	    RDestroyImage(gradient);
	    goto error;
	}

	tiled = RMakeTiledImage(image, twidth, theight);
	if (!tiled) {
	    wwarning("could not render texture:%s",
		     RMessageForError(RErrorCode));
	    RDestroyImage(gradient);
	    RDestroyImage(image);
	    goto error;
	}
	RDestroyImage(image);

	RCombineImagesWithOpaqueness(tiled, gradient, opaq);
	RDestroyImage(gradient);

	if (!RConvertImage(rc, tiled, &pixmap)) {
	    wwarning("could not convert texture:%s", 
		     RMessageForError(RErrorCode));
	    RDestroyImage(image);
	    goto error;
	}
	texture->width = tiled->width;
	texture->height = tiled->height;

	RDestroyImage(tiled);

	texture->pixmap = pixmap;	
    } else {
	wwarning("invalid texture type %s", text);
	goto error;
    }

    texture->spec = wstrdup(text);

    return texture;

error:
    if (texture)
	free(texture);
    if (texarray)
	PLRelease(texarray);

    return NULL;
}


void
freeTexture(BackgroundTexture *texture)
{
    if (texture->solid) {
	long pixel[1];

	pixel[0] = texture->color.pixel;
	/* dont free black/white pixels */
	if (pixel[0]!=BlackPixelOfScreen(DefaultScreenOfDisplay(dpy))
	    && pixel[0]!=WhitePixelOfScreen(DefaultScreenOfDisplay(dpy)))
	    XFreeColors(dpy, DefaultColormap(dpy, scr), pixel, 1, 0);
    }
    free(texture->spec);
    free(texture);
}


void
setupTexture(RContext *rc, BackgroundTexture **textures, int *maxTextures,
	     int workspace, char *texture)
{
    BackgroundTexture *newTexture = NULL;
    int i;

    /* unset the texture */
    if (!texture) {
	if (textures[workspace]!=NULL) {
	    textures[workspace]->refcount--;

	    if (textures[workspace]->refcount == 0)
		freeTexture(textures[workspace]);
	}
	textures[workspace] = NULL;
	return;
    }

    if (textures[workspace] 
	&& strcasecmp(textures[workspace]->spec, texture)==0) {
	/* texture did not change */
	return;
    }

    /* check if the same texture is already created */
    for (i = 0; i < *maxTextures; i++) {
	if (textures[i] && strcasecmp(textures[i]->spec, texture)==0) {
	    newTexture = textures[i];
	    break;
	}
    }

    if (!newTexture) {
	/* create the texture */
	newTexture = parseTexture(rc, texture);
    }
    if (!newTexture)
	return;

    if (textures[workspace]!=NULL) {

	textures[workspace]->refcount--;

	if (textures[workspace]->refcount == 0)
	    freeTexture(textures[workspace]);
    }

    newTexture->refcount++;
    textures[workspace] = newTexture;

    if (*maxTextures < workspace)
	*maxTextures = workspace;
}



Pixmap
duplicatePixmap(Pixmap pixmap, int width, int height)
{
    Display *tmpDpy;
    Pixmap copyP;

    /* must open a new display or the RetainPermanent will
     * leave stuff allocated in RContext unallocated after exit */
    tmpDpy = XOpenDisplay(display);
    if (!tmpDpy) {
	wwarning("could not open display to update background image information");

	return None;
    } else {
	XSync(dpy, False);

	copyP = XCreatePixmap(tmpDpy, root, width, height,
			       DefaultDepth(tmpDpy, scr));
	XCopyArea(tmpDpy, pixmap, copyP, DefaultGC(tmpDpy, scr),
		  0, 0, width, height, 0, 0);
	XSync(tmpDpy, False);

	XSetCloseDownMode(tmpDpy, RetainPermanent);
	XCloseDisplay(tmpDpy);
    }

    return copyP;
}


static int
dummyErrorHandler(Display *dpy, XErrorEvent *err)
{
    return 0;
}

void
setPixmapProperty(Pixmap pixmap)
{
    static Atom prop = 0;
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;
    int mode;
    
    if (!prop) {
	prop = XInternAtom(dpy, "_XROOTPMAP_ID", False);
    }

    XGrabServer(dpy);

    /* Clear out the old pixmap */
    XGetWindowProperty(dpy, root, prop, 0L, 1L, False, AnyPropertyType,
                       &type, &format, &length, &after, &data);

    if ((type == XA_PIXMAP) && (format == 32) && (length == 1)) {
	XSetErrorHandler(dummyErrorHandler);
        XKillClient(dpy, *((Pixmap *)data));
	XSync(dpy, False);
	XSetErrorHandler(NULL);
	mode = PropModeReplace;
    } else {
	mode = PropModeAppend;
    }
    if (pixmap)
	XChangeProperty(dpy, root, prop, XA_PIXMAP, 32, mode,
			(unsigned char *) &pixmap, 1);
    else
	XDeleteProperty(dpy, root, prop);


    XUngrabServer(dpy);
    XFlush(dpy);
}



void
changeTexture(BackgroundTexture *texture)
{
    if (!texture)
	return;

    if (texture->solid) {
	XSetWindowBackground(dpy, root, texture->color.pixel);
    } else {
	XSetWindowBackgroundPixmap(dpy, root, texture->pixmap);
    }
    XClearWindow(dpy, root);

    XSync(dpy, False);

    {
	Pixmap pixmap;
	
	pixmap = duplicatePixmap(texture->pixmap, texture->width, 
				 texture->height);

	setPixmapProperty(pixmap);
    }
}


int
readmsg(int fd, unsigned char *buffer, int size)
{
    int count;
    
    count = 0;
    while (size>0) {
	count = read(fd, buffer, size);
	if (count < 0)
	    return -1;
	size -= count;
	buffer += count;
	*buffer = 0;
    }

    return size;
}


/*
 * Message Format:
 * sizeSntexture_spec - sets the texture for workspace n
 * sizeCn - change background texture to the one for workspace n
 * sizePpath - set the pixmap search path
 * 
 * n is 4 bytes
 * size = 4 bytes for length of the message data
 */
void
helperLoop(RContext *rc)
{
    BackgroundTexture *textures[WORKSPACE_COUNT];
    int maxTextures = 0;
    unsigned char buffer[2048], buf[8];
    int size;
    int errcount = 4;

    memset(textures, 0, WORKSPACE_COUNT*sizeof(BackgroundTexture*));


    while (1) {
	int workspace;

	/* get length of message */
	if (readmsg(0, buffer, 4) < 0) {
	    wsyserror("error reading message from Window Maker");
	    errcount--;
	    if (errcount == 0) {
		wfatal("quitting");
		exit(1);
	    }
	    continue;
	}
	memcpy(buf, buffer, 4);
	buf[4] = 0;
	size = atoi(buf);

	/* get message */
	if (readmsg(0, buffer, size) < 0) {
	    wsyserror("error reading message from Window Maker");
	    errcount--;
	    if (errcount == 0) {
		wfatal("quitting");
		exit(1);
	    }
	    continue;
	}
#ifdef DEBUG
	printf("RECEIVED %s\n",buffer);
#endif
	if (buffer[0]!='P' && buffer[0]!='K') {
	    memcpy(buf, &buffer[1], 4);
	    buf[4] = 0;
	    workspace = atoi(buf);
	    if (workspace < 0 || workspace >= WORKSPACE_COUNT) {
		wwarning("received message with invalid workspace number %i\n",
			 workspace);
		continue;
	    }
	}

	switch (buffer[0]) {
	 case 'S':
#ifdef DEBUG
	    printf("set texture %s\n", &buffer[5]);
#endif
	    setupTexture(rc, textures, &maxTextures, workspace, &buffer[5]);
	    break;

	 case 'C':
#ifdef DEBUG
	    printf("change texture %i\n", workspace);
#endif
	    if (!textures[workspace])
		changeTexture(textures[0]);
	    else
		changeTexture(textures[workspace]);
	    break;

	 case 'P':
#ifdef DEBUG
	    printf("change pixmappath %s\n", &buffer[1]);
#endif
	    if (PixmapPath)
		free(PixmapPath);
	    PixmapPath = wstrdup(&buffer[1]);
	    break;

	 case 'U':
#ifdef DEBUG
	    printf("unset workspace %i\n", workspace);
#endif
	    setupTexture(rc, textures, &maxTextures, workspace, NULL);
	    break;

	 case 'K':
#ifdef DEBUG
	    printf("exit command\n");
#endif
	    exit(0);

	 default:
	    wwarning("unknown message received");
	    break;
	}
    }
}


void
updateDomain(char *domain, char *key, char *texture)
{
    char *program = "wdwrite";

    execlp(program, program, domain, key, texture, NULL);
    wwarning("warning could not run \"%s\"", program);
}



char*
globalDefaultsPathForDomain(char *domain)
{
    char path[1024];

    sprintf(path, "%s/%s", SYSCONFDIR, domain);

    return wstrdup(path);
}


proplist_t
getValueForKey(char *domain, char *keyName)
{
    char *path;
    proplist_t key;
    proplist_t d;
    proplist_t val;

    key = PLMakeString(keyName);

    /* try to find PixmapPath in user defaults */
    path = wdefaultspathfordomain(domain);
    d = PLGetProplistWithPath(path);
    if (!d) {
	wwarning("could not open domain file %s", path);
    }
    free(path);

    if (d && !PLIsDictionary(d)) {
	PLRelease(d);
	d = NULL;
    }
    if (d) {
	val = PLGetDictionaryEntry(d, key);
    } else {
	val = NULL;
    }
    /* try to find PixmapPath in global defaults */
    if (!val) {
	path = globalDefaultsPathForDomain(domain);
	if (!path) {
	    wwarning("could not locate file for domain %s", domain);
	    d = NULL;
	} else {
	    d = PLGetProplistWithPath(path);
	    free(path);
	}

	if (d && !PLIsDictionary(d)) {
	    PLRelease(d);
	    d = NULL;
	}
	if (d) {
	    val = PLGetDictionaryEntry(d, key);
	    
	} else {
	    val = NULL;
	}
    }

    if (val)
	PLRetain(val);

    PLRelease(key);
    if (d)
	PLRelease(d);

    return val;
}

char*
getPixmapPath(char *domain)
{
    proplist_t val;
    char *ptr, *data;
    int len, i, count;

    val = getValueForKey(domain, "PixmapPath");
    
    if (!val || !PLIsArray(val)) {
	if (val)
	    PLRelease(val);
	return wstrdup("");
    }

    count = PLGetNumberOfElements(val);
    len = 0;
    for (i=0; i<count; i++) {
	proplist_t v;

	v = PLGetArrayElement(val, i);
	if (!v || !PLIsString(v)) {
	    continue;
	}
	len += strlen(PLGetString(v))+1;
    }

    ptr = data = wmalloc(len+1);
    *ptr = 0;

    for (i=0; i<count; i++) {
	proplist_t v;

	v = PLGetArrayElement(val, i);
	if (!v || !PLIsString(v)) {
	    continue;
	}
	strcpy(ptr, PLGetString(v));

	ptr += strlen(PLGetString(v));
	*ptr = ':';
	ptr++;
    }
    if (i>0)
	ptr--; *(ptr--) = 0;

    PLRelease(val);

    return data;
}


void
wAbort()
{
    wfatal("aborting");
    exit(1);
}



void
print_help(char *ProgName)
{
    printf("Usage: %s [options] [image]\n", ProgName);
    puts("Sets the workspace background to the specified image or a texture and optionally update Window Maker configuration");
    puts("");
#define P(m) puts(m)
P(" -display	 		display to use");
P(" -d, --dither	 		dither image");
P(" -m, --match			match  colors");
P(" -b, --back-color <color>	background color");
P(" -t, --tile			tile   image");
P(" -e, --center			center image");
P(" -s, --scale			scale  image (default)");
P(" -a, --maxscale			scale  image and keep aspect ratio");
P(" -u, --update-wmaker		update WindowMaker domain database");
P(" -D, --update-domain <domain>	update <domain> database");
P(" -c, --colors <cpc>		colors per channel to use");
P(" -p, --parse <texture>		proplist style texture specification");
P(" -w, --workspace <workspace>	update background for the specified workspace");
P("     --version 			show version of wmsetbg and exit");
P("     --help 			show this help and exit");    
#undef P
}



void
changeTextureForWorkspace(char *domain, char *texture, int workspace)
{
    proplist_t array;
    proplist_t val;
    char *value;
    int j;
    
    val = PLGetProplistWithDescription(texture);
    if (!val) {
	wwarning("could not parse texture %s", texture);
	return;
    }

    array = getValueForKey("WindowMaker", "WorkspaceSpecificBack");

    if (!array) {
	array = PLMakeArrayFromElements(NULL, NULL);
    }

    j = PLGetNumberOfElements(array);
    if (workspace >= j) {
	proplist_t empty;

	empty = PLMakeArrayFromElements(NULL, NULL);

	while (j++ < workspace-1) {
	    PLAppendArrayElement(array, empty);
	}
	PLAppendArrayElement(array, val);
    } else {
	PLRemoveArrayElement(array, workspace);
	PLInsertArrayElement(array, val, workspace);
    }

    value = PLGetDescription(array);
    updateDomain(domain, "WorkspaceSpecificBack", value);
}


int
main(int argc, char **argv)
{
    int i;
    int helperMode = 0;
    RContext *rc;
    RContextAttributes rattr;
    char *style = "spixmap";
    char *back_color = "gray20";
    char *image_name = NULL;
    char *domain = "WindowMaker";
    int update=0, cpc=4, render_mode=RM_DITHER, obey_user=0;
    char *texture = NULL;
    int workspace = -1;
    
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    
    WMInitializeApplication("wmsetbg", &argc, argv);
    
    for (i=1; i<argc; i++) {
	if (strcmp(argv[i], "-helper")==0) {
	    helperMode = 1;
	} else if (strcmp(argv[i], "-display")==0) {
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    display = argv[i];
	} else if (strcmp(argv[i], "-s")==0
		   || strcmp(argv[i], "--scale")==0) {
	    style = "spixmap";
	} else if (strcmp(argv[i], "-t")==0
		   || strcmp(argv[i], "--tile")==0) {
	    style = "tpixmap";
	} else if (strcmp(argv[i], "-e")==0
		   || strcmp(argv[i], "--center")==0) {
	    style = "cpixmap";
	} else if (strcmp(argv[i], "-a")==0
		   || strcmp(argv[i], "--maxscale")==0) {
	    style = "mpixmap";	    
	} else if (strcmp(argv[i], "-d")==0
		   || strcmp(argv[i], "--dither")==0) {
	    render_mode = RM_DITHER;
	    obey_user++;
	} else if (strcmp(argv[i], "-m")==0
		   || strcmp(argv[i], "--match")==0) {
	    render_mode = RM_MATCH;
	    obey_user++;
	} else if (strcmp(argv[i], "-u")==0
		   || strcmp(argv[i], "--update-wmaker")==0) {
	    update++;
	} else if (strcmp(argv[i], "-D")==0
		   || strcmp(argv[i], "--update-domain")==0) {
	    update++;
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    domain = wstrdup(argv[i]);
	} else if (strcmp(argv[i], "-c")==0
		   || strcmp(argv[i], "--colors")==0) {
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    if (sscanf(argv[i], "%i", &cpc)!=1) {
		wfatal("bad value for colors per channel: \"%s\"\n", argv[i]);
		exit(1);
	    }
	} else if (strcmp(argv[i], "-b")==0
		   || strcmp(argv[i], "--back-color")==0) {
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    back_color = argv[i];
	} else if (strcmp(argv[i], "-p")==0
		   || strcmp(argv[i], "--parse")==0) {
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    texture = argv[i];
	} else if (strcmp(argv[i], "-w")==0
		   || strcmp(argv[i], "--workspace")==0) {
	    i++;
	    if (i>=argc) {
		wfatal("too few arguments for %s\n", argv[i-1]);
		exit(1);
	    }
	    if (sscanf(argv[i], "%i", &workspace)!=1) {
		wfatal("bad value for workspace number: \"%s\"", 
			argv[i]);
		exit(1);
	    }
	} else if (strcmp(argv[i], "--version")==0) {

	    printf(PROG_VERSION);
	    exit(0);

	} else if (strcmp(argv[i], "--help")==0) {
	    print_help(argv[0]);
	    exit(0);
	} else if (argv[i][0] != '-') {
	    image_name = argv[i];
	} else {
	    printf("%s: invalid argument '%s'\n", argv[0], argv[i]);
	    printf("Try '%s --help' for more information\n", argv[0]);
	    exit(1);
	}
    }
    if (!image_name && !texture && !helperMode) {
	printf("%s: you must specify a image file name or a texture\n", 
	       argv[0]);
	printf("Try '%s --help' for more information\n", argv[0]);
	exit(1);
    }

    
    PixmapPath = getPixmapPath(domain);

    dpy = XOpenDisplay(display);
    if (!dpy) {
	wfatal("could not open display");
	exit(1);
    }
#if 0
    XSynchronize(dpy, 1);
#endif

    root = DefaultRootWindow(dpy);

    scr = DefaultScreen(dpy);
    
    scrWidth = WidthOfScreen(DefaultScreenOfDisplay(dpy));
    scrHeight = HeightOfScreen(DefaultScreenOfDisplay(dpy));

    if (!obey_user && DefaultDepth(dpy, scr) <= 8)
        render_mode = RM_DITHER;

    rattr.flags = RC_RenderMode | RC_ColorsPerChannel | RC_DefaultVisual;
    rattr.render_mode = render_mode;
    rattr.colors_per_channel = cpc;

    rc = RCreateContext(dpy, scr, &rattr);

    if (helperMode) {
	/* lower priority, so that it wont use all the CPU */
	nice(1000);

	helperLoop(rc);
    } else {
	BackgroundTexture *tex;
	char buffer[4098];

	if (!texture) {
	    sprintf(buffer, "(%s, \"%s\", %s)", style, image_name, back_color);
	    texture = (char*)buffer;
	}

	if (update && workspace < 0) {
	    updateDomain(domain, "WorkspaceBack", texture);
	}

	tex = parseTexture(rc, texture);
	if (!tex)
	    exit(1);

	if (workspace<0)
	    changeTexture(tex);
	else {
	    /* always update domain */
	    changeTextureForWorkspace(domain, texture, workspace-1);
	}
    }

    return 0;
}

