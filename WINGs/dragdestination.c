

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
	    WMAddNotificationObserver(realizedObserver, view, 
				      WMViewRealizedNotification, 
				      /* just use as an id */
				      view->dragDestinationProcs);
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
	free(view->droppableTypes);
    view->droppableTypes = NULL;
}

/***********************************************************************/


static unsigned defDraggingEntered(WMView *self, WMDraggingInfo *info)
{
    return WDOperationNone;
}

static unsigned defDraggingUpdated(WMView *self, WMDraggingInfo *info)
{
    return WDOperationNone;
}

static void defDraggingExited(WMView *self, WMDraggingInfo *info)
{
}

static char* defPrepareForDragOperation(WMView *self, WMDraggingInfo *info)
{
    return NULL;
}

static Bool defPerformDragOperation(WMView *self, WMDraggingInfo *info,
				    WMData *data)
{
    return False;
}

static void defConcludeDragOperation(WMView *self, WMDraggingInfo *info)
{
}



void
WMSetViewDragDestinationProcs(WMView *view, WMDragDestinationProcs *procs)
{
    if (view->dragDestinationProcs == NULL) {
	free(view->dragDestinationProcs);
	view->dragDestinationProcs = wmalloc(sizeof(WMDragDestinationProcs));
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





