#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "WindowMaker.h"
#include "funcs.h"
#include "wsound.h"




extern WPreferences wPreferences;

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


