

#include <stdlib.h>

#include <X11/Xatom.h>

#include "WINGsP.h"

#define MAX_PROPERTY_SIZE 8*1024


typedef struct SelectionHandler {
    WMWidget *widget;
    Atom selection;
    Time timestamp;
    WMConvertSelectionProc *convProc;
    WMLoseSelectionProc *loseProc;
    WMSelectionDoneProc *doneProc;

    struct {
	unsigned delete_pending:1;
	unsigned done_pending:1;
    } flags;

    struct SelectionHandler *next;
} SelectionHandler;


static SelectionHandler *selHandlers = NULL;


void
WMDeleteSelectionHandler(WMWidget *widget, Atom selection)
{
    SelectionHandler *handler, *tmp;
    Display *dpy = WMWidgetScreen(widget)->display;
    Window win = WMWidgetXID(widget);
    Time timestamp;

    if (!selHandlers)
        return;

    tmp = selHandlers;

    if (tmp->widget == widget) {

	if (tmp->flags.done_pending) {
	    tmp->flags.delete_pending = 1;
	    return;
	}
        selHandlers = tmp->next;
	timestamp = tmp->timestamp;
        free(tmp);
    } else {
        while (tmp->next) {
            if (tmp->next->widget == widget) {

		if (tmp->next->flags.done_pending) {
		    tmp->next->flags.delete_pending = 1;
		    return;
		}
                handler = tmp->next;
                tmp->next = handler->next;
		timestamp = handler->timestamp;
                free(handler);
                break;
            }
            tmp = tmp->next;
        }
    }

    XGrabServer(dpy);
    if (XGetSelectionOwner(dpy, selection) == win) {
	XSetSelectionOwner(dpy, selection, None, timestamp);
    }
    XUngrabServer(dpy);
}


static Bool gotError = 0;
/*
static int
errorHandler(XErrorEvent *error)
{
    return 0;
}
*/

static Bool
writeSelection(Display *dpy, Window requestor, Atom property, Atom type,
	       void *value, long length, int format)
{
/*
    printf("write to %x: %s\n", requestor, XGetAtomName(dpy, property));
*/
    gotError = False;

    if (!XChangeProperty(dpy, requestor, property, type, format, 
			 PropModeReplace, value, length))
	return False;
    XFlush(dpy);

    return !gotError;
}


static void
notifySelection(XEvent *event, Atom prop)
{
    XEvent ev;
/*
    printf("envent to %x\n", event->xselectionrequest.requestor);
*/
    ev.xselection.type = SelectionNotify;
    ev.xselection.serial = 0;
    ev.xselection.send_event = True;
    ev.xselection.display = event->xselectionrequest.display;
    ev.xselection.requestor = event->xselectionrequest.requestor;
    ev.xselection.target = event->xselectionrequest.target;
    ev.xselection.property = prop;
    ev.xselection.time = event->xselectionrequest.time;
    
    XSendEvent(event->xany.display, event->xselectionrequest.requestor, 
	       False, 0, &ev);
    XFlush(event->xany.display);
}


void
W_HandleSelectionEvent(XEvent *event)
{
    SelectionHandler *handler;

    handler = selHandlers;

    while (handler) {
	if (WMWidgetXID(handler->widget)==event->xany.window
/*	    && handler->selection == event->selection*/) {

	    switch (event->type) {
	     case SelectionClear:
		if (handler->loseProc)
		    (*handler->loseProc)(handler->widget, handler->selection);
		break;

	     case SelectionRequest:
		if (handler->convProc) {
		    Atom atom;
		    void *data;
		    unsigned length;
		    int format;
		    Atom prop;

		    /* they're requesting for something old */
		    if (event->xselectionrequest.time < handler->timestamp
			&& event->xselectionrequest.time != CurrentTime) {

			notifySelection(event, None);
			break;
		    }

		    handler->flags.done_pending = 1;

		    if (!(*handler->convProc)(handler->widget,
					     handler->selection,
					     event->xselectionrequest.target,
					     &atom, &data, &length, &format)) {

			notifySelection(event, None);
			break;
		    }

		    
		    prop = event->xselectionrequest.property;
		    /* obsolete clients that don't set the property field */
		    if (prop == None)
			prop = event->xselectionrequest.target;

		    if (!writeSelection(event->xselectionrequest.display,
					event->xselectionrequest.requestor,
					prop, atom, data, length, format)) {

			free(data);
			notifySelection(event, None);
			break;
		    }
		    free(data);

		    notifySelection(event, prop);

		    if (handler->doneProc) {
			(*handler->doneProc)(handler->widget, 
					     handler->selection,
					     event->xselectionrequest.target);
		    }

		    handler->flags.done_pending = 0;

		    /* in case the handler was deleted from some
		     * callback */
		    if (handler->flags.delete_pending) {
			WMDeleteSelectionHandler(handler->widget,
						 handler->selection);
		    }
		}
		break;

	     case SelectionNotify:
		
		break;
	    }
	}

	handler = handler->next;
    }
}




Bool
WMCreateSelectionHandler(WMWidget *w, Atom selection, Time timestamp,
			 WMConvertSelectionProc *convProc,
			 WMLoseSelectionProc *loseProc,
			 WMSelectionDoneProc *doneProc)
{
    SelectionHandler *handler, *tmp;
    Display *dpy = WMWidgetScreen(w)->display;

    XSetSelectionOwner(dpy, selection, WMWidgetXID(w), timestamp);
    if (XGetSelectionOwner(dpy, selection) != WMWidgetXID(w))
	return False;

    handler = malloc(sizeof(SelectionHandler));
    if (!handler)
	return False;

    handler->widget = w;
    handler->selection = selection;
    handler->timestamp = timestamp;
    handler->convProc = convProc;
    handler->loseProc = loseProc;
    handler->doneProc = doneProc;
    memset(&handler->flags, 0, sizeof(handler->flags));

    if (!selHandlers) {
	/* first in the queue */
	handler->next = selHandlers;
	selHandlers = handler;
    } else {
	tmp = selHandlers;
	while (tmp->next) {
	    tmp = tmp->next;
	}
	handler->next = tmp->next;
	tmp->next = handler;
    }

    return True;
}




static void
timeoutHandler(void *data)
{
    *(int*)data = 1;
}


static Bool
getInternalSelection(WMScreen *scr, Atom selection, Atom target,
		     void **data, unsigned *length)
{
    Window owner;
    SelectionHandler *handler;

    /*
     * Check if the selection is owned by this application and if so,
     * do the conversion directly.
     */

    data = NULL;

    owner = XGetSelectionOwner(scr->display, selection);
    if (!owner)
	return False;

    handler = selHandlers;

    while (handler) {
	if (WMWidgetXID(handler->widget) == owner
	    /*	    && handler->selection == event->selection*/) {
	    break;
	}
	handler = handler->next;
    }

    if (!handler)
	return False;

    if (handler->convProc) {
	Atom atom;
	int format;

	if (!(*handler->convProc)(handler->widget, handler->selection, 
				  target, &atom, data, length, &format)) {
	    return True;
	}

	if (handler->doneProc) {
	    (*handler->doneProc)(handler->widget, handler->selection, target);
	}
    }

    return True;
}


char*
W_GetTextSelection(WMScreen *scr, Atom selection)
{
    int buffer = -1;
    
    switch (selection) {
     case XA_CUT_BUFFER0:
	buffer = 0;
	break;
     case XA_CUT_BUFFER1:
	buffer = 1;
	break;
     case XA_CUT_BUFFER2:
	buffer = 2;
	break;
     case XA_CUT_BUFFER3:
	buffer = 3;
	break;
     case XA_CUT_BUFFER4:
	buffer = 4;
	break;
     case XA_CUT_BUFFER5:
	buffer = 5;
	break;
     case XA_CUT_BUFFER6:
	buffer = 6;
	break;
     case XA_CUT_BUFFER7:
	buffer = 7;
	break;
    }
    if (buffer >= 0) {
	char *data;
	int size;

	data = XFetchBuffer(scr->display, &size, buffer);

	return data;
    } else {
	char *data;
	int bits;
	Atom rtype;
	unsigned long len, bytes;
	WMHandlerID timer;
	int timeout = 0;
	XEvent ev;
	unsigned length;
	
	XDeleteProperty(scr->display, scr->groupLeader, scr->clipboardAtom);

	if (getInternalSelection(scr, selection, XA_STRING, (void**)&data,
				 &length)) {

	    return data;
	}
	
	XConvertSelection(scr->display, selection, XA_STRING,
			  scr->clipboardAtom, scr->groupLeader,
			  scr->lastEventTime);
	
	timer = WMAddTimerHandler(1000, timeoutHandler, &timeout);
	
	while (!XCheckTypedWindowEvent(scr->display, scr->groupLeader,
				       SelectionNotify, &ev) && !timeout);
	
	if (!timeout) {
	    WMDeleteTimerHandler(timer);
	} else {
	    wwarning("selection retrieval timed out");
	    return NULL;
	}

	/* nobody owns the selection or the current owner has
	 * nothing to do with what we need */
	if (ev.xselection.property == None) {
	    return NULL;
	}

	if (XGetWindowProperty(scr->display, scr->groupLeader, 
			       scr->clipboardAtom, 0, MAX_PROPERTY_SIZE, 
			       False, XA_STRING, &rtype, &bits, &len,
			       &bytes, (unsigned char**)&data)!=Success) {
	    return NULL;
	}
	if (rtype!=XA_STRING || bits!=8) {
	    wwarning("invalid data in text selection");
	    if (data)
		XFree(data);
	    return NULL;
	}
	return data;
    }
}

