#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "WindowMaker.h"
#include "wmsound.h"

#ifdef WMSOUND

Window soundServer;
XEvent sound_event;

extern WPreferences wPreferences;
extern Atom _XA_WINDOWMAKER_WM_FUNCTION;

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
		if (strcmp("WMSoundServer", retHint->res_class) == 0) {
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


void
wSoundPlay(long event_sound)
{
    if (!soundServer) {
      wSoundServerGrab(DefaultRootWindow(dpy));
    }
    if(soundServer!=None && !wPreferences.no_sound) {
        sound_event.xclient.type = ClientMessage;
        sound_event.xclient.message_type = _XA_WINDOWMAKER_WM_FUNCTION;
        sound_event.xclient.format = 32;
        sound_event.xclient.display = dpy;
        sound_event.xclient.window = soundServer;
        sound_event.xclient.data.l[0] = event_sound;
        if (XSendEvent(dpy, soundServer, False,
                       NoEventMask, &sound_event)==BadWindow) {
            soundServer = None;
            return;
        } else {
            XFlush(dpy);
        }
    }
}

#endif /* WMSOUND */
