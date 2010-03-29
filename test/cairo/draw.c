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
static void curve_rectangle(cairo_t *cr,
		double x0, double y0, double rect_width, double rect_height,
		double radius)
{
	double x1,y1;

	x1=x0+rect_width;
	y1=y0+rect_height;
	if (!rect_width || !rect_height)
		return;
	if (rect_width/2<radius) {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	} else {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	}
	cairo_close_path (cr);
}

void draw(Display *dpy, Screen *scr, Visual *vis, Window win) {
	cairo_surface_t *surf;
	cairo_t *ct;

	int width=200;
	int height = 50;
	int x = 10;
	int y = 10;

	unsigned char border[4]= {0x00, 0x00, 0x00, 0x70};
	//unsigned char color1[4]= {0x8c, 0xb1, 0xbc, 0xff};
	//unsigned char color2[4]= {0xcb, 0xf3, 0xff, 0xff};
	unsigned char color1[4]= {0x0, 0x0, 0x0, 0xff};
	unsigned char color2[4]= {0xcf, 0xcf, 0xcf, 0xff};
	unsigned char scolor1[4]= {0xff, 0xff, 0xff, 0xe5};
	unsigned char scolor2[4]= {0xff, 0xff, 0xff, 0x70};

	surf = (cairo_surface_t *) cairo_xlib_surface_create(dpy, win, vis, 500, 500);
	ct = cairo_create(surf);

	cairo_pattern_t *shine;
	cairo_pattern_t *base;

	shine= cairo_pattern_create_linear(0, 0, 0, height*2/5);
	cairo_pattern_add_color_stop_rgba(shine, 0,
			scolor1[0]/255.0, scolor1[1]/255.0, scolor1[2]/255.0,
			scolor1[3]/255.0);
	cairo_pattern_add_color_stop_rgba(shine, 1,
			scolor2[0]/255.0, scolor2[1]/255.0, scolor2[2]/255.0,
			scolor2[3]/255.0);

	base= cairo_pattern_create_linear(0, 0, 0, height-1);
	cairo_pattern_add_color_stop_rgba(base, 0,
			color1[0]/255.0, color1[1]/255.0, color1[2]/255.0,
			color1[3]/255.0);
	cairo_pattern_add_color_stop_rgba(base, 1,
			color2[0]/255.0, color2[1]/255.0, color2[2]/255.0,
			color2[3]/255.0);


	curve_rectangle(ct, x, y, width-1, height-1, height*2/3);
	cairo_set_source(ct, base);
	cairo_fill_preserve(ct);
	cairo_clip(ct);

	curve_rectangle(ct, x, y, width-1, height*2/5, width);
	cairo_set_source(ct, shine);
	cairo_fill(ct);

	curve_rectangle(ct, x, y, width-1, height-1, height*2/3);
	cairo_set_source_rgba(ct, border[0]/255.0, border[1]/255.0, border[2]/255.0, border[3]/255.0);
	cairo_set_line_width(ct, 2.0);
	cairo_stroke(ct);

	cairo_pattern_destroy(shine);
	cairo_pattern_destroy(base);

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
