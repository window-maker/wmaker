

#include <unistd.h>

#include "WINGsP.h"

#include <X11/Xutil.h>

/* Xmd.h which is indirectly included by GNUstep.h defines BOOL, 
 * but libPropList also defines it. So we do this kluge to get rid of BOOL
 * temporarily */
#ifdef BOOL
# define WINGS_BOOL
# undef BOOL
#endif

#include "GNUstep.h"

#ifdef WINGS_BOOL
# define BOOL
# undef WINGS_BOOL
#endif


extern struct W_Application WMApplication;


void
WMSetApplicationIconWindow(WMScreen *scr, Window window)
{    
    scr->applicationIconWindow = window;
    
    if (scr->groupLeader) {
	XWMHints *hints;

	hints = XGetWMHints(scr->display, scr->groupLeader);
	hints->flags |= IconWindowHint;
	hints->icon_window = window;
    
	XSetWMHints(scr->display, scr->groupLeader, hints);
	XFree(hints);
    }
}


void
WMSetApplicationIconImage(WMScreen *scr, WMPixmap *icon)
{    
    if (scr->applicationIcon)
	WMReleasePixmap(scr->applicationIcon);
    
    scr->applicationIcon = WMRetainPixmap(icon);
    
    if (scr->groupLeader) {
	XWMHints *hints;

	hints = XGetWMHints(scr->display, scr->groupLeader);
	hints->flags |= IconPixmapHint|IconMaskHint;
	hints->icon_pixmap = icon->pixmap;
	hints->icon_mask = icon->mask;
    
	XSetWMHints(scr->display, scr->groupLeader, hints);
	XFree(hints);
    }
}


WMPixmap*
WMGetApplicationIconImage(WMScreen *scr)
{
    return scr->applicationIcon;
}


void
WMSetApplicationHasAppIcon(WMScreen *scr, Bool flag)
{
    scr->aflags.hasAppIcon = flag;
}


void
W_InitApplication(WMScreen *scr)
{
    Window leader;
    XClassHint *classHint;
    XWMHints *hints;

    leader = XCreateSimpleWindow(scr->display, scr->rootWin, -1, -1,
				 1, 1, 0, 0, 0);

    if (!scr->aflags.simpleApplication) {
	classHint = XAllocClassHint();
	classHint->res_name = "groupLeader";
	classHint->res_class = WMApplication.applicationName;
	XSetClassHint(scr->display, leader, classHint);
	XFree(classHint);

	XSetCommand(scr->display, leader, WMApplication.argv,
		    WMApplication.argc);

	hints = XAllocWMHints();

	hints->flags = WindowGroupHint;
	hints->window_group = leader;

	if (scr->applicationIcon) {
	    hints->flags |= IconPixmapHint;
	    hints->icon_pixmap = scr->applicationIcon->pixmap;
	    if (scr->applicationIcon->mask) {
		hints->flags |= IconMaskHint;
		hints->icon_mask = scr->applicationIcon->mask;
	    }
	}
	
	XSetWMHints(scr->display, leader, hints);

	XFree(hints);
    }
    scr->groupLeader = leader;
}


