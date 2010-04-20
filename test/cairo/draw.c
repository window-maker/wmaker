#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <cairo/cairo.h>


/* This is meant to be a simple app to quickly test some cairo stuff.
 * It creates a single window and runs the "draw" function when it
 * receives an ExposeEvent. The idea is that you can put some quick
 * cairo code you want to try in draw and very quickly see the results
 * -- JH
 */

void draw(Display *dpy, Screen *scr, Visual *vis, Window win) {
	cairo_surface_t *surf;
	cairo_surface_t *imgsurf;
	cairo_t *ct;

	surf = (cairo_surface_t *) cairo_xlib_surface_create(dpy, win, vis, 500, 500);
	ct = cairo_create(surf);

	cairo_set_source_rgba(ct,0.5,0.5,0.5,1.0);
	cairo_rectangle(ct,0,0,499,499);
	cairo_fill(ct);
	cairo_stroke(ct);

	imgsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,10,10);
	char *buf = cairo_image_surface_get_data(imgsurf);
	int stride = cairo_image_surface_get_stride(imgsurf);

	int offs;
	int i;
	int j;
	for (i=0; i < 10; i++) {
		offs = i*stride;
		for (j=0; j < 10; j++) {
			//doesn't work, no black square
			buf[offs++] = 0x00; //alpha
			buf[offs++] = 0x00; //alpha
			buf[offs++] = 0x00; //alpha
			buf[offs++] = 0xff; //alpha


			//works <- white square
			//buf[offs++] = 0xff; //alpha
			//buf[offs++] = 0xff; //red
			//buf[offs++] = 0xff; //green
			//buf[offs++] = 0xff; //blue
		}
	}

	cairo_set_operator(ct,CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(ct,imgsurf,0,0);
	cairo_mask_surface(ct,imgsurf,0,0);
	cairo_stroke(ct);

	cairo_destroy(ct);
	cairo_surface_destroy(surf);
}

int main() {

	Display *dpy;
	Window win;
	Screen *scr;
	int scrnum;
	Visual *vis;
	Atom delete_win;
	Atom prots[6];
	XClassHint classhint;
	XSetWindowAttributes win_attr;
	char win_title[32];
	char alive;

	dpy = XOpenDisplay("");
	if (!dpy) {
		printf("could not open display");
		return 1;
	}

	delete_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	prots[0] = delete_win;

	scr = DefaultScreenOfDisplay(dpy);
	scrnum = DefaultScreen(dpy);
	vis = DefaultVisualOfScreen(scr);

	win_attr.background_pixel = WhitePixel(dpy,scrnum);
	win_attr.event_mask = ExposureMask;

	win = XCreateWindow(dpy,
			DefaultRootWindow(dpy),
			10,
			10,
			200,
			200,
			0,
			CopyFromParent,
			InputOutput,
			vis,
			CWEventMask | CWBackPixel,
			&win_attr);

	XSetWMProtocols(dpy,win,prots,1);

	sprintf(win_title,"Cairo Test");
	XStoreName(dpy,win,win_title);

	classhint.res_name = "cairo";
	classhint.res_class = "Cairo";
	XSetClassHint(dpy,win,&classhint);

	XMapWindow(dpy,win);

	XFlush(dpy);
	alive = 1;
	while (alive) {
		XEvent ev;
		XNextEvent(dpy, &ev);
		if (ev.type == ClientMessage) {
			if (ev.xclient.data.l[0] == delete_win) {
				XDestroyWindow(dpy, ev.xclient.window);
				alive = 0;
			}
		}
		if (ev.type == Expose) {
			draw(dpy,scr,vis,win);
		}
	}


	return 0;
}
