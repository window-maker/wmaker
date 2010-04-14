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
	cairo_t *ct;
	cairo_pattern_t *linpat;
	int height = 35;
	int width = 130;

	surf = (cairo_surface_t *) cairo_xlib_surface_create(dpy, win, vis, 500, 500);
	ct = cairo_create(surf);

	//cairo_set_line_width(ct, 1);

	linpat = cairo_pattern_create_linear(0,0,0,height);
	cairo_pattern_add_color_stop_rgb(linpat, 0, 194.0/255, 190.0/255, 194.0/255);
	cairo_pattern_add_color_stop_rgb(linpat, 1, 174.0/255, 170.0/255, 174.0/255);
	cairo_set_source(ct,linpat);
	cairo_rectangle(ct,0,0,width,height);
	cairo_fill(ct);

	cairo_set_source_rgb(ct, 1, 1, 1);
	cairo_rectangle(ct,0,0,1,height-1);
	cairo_rectangle(ct,0,0,width-1,1);
	cairo_fill(ct);

	cairo_set_source_rgb(ct, 81.0/255, 85.0/255, 81.0/255);
	cairo_rectangle(ct,1,height-2,width-1,1);
	cairo_rectangle(ct,width-2,1,1,height-1);
	cairo_fill(ct);

	cairo_set_source_rgb(ct, 0, 0, 0);
	cairo_rectangle(ct,0,height-1,width,1);
	cairo_rectangle(ct,width-1,0,1,height);
	cairo_fill(ct);

	cairo_stroke(ct);

	cairo_pattern_destroy(linpat);
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
