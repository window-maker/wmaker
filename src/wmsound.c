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
wSoundServerGrab(char *name, Window window)
{
    if(soundServer==None && name!=NULL && strcmp(name,"WMSoundServer")==0) {
        soundServer = window;
    }
}


void
wSoundInit(Display *dpy)
{
    soundServer = 0;
    sound_event.xclient.type = ClientMessage;
    sound_event.xclient.message_type = _XA_WINDOWMAKER_WM_FUNCTION;
    sound_event.xclient.format = 32;
    sound_event.xclient.display = dpy;
}


void
wSoundPlay(long event_sound)
{
    if(soundServer!=None && !wPreferences.no_sound) {
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
