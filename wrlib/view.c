
#include <X11/Xlib.h>
#include "wraster.h"
#include <stdlib.h>
#include <stdio.h>
#include "tile.xpm"
Display *dpy;
Window win;
RContext *ctx;
RImage *img;
Pixmap pix;

void main(int argc, char **argv)
{
    RContextAttributes attr;

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("cant open display");
	exit(1);
    }

    attr.flags = RC_RenderMode | RC_ColorsPerChannel;
    attr.render_mode = RM_DITHER;
    attr.colors_per_channel = 4;
    ctx = RCreateContext(dpy, DefaultScreen(dpy), &attr);

    if (argc<2) 
	img = RGetImageFromXPMData(ctx, image_name);
    else
	img = RLoadImage(ctx, argv[1], argc>2 ? atol(argv[2]) : 0);

    if (!img) {
	puts(RMessageForError(RErrorCode));
	exit(1);
    }
    if (!RConvertImage(ctx, img, &pix)) {
	puts(RMessageForError(RErrorCode));
	exit(1);
    }

    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, img->width,
			      img->height, 0, 0, 0);
    XSelectInput(dpy, win, ExposureMask);
    XMapRaised(dpy, win);
    XSync(dpy, False); 
    XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 
	      0, 0);

    XSync(dpy, False); 
    while (1) {
	XEvent ev;
	XNextEvent(dpy, &ev);
	if (ev.type==Expose)
    	XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, img->width, img->height, 
	      0, 0);
    }
    XFlush(dpy);

    getchar();
}
