#ifndef _WUTIL_H_
#define _WUTIL_H_

#include <X11/Xlib.h>

#include <sys/types.h>


/* SunOS 4.x Blargh.... */
#ifndef NULL
#define NULL ((void*)0)
#endif


/*
 * Warning: proplist.h #defines BOOL which will clash with the
 * typedef BOOL in Xmd.h
 * proplist.h should use Bool (which is a #define in Xlib.h) instead.
 *
 */
#include <proplist.h>


#ifndef WMAX
# define WMAX(a,b)	((a)>(b) ? (a) : (b))
#endif
#ifndef WMIN
# define WMIN(a,b)	((a)<(b) ? (a) : (b))
#endif


#if (!defined (__GNUC__) || __GNUC__ < 2 || \
     __GNUC_MINOR__ < (defined (__cplusplus) ? 6 : 4))
#define __ASSERT_FUNCTION       ((char *) 0)
#else
#define __ASSERT_FUNCTION       __PRETTY_FUNCTION__
#endif


#ifdef NDEBUG

#define wassertr(expr, val)		((void)0)

#else /* !NDEBUG */

#define wassertr(expr) \
	if (!(expr)) { \
		wwarning("%s line %i (%s): assertion %s failed",\
			__FILE__, __LINE__, __ASSERT_FUNCTION, #expr);\
		return;\
	}

#define wassertrv(expr, val) \
	if (!(expr)) { \
		wwarning("%s line %i (%s): assertion %s failed",\
			__FILE__, __LINE__, __ASSERT_FUNCTION, #expr);\
		return (val);\
	}

#endif /* !NDEBUG */



typedef enum {
    WMPostWhenIdle = 1,
	WMPostASAP = 2,
	WMPostNow = 3
} WMPostingStyle;


typedef enum {
    WNCNone = 0,
	WNCOnName = 1,
	WNCOnSender = 2
} WMNotificationCoalescing;




typedef struct W_HashTable WMHashTable;
typedef struct W_UserDefaults WMUserDefaults;
typedef struct W_Notification WMNotification;
typedef struct W_NotificationQueue WMNotificationQueue;


/* DO NOT ACCESS THE CONTENTS OF THIS STRUCT */
typedef struct {
    void *table;
    void *nextItem;
    int index;
} WMHashEnumerator;


typedef struct {
    /* NULL is pointer hash */
    unsigned 	(*hash)(const void *);
    /* NULL is pointer compare */
    Bool	(*keyIsEqual)(const void *, const void *);
    /* NULL does nothing */
    void*	(*retainKey)(const void *);
    /* NULL does nothing */
    void	(*releaseKey)(const void *);    
} WMHashTableCallbacks;


#if 0
typedef struct {
    char character;		       /* the escape character */
    char *value;		       /* value to place */
} WMSEscapes;
#endif


typedef void WMNotificationObserverAction(void *observerData, 
					  WMNotification *notification);



/*......................................................................*/


void wfatal(const char *msg, ...);
void wwarning(const char *msg, ...);
void wsyserror(const char *msg, ...);

char *wfindfile(char *paths, char *file);

char *wfindfileinlist(char **path_list, char *file);

char *wexpandpath(char *path);

/* don't free the returned string */
char *wgethomedir();

void *wmalloc(size_t size);
void *wrealloc(void *ptr, size_t newsize);

void wrelease(void *ptr);
void *wretain(void *ptr);

char *wstrdup(char *str);

char *wstrappend(char *dst, char *src);

char *wusergnusteppath();

char *wdefaultspathfordomain(char *domain);

void wusleep(unsigned int microsec);

#if 0
int wsprintesc(char *buffer, int length, char *format, WMSEscapes **escapes,
	       int count);
#endif
/*......................................................................*/


WMHashTable *WMCreateHashTable(WMHashTableCallbacks callbacks);

void WMFreeHashTable(WMHashTable *table);

void WMResetHashTable(WMHashTable *table);

void *WMHashGet(WMHashTable *table, const void *key);

/* put data in table, replacing already existing data and returning
 * the old value */
void *WMHashInsert(WMHashTable *table, void *key, void *data);

void WMHashRemove(WMHashTable *table, const void *key);

/* warning: do not manipulate the table while using these functions */
WMHashEnumerator WMEnumerateHashTable(WMHashTable *table);

void *WMNextHashEnumeratorItem(WMHashEnumerator *enumerator);

unsigned WMCountHashTable(WMHashTable *table);




/* some predefined callback sets */

extern const WMHashTableCallbacks WMIntHashCallbacks;
/* sizeof(keys) are <= sizeof(void*) */

extern const WMHashTableCallbacks WMStringHashCallbacks;
/* keys are strings. Strings will be copied with wstrdup() 
 * and freed with free() */

extern const WMHashTableCallbacks WMStringPointerHashCallbacks;
/* keys are strings, bug they are not copied */

/*......................................................................*/

WMNotification *WMCreateNotification(char *name, void *object, void *clientData);

void WMReleaseNotification(WMNotification *notification);

WMNotification *WMRetainNotification(WMNotification *notification);

void *WMGetNotificationClientData(WMNotification *notification);

void *WMGetNotificationObject(WMNotification *notification);

char *WMGetNotificationName(WMNotification *notification);


void WMAddNotificationObserver(WMNotificationObserverAction *observerAction, 
			       void *observer, char *name, void *object);

void WMPostNotification(WMNotification *notification);

void WMRemoveNotificationObserver(void *observer);

void WMRemoveNotificationObserverWithName(void *observer, char *name, 
					  void *object);

void WMPostNotificationName(char *name, void *object, void *clientData);

WMNotificationQueue *WMGetDefaultNotificationQueue(void);

WMNotificationQueue *WMCreateNotificationQueue(void);

void WMDequeueNotificationMatching(WMNotificationQueue *queue, unsigned mask);

void WMEnqueueNotification(WMNotificationQueue *queue, WMNotification *notification,
			   WMPostingStyle postingStyle);

void WMEnqueueCoalesceNotification(WMNotificationQueue *queue, 
				   WMNotification *notification,
				   WMPostingStyle postingStyle,
				   unsigned coalesceMask);


/*......................................................................*/

WMUserDefaults *WMGetStandardUserDefaults(void);

proplist_t WMGetUDObjectForKey(WMUserDefaults *database, char *defaultName);

void WMSetUDObjectForKey(WMUserDefaults *database, proplist_t object,
			 char *defaultName);

void WMRemoveUDObjectForKey(WMUserDefaults *database, char *defaultName);

/* you can free the returned string */
char *WMGetUDStringForKey(WMUserDefaults *database, char *defaultName);

int WMGetUDIntegerForKey(WMUserDefaults *database, char *defaultName);

int WMGetUDFloatForKey(WMUserDefaults *database, char *defaultName);

Bool WMGetUDBoolForKey(WMUserDefaults *database, char *defaultName);

void WMSetUDStringForKey(WMUserDefaults *database, char *value, 
			 char *defaultName);

void WMSetUDIntegerForKey(WMUserDefaults *database, int value, 
			  char *defaultName);

void WMSetUDFloatForKey(WMUserDefaults *database, float value, 
			char *defaultName);

void WMSetUDBoolForKey(WMUserDefaults *database, Bool value,
		       char *defaultName);

proplist_t WMGetUDSearchList(WMUserDefaults *database);

void WMSetUDSearchList(WMUserDefaults *database, proplist_t list);



#endif
