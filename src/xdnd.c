/* Many part of code are ripped of an example from JX's site */

#include "wconfig.h"

#ifdef XDND

#include <X11/Xlib.h>
#include "WindowMaker.h"
#include "window.h"
#include "dock.h"
#include "xdnd.h"
#include "motif.h"

#include "workspace.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/Xatom.h>

#define XDND_VERSION 3L

static Atom _XA_XdndAware;
static Atom _XA_XdndEnter;
static Atom _XA_XdndLeave;
static Atom _XA_XdndDrop;
static Atom _XA_XdndPosition;
static Atom _XA_XdndStatus;
static Atom _XA_XdndActionCopy;
static Atom _XA_XdndSelection;
static Atom _XA_XdndFinished;
static Atom _XA_WINDOWMAKER_XDNDEXCHANGE;

/*
 Atom _XA_MOTIF_DRAG_RECEIVER_INFO;
 Atom _XA_MOTIF_DRAG_AND_DROP_MESSAGE;
 */

static Atom atom_support;

void wXDNDInitializeAtoms()
{

	_XA_XdndAware = XInternAtom(dpy, "XdndAware", False);
	_XA_XdndEnter = XInternAtom(dpy, "XdndEnter", False);
	_XA_XdndLeave = XInternAtom(dpy, "XdndLeave", False);
	_XA_XdndDrop = XInternAtom(dpy, "XdndDrop", False);
	_XA_XdndPosition = XInternAtom(dpy, "XdndPosition", False);
	_XA_XdndStatus = XInternAtom(dpy, "XdndStatus", False);
	_XA_XdndActionCopy = XInternAtom(dpy, "XdndActionCopy", False);
	_XA_XdndSelection = XInternAtom(dpy, "XdndSelection", False);
	_XA_XdndFinished = XInternAtom(dpy, "XdndFinished", False);

	_XA_WINDOWMAKER_XDNDEXCHANGE = XInternAtom(dpy, "_WINDOWMAKER_XDNDEXCHANGE", False);

	/*
	   _XA_MOTIF_DRAG_RECEIVER_INFO = XInternAtom(dpy, "_MOTIF_DRAG_RECEIVER_INFO",False);
	   _XA_MOTIF_DRAG_AND_DROP_MESSAGE = XInternAtom(dpy, "_MOTIF_DRAG_AND_DROP_MESSAGE", False);
	 */
}

void wXDNDMakeAwareness(Window window)
{
	long int xdnd_version = 3;
	/*
	   MotifDragReceiverInfo info;
	 */
	XChangeProperty(dpy, window, _XA_XdndAware, XA_ATOM, 32, PropModeAppend, (unsigned char *)&xdnd_version, 1);

    /*** MOTIF ***
     info.byte_order = '\0';
     info.protocol_version = 0;
     info.protocol_style = XmDRAG_DYNAMIC;
     info.proxy_window = 0;
     info.num_drop_sites = 0;
     info.total_size = sizeof(info);

     XChangeProperty (dpy, window,
     _XA_MOTIF_DRAG_RECEIVER_INFO,
     _XA_MOTIF_DRAG_RECEIVER_INFO,
     8, PropModeReplace,
     (unsigned char *)&info,
     sizeof (info));
     */
}

void wXDNDDecodeURI(char *uri) {
	char *last = uri + strlen(uri);
	while (uri < last-2) {
		if (*uri == '%') {
			int h;
			if (sscanf(uri+1, "%2X", &h) != 1)
				break;
			*uri = h;
			memmove(uri+1, uri+3, last - (uri+2));
			last -= 2;
		}
		uri++;
	}
}

Bool wXDNDProcessSelection(XEvent * event)
{
	WScreen *scr = wScreenForWindow(event->xselection.requestor);
	char *retain;
	Atom ret_type;
	int ret_format;
	unsigned long ret_item;
	unsigned long remain_byte;
	char *delme;
	XEvent xevent;
	Window selowner = XGetSelectionOwner(dpy, _XA_XdndSelection);

	XGetWindowProperty(dpy, event->xselection.requestor,
			   _XA_WINDOWMAKER_XDNDEXCHANGE,
			   0, 65536, True, atom_support, &ret_type, &ret_format,
			   &ret_item, &remain_byte, (unsigned char **)&delme);

	/*send finished */
	memset(&xevent, 0, sizeof(xevent));
	xevent.xany.type = ClientMessage;
	xevent.xany.display = dpy;
	xevent.xclient.window = selowner;
	xevent.xclient.message_type = _XA_XdndFinished;
	xevent.xclient.format = 32;
	XDND_FINISHED_TARGET_WIN(&xevent) = event->xselection.requestor;
	XSendEvent(dpy, selowner, 0, 0, &xevent);

	/*process dropping */
	if (delme) {
		WMArray *items;
		WMArrayIterator iter;
		int length, str_size;
		int total_size = 0;
		char *tmp;

		scr->xdestring = delme;
		items = WMCreateArray(4);
		retain = wstrdup(scr->xdestring);
		XFree(scr->xdestring);	/* since xdestring was created by Xlib */

		length = strlen(retain);

		/* search in string */
		while (length--) {
			if (retain[length] == '\r') {	/* useless char, nuke it */
				retain[length] = 0;
			}
			if (retain[length] == '\n') {
				str_size = strlen(&retain[length + 1]);
				if (str_size) {
					WMAddToArray(items, wstrdup(&retain[length + 1]));
					total_size += str_size + 3;	/* reserve for " \"\"" */
					/* this is nonsense -- if (length)
					   WMAppendArray(items, WMCreateArray(1)); */
				}
				retain[length] = 0;
			}
		}
		/* final one */
		WMAddToArray(items, wstrdup(retain));
		total_size += strlen(retain) + 3;
		wfree(retain);

		/* now pack new string */
		scr->xdestring = wmalloc(total_size);
		scr->xdestring[0] = 0;	/* empty string */
		WM_ETARETI_ARRAY(items, tmp, iter) {
			/* only supporting file: URI objects */
			if (!strncmp(tmp, "file://", 7)) {
				/* drag-and-drop file name format as per the spec is encoded as an URI */
				wXDNDDecodeURI(&tmp[7]);
				strcat(scr->xdestring, " \"");
				strcat(scr->xdestring, &tmp[7]);
				strcat(scr->xdestring, "\"");
			}
			wfree(tmp);
		}
		WMFreeArray(items);
		if (scr->xdestring[0]) {
			wDockReceiveDNDDrop(scr, event);
		}
		wfree(scr->xdestring);	/* this xdestring is not from Xlib (no XFree) */
	}

	return True;
}

Bool isAwareXDND(Window window)
{
	Atom actual;
	int format;
	unsigned long count, remaining;
	unsigned char *data = 0;

	if (!window)
		return False;
	XGetWindowProperty(dpy, window, _XA_XdndAware,
			   0, 0x8000000L, False, XA_ATOM, &actual, &format, &count, &remaining, &data);
	if (actual != XA_ATOM || format != 32 || count == 0 || !data) {
		if (data)
			XFree(data);
		return False;
	}
	if (data)
		XFree(data);
	return True;
}

Bool acceptXDND(Window window)
{
	WScreen *scr = wScreenForWindow(window);
	WDock *dock;
	int icon_pos, i;

	icon_pos = -1;
	if ((dock = scr->dock) != NULL) {
		for (i = 0; i < dock->max_icons; i++) {
			if (dock->icon_array[i]
			    && dock->icon_array[i]->icon->core->window == window) {
				icon_pos = i;
				break;
			}
		}
	}
	if (icon_pos < 0 && (dock = w_global.workspace.array[w_global.workspace.current]->clip) != NULL) {
		for (i = 0; i < dock->max_icons; i++) {
			if (dock->icon_array[i]
			    && dock->icon_array[i]->icon->core->window == window) {
				icon_pos = i;
				break;
			}
		}
	}
	if (icon_pos < 0)
		return False;
	if (!dock)
		return False;
	if (isAwareXDND(dock->icon_array[icon_pos]->icon->icon_win))
		return False;

	if (dock->icon_array[icon_pos]->dnd_command != NULL)
		return True;

	return False;
}

Bool wXDNDProcessClientMessage(XClientMessageEvent * event)
{
	/* test */
	{
		char *name = XGetAtomName(dpy, event->message_type);
		/*
		   printf("Get %s\n",name);
		 */
		XFree(name);
	}

	/*
	   if (event->message_type == _XA_MOTIF_DRAG_AND_DROP_MESSAGE) {
	   printf("motif dnd msg %d\n",event->data.b[0]);
	   if (event->data.b[0] == XmDROP_START){
	   unsigned x_root, y_root, flags;
	   unsigned char reason;
	   unsigned long timestamp;
	   Atom atom;
	   Window source_window;
	   MotifDragInitiatorInfo *initiator_info;
	   Atom ret_type;
	   int ret_format;
	   unsigned long ret_item;
	   unsigned long remain_byte;

	   reason = event->data.b[0];
	   flags = event->data.s[1];
	   timestamp = event->data.l[1];
	   x_root = event->data.s[4];
	   y_root = event->data.s[5];
	   atom = event->data.l[3];
	   source_window = event->data.l[4];

	   XGetWindowProperty(dpy, source_window, atom,
	   0, sizeof(*initiator_info), True, atom_support,
	   &ret_type, &ret_format,
	   &ret_item, &remain_byte, (unsigned char **)&initiator_info);
	   }
	   }
	   else */
	if (event->message_type == _XA_XdndEnter) {
		if ((event->data.l[1] & 1) == 0) {
			atom_support = event->data.l[2];
		}
		/*
		   else puts("enter more than 3 types");
		 */
		return True;
	} else if (event->message_type == _XA_XdndLeave) {
		return True;
	} else if (event->message_type == _XA_XdndDrop) {
		if (event->data.l[0] == XGetSelectionOwner(dpy, _XA_XdndSelection)) {
			XConvertSelection(dpy, _XA_XdndSelection, atom_support,
					  _XA_WINDOWMAKER_XDNDEXCHANGE, event->window, CurrentTime);
		} else {
			//printf("weird selection owner? QT?\n");
			XConvertSelection(dpy, _XA_XdndSelection, atom_support,
					  _XA_WINDOWMAKER_XDNDEXCHANGE, event->window, CurrentTime);
		}
		return True;
	} else if (event->message_type == _XA_XdndPosition) {
		XEvent xevent;
		Window srcwin = event->data.l[0];
		if (atom_support != XInternAtom(dpy, "text/uri-list", False)) {
			return True;
		}
		{
			memset(&xevent, 0, sizeof(xevent));
			xevent.xany.type = ClientMessage;
			xevent.xany.display = dpy;
			xevent.xclient.window = srcwin;
			xevent.xclient.message_type = _XA_XdndStatus;
			xevent.xclient.format = 32;

			XDND_STATUS_TARGET_WIN(&xevent) = event->window;
			XDND_STATUS_WILL_ACCEPT_SET(&xevent, acceptXDND(event->window));
			XDND_STATUS_WANT_POSITION_SET(&xevent, True);
			XDND_STATUS_RECT_SET(&xevent, 0, 0, 1024, 768);
			XDND_STATUS_ACTION(&xevent) = _XA_XdndActionCopy;

			XSendEvent(dpy, srcwin, 0, 0, &xevent);
		}
		return True;
	}
	return False;
}

#endif
