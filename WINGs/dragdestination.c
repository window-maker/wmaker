

#include <X11/Xatom.h>

#include "WINGsP.h"


/* dropping */

typedef struct W_DNDTargetInfo {
    /* data types accepted for drops */
    Atom *dropTypes;
    int dropTypeCount;


} DNDTargetInfo;


static Atom XDNDversion = XDND_VERSION;


static void
realizedObserver(void *self, WMNotification *notif)
{
    WMView *view = (WMView*)WMGetNotificationObject(notif);

    XChangeProperty(W_VIEW_SCREEN(view)->display, W_VIEW_DRAWABLE(view),
		    W_VIEW_SCREEN(view)->xdndAwareAtom,
		    XA_ATOM, 32, PropModeReplace,
		    (unsigned char*)&XDNDversion, 1);

    WMRemoveNotificationObserver(self);
}


void
W_SetXdndAwareProperty(WMScreen *scr, WMView *view, Atom *types, int typeCount)
{
    Display *dpy = scr->display;

    view = W_TopLevelOfView(view);
    
    if (!view->flags.xdndHintSet) {	
	view->flags.xdndHintSet = 1;
	
	if (view->flags.realized) {
	    XChangeProperty(dpy, W_VIEW_DRAWABLE(view), scr->xdndAwareAtom,
			    XA_ATOM, 32, PropModeReplace,
			    (unsigned char*)&XDNDversion, 1);
	} else {
	    WMAddNotificationObserver(realizedObserver, 
				      /* just use as an id */
				      &view->dragDestinationProcs,
				      WMViewRealizedNotification, 
				      view);
	}
    }
}



void
WMRegisterViewForDraggedTypes(WMView *view, char *acceptedTypes[])
{
    Atom *types;
    int typeCount;
    int i;
    
    typeCount = 0;
    while (acceptedTypes[typeCount++]);
    
    types = wmalloc(sizeof(Atom)*(typeCount+1));

    for (i = 0; i < typeCount; i++) {
	types[i] = XInternAtom(W_VIEW_SCREEN(view)->display, 
			       acceptedTypes[i], False);
    }
    types[i] = 0;

    view->droppableTypes = types;

    W_SetXdndAwareProperty(W_VIEW_SCREEN(view), view, types, typeCount);
}


void
WMUnregisterViewDraggedTypes(WMView *view)
{
    if (view->droppableTypes != NULL)
	wfree(view->droppableTypes);
    view->droppableTypes = NULL;
}

/***********************************************************************/


static unsigned defDraggingEntered(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag entered\n", W_VIEW_DRAWABLE(self));
    return WDOperationNone;
}

static unsigned defDraggingUpdated(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag updat\n", W_VIEW_DRAWABLE(self));    
    return WDOperationNone;
}

static void defDraggingExited(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag exit\n", W_VIEW_DRAWABLE(self));    
}

static Bool defPrepareForDragOperation(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag prep\n", W_VIEW_DRAWABLE(self));    
    return False;
}

static Bool defPerformDragOperation(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag perf\n", W_VIEW_DRAWABLE(self));    
    return False;
}

static void defConcludeDragOperation(WMView *self, WMDraggingInfo *info)
{
    printf("%x drag concl\n", W_VIEW_DRAWABLE(self));    
}



void
WMSetViewDragDestinationProcs(WMView *view, WMDragDestinationProcs *procs)
{
    if (view->dragDestinationProcs == NULL) {
	view->dragDestinationProcs = wmalloc(sizeof(WMDragDestinationProcs));
    } else {
	free(view->dragDestinationProcs);
    }
    *view->dragDestinationProcs = *procs;
    
    /*XXX fill in non-implemented stuffs */
    if (procs->draggingEntered == NULL) {
	view->dragDestinationProcs->draggingEntered = defDraggingEntered;
    }
    if (procs->draggingUpdated == NULL) {
	view->dragDestinationProcs->draggingUpdated = defDraggingUpdated;
    }
    if (procs->draggingExited == NULL) {
	view->dragDestinationProcs->draggingExited = defDraggingExited;
    }
    if (procs->prepareForDragOperation == NULL) {
	view->dragDestinationProcs->prepareForDragOperation =
	    defPrepareForDragOperation;
    }
    if (procs->performDragOperation == NULL) {
	view->dragDestinationProcs->performDragOperation = 
	    defPerformDragOperation;
    }
    if (procs->concludeDragOperation == NULL) {
	view->dragDestinationProcs->concludeDragOperation =
	    defConcludeDragOperation;
    }
}







WMPoint WMGetDraggingInfoImageLocation(WMDraggingInfo *info)
{
    return info->imageLocation;
}



static void
receivedData(WMView *view, Atom selection, Atom target, Time timestamp, 
	     void *cdata, WMData *data)
{
    
}



Bool WMRequestDroppedData(WMView *view, WMDraggingInfo *info, char *type,
			  WMDropDataCallback *callback)
{
#if 0
    WMScreen *scr = W_VIEW_SCREEN(view);
    if (info->finished) {
	return False;
    }
    
    if (type != NULL) {
	if (!WMRequestSelection(scr->dragInfo.destView,
				scr->xdndSelectionAtom,
				XInternAtom(scr->display, type, False),
				scr->dragInfo.timestamp,
				receivedData, &scr->dragInfo)) {
	    wwarning("could not request data for dropped data");

	    /* send finished message */
	    sendClientMessage(scr->display, source,
			      scr->xdndFinishedAtom,
			      scr->dragInfo.destinationWindow,
			      0, 0, 0, 0);
	}
    } else {
	/* send finished message */
	sendClientMessage(scr->display, source,
			  scr->xdndFinishedAtom,
			  scr->dragInfo.destinationWindow,
			  0, 0, 0, 0);
    }
#endif
}


