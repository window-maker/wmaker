#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "WindowMaker.h"
#include "funcs.h"
#include "wmsound.h"

#ifdef WMSOUND



extern WPreferences wPreferences;

#if 0
void
wSoundServerGrab(Window wm_win)
{
    Window	*lstChildren;
    Window	retRoot;
    Window	retParent;
    int		indexCount;
    u_int	numChildren;
    XClassHint	*retHint;
    
    if (XQueryTree(dpy, wm_win, &retRoot, &retParent, &lstChildren, &numChildren)) {
	for (indexCount = 1; indexCount < numChildren; indexCount++) {
	    retHint = XAllocClassHint();
	    if (!retHint) {
		XFree(lstChildren);
		return;
	    }
	    
	    XGetClassHint (dpy, lstChildren[indexCount], retHint);

            if (retHint->res_class) {
                if (strcmp("WMSoundServer", retHint->res_class)==0 ||
                    /*strcmp("WSoundServer", retHint->res_class)==0 ||*/
                    (retHint->res_name &&
                     strcmp("wsoundserver", retHint->res_name)==0 &&
                     strcmp("DockApp", retHint->res_class)==0)) {

                    soundServer = lstChildren[indexCount];
                    XFree(lstChildren);
                    if(retHint) {
                        XFree(retHint);
                    }
                    return;
                }
	    }
	    XFree(retHint);
	    retHint = 0;
	}
	XFree(lstChildren);
    }
    return;
}
#endif

void
wSoundPlay(long event_sound)
{
    static Atom atom = 0;
    XEvent sound_event;

    if (!atom) {
	atom = XInternAtom(dpy, "_WINDOWMAKER_EVENT", False);
    }

    if(!wPreferences.no_sound) {
	Window win = wScreenWithNumber(0)->info_window;

        sound_event.xclient.type = ClientMessage;
        sound_event.xclient.message_type = atom;
        sound_event.xclient.format = 32;
        sound_event.xclient.display = dpy;
        sound_event.xclient.window = win;
        sound_event.xclient.data.l[0] = event_sound;
        XSendEvent(dpy, win, False, StructureNotifyMask, &sound_event);
	XFlush(dpy);
    }
}

#endif /* WMSOUND */
