

#include <stdlib.h>

#include <X11/Xatom.h>

#include "WINGsP.h"

#if 0

typedef struct W_SelectionHandler {
    WMWidget *widget;
    Atom selection;
    void *clientData;
    WMSelectionProc *proc;
    WMHandlerID timerID;
    W_SelectionHandler *next;
    W_SelectionHandler *prev;
} W_SelectionHandler;
#endif

#define SELECTION_TIMEOUT	2000
#define MAX_PROPERTY_SIZE	10*1024
#if 0


void
WMWriteSelectionToClipboard(WMSelection *selection)
{
}


WMSelection*
WMCreateSelectionWithData(WMData *data, Atom type)
{
    
}
#endif

#if 0

#define MAX_PROPERTY_SIZE	100*1024


static void
handleSelectionEvent(XEvent *event, void *data)
{
    W_SelectionHandler *handler = (W_SelectionHandler*)data;
    char *data = NULL;
    Atom type;
    int format, result;
    unsigned long numItems, bytesAfter;
    WMScreen *scr = WMWidgetScreen(handler->widget);

    WMDeleteTimerHandler(handler->timerID);

    if (handler->next)
	handler->next->prev = handler->prev;
    if (handler->prev)
	handler->prev->next = handler->next;
    if (handler == WMWidgetScreen(handler->widget)->selectionHandlerList)
	WMWidgetScreen(handler->widget)->selectionHandlerList = handler->next;
    
    if (event->xselection.property == None) {
	char *name = XGetAtomName(event->xselection.display, 
				  handler->selection);
	char *form = XGetAtomName(event->xselection.display, handler->type);
	wwarning("error retrieving selection %s with form %s\n", name, form);
	if (name)
	    XFree(name);
	if (form)
	    XFree(form);
	free(handler);
	return;
    }

    if (XGetWindowProperty(event->xselection.display,
			   event->xselection.requestor, handler->property,
			   0, MAX_PROPERTY_SIZE, False, AnyPropertyType,
			   &type, &format, &numItems, &bytesAfter,
			   &data) != Success || type == None) {
	if (data)
	    XFree(data);
	free(handler);
	return;
    }
    if (bytesAfter!=0) {
	wwarning("data in selection is too large");
	if (data)
	    XFree(data);
	free(handler);
	return;
    }
    if (type == XA_STRING || type == scr->compoundTextAtom) {
	if (format!=8) {
	    wwarning("string in selection has format %i, which is invalid",
		     format);
	    if (data)
		XFree(data);
	    free(handler);
	    return;
	}
	(*handler->proc)();
    }
}


static void
timeoutHandler(void *data)
{
    W_SelectionHandler *handler = (W_SelectionHandler*)data;

    wwarning("selection timed out");
    WMDeleteEventHandler(WMWidgetView(handler->widget), SelectionNotifyMask,
			 handleSelectionEvent, data);
    if (handler->next)
	handler->next->prev = handler->prev;
    if (handler->prev)
	handler->prev->next = handler->next;
    if (handler == WMWidgetScreen(handler->widget)->selectionHandlerList)
	WMWidgetScreen(handler->widget)->selectionHandlerList = handler->next;
}



void
WMGetSelection(WMWidget *widget, Atom selection, Atom type, Atom property,
	       WMSelectionProc *proc, void *clientData, Time time)
{
    WMScreen *scr = WMWidgetScreen(widget);
    void *data;
    Atom rtype;
    int bits;
    unsigned long len, bytes;
    unsigned char *data;
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
	
    } else {
	W_SelectionHandler *handler;
	
	XDeleteProperty(scr->display, WMWidgetXID(widget), selection);
	XConvertSelection(scr->display, selection, type, property,
			  WMWidgetXID(widget), time);

	handler = wmalloc(sizeof(W_SelectionHandler));
	handler->widget = widget;
	handler->selection = selection;
	handler->type = type;
	handler->property = property;
	handler->clientData = clientData;
	handler->proc = proc;
	handler->timerID = WMAddTimerHandler(SELECTION_TIMEOUT,
					     timeoutHandler, handler);

	handler->next = scr->selectionHandlerList;
	handler->prev = NULL;
	if (scr->selectionHandlerList)
	    scr->selectionHandlerList->prev = handler;
	scr->selectionHandlerList = handler;

	WMCreateEventHandler(WMWidgetView(widget), SelectionNotifyMask,
			     handleSelectionEvent, handler);
    }
}

#endif



static void
timeoutHandler(void *data)
{
    *(int*)data = 1;
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
	unsigned char *data;
	int bits;
	Atom rtype;
	unsigned long len, bytes;
	WMHandlerID timer;
	int timeout = 0;
	XEvent ev;
	
	XDeleteProperty(scr->display, scr->groupLeader, scr->clipboardAtom);
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
	
	/* nobody owns the selection */
	if (ev.xselection.property == None) {
	    return NULL;
	}

	if (XGetWindowProperty(scr->display, scr->groupLeader, 
			       scr->clipboardAtom, 0, MAX_PROPERTY_SIZE, 
			       False, XA_STRING, &rtype, &bits, &len,
			       &bytes, &data)!=Success) {
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

