

#include "WINGsP.h"
#include <X11/Xatom.h>

#define XDND_SOURCE_RESPONSE_MAX_DELAY 3000

#define VERSION_INFO(dragInfo) dragInfo->protocolVersion

#define XDND_PROPERTY_FORMAT 32
#define XDND_ACTION_DESCRIPTION_FORMAT 8

#define XDND_DEST_INFO(dragInfo) dragInfo->destInfo
#define XDND_SOURCE_WIN(dragInfo) dragInfo->destInfo->sourceWindow
#define XDND_DEST_VIEW(dragInfo) dragInfo->destInfo->destView
#define XDND_DEST_STATE(dragInfo) dragInfo->destInfo->state
#define XDND_SOURCE_TYPES(dragInfo) dragInfo->destInfo->sourceTypes
#define XDND_TYPE_LIST_AVAILABLE(dragInfo) dragInfo->destInfo->typeListAvailable
#define XDND_REQUIRED_TYPES(dragInfo) dragInfo->destInfo->requiredTypes
#define XDND_SOURCE_ACTION(dragInfo) dragInfo->sourceAction
#define XDND_DEST_ACTION(dragInfo) dragInfo->destinationAction
#define XDND_SOURCE_OPERATIONS(dragInfo) dragInfo->destInfo->sourceOperations
#define XDND_DROP_DATAS(dragInfo) dragInfo->destInfo->dropDatas
#define XDND_DROP_DATA_COUNT(dragInfo) dragInfo->destInfo->dropDataCount
#define XDND_DEST_VIEW_STORED(dragInfo) ((dragInfo->destInfo) != NULL)\
    && ((dragInfo->destInfo->destView) != NULL)


static unsigned char XDNDversion = XDND_VERSION;
static WMHandlerID dndDestinationTimer = NULL;


static void* idleState(WMView *destView, XClientMessageEvent *event,
                       WMDraggingInfo *info);
static void* waitEnterState(WMView *destView, XClientMessageEvent *event,
                            WMDraggingInfo *info);
static void* inspectDropDataState(WMView *destView, XClientMessageEvent *event,
                                  WMDraggingInfo *info);
static void* dropAllowedState(WMView *destView, XClientMessageEvent *event,
                              WMDraggingInfo *info);
static void* dropNotAllowedState(WMView *destView, XClientMessageEvent *event,
                                 WMDraggingInfo *info);
static void* waitForDropDataState(WMView *destView, XClientMessageEvent *event,
                                  WMDraggingInfo *info);

/* ----- Types & datas list ----- */
static void
freeSourceTypeArrayItem(void *type)
{
    XFree(type);
}


static WMArray*
createSourceTypeArray(int initialSize)
{
    return WMCreateArrayWithDestructor(initialSize, freeSourceTypeArrayItem);
}


static void
freeDropDataArrayItem(void* data)
{
    if (data != NULL)
        WMReleaseData((WMData*) data);
}


static WMArray*
createDropDataArray(WMArray *requiredTypes)
{
    if (requiredTypes != NULL)
        return WMCreateArrayWithDestructor(
                                           WMGetArrayItemCount(requiredTypes),
                                           freeDropDataArrayItem);

    else
        return WMCreateArray(0);
}

static WMArray*
getTypesFromTypeList(WMScreen *scr, Window sourceWin)
{
    /* // WMDraggingInfo *info = &scr->dragInfo;*/
    Atom dataType;
    Atom* typeAtomList;
    WMArray* typeList;
    int i, format;
    unsigned long count, remaining;
    unsigned char *data = NULL;

    XGetWindowProperty(scr->display, sourceWin, scr->xdndTypeListAtom,
                       0, 0x8000000L, False, XA_ATOM, &dataType, &format,
                       &count, &remaining, &data);

    if (dataType!=XA_ATOM || format!=XDND_PROPERTY_FORMAT || count==0 || !data) {
        if (data) {
            XFree(data);
        }
        return createSourceTypeArray(0);
    }

    typeList = createSourceTypeArray(count);
    typeAtomList = (Atom*) data;
    for (i=0; i < count; i++) {
        WMAddToArray(typeList, XGetAtomName(scr->display, typeAtomList[i]));
    }

    XFree(data);

    return typeList;
}


static WMArray*
getTypesFromThreeTypes(WMScreen *scr, XClientMessageEvent *event)
{
    WMArray* typeList;
    Atom atom;
    int i;

    typeList = createSourceTypeArray(3);
    for (i = 2; i < 5; i++) {
        if (event->data.l[i] != None) {
            atom = (Atom)event->data.l[i];
            WMAddToArray(typeList, XGetAtomName(scr->display, atom));
        }
    }

    return typeList;
}


void
storeRequiredTypeList(WMDraggingInfo *info)
{
    WMView *destView = XDND_DEST_VIEW(info);
    WMScreen *scr = W_VIEW_SCREEN(destView);
    WMArray *requiredTypes;

    /* First, see if the 3 source types are enough for dest requirements */
    requiredTypes = destView->dragDestinationProcs->requiredDataTypes(
                                                                      destView,
                                                                      W_ActionToOperation(scr, XDND_SOURCE_ACTION(info)),
                                                                      XDND_SOURCE_TYPES(info));

    if (requiredTypes == NULL && XDND_TYPE_LIST_AVAILABLE(info)) {
        /* None of the 3 source types fits, get the whole type list */
        requiredTypes =
            destView->dragDestinationProcs->requiredDataTypes(
                                                              destView,
                                                              W_ActionToOperation(scr, XDND_SOURCE_ACTION(info)),
                                                              getTypesFromTypeList(scr, XDND_SOURCE_WIN(info)));
    }


    XDND_REQUIRED_TYPES(info) = requiredTypes;
}


char*
getNextRequestedDataType(WMDraggingInfo *info)
{
    /* get the type of the first data not yet retrieved from selection */
    int nextTypeIndex;

    if (XDND_REQUIRED_TYPES(info) != NULL) {
        nextTypeIndex = WMGetArrayItemCount(XDND_DROP_DATAS(info));
        return WMGetFromArray(XDND_REQUIRED_TYPES(info), nextTypeIndex);
        /* NULL if no more type */
    } else
        return NULL;
}


/* ----- Action list ----- */

WMArray*
sourceOperationList(WMScreen *scr, Window sourceWin)
{
    Atom dataType, *actionList;
    int i, size;
    unsigned long count, remaining;
    unsigned char* actionDatas = NULL;
    unsigned char* descriptionList = NULL;
    WMArray* operationArray;
    WMDragOperationItem* operationItem;
    char* description;

    remaining = 0;
    XGetWindowProperty(scr->display, sourceWin, scr->xdndActionListAtom,
                       0, 0x8000000L, False, XA_ATOM, &dataType, &size,
                       &count, &remaining, &actionDatas);

    if (dataType!=XA_ATOM || size!=XDND_PROPERTY_FORMAT || count==0 || !actionDatas) {
        wwarning("Cannot read action list");
        if (actionDatas) {
            XFree(actionDatas);
        }
        return NULL;
    }

    actionList = (Atom*)actionDatas;

    XGetWindowProperty(scr->display, sourceWin, scr->xdndActionDescriptionAtom,
                       0, 0x8000000L, False, XA_STRING, &dataType, &size,
                       &count, &remaining, &descriptionList);

    if (dataType!=XA_STRING || size!=XDND_ACTION_DESCRIPTION_FORMAT ||
        count==0 || !descriptionList) {
        wwarning("Cannot read action description list");
        if (actionList) {
            XFree(actionList);
        }
        if (descriptionList) {
            XFree(descriptionList);
        }
        return NULL;
    }

    operationArray = WMCreateDragOperationArray(count);
    description = descriptionList;

    for (i=0; count > 0; i++) {
        size = strlen(description);
        operationItem = WMCreateDragOperationItem(
                                                  W_ActionToOperation(scr, actionList[i]),
                                                  wstrdup(description));

        WMAddToArray(operationArray, operationItem);
        count -= (size + 1); /* -1 : -NULL char */

        /* next description */
        description = &(description[size + 1]);
    }

    XFree(actionList);
    XFree(descriptionList);

    return operationArray;
}


/* ----- Dragging Info ----- */
static void
updateSourceWindow(WMDraggingInfo *info, XClientMessageEvent *event)
{
    XDND_SOURCE_WIN(info) = (Window) event->data.l[0];
}


static Window
findChildInWindow(Display *dpy, Window toplevel, int x, int y)
{
    Window foo, bar;
    Window *children;
    unsigned nchildren;
    int i;

    if (!XQueryTree(dpy, toplevel, &foo, &bar,
                    &children, &nchildren) || children == NULL) {
        return None;
    }

    /* first window that contains the point is the one */
    for (i = nchildren-1; i >= 0; i--) {
        XWindowAttributes attr;

        if (XGetWindowAttributes(dpy, children[i], &attr)
            && attr.map_state == IsViewable
            && x >= attr.x && y >= attr.y
            && x < attr.x + attr.width && y < attr.y + attr.height) {
            Window child, tmp;

            tmp = children[i];
            child = findChildInWindow(dpy, tmp, x - attr.x, y - attr.y);
            XFree(children);

            if (child == None)
                return tmp;
            else
                return child;
        }
    }

    XFree(children);
    return None;
}


static WMView*
findXdndViewInToplevel(WMView* toplevel, int x, int y)
{
    WMScreen *scr = W_VIEW_SCREEN(toplevel);
    Window toplevelWin = WMViewXID(toplevel);
    int xInToplevel, yInToplevel;
    Window child, foo;
    WMView *childView;

    XTranslateCoordinates(scr->display, scr->rootWin, toplevelWin,
                          x, y, &xInToplevel, &yInToplevel,
                          &foo);

    child = findChildInWindow(scr->display, toplevelWin,
                              xInToplevel, yInToplevel);

    if (child != None) {
        childView = W_GetViewForXWindow(scr->display, child);

        /* if childView supports Xdnd, return childView */
        if (childView != NULL
            && childView->dragDestinationProcs != NULL)
            return childView;
    }

    return NULL;
}


/* Clear datas only used by current destination view */
static void
freeDestinationViewInfos(WMDraggingInfo *info)
{
    if (XDND_SOURCE_TYPES(info) != NULL) {
        WMFreeArray(XDND_SOURCE_TYPES(info));
        XDND_SOURCE_TYPES(info) = NULL;
    }

    if (XDND_DROP_DATAS(info) != NULL) {
        WMFreeArray(XDND_DROP_DATAS(info));
        XDND_DROP_DATAS(info) = NULL;
    }

    XDND_REQUIRED_TYPES(info) = NULL;
}

void
W_DragDestinationInfoClear(WMDraggingInfo *info)
{
    W_DragDestinationStopTimer();

    if (XDND_DEST_INFO(info) != NULL) {
        freeDestinationViewInfos(info);

        wfree(XDND_DEST_INFO(info));
        XDND_DEST_INFO(info) = NULL;
    }
}

static void
initDestinationDragInfo(WMDraggingInfo *info)
{
    XDND_DEST_INFO(info) =
        (W_DragDestinationInfo*) wmalloc(sizeof(W_DragDestinationInfo));

    XDND_DEST_STATE(info) = idleState;
    XDND_DEST_VIEW(info) = NULL;

    XDND_SOURCE_TYPES(info) = NULL;
    XDND_REQUIRED_TYPES(info) = NULL;
    XDND_DROP_DATAS(info) = NULL;
}


void
W_DragDestinationStoreEnterMsgInfo(WMDraggingInfo *info,
                                   WMView *toplevel, XClientMessageEvent *event)
{
    WMScreen *scr = W_VIEW_SCREEN(toplevel);

    if (XDND_DEST_INFO(info) == NULL)
        initDestinationDragInfo(info);

    updateSourceWindow(info, event);

    /* store xdnd version for source */
    info->protocolVersion = (event->data.l[1] >> 24);

    XDND_SOURCE_TYPES(info) = getTypesFromThreeTypes(scr, event);

    /* to use if the 3 types are not enough */
    XDND_TYPE_LIST_AVAILABLE(info) = (event->data.l[1] & 1);
}


static void
cancelDrop(WMView *destView, WMDraggingInfo *info);

static void
suspendDropAuthorization(WMView *destView, WMDraggingInfo *info);


void
    W_DragDestinationStorePositionMsgInfo(WMDraggingInfo *info,
                                          WMView *toplevel, XClientMessageEvent *event)
{
    int x = event->data.l[2] >> 16;
    int y = event->data.l[2] & 0xffff;
    WMView *oldDestView;
    WMView *newDestView;

    newDestView = findXdndViewInToplevel(toplevel, x, y);

    if (XDND_DEST_INFO(info) == NULL) {
        initDestinationDragInfo(info);
        updateSourceWindow(info, event);
        XDND_DEST_VIEW(info) = newDestView;
    }
    else {
        oldDestView = XDND_DEST_VIEW(info);

        if (newDestView != oldDestView) {
            if (oldDestView != NULL) {
                suspendDropAuthorization(oldDestView, info);
                XDND_DEST_STATE(info) = dropNotAllowedState;
            }

            updateSourceWindow(info, event);
            XDND_DEST_VIEW(info) = newDestView;

            if (newDestView != NULL) {
                if (XDND_DEST_STATE(info) != waitEnterState)
                    XDND_DEST_STATE(info) = idleState;
            }
        }
    }

    XDND_SOURCE_ACTION(info) = event->data.l[4];

    /* note: source position is not stored */
}

/* ----- End of Dragging Info ----- */


/* ----- Messages ----- */

/* send a DnD message to the source window */
static void
sendDnDClientMessage(WMView *destView, Atom message,
                     unsigned long data1,
                     unsigned long data2,
                     unsigned long data3,
                     unsigned long data4)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    WMDraggingInfo *info = &scr->dragInfo;

    if (XDND_DEST_INFO(info) != NULL) {
        if (! W_SendDnDClientMessage(scr->display,
                                     XDND_SOURCE_WIN(info),
                                     message,
                                     WMViewXID(destView),
                                     data1,
                                     data2,
                                     data3,
                                     data4)) {
            /* drop failed */
            W_DragDestinationInfoClear(info);
        }
    }
}


static void
storeDropData(WMView *destView, Atom selection, Atom target,
              Time timestamp, void *cdata, WMData *data)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    WMDraggingInfo *info = &(scr->dragInfo);
    WMData *dataToStore = NULL;

    if (data != NULL)
        dataToStore = WMRetainData(data);

    if (XDND_DEST_INFO(info) != NULL && XDND_DROP_DATAS(info) != NULL) {
        WMAddToArray(XDND_DROP_DATAS(info), dataToStore);
        W_SendDnDClientMessage(scr->display, WMViewXID(destView),
                               scr->xdndSelectionAtom,
                               WMViewXID(destView),
                               0, 0, 0, 0);
    }
}


Bool
requestDropDataInSelection(WMView *destView, char* type)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);

    if (type != NULL) {
        if (!WMRequestSelection(destView,
                                scr->xdndSelectionAtom,
                                XInternAtom(scr->display, type, False),
                                CurrentTime,
                                storeDropData, NULL)) {
            wwarning("could not request data for dropped data");
            return False;
        }

        return True;
    }

    return False;
}


Bool
requestDropData(WMDraggingInfo *info)
{
    WMView *destView = XDND_DEST_VIEW(info);
    char* nextType = getNextRequestedDataType(info);

    while ((nextType != NULL)
           && (!requestDropDataInSelection(destView, nextType)) ) {
        /* store NULL if request failed, and try with next type */
        WMAddToArray(XDND_DROP_DATAS(info), NULL);
        nextType = getNextRequestedDataType(info);
    }

    /* remains types to retrieve ? */
    return (nextType != NULL);
}


static void
concludeDrop(WMView *destView)
{
    destView->dragDestinationProcs->concludeDragOperation(destView);
}


/* send cancel message to the source */
static void
cancelDrop(WMView *destView, WMDraggingInfo *info)
{
    /* send XdndStatus with action None */
    sendDnDClientMessage(destView,
                         W_VIEW_SCREEN(destView)->xdndStatusAtom,
                         0, 0, 0, None);
    concludeDrop(destView);
    freeDestinationViewInfos(info);
}


/* suspend drop, when dragged icon enter an unaware subview of destView */
static void
suspendDropAuthorization(WMView *destView, WMDraggingInfo *info)
{
    /* free datas that depend on destination behaviour */
    /* (in short: only keep source's types) */
    if (XDND_DROP_DATAS(info) != NULL) {
        WMFreeArray(XDND_DROP_DATAS(info));
        XDND_DROP_DATAS(info) = NULL;
    }
    XDND_REQUIRED_TYPES(info) = NULL;

    /* send XdndStatus with action None */
    sendDnDClientMessage(destView,
                         W_VIEW_SCREEN(destView)->xdndStatusAtom,
                         0, 0, 0, None);
}


/* cancel drop on Enter message, if protocol version is nok */
void
W_DragDestinationCancelDropOnEnter(WMView *toplevel, WMDraggingInfo *info)
{
    if (XDND_DEST_VIEW_STORED(info))
        cancelDrop(XDND_DEST_VIEW(info), info);
    else {
        /* send XdndStatus with action None */
        sendDnDClientMessage(toplevel,
                             W_VIEW_SCREEN(toplevel)->xdndStatusAtom,
                             0, 0, 0, None);
    }

    W_DragDestinationInfoClear(info);
}


static void
finishDrop(WMView *destView, WMDraggingInfo *info)
{
    sendDnDClientMessage(destView,
                         W_VIEW_SCREEN(destView)->xdndFinishedAtom,
                         0, 0, 0, 0);
    concludeDrop(destView);
    W_DragDestinationInfoClear(info);
}


static Atom
getAllowedAction(WMView *destView, WMDraggingInfo *info)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);

    return W_OperationToAction(scr,
                               destView->dragDestinationProcs->allowedOperation(
                                                                                destView,
                                                                                W_ActionToOperation(scr, XDND_SOURCE_ACTION(info)),
                                                                                XDND_SOURCE_TYPES(info)));
}


/*  send the action that can be performed,
 and the limits outside wich the source must re-send
 its position and action */
static void
sendAllowedAction(WMView *destView, Atom action)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    /* // WMPoint destPos = WMGetViewScreenPosition(destView); */
    WMSize destSize = WMGetViewSize(destView);
    int destX, destY;
    Window foo;

    XTranslateCoordinates(scr->display, scr->rootWin, WMViewXID(destView),
                          0, 0, &destX, &destY,
                          &foo);

    sendDnDClientMessage(destView,
                         scr->xdndStatusAtom,
                         1,
                         (destX << 16)|destY,
                         (destSize.width << 16)|destSize.height,
                         action);
}


static void*
checkActionAllowed(WMView *destView, WMDraggingInfo* info)
{
    XDND_DEST_ACTION(info) =
        getAllowedAction(destView, info);

    if (XDND_DEST_ACTION(info) == None) {
        suspendDropAuthorization(destView, info);
        return dropNotAllowedState;
    }

    sendAllowedAction(destView, XDND_DEST_ACTION(info));
    return dropAllowedState;
}

static void*
checkDropAllowed(WMView *destView, XClientMessageEvent *event,
                 WMDraggingInfo* info)
{
    storeRequiredTypeList(info);

    if (destView->dragDestinationProcs->inspectDropData != NULL) {
        XDND_DROP_DATAS(info) = createDropDataArray(
                                                    XDND_REQUIRED_TYPES(info));

        /* store first available data */
        if (requestDropData(info))
            return inspectDropDataState;

        /* no data retrieved, but inspect can allow it */
        if (destView->dragDestinationProcs->inspectDropData(
                                                            destView,
                                                            XDND_DROP_DATAS(info)))
            return checkActionAllowed(destView, info);

        suspendDropAuthorization(destView, info);
        return dropNotAllowedState;
    }

    return checkActionAllowed(destView, info);
}

static WMPoint*
getDropLocationInView(WMView *view)
{
    Window rootWin, childWin;
    int rootX, rootY;
    unsigned int mask;
    WMPoint* location;

    location = (WMPoint*) wmalloc(sizeof(WMPoint));

    XQueryPointer(
                  W_VIEW_SCREEN(view)->display,
                  WMViewXID(view), &rootWin, &childWin,
                  &rootX, &rootY,
                  &(location->x), &(location->y),
                  &mask);

    return location;
}

static void
callPerformDragOperation(WMView *destView, WMDraggingInfo *info)
{
    WMArray *operationList = NULL;
    WMScreen *scr = W_VIEW_SCREEN(destView);
    WMPoint* dropLocation;

    if (XDND_SOURCE_ACTION(info) == scr->xdndActionAsk)
        operationList = sourceOperationList(scr, XDND_SOURCE_WIN(info));

    dropLocation = getDropLocationInView(destView);
    destView->dragDestinationProcs->performDragOperation(
                                                         destView,
                                                         XDND_DROP_DATAS(info),
                                                         operationList,
                                                         dropLocation);

    wfree(dropLocation);
    if (operationList != NULL)
        WMFreeArray(operationList);
}


/* ----- Destination timer ----- */
static void
dragSourceResponseTimeOut(void *destView)
{
    WMView *view = (WMView*) destView;
    WMDraggingInfo *info;

    wwarning("delay for drag source response expired");
    if (view != NULL) {
        info = &(W_VIEW_SCREEN(view)->dragInfo);
        if (XDND_DEST_VIEW_STORED(info))
            cancelDrop(view, info);
        else {
            /* send XdndStatus with action None */
            sendDnDClientMessage(view,
                                 W_VIEW_SCREEN(view)->xdndStatusAtom,
                                 0, 0, 0, None);
        }

        W_DragDestinationInfoClear(info);
    }
}

void
W_DragDestinationStopTimer()
{
    if (dndDestinationTimer != NULL) {
        WMDeleteTimerHandler(dndDestinationTimer);
        dndDestinationTimer = NULL;
    }
}

void
W_DragDestinationStartTimer(WMDraggingInfo *info)
{
    W_DragDestinationStopTimer();

    if (XDND_DEST_STATE(info) != idleState
        || XDND_DEST_VIEW(info) == NULL) {
        /* note: info->destView == NULL means :
         Enter message has been received, waiting for Position message */

        dndDestinationTimer = WMAddTimerHandler(
                                                XDND_SOURCE_RESPONSE_MAX_DELAY,
                                                dragSourceResponseTimeOut,
                                                XDND_DEST_VIEW(info));
    }
}
/* ----- End of Destination timer ----- */


/* ----- Destination states ----- */

#ifdef XDND_DEBUG
static const char*
stateName(W_DndState *state)
{
    if (state == NULL)
        return "no state defined";

    if (state == idleState)
        return "idleState";

    if (state == waitEnterState)
        return "waitEnterState";

    if (state == inspectDropDataState)
        return "inspectDropDataState";

    if (state == dropAllowedState)
        return "dropAllowedState";

    if (state == dropNotAllowedState)
        return "dropNotAllowedState";

    if (state == waitForDropDataState)
        return "waitForDropDataState";

    return "unknown state";
}
#endif

static void*
idleState(WMView *destView, XClientMessageEvent *event,
          WMDraggingInfo *info)
{
    WMScreen *scr;
    Atom sourceMsg;

    scr = W_VIEW_SCREEN(destView);
    sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndPositionAtom) {
        destView->dragDestinationProcs->prepareForDragOperation(destView);

        if (XDND_SOURCE_TYPES(info) != NULL) {
            /* enter message infos are available */
            return checkDropAllowed(destView, event, info);
        }

        /* waiting for enter message */
        return waitEnterState;
    }

    return idleState;
}


/*  Source position and action are stored,
 waiting for xdnd protocol version and source type */
static void*
waitEnterState(WMView *destView, XClientMessageEvent *event,
               WMDraggingInfo *info)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    Atom sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndEnterAtom) {
        W_DragDestinationStoreEnterMsgInfo(info, destView, event);
        return checkDropAllowed(destView, event, info);
    }

    return waitEnterState;
}


/* We have requested a data, and have received it */
static void*
inspectDropDataState(WMView *destView, XClientMessageEvent *event,
                     WMDraggingInfo *info)
{
    WMScreen *scr;
    Atom sourceMsg;

    scr = W_VIEW_SCREEN(destView);
    sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndSelectionAtom) {
        /* a data has been retrieved, store next available */
        if (requestDropData(info))
            return inspectDropDataState;

        /* all required (and available) datas are stored */
        if (destView->dragDestinationProcs->inspectDropData(
                                                            destView,
                                                            XDND_DROP_DATAS(info)))
            return checkActionAllowed(destView, info);

        suspendDropAuthorization(destView, info);
        return dropNotAllowedState;
    }

    return inspectDropDataState;
}


static void*
dropNotAllowedState(WMView *destView, XClientMessageEvent *event,
                    WMDraggingInfo *info)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    Atom sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndDropAtom) {
        finishDrop(destView, info);
        return idleState;
    }

    return dropNotAllowedState;
}


static void*
dropAllowedState(WMView *destView, XClientMessageEvent *event,
                 WMDraggingInfo *info)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    Atom sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndDropAtom) {
        if (XDND_DROP_DATAS(info) != NULL) {
            /* drop datas were cached with inspectDropData call */
            callPerformDragOperation(destView, info);
        } else {
            XDND_DROP_DATAS(info) = createDropDataArray(
                                                        XDND_REQUIRED_TYPES(info));

            /* store first available data */
            if (requestDropData(info))
                return waitForDropDataState;

            /* no data retrieved */
            callPerformDragOperation(destView, info);
        }

        finishDrop(destView, info);
        return idleState;
    }

    return dropAllowedState;
}


static void*
waitForDropDataState(WMView *destView, XClientMessageEvent *event,
                     WMDraggingInfo *info)
{
    WMScreen *scr = W_VIEW_SCREEN(destView);
    Atom sourceMsg = event->message_type;

    if (sourceMsg == scr->xdndSelectionAtom) {
        /* store next data */
        if (requestDropData(info))
            return waitForDropDataState;

        /* all required (and available) datas are stored */
        callPerformDragOperation(destView, info);

        finishDrop(destView, info);
        return idleState;
    }

    return waitForDropDataState;
}

/* ----- End of Destination states ----- */


void
W_DragDestinationStateHandler(WMDraggingInfo *info, XClientMessageEvent *event)
{
    WMView *destView;
    W_DndState* newState;

    if (XDND_DEST_VIEW_STORED(info)) {
        destView = XDND_DEST_VIEW(info);
        if (XDND_DEST_STATE(info) == NULL)
            XDND_DEST_STATE(info) = idleState;

#ifdef XDND_DEBUG

        printf("current dest state: %s\n",
               stateName(XDND_DEST_STATE(info)));
#endif

        newState = (W_DndState*) XDND_DEST_STATE(info)(destView, event, info);

#ifdef XDND_DEBUG

        printf("new dest state: %s\n", stateName(newState));
#endif

        if (XDND_DEST_INFO(info) != NULL) {
            XDND_DEST_STATE(info) = newState;
            if (XDND_DEST_STATE(info) != idleState)
                W_DragDestinationStartTimer(info);
        }
    }
}


static void
realizedObserver(void *self, WMNotification *notif)
{
    WMView *view = (WMView*)WMGetNotificationObject(notif);
    WMScreen *scr = W_VIEW_SCREEN(view);

    XChangeProperty(scr->display, W_VIEW_DRAWABLE(view),
                    scr->xdndAwareAtom,
                    XA_ATOM, XDND_PROPERTY_FORMAT, PropModeReplace,
                    &XDNDversion, 1);

    WMRemoveNotificationObserver(self);
}


void
W_SetXdndAwareProperty(WMScreen *scr, WMView *view, Atom *types,
                       int typeCount)
{
    WMView *toplevel = W_TopLevelOfView(view);

    if (!toplevel->flags.xdndHintSet) {
        toplevel->flags.xdndHintSet = 1;

        if (toplevel->flags.realized) {
            XChangeProperty(scr->display, W_VIEW_DRAWABLE(toplevel),
                            scr->xdndAwareAtom, XA_ATOM, XDND_PROPERTY_FORMAT,
                            PropModeReplace, &XDNDversion, 1);
        } else {
            WMAddNotificationObserver(realizedObserver,
                                      /* just use as an id */
                                      &view->dragDestinationProcs,
                                      WMViewRealizedNotification,
                                      toplevel);
        }
    }
}


void
WMRegisterViewForDraggedTypes(WMView *view, WMArray *acceptedTypes)
{
    Atom *types;
    int typeCount;
    int i;

    typeCount = WMGetArrayItemCount(acceptedTypes);
    types = wmalloc(sizeof(Atom)*(typeCount+1));

    for (i = 0; i < typeCount; i++) {
        types[i] = XInternAtom(W_VIEW_SCREEN(view)->display,
                               WMGetFromArray(acceptedTypes, i),
                               False);
    }
    types[i] = 0;

    view->droppableTypes = types;
    /* WMFreeArray(acceptedTypes); */

    W_SetXdndAwareProperty(W_VIEW_SCREEN(view), view, types, typeCount);
}


void
WMUnregisterViewDraggedTypes(WMView *view)
{
    if (view->droppableTypes != NULL)
        wfree(view->droppableTypes);
    view->droppableTypes = NULL;
}


/*
 requestedOperation: operation requested by the source
 sourceDataTypes:  data types (mime-types strings) supported by the source
 (never NULL, destroyed when drop ends)
 return operation allowed by destination (self)
 */
static WMDragOperationType
defAllowedOperation(WMView *self,
                    WMDragOperationType requestedOperation,
                    WMArray* sourceDataTypes)
{
    /* no operation allowed */
    return WDOperationNone;
}


/*
 requestedOperation: operation requested by the source
 sourceDataTypes:  data types (mime-types strings) supported by the source
 (never NULL, destroyed when drop ends)
 return data types (mime-types strings) required by destination (self)
 or NULL if no suitable data type is available (force
 to 2nd pass with full source type list).
 */
static WMArray*
defRequiredDataTypes (WMView *self,
                      WMDragOperationType requestedOperation,
                      WMArray* sourceDataTypes)
{
    /* no data type allowed (NULL even at 2nd pass) */
    return NULL;
}


/*
 Executed when the drag enters destination (self)
 */
static void
defPrepareForDragOperation(WMView *self)
{
}


/*
 Checks datas to be dropped (optional).
 dropDatas: datas (WMData*) required by destination (self)
 (given in same order as returned by requiredDataTypes).
 A NULL data means it couldn't be retreived.
 Destroyed when drop ends.
 return true if data array is ok
 */
/* Bool (*inspectDropData)(WMView *self, WMArray *dropDatas); */


/*
 Process drop
 dropDatas: datas (WMData*) required by destination (self)
 (given in same order as returned by requiredDataTypes).
 A NULL data means it couldn't be retrivied.
 Destroyed when drop ends.
 operationList: if source operation is WDOperationAsk, contains
 operations (and associated texts) that can be asked
 to source. (destroyed after performDragOperation call)
 Otherwise this parameter is NULL.
 */
static void
defPerformDragOperation(WMView *self, WMArray *dropDatas,
                        WMArray *operationList, WMPoint *dropLocation)
{
}


/* Executed after drop */
static void
defConcludeDragOperation(WMView *self)
{
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
    if (procs->allowedOperation == NULL) {
        view->dragDestinationProcs->allowedOperation = defAllowedOperation;
    }
    if (procs->allowedOperation == NULL) {
        view->dragDestinationProcs->requiredDataTypes = defRequiredDataTypes;
    }

    /* note: inspectDropData can be NULL, if data consultation is not needed
     to give drop permission */

    if (procs->prepareForDragOperation == NULL) {
        view->dragDestinationProcs->prepareForDragOperation = defPrepareForDragOperation;
    }
    if (procs->performDragOperation == NULL) {
        view->dragDestinationProcs->performDragOperation = defPerformDragOperation;
    }
    if (procs->concludeDragOperation == NULL) {
        view->dragDestinationProcs->concludeDragOperation = defConcludeDragOperation;
    }
}


