
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "WUtil.h"


typedef struct W_Notification {
    char *name;
    void *object;
    void *clientData;
    int refCount;
} Notification;



char*
WMGetNotificationName(WMNotification *notification)
{
    return notification->name;
}


void*
WMGetNotificationObject(WMNotification *notification)
{
    return notification->object;
}


void*
WMGetNotificationClientData(WMNotification *notification)
{
    return notification->clientData;
}


WMNotification*
WMCreateNotification(char *name, void *object, void *clientData)
{
    Notification *nPtr;
    
    nPtr = wmalloc(sizeof(Notification));
    
    nPtr->name = name;
    nPtr->object = object;
    nPtr->clientData = clientData;
    
    nPtr->refCount = 1;
    
    return nPtr;
}


void
WMReleaseNotification(WMNotification *notification)
{
    notification->refCount--;
    
    if (notification->refCount < 1) {
	free(notification);
    }
}


WMNotification*
WMRetainNotification(WMNotification *notification)
{
    notification->refCount++;
    
    return notification;
}


/***************** Notification Center *****************/

typedef struct NotificationObserver {
    WMNotificationObserverAction *observerAction;
    void *observer;

    char *name;
    void *object;

    struct NotificationObserver *prev; /* for tables */
    struct NotificationObserver *next;
    struct NotificationObserver *nextAction;   /* for observerTable */
} NotificationObserver;


typedef struct W_NotificationCenter {
    WMHashTable *nameTable;	       /* names -> observer lists */
    WMHashTable *objectTable;	       /* object -> observer lists */
    NotificationObserver *nilList;     /* obervers that catch everything */
    
    WMHashTable *observerTable;	       /* observer -> NotificationObserver */
} NotificationCenter;


/* default (and only) center */
static NotificationCenter *notificationCenter = NULL;


void
W_InitNotificationCenter(void)
{
    notificationCenter = wmalloc(sizeof(NotificationCenter));
    
    notificationCenter->nameTable = WMCreateHashTable(WMStringPointerHashCallbacks);
    notificationCenter->objectTable = WMCreateHashTable(WMIntHashCallbacks);
    notificationCenter->nilList = NULL;
    
    notificationCenter->observerTable = WMCreateHashTable(WMIntHashCallbacks);
}


void
WMAddNotificationObserver(WMNotificationObserverAction *observerAction,
			  void *observer, char *name, void *object)
{
    NotificationObserver *oRec, *rec;
    
    oRec = wmalloc(sizeof(NotificationObserver));
    oRec->observerAction = observerAction;
    oRec->observer = observer;
    oRec->name = name;
    oRec->object = object;
    oRec->next = NULL;
    oRec->prev = NULL;
    
    
    /* put this action in the list of actions for this observer */
    rec = WMHashInsert(notificationCenter->observerTable, observer, oRec);
    
    if (rec) {
	/* if this is not the first action for the observer */
	oRec->nextAction = rec;
    } else {
	oRec->nextAction = NULL;
    }

    if (!name && !object) {
	/* catch-all */
	oRec->next = notificationCenter->nilList;
	if (notificationCenter->nilList) {
	    notificationCenter->nilList->prev = oRec;
	}
	notificationCenter->nilList = oRec;
    } else if (!name) {
	/* any message coming from object */
	rec = WMHashInsert(notificationCenter->objectTable, object, oRec);
	oRec->next = rec;
	if (rec) {
	    rec->prev = oRec;
	}
    } else {
	/* name && (object || !object) */
	rec = WMHashInsert(notificationCenter->nameTable, name, oRec);
	oRec->next = rec;
	if (rec) {
	    rec->prev = oRec;
	}
    }
}


void
WMPostNotification(WMNotification *notification)
{
    NotificationObserver *orec, *tmp;

    WMRetainNotification(notification);
    
    /* tell the observers that want to know about a particular message */
    orec = WMHashGet(notificationCenter->nameTable, notification->name);
    
    while (orec) {
	tmp = orec->next;
	
	if (!orec->object || orec->object == notification->object) {
	    /* tell the observer */
	    if (orec->observerAction) {
		(*orec->observerAction)(orec->observer, notification);
	    }
	}

	orec = tmp;
    }

    /* tell the observers that want to know about an object */
    orec = WMHashGet(notificationCenter->objectTable, notification->object);
    
    while (orec) {
	tmp = orec->next;
	
	/* tell the observer */
	if (orec->observerAction) {
	    (*orec->observerAction)(orec->observer, notification);
	}
	orec = tmp;
    }

    /* tell the catch all observers */
    orec = notificationCenter->nilList;
    while (orec) {
	tmp = orec->next;

	/* tell the observer */
	if (orec->observerAction) {
	    (*orec->observerAction)(orec->observer, notification);
	}
	orec = tmp;
    }

    WMReleaseNotification(notification);
}




void
WMRemoveNotificationObserver(void *observer)
{
    NotificationObserver *orec, *tmp, *rec;


    /* get the list of actions the observer is doing */
    orec = WMHashGet(notificationCenter->observerTable, observer);

    /*
     * FOREACH orec IN actionlist for observer
     * DO
     *   remove from respective lists/tables
     *   free
     * END
     */
    while (orec) {
	tmp = orec->nextAction;

	if (!orec->name && !orec->object) {
	    /* catch-all */
	    if (notificationCenter->nilList==orec)
		notificationCenter->nilList = orec->next;
	} else if (!orec->name) {
	    /* any message coming from object */
	    rec = WMHashGet(notificationCenter->objectTable, orec->object);
	    if (rec==orec) {
		/* replace table entry */
		if (orec->next) {
		    WMHashInsert(notificationCenter->objectTable, orec->object,
				 orec->next);
		} else {
		    WMHashRemove(notificationCenter->objectTable, orec->object);
		}
	    }
	} else {
	    /* name && (object || !object) */
	    rec = WMHashGet(notificationCenter->nameTable, orec->name);
	    if (rec==orec) {
		/* replace table entry */
		if (orec->next) {
		    WMHashInsert(notificationCenter->nameTable, orec->name,
				 orec->next);
		} else {
		    WMHashRemove(notificationCenter->nameTable, orec->name);
		}
	    }
	}
	if (orec->prev)
	    orec->prev->next = orec->next;
	if (orec->next)
	    orec->next->prev = orec->prev;

	free(orec);
	
	orec = tmp;
    }

    WMHashRemove(notificationCenter->observerTable, observer);
}


void
WMRemoveNotificationObserverWithName(void *observer, char *name, void *object)
{
    NotificationObserver *orec, *tmp, *rec;

    /* get the list of actions the observer is doing */
    orec = WMHashGet(notificationCenter->observerTable, observer);

    while (orec) {
	tmp = orec->nextAction;
	if (orec->name == name && orec->object == object) {
	    if (!name && !object) {
		if (notificationCenter->nilList == orec)
		    notificationCenter->nilList = orec->next;
	    } else if (!name) {
		rec = WMHashGet(notificationCenter->objectTable, orec->object);
		if (rec==orec) {
		    assert(rec->prev==NULL);
		    /* replace table entry */
		    if (orec->next) {
			WMHashInsert(notificationCenter->objectTable, orec->object,
				     orec->next);
		    } else {
			WMHashRemove(notificationCenter->objectTable, 
				     orec->object);
		    }
		}
	    } else {
		rec = WMHashGet(notificationCenter->nameTable, orec->name);
		if (rec==orec) {
		    assert(rec->prev==NULL);
		    /* replace table entry */
		    if (orec->next) {
			WMHashInsert(notificationCenter->nameTable, orec->name,
				     orec->next);
		    } else {
			WMHashRemove(notificationCenter->nameTable, orec->name);
		    }
		}
	    }
	    
	    /* update the action list for the observer */
	    rec = WMHashGet(notificationCenter->observerTable, observer);
	    
	    if (rec == orec) {
		if (orec->nextAction) {
		    WMHashInsert(notificationCenter->nameTable, observer,
				 orec->nextAction);
		} else {
		    WMHashRemove(notificationCenter->nameTable, observer);
		}
	    }

	    if (orec->prev)
		orec->prev->next = orec->next;
	    if (orec->next)
		orec->next->prev = orec->prev;
	    free(orec);
	}
	orec = tmp;
    }
}


void
WMPostNotificationName(char *name, void *object, void *clientData)
{
    WMNotification *notification;

    notification = WMCreateNotification(name, object, clientData);
    
    WMPostNotification(notification);

    WMReleaseNotification(notification);
}



/**************** Notification Queues ****************/


typedef struct W_NotificationQueue {
    NotificationCenter *center;
    void *asapQueue;
    void *idleQueue;
} NotificationQueue;


/* default queue */
static WMNotificationQueue *notificationQueue = NULL;

WMNotificationQueue*
WMGetDefaultNotificationQueue(void)
{
    return notificationQueue;
}


WMNotificationQueue*
WMCreateNotificationQueue(void)
{
    return NULL;
}


void
WMDequeueNotificationMatching(WMNotificationQueue *queue, unsigned mask)
{
}


void
WMEnqueueNotification(WMNotificationQueue *queue, WMNotification *notification,
		      WMPostingStyle postingStyle)
{
}


void
WMEnqueueCoalesceNotification(WMNotificationQueue *queue, 
			      WMNotification *notification,
			      WMPostingStyle postingStyle,
			      unsigned coalesceMask)
{
}






