/*
 * magnify - a X utility for magnifying screen image
 * 
 * 2000/5/21  Alfredo K. Kojima
 * 
 * This program is in the Public Domain.
 */

#include <X11/Xproto.h>

#include <WINGs.h>
#include <stdlib.h>
#include <stdio.h>


int refreshrate = 200;


typedef struct {
    Drawable d;
    XRectangle *rects;
    int rectP;
    unsigned long lastpixel;
    unsigned long *buffer;
    int width, height;
    int magfactor;

    WMWindow *win;
    WMLabel *label;
    WMPixmap *pixmap;
    
    int x, y;
    Bool frozen;
    
    WMHandlerID tid;
} BufferData;



static BufferData *newWindow(int magfactor);


int windowCount = 0;

int rectBufferSize = 32;
Display *dpy;
WMScreen *scr;
unsigned int black;

#ifndef __GNUC__
#define inline
#endif


static BufferData*
makeBufferData(WMWindow *win, WMLabel *label, int width, int height,
	       int magfactor)
{
    BufferData *data;
    
    data = wmalloc(sizeof(BufferData));
    
    data->magfactor = magfactor;
    
    data->rects = wmalloc(sizeof(XRectangle)*rectBufferSize);
    data->rectP = 0;
    
    data->win = win;
    data->label = label;

    data->pixmap = WMCreatePixmap(scr, width, height,
				  WMScreenDepth(scr), False);
    WMSetLabelImage(data->label, data->pixmap);

    data->d = WMGetPixmapXID(data->pixmap);
    
    data->frozen = False;

    width /= magfactor;
    height /= magfactor;
    data->buffer = wmalloc(sizeof(unsigned long)*width*height);
    memset(data->buffer, 0, width*height*sizeof(unsigned long));
    data->width = width;
    data->height = height;

    return data;
}


static void
resizeBufferData(BufferData *data, int width, int height, int magfactor)
{
    int w = width/magfactor;
    int h = height/magfactor;

    data->magfactor = magfactor;
    data->buffer = wrealloc(data->buffer, sizeof(unsigned long)*w*h);
    data->width = w;
    data->height = h;
    memset(data->buffer, 0, w*h*sizeof(unsigned long));

    WMResizeWidget(data->label, width, height);

    WMReleasePixmap(data->pixmap);
    data->pixmap = WMCreatePixmap(scr, width, height, WMScreenDepth(scr), 
				  False);
    WMSetLabelImage(data->label, data->pixmap);
    
    data->d = WMGetPixmapXID(data->pixmap);
}


static int
drawpoint(BufferData *data, unsigned long pixel, int x, int y)
{
    static GC gc = NULL;

    if (data->buffer[x+data->width*y] == pixel)
	return 0;
    
    data->buffer[x+data->width*y] = pixel;

    if (gc == NULL) {
	gc = XCreateGC(dpy, DefaultRootWindow(dpy), 0, NULL);
    }
    
    if (data->lastpixel == pixel && data->rectP < rectBufferSize) {
	data->rects[data->rectP].x = x*data->magfactor;
	data->rects[data->rectP].y = y*data->magfactor;
	data->rects[data->rectP].width = data->magfactor;
	data->rects[data->rectP].height = data->magfactor;
	data->rectP++;

	return 0;
    }
    XSetForeground(dpy, gc, data->lastpixel);
    XFillRectangles(dpy, data->d, gc, data->rects, data->rectP);
    data->rectP = 0;
    data->rects[data->rectP].x = x*data->magfactor;
    data->rects[data->rectP].y = y*data->magfactor;
    data->rects[data->rectP].width = data->magfactor;
    data->rects[data->rectP].height = data->magfactor;
    data->rectP++;
   
    data->lastpixel = pixel;

    return 1;
}


static inline unsigned long
getpix(XImage *image, int x, int y, int xoffs, int yoffs)
{
    if (x < xoffs || y < yoffs
	|| x >= xoffs + image->width || y >= yoffs + image->height) {
	return black;
    }
    return XGetPixel(image, x-xoffs, y-yoffs);
}


static void
updateImage(BufferData *data, int rx, int ry)
{
    int gx, gy, gw, gh;
    int x, y;
    int xoffs, yoffs;
    int changedPixels = 0;
    XImage *image;

    gw = data->width;
    gh = data->height;
    
    gx = rx - gw/2;
    gy = ry - gh/2;
    
    xoffs = yoffs = 0;
    if (gx < 0) {
	xoffs = abs(gx);
	gw += gx;
	gx = 0;
    }
    if (gx + gw >= WidthOfScreen(DefaultScreenOfDisplay(dpy))) {
	gw = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - gx;
    }
    if (gy < 0) {
	yoffs = abs(gy);
	gh += gy;
	gy = 0;
    }
    if (gy + gh >= HeightOfScreen(DefaultScreenOfDisplay(dpy))) {
	gh = HeightOfScreen(DefaultScreenOfDisplay(dpy)) - gy;
    }

    image = XGetImage(dpy, DefaultRootWindow(dpy), gx, gy, gw, gh,
		      AllPlanes, ZPixmap);


    for (y = 0; y < data->height; y++) {
	for (x = 0; x < data->width; x++) {
	    unsigned long pixel;

	    pixel = getpix(image, x-xoffs, y-yoffs, xoffs, yoffs);

	    if (drawpoint(data, pixel, x, y))
		changedPixels++;
	}
    }

    XDestroyImage(image);

    if (changedPixels > 0) {
	WMRedisplayWidget(data->label);
    }
}


static void
update(void *d)
{
    BufferData *data = (BufferData*)d;
    Window win;
    int rx, ry;
    int bla;
    unsigned ubla;
    

    if (data->frozen) {
	rx = data->x;
	ry = data->y;
    } else {
	XQueryPointer(dpy, DefaultRootWindow(dpy), &win, &win, &rx, &ry, 
		      &bla, &bla, &ubla);
    }
    updateImage(data, rx, ry);

    data->tid = WMAddTimerHandler(refreshrate, update, data);
}

void resizedWindow(void *d, WMNotification *notif)
{
    BufferData *data = (BufferData*)d;
    WMView *view = (WMView*)WMGetNotificationObject(notif);
    WMSize size;

    size = WMGetViewSize(view);
    
    resizeBufferData(data, size.width, size.height, data->magfactor);
}



void closeWindow(WMWidget *w, void *d)
{
    BufferData *data = (BufferData*)d;

    windowCount--;
    if (windowCount == 0) {
	exit(0);
    } else {
	WMDeleteTimerHandler(data->tid);
	WMDestroyWidget(w);
	free(data->buffer);
	free(data->rects);
	WMReleasePixmap(data->pixmap);
	free(data);
    }
}


static void
keyHandler(XEvent *event, void *d)
{
    BufferData *data = (BufferData*)d;
    char buf[32];
    KeySym ks;
    WMView *view = WMWidgetView(data->win);
    WMSize size;

    size = WMGetViewSize(view);

    if (XLookupString(&event->xkey, buf, 31, &ks, NULL) > 0) {
	switch (buf[0]) {
	 case 'n':
	    newWindow(data->magfactor);
	    break;
	 case 'f':
	 case ' ':
	    data->frozen = !data->frozen;
	    if (data->frozen) {
		data->x = event->xkey.x_root;
		data->y = event->xkey.y_root;
		sprintf(buf, "[Magnify %ix]", data->magfactor);
	    } else {
		sprintf(buf, "Magnify %ix", data->magfactor);
	    }
	    WMSetWindowTitle(data->win, buf);
	    break;
	 case '1':
	 case '2':
	 case '3':
	 case '4':
	 case '5':
	 case '6':
	 case '7':
	 case '8':
	 case '9':
	    resizeBufferData(data, size.width, size.height, buf[0]-'0');
	    if (data->frozen) {
		sprintf(buf, "[Magnify %ix]", data->magfactor);
	    } else {
		sprintf(buf, "Magnify %ix", data->magfactor);
	    }
	    WMSetWindowTitle(data->win, buf);
	    break;
	}
    }
}


static BufferData*
newWindow(int magfactor)
{
    WMWindow *win;
    WMLabel *label;
    BufferData *data;
    char buf[32];

    windowCount++;
    
    win = WMCreateWindow(scr, "magnify");
    WMResizeWidget(win, 300, 200);
    sprintf(buf, "Magnify %ix", magfactor);
    WMSetWindowTitle(win, buf);
    WMSetViewNotifySizeChanges(WMWidgetView(win), True);

    label = WMCreateLabel(win);
    WMResizeWidget(label, 300, 200);
    WMMoveWidget(label, 0, 0);
    WMSetLabelRelief(label, WRSunken);
    WMSetLabelImagePosition(label, WIPImageOnly);
    
    data = makeBufferData(win, label, 300, 200, magfactor);

    WMCreateEventHandler(WMWidgetView(win), KeyReleaseMask,
			 keyHandler, data);
    
    WMAddNotificationObserver(resizedWindow, data,
			      WMViewSizeDidChangeNotification,
			      WMWidgetView(win));

    WMRealizeWidget(win);

    WMMapSubwidgets(win);
    WMMapWidget(win);

    WMSetWindowCloseAction(win, closeWindow, data);
    data->tid = WMAddTimerHandler(refreshrate, update, data);

    return data;
}


int main(int argc, char **argv)
{
    BufferData *data;
    int i;
    char *display = "";
    int magfactor = 2;
#if 0
    WMButton *radio, *tradio;
#endif
    WMInitializeApplication("Magnify", &argc, argv);
    
    
    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-display")==0) {
	    i++;
	    if (i >= argc)
		goto help;
	    display = argv[i];
	} else if (strcmp(argv[i], "-m")==0) {
	    i++;
	    if (i >= argc)
		goto help;
	    magfactor = atoi(argv[i]);
	    if (magfactor < 1 || magfactor > 32) {
		printf("%s:invalid magnification factor ``%s''\n", argv[0],
		       argv[i]);
		exit(1);
	    }
	} else if (strcmp(argv[i], "-r")==0) {
	    i++;
	    if (i >= argc)
		goto help;
	    refreshrate = atoi(argv[i]);
	    if (refreshrate < 1) {
		printf("%s:invalid refresh rate ``%s''\n", argv[0], argv[i]);
		exit(1);
	    }
	} else if (strcmp(argv[i], "-h")==0 
		   || strcmp(argv[i], "--help")==0) {
	help:
	    printf("Syntax: %s [-display <display>] [-m <number>] [-r <number>]\n",
		   argv[0]);
	    puts("-display <display>	display that should be used");
	    puts("-m <number>		change magnification factor (default 2)");
	    puts("-r <number>		change refresh delay, in milliseconds (default 200)");
	    exit(0);
	}
    }

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("couldnt open display");
	exit(1);
    }

    /* calculate how many rectangles we can send in a trip to the server */
    rectBufferSize = XMaxRequestSize(dpy) - 128;
    rectBufferSize /= sizeof(XRectangle);
    if (rectBufferSize < 1)
	rectBufferSize = 1;

    black = BlackPixel(dpy, DefaultScreen(dpy));

    scr = WMCreateScreen(dpy, 0);

    data = newWindow(magfactor);

    WMScreenMainLoop(scr);

    return 0;
}

