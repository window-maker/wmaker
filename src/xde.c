
#include "wconfig.h"

#ifdef XDE_DND

#include <X11/Xlib.h>
#include "WindowMaker.h"
#include "window.h"

#include "stdlib.h"

#include <X11/Xatom.h>
#include <gdk/gdk.h>


extern Atom _XA_XDE_REQUEST;
extern Atom _XA_XDE_ENTER;
extern Atom _XA_XDE_LEAVE;
extern Atom _XA_XDE_DATA_AVAILABLE;
extern Atom _XDE_FILETYPE;
extern Atom _XDE_URLTYPE;



Bool
wXDEProcessClientMessage(XClientMessageEvent *event)
{
    Bool done = False;

    if (event->message_type==_XA_XDE_DATA_AVAILABLE) {
	GdkEvent gdkev;
	WScreen *scr = wScreenForWindow(event->window);
	Atom tmpatom;
	int datalenght;
	long tmplong;
	char * tmpstr, * runstr, * freestr, * tofreestr;
	    printf("x\n");
	gdkev.dropdataavailable.u.allflags = event->data.l[1];
	gdkev.dropdataavailable.timestamp = event->data.l[4];

	if(gdkev.dropdataavailable.u.flags.isdrop){
		gdkev.dropdataavailable.type = GDK_DROP_DATA_AVAIL;
		gdkev.dropdataavailable.requestor = event->data.l[0];
		XGetWindowProperty(dpy,gdkev.dropdataavailable.requestor,
				event->data.l[2],
				0, LONG_MAX -1,
				0, XA_PRIMARY, &tmpatom,
				&datalenght,
				&gdkev.dropdataavailable.data_numbytes,
				&tmplong,
				&tmpstr);
		datalenght=gdkev.dropdataavailable.data_numbytes-1;
		tofreestr=tmpstr;
		runstr=NULL;
                for(;datalenght>0;datalenght-=(strlen(tmpstr)+1),tmpstr=&tmpstr[strlen(tmpstr)+1]){
		freestr=runstr;runstr=wstrappend(runstr,tmpstr);free(freestr);
		freestr=runstr;runstr=wstrappend(runstr," ");free(freestr);
		}
                free(tofreestr);
		scr->xdestring=runstr;
		/* no need to redirect ? */
	        wDockReceiveDNDDrop(scr,event);
		free(runstr);
		scr->xdestring=NULL;
	}
	done = True;
    } else if (event->message_type==_XA_XDE_LEAVE) {
	    printf("leave\n");
	done = True;
    } else if (event->message_type==_XA_XDE_ENTER) {
	GdkEvent gdkev;
	XEvent replyev;

	gdkev.dropenter.u.allflags=event->data.l[1];
	printf("from win %x\n",event->data.l[0]);
	printf("to win %x\n",event->window);
        printf("enter %x\n",event->data.l[1]);
        printf("v %x ",event->data.l[2]);
        printf("%x ",event->data.l[3]);
        printf("%x\n",event->data.l[4]);

	if(event->data.l[2]==_XDE_FILETYPE ||
	   event->data.l[3]==_XDE_FILETYPE ||
	   event->data.l[4]==_XDE_FILETYPE ||
	   event->data.l[2]==_XDE_URLTYPE ||
	   event->data.l[3]==_XDE_URLTYPE ||
	   event->data.l[4]==_XDE_URLTYPE)
        if(gdkev.dropenter.u.flags.sendreply){
		/*reply*/
            replyev.xclient.type = ClientMessage;
            replyev.xclient.window = event->data.l[0];
            replyev.xclient.format = 32;
            replyev.xclient.message_type = _XA_XDE_REQUEST;
            replyev.xclient.data.l[0] = event->window;

            gdkev.dragrequest.u.allflags = 0;
            gdkev.dragrequest.u.flags.protocol_version = 0;
            gdkev.dragrequest.u.flags.willaccept = 1;
            gdkev.dragrequest.u.flags.delete_data = 0;

            replyev.xclient.data.l[1] = gdkev.dragrequest.u.allflags;
            replyev.xclient.data.l[2] = replyev.xclient.data.l[3] = 0;
            replyev.xclient.data.l[4] = event->data.l[2];
            XSendEvent(dpy, replyev.xclient.window, 0, NoEventMask, &replyev);
            XSync(dpy, 0);
        }
	done = True;
    }
    
    return done;
}

#endif
