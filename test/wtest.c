/* quick and dirty test application that demonstrates: application hiding,
 *	application defined titlebar button images, application defined
 *	titlebar button actions, application menus, docking and
 *	window manager commands 
 * 
 * Note that the windows don't have a window command menu.
 *
 * TODO: remake
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <WMaker.h>

static unsigned char bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned char mbits[] = {
   0xff, 0x03, 0xff, 0x01, 0xff, 0x00, 0x7f, 0x00, 0x3f, 0x00, 0x1f, 0x00,
   0x0f, 0x00, 0x07, 0x00, 0x03, 0x00, 0x01, 0x00};


Display *dpy;
Window leader;
WMAppContext *app;


static void 
callback(void *foo, int item, Time time)
{
    printf("pushed item %i\n", item);
}

static void
quit(void *foo, int item, Time time)
{
    exit(0);
}



static void
hide(void *foo, int item, Time time)
{
    WMHideApplication(app);
}


Atom delete_win, miniaturize_win;
Atom prots[6];
GNUstepWMAttributes attr;
XWMHints *hints;
WMMenu *menu;
WMMenu *submenu;
int wincount=0;

static void 
newwin(void *foo, int item, Time time)
{
    Window win;
    XClassHint classhint;
    char title[100];

    wincount++;
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 
			      10*wincount, 10*wincount, 200, 100, 0, 0, 0);
    prots[0] = delete_win;
    prots[1] = miniaturize_win;
    XSetWMProtocols(dpy, win, prots, 2);
    sprintf(title, "Test Window %i", wincount);
    XStoreName(dpy, win, title);
    
    /* set class hint */
    classhint.res_name = "test";
    classhint.res_class = "Test";
    XSetClassHint(dpy, win, &classhint);

    /* set WindowMaker hints */
    attr.flags = GSMiniaturizePixmapAttr|GSMiniaturizeMaskAttr;
    attr.miniaturize_pixmap = 
      XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), bits, 10, 10);
    
    attr.miniaturize_mask =
      XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), mbits, 10, 10);
 /*   
    attr.flags |= GSWindowStyleAttr;
    attr.window_style = NSTitledWindowMask|NSClosableWindowMask;
*/
    
    WMSetWindowAttributes(dpy, win, &attr);

    hints = XAllocWMHints();
    /* set window group leader */
    hints->window_group = leader;
    hints->flags = WindowGroupHint;
    XSetWMHints(dpy, win, hints);
 
    WMAppAddWindow(app, win);   
    XMapWindow(dpy, win);
}

int main(int argc, char **argv)
{    
    XClassHint classhint;

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("could not open display!");
	exit(1);
    }
    delete_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    miniaturize_win = XInternAtom(dpy, "_GNUSTEP_WM_MINIATURIZE_WINDOW", 
				  False);

    leader = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, 10, 10,
			      0, 0, 0);
    /* set class hint */
    classhint.res_name = "test";
    classhint.res_class = "Test";
    XSetClassHint(dpy, leader, &classhint);

    /* set window group leader to self */
    hints = XAllocWMHints();
    hints->window_group = leader;
    hints->flags = WindowGroupHint;
    XSetWMHints(dpy, leader, hints);

    /* create app context */
    app = WMAppCreateWithMain(dpy, DefaultScreen(dpy), leader);
    menu = WMMenuCreate(app, "Test Menu");
    submenu = WMMenuCreate(app, "File");
    WMMenuAddSubmenu(menu, "File", submenu);
    
    WMMenuAddItem(menu, "Hide", (WMMenuAction)hide, NULL, NULL, NULL);
    WMMenuAddItem(menu, "Quit", (WMMenuAction)quit, NULL, NULL, NULL);
    WMMenuAddItem(submenu, "New", (WMMenuAction)newwin, NULL, NULL, NULL);
    WMMenuAddItem(submenu, "Open", (WMMenuAction)callback, NULL, NULL, NULL);
    WMMenuAddItem(submenu, "Save", (WMMenuAction)callback, NULL, NULL, NULL);
    WMMenuAddItem(submenu, "Save As...", (WMMenuAction)callback, NULL, NULL, NULL);
    
    WMAppSetMainMenu(app, menu);
    
    WMRealizeMenus(app);

    /* set command to use to startup this */
    XSetCommand(dpy, leader, argv, argc);
    
    /* create first window */
    newwin(NULL, 0, 0);


    XFlush(dpy);
    puts("Run xprop on the test window to see the properties defined");
    while (wincount>0) {
	XEvent ev;
	XNextEvent(dpy, &ev);
	if (ev.type==ClientMessage) {
	    if (ev.xclient.data.l[0]==delete_win) {
		XDestroyWindow(dpy,ev.xclient.window);
		wincount--;
	    } else if (ev.xclient.data.l[0]==miniaturize_win) {
		puts("You've pushed the maximize window button");
	    }
	} 
	WMProcessEvent(app, &ev);
    }
    exit(0);
}

