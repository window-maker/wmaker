

#include "WINGsP.h"

#define VERSION_INFO(dragInfo) dragInfo->protocolVersion
#define XDND_DEST_INFO(dragInfo) dragInfo->destInfo
#define XDND_DEST_VIEW(dragInfo) dragInfo->destInfo->destView
#define XDND_DEST_VIEW_STORED(dragInfo) ((dragInfo->destInfo) != NULL)\
    && ((dragInfo->destInfo->destView) != NULL)


static Bool _WindowExists;


Atom
W_OperationToAction(WMScreen *scr, WMDragOperationType operation)
{
    switch (operation) {
    case WDOperationNone:
        return None;

    case WDOperationCopy:
        return scr->xdndActionCopy;

    case WDOperationMove:
        return scr->xdndActionMove;

    case WDOperationLink:
        return scr->xdndActionLink;

    case WDOperationAsk:
        return scr->xdndActionAsk;

    case WDOperationPrivate:
        return scr->xdndActionPrivate;

    default:
        return None;
    }
}


WMDragOperationType
W_ActionToOperation(WMScreen *scr, Atom action)
{
    if (action == scr->xdndActionCopy) {
        return WDOperationCopy;

    } else if (action == scr->xdndActionMove) {
        return WDOperationMove;

    } else if (action == scr->xdndActionLink) {
        return WDOperationLink;

    } else if (action == scr->xdndActionAsk) {
        return WDOperationAsk;

    } else if (action == scr->xdndActionPrivate) {
        return WDOperationPrivate;

    } else if (action == None) {

        return WDOperationNone;
    } else {
        char *tmp = XGetAtomName(scr->display, action);

        wwarning("unknown XDND action %s ", tmp);
        XFree(tmp);

        return WDOperationCopy;
    }
}


static void
freeDragOperationItem(void* item)
{
    wfree(item);
}

WMArray*
WMCreateDragOperationArray(int initialSize)
{
    return WMCreateArrayWithDestructor(initialSize, freeDragOperationItem);
}


WMDragOperationItem*
WMCreateDragOperationItem(WMDragOperationType type, char* text)
{
    W_DragOperationItem *result = wmalloc(sizeof(W_DragOperationItem));

    result->type = type;
    result->text = text;

    return (WMDragOperationItem*) result;
}

WMDragOperationType
WMGetDragOperationItemType(WMDragOperationItem* item)
{
    return ((W_DragOperationItem*)item)->type;
}


char*
WMGetDragOperationItemText(WMDragOperationItem* item)
{
    return ((W_DragOperationItem*)item)->text;
}

static int
handleNoWindowXError(Display *dpy, XErrorEvent *errEvt)
{
    if (errEvt->error_code == BadWindow
        || errEvt->error_code == BadDrawable) {
        _WindowExists = False;
        return Success;
    }

    return errEvt->error_code;
}


static Bool
windowExists(Display *dpy, Window win)
{
    void* previousErrorHandler;
    XWindowAttributes attr;

    XSynchronize(dpy, True);
    previousErrorHandler = XSetErrorHandler(handleNoWindowXError);
    _WindowExists = True;

    /* can generate BadDrawable or BadWindow */
    XGetWindowAttributes(dpy, win, &attr);

    XSetErrorHandler(previousErrorHandler);
    XSynchronize(dpy, False);
    return _WindowExists;
}


Bool
W_SendDnDClientMessage(Display *dpy, Window win, Atom message,
                       unsigned long data0,
                       unsigned long data1,
                       unsigned long data2,
                       unsigned long data3,
                       unsigned long data4)
{
    XEvent ev;

    if (! windowExists(dpy, win)) {
        wwarning("xdnd message target %d does no longer exist.", win);
        return False; /* message not sent */
    }

    ev.type = ClientMessage;
    ev.xclient.message_type = message;
    ev.xclient.format = 32;
    ev.xclient.window = win;
    ev.xclient.data.l[0] = data0;
    ev.xclient.data.l[1] = data1;
    ev.xclient.data.l[2] = data2;
    ev.xclient.data.l[3] = data3;
    ev.xclient.data.l[4] = data4;


    XSendEvent(dpy, win, False, 0, &ev);
    XFlush(dpy);

    return True; /* message sent */
}


static void
handleLeaveMessage(WMDraggingInfo *info)
{
    if (XDND_DEST_INFO(info) != NULL) {
        if (XDND_DEST_VIEW(info) != NULL) {
            XDND_DEST_VIEW(info)->dragDestinationProcs->concludeDragOperation(
                                                                              XDND_DEST_VIEW(info));
        }
        W_DragDestinationInfoClear(info);
    }
}


void
W_HandleDNDClientMessage(WMView *toplevel, XClientMessageEvent *event)
{
    WMScreen *scr = W_VIEW_SCREEN(toplevel);
    WMDraggingInfo *info = &scr->dragInfo;
    Atom messageType = event->message_type;

#ifdef XDND_DEBUG
    {
        char* msgTypeName = XGetAtomName(scr->display, messageType);

        if (msgTypeName != NULL)
            printf("event type = %s\n", msgTypeName);
        else
            printf("pb with event type !\n");
    }
#endif


    /* Messages from destination to source */
    if (messageType == scr->xdndStatusAtom
        || messageType == scr->xdndFinishedAtom) {
        W_DragSourceStopTimer();
        W_DragSourceStateHandler(info, event);
        return;
    }

    /* Messages from source to destination */
    if (messageType == scr->xdndEnterAtom) {
        W_DragDestinationStopTimer();
        W_DragDestinationStoreEnterMsgInfo(info, toplevel, event);

        if (VERSION_INFO(info) <= XDND_VERSION) {
            if (XDND_DEST_VIEW_STORED(info)) {
                /* xdndPosition previously received on xdnd aware view */
                W_DragDestinationStateHandler(info, event);
                return;
            } else {
                W_DragDestinationStartTimer(info);
                return;
            }
        } else {
            wwarning("received dnd enter msg with unsupported version %i",
                     VERSION_INFO(info));
            W_DragDestinationCancelDropOnEnter(toplevel, info);
            return;
        }
    }

    if (messageType == scr->xdndPositionAtom) {
        W_DragDestinationStopTimer();
        W_DragDestinationStorePositionMsgInfo(info, toplevel, event);
        W_DragDestinationStateHandler(info, event);
        return;
    }

    if (messageType == scr->xdndSelectionAtom
        || messageType == scr->xdndDropAtom) {
        W_DragDestinationStopTimer();
        W_DragDestinationStateHandler(info, event);
        return;
    }

    if (messageType == scr->xdndLeaveAtom) {
        /* conclude drop operation, and clear dragging info */
        W_DragDestinationStopTimer();
        handleLeaveMessage(info);
    }
}


/* called in destroyView (wview.c) */
void
W_FreeViewXdndPart(WMView *view)
{
    WMUnregisterViewDraggedTypes(view);

    if (view->dragSourceProcs)
        wfree(view->dragSourceProcs);

    if (view->dragDestinationProcs)
        wfree(view->dragDestinationProcs);

    if (view->dragImage)
        WMReleasePixmap(view->dragImage);
}

