
#include <X11/Xlib.h>
#include "wraster.h"
#include <stdio.h>

Display *dpy;
Window win;
RContext *ctx;
RImage *img, *tile, *new, *mini, *tiled;
Pixmap pix;

void main(int argc, char **argv)
{
    RContextAttributes attr;
	int a=0;

	if (argc<1) {
		puts("You must supply t,p or x as the file type to load");
		puts("t is tiff, p is png and x is xpm");
		exit(0);
	}

	if (argc>1) {
		if (argv[1][0]=='t') 
			a=1;
		else if (argv[1][0]=='p')
			a=2;
		else a=0;
	}

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("cant open display");
	exit(1);
    }
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, 250, 250,
			      0, 0, 0);
    XMapRaised(dpy, win);
    XFlush(dpy);    
    attr.flags = RC_RenderMode | RC_ColorsPerChannel | RC_DefaultVisual;

    attr.render_mode = RM_DITHER;
    attr.colors_per_channel = 4;
    ctx = RCreateContext(dpy, DefaultScreen(dpy), &attr);
#ifdef USE_TIFF	
    if (a==1)
    img = RLoadImage(ctx, "ballot_box.tiff", 0);
#endif
#ifdef USE_PNG
    if (a==2)
    img = RLoadImage(ctx, "test.png", 0);
#endif
#ifdef USE_XPM
    if (a==0)
    img = RLoadImage(ctx, "ballot_box.xpm", 0);
#endif

    if (!img) {
	puts(RMessageForError(RErrorCode));
	exit(1);
    }
    new = RLoadImage(ctx, "tile.xpm", 0);
    if (!new) {
	puts(RMessageForError(RErrorCode));
	exit(1);
    }
    RCombineArea(new, img, 0, 0, img->width, img->height, 8, 8);
    RConvertImage(ctx, new, &pix);
    XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, new->width, new->height, 
	      0, 0);

    mini = RScaleImage(new, 20, 20);
    RConvertImage(ctx, mini, &pix);
    XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, 20, 20, 
		new->width, new->height);
    
    tiled = RMakeTiledImage(img, 160, 160);
    RConvertImage(ctx, tiled, &pix);
    XCopyArea(dpy, pix, win, ctx->copy_gc, 0, 0, 160, 160, 
		new->width+mini->width, new->height+mini->height);
    
    XFlush(dpy);
    getchar();  
}
