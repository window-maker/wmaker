#ifndef _WUTIL_H_
#define _WUTIL_H_

#include <X11/Xlib.h>
#include <limits.h>
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

#define wassertr(expr)		{}
#define wassertrv(expr, val)	{}

#else /* !NDEBUG */

#ifdef DEBUG

#include <assert.h>

#define wassertr(expr) 	assert(expr)

#define wassertrv(expr, val)	assert(expr)

#else /* !DEBUG */

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
#endif /* !DEBUG */

#endif /* !NDEBUG */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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


/* The possible states for connections */
typedef enum {
    WCNotConnected=0,
    WCListening,
    WCInProgress,
    WCFailed,
    WCConnected,
    WCTimedOut,
    WCDied,
    WCClosed
} WMConnectionState;


/* The possible states for connection timeouts */
typedef enum {
    WCTNone=0,
    WCTWhileOpening,
    WCTWhileSending
} WMConnectionTimeoutState;



enum {
    WBNotFound = INT_MIN, /* element was not found in WMBag   */
    WANotFound = -1       /* element was not found in WMArray */
};


typedef struct W_Array WMArray;
typedef struct W_Bag WMBag;
typedef struct W_Data WMData;
typedef struct W_TreeNode WMTreeNode;
typedef struct W_TreeNode WMTree;
typedef struct W_HashTable WMDictionary;
typedef struct W_HashTable WMHashTable;
typedef struct W_UserDefaults WMUserDefaults;
typedef struct W_Notification WMNotification;
typedef struct W_NotificationQueue WMNotificationQueue;
typedef struct W_Host WMHost;
typedef struct W_Connection WMConnection;


typedef int WMCompareDataProc(const void *item1, const void *item2);

typedef void WMFreeDataProc(void *data);

/* Used by WMBag or WMArray for matching data */
typedef int WMMatchDataProc(void *item, void *cdata);




typedef struct {
    int position;
    int count;
} WMRange;



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


typedef void *WMBagIterator;


#if 0
typedef struct {
    char character;		       /* the escape character */
    char *value;		       /* value to place */
} WMSEscapes;
#endif


/* The connection callbacks */
typedef struct ConnectionDelegate {
    void *data;

    void (*didCatchException)(struct ConnectionDelegate *self,
                              WMConnection *cPtr);

    void (*didDie)(struct ConnectionDelegate *self, WMConnection *cPtr);

    void (*didInitialize)(struct ConnectionDelegate *self, WMConnection *cPtr);

    void (*didReceiveInput)(struct ConnectionDelegate *self, WMConnection *cPtr);

    void (*didTimeout)(struct ConnectionDelegate *self, WMConnection *cPtr);

} ConnectionDelegate;


typedef void WMNotificationObserverAction(void *observerData, 
					  WMNotification *notification);



/*......................................................................*/

typedef void (waborthandler)(int);

waborthandler* wsetabort(waborthandler*);


/* don't free the returned string */
char* wstrerror(int errnum);

void wmessage(const char *msg, ...);
void wwarning(const char *msg, ...);
void wfatal(const char *msg, ...);
void wsyserror(const char *msg, ...);
void wsyserrorwithcode(int error, const char *msg, ...);

char* wfindfile(char *paths, char *file);

char* wfindfileinlist(char **path_list, char *file);

char* wfindfileinarray(proplist_t array, char *file);

char* wexpandpath(char *path);

/* don't free the returned string */
char* wgethomedir();

void* wmalloc(size_t size);
void* wrealloc(void *ptr, size_t newsize);
void wfree(void *ptr);


void wrelease(void *ptr);
void* wretain(void *ptr);

char* wstrdup(char *str);

/* Concatenate str1 with str2 and return that in a newly malloc'ed string.
 * str1 and str2 can be any strings including static and constant strings.
 * str1 and str2 will not be modified.
 * Free the returned string when you're done with it. */
char* wstrconcat(char *str1, char *str2);

/* This will append src to dst, and return dst. dst MUST be either NULL
 * or a string that was a result of a dynamic allocation (malloc, realloc
 * wmalloc, wrealloc, ...). dst CANNOT be a static or a constant string!
 * Modifies dst (no new string is created except if dst==NULL in which case
 * it is equivalent to calling wstrdup(src) ).
 * The returned address may be different from the original address of dst,
 * so always assign the returned address to avoid dangling pointers. */
char* wstrappend(char *dst, char *src);


void wtokensplit(char *command, char ***argv, int *argc);

char* wtokennext(char *word, char **next);

char* wtokenjoin(char **list, int count);

void wtokenfree(char **tokens, int count);

char* wtrimspace(char *s);



char* wusergnusteppath();

char* wdefaultspathfordomain(char *domain);

void wusleep(unsigned int microsec);

#if 0
int wsprintesc(char *buffer, int length, char *format, WMSEscapes **escapes,
	       int count);
#endif

/*......................................................................*/

/* This function is used _only_ if you create a NON-GUI program.
 * For GUI based programs use WMNextEvent()/WMHandleEvent() instead.
 * This function will handle all input/timer/idle events, then return.
 */

void WHandleEvents();

/*......................................................................*/


WMHashTable* WMCreateHashTable(WMHashTableCallbacks callbacks);

void WMFreeHashTable(WMHashTable *table);

void WMResetHashTable(WMHashTable *table);

void* WMHashGet(WMHashTable *table, const void *key);

/* put data in table, replacing already existing data and returning
 * the old value */
void* WMHashInsert(WMHashTable *table, const void *key, const void *data);

void WMHashRemove(WMHashTable *table, const void *key);

/* warning: do not manipulate the table while using these functions */
WMHashEnumerator WMEnumerateHashTable(WMHashTable *table);

void* WMNextHashEnumeratorItem(WMHashEnumerator *enumerator);

void* WMNextHashEnumeratorKey(WMHashEnumerator *enumerator);

unsigned WMCountHashTable(WMHashTable *table);




/* some predefined callback sets */

extern const WMHashTableCallbacks WMIntHashCallbacks;
/* sizeof(keys) are <= sizeof(void*) */

extern const WMHashTableCallbacks WMStringHashCallbacks;
/* keys are strings. Strings will be copied with wstrdup() 
 * and freed with wfree() */

extern const WMHashTableCallbacks WMStringPointerHashCallbacks;
/* keys are strings, bug they are not copied */


/*......................................................................*/

/*
 * WMArray use an array to store the elements.
 * Item indexes may be only positive integer numbers.
 * The array cannot contain holes in it.
 *
 * Pros:
 * Fast [O(1)] access to elements
 * Fast [O(1)] push/pop
 *
 * Cons:
 * A little slower [O(n)] for insertion/deletion of elements that 
 * 	aren't in the end
 */

WMArray* WMCreateArray(int initialSize);

WMArray* WMCreateArrayWithDestructor(int initialSize, WMFreeDataProc *destructor);

WMArray* WMCreateArrayWithArray(WMArray *array);

#define WMDuplicateArray(array) WMCreateArrayWithArray(array)

void WMEmptyArray(WMArray *array);

void WMFreeArray(WMArray *array);

int WMGetArrayItemCount(WMArray *array);

/* appends other to array. other remains unchanged */
void WMAppendArray(WMArray *array, WMArray *other);

/* add will place the element at the end of the array */
void WMAddToArray(WMArray *array, void *item);

#define WMPushInArray(array, item) WMAddToArray(array, item)

/* insert will increment the index of elements after it by 1 */
void WMInsertInArray(WMArray *array, int index, void *item);

/* replace and set will return the old item WITHOUT calling the
 * destructor on it even if its available. Free the returned item yourself.
 */
void* WMReplaceInArray(WMArray *array, int index, void *item);

#define WMSetInArray(array, index, item) WMReplaceInArray(array, index, item)

/* delete and remove will remove the elements and cause the elements
 * after them to decrement their indexes by 1. Also will call the
 * destructor on the deleted element if there's one available.
 */
int WMDeleteFromArray(WMArray *array, int index);

int WMRemoveFromArray(WMArray *array, void *item);

void* WMGetFromArray(WMArray *array, int index);

#define WMGetFirstInArray(array, item) WMFindInArray(array, NULL, item)

/* pop will return the last element from the array, also removing it
 * from the array. The destructor is NOT called, even if available.
 * Free the returned element if needed by yourself
 */
void* WMPopFromArray(WMArray *array);

int WMFindInArray(WMArray *array, WMMatchDataProc *match, void *cdata);

int WMCountInArray(WMArray *array, void *item);

/* comparer must return:
 * < 0 if a < b
 * > 0 if a > b
 * = 0 if a = b
 */
void WMSortArray(WMArray *array, WMCompareDataProc *comparer);

void WMMapArray(WMArray *array, void (*function)(void*, void*), void *data);

WMArray* WMGetSubarrayWithRange(WMArray* array, WMRange aRange);


/*..........................................................................*/

/*
 * Tree bags use a red-black tree for storage.
 * Item indexes may be any integer number.
 * 
 * Pros:
 * O(lg n) insertion/deletion/search
 * Good for large numbers of elements with sparse indexes
 * 
 * Cons:
 * O(lg n) insertion/deletion/search
 * Slow for storing small numbers of elements
 */

#define WMCreateBag(size) WMCreateTreeBag()

#define WMCreateBagWithDestructor(size, d) WMCreateTreeBagWithDestructor(d)

WMBag* WMCreateTreeBag(void);

WMBag* WMCreateTreeBagWithDestructor(WMFreeDataProc *destructor);

int WMGetBagItemCount(WMBag *bag);

void WMAppendBag(WMBag *bag, WMBag *other);

void WMPutInBag(WMBag *bag, void *item);

/* insert will increment the index of elements after it by 1 */
void WMInsertInBag(WMBag *bag, int index, void *item);

/* this is slow */
/* erase will remove the element from the bag,
 * but will keep the index of the other elements unchanged */
int WMEraseFromBag(WMBag *bag, int index);

/* delete and remove will remove the elements and cause the elements
 * after them to decrement their indexes by 1 */
int WMDeleteFromBag(WMBag *bag, int index);

int WMRemoveFromBag(WMBag *bag, void *item);

void* WMGetFromBag(WMBag *bag, int index);

void* WMReplaceInBag(WMBag *bag, int index, void *item);

#define WMSetInBag(bag, index, item) WMReplaceInBag(bag, index, item)

/* comparer must return:
 * < 0 if a < b
 * > 0 if a > b
 * = 0 if a = b
 */
void WMSortBag(WMBag *bag, WMCompareDataProc *comparer);

void WMEmptyBag(WMBag *bag);

void WMFreeBag(WMBag *bag);

void WMMapBag(WMBag *bag, void (*function)(void*, void*), void *data);

int WMGetFirstInBag(WMBag *bag, void *item);

int WMCountInBag(WMBag *bag, void *item);

int WMFindInBag(WMBag *bag, WMMatchDataProc *match, void *cdata);

void* WMBagFirst(WMBag *bag, WMBagIterator *ptr);

void* WMBagLast(WMBag *bag, WMBagIterator *ptr);

void* WMBagNext(WMBag *bag, WMBagIterator *ptr);

void* WMBagPrevious(WMBag *bag, WMBagIterator *ptr);

void* WMBagIteratorAtIndex(WMBag *bag, int index, WMBagIterator *ptr);

int WMBagIndexForIterator(WMBag *bag, WMBagIterator ptr);


#define WM_ITERATE_BAG(bag, var, i) \
      	for (var = WMBagFirst(bag, &(i)); (i) != NULL; \
		var = WMBagNext(bag, &(i)))

#define WM_ETARETI_BAG(bag, var, i) \
      	for (var = WMBagLast(bag, &(i)); (i) != NULL; \
		var = WMBagPrevious(bag, &(i)))



/*-------------------------------------------------------------------------*/

/* WMData handling */

/* Creating/destroying data */

WMData* WMCreateDataWithCapacity(unsigned capacity);

WMData* WMCreateDataWithLength(unsigned length);

WMData* WMCreateDataWithBytes(void *bytes, unsigned length);

/* destructor is a function called to free the data when releasing the data
 * object, or NULL if no freeing of data is necesary. */
WMData* WMCreateDataWithBytesNoCopy(void *bytes, unsigned length,
                                    WMFreeDataProc *destructor);

WMData* WMCreateDataWithData(WMData *aData);

WMData* WMRetainData(WMData *aData);

void WMReleaseData(WMData *aData);

/* Adjusting capacity */

void WMSetDataCapacity(WMData *aData, unsigned capacity);

void WMSetDataLength(WMData *aData, unsigned length);

void WMIncreaseDataLengthBy(WMData *aData, unsigned extraLength);

/* Accessing data */

const void* WMDataBytes(WMData *aData);

void WMGetDataBytes(WMData *aData, void *buffer);

void WMGetDataBytesWithLength(WMData *aData, void *buffer, unsigned length);

void WMGetDataBytesWithRange(WMData *aData, void *buffer, WMRange aRange);

WMData* WMGetSubdataWithRange(WMData *aData, WMRange aRange);

/* Testing data */

Bool WMIsDataEqualToData(WMData *aData, WMData *anotherData);

unsigned WMGetDataLength(WMData *aData);

/* Adding data */

void WMAppendDataBytes(WMData *aData, void *bytes, unsigned length);

void WMAppendData(WMData *aData, WMData *anotherData);

/* Modifying data */

void WMReplaceDataBytesInRange(WMData *aData, WMRange aRange, void *bytes);

void WMResetDataBytesInRange(WMData *aData, WMRange aRange);

void WMSetData(WMData *aData, WMData *anotherData);


void WMSetDataFormat(WMData *aData, unsigned format);

unsigned WMGetDataFormat(WMData *aData);
/* Storing data */


/*--------------------------------------------------------------------------*/

/* Generic Tree and TreeNode */

WMTreeNode* WMCreateTreeNode(void *data);

WMTreeNode* WMCreateTreeNodeWithDestructor(void *data, WMFreeDataProc *destructor);

WMTreeNode* WMInsertItemInTree(WMTreeNode *parent, int index, void *item);

WMTreeNode* WMAddItemToTree(WMTreeNode *parent, void *item);

WMTreeNode* WMInsertNodeInTree(WMTreeNode *parent, int index, WMTreeNode *node);

WMTreeNode* WMAddNodeToTree(WMTreeNode *parent, WMTreeNode *node);

int WMGetTreeNodeDepth(WMTreeNode *node);

void WMDestroyTreeNode(WMTreeNode *node);


/*--------------------------------------------------------------------------*/

/* Dictionaries */
/*
WMDictionary* WMCreateDictionaryFromElements(void *key, void *value, ...);

#define WMGetDictionaryEntryForKey(dict, key) WMHashGet(dict, key)

WMArray* WMGetAllDictionaryKeys(WMDictionary *dPtr);
*/

/*--------------------------------------------------------------------------*/


WMNotification* WMCreateNotification(const char *name, void *object, void *clientData);

void WMReleaseNotification(WMNotification *notification);

WMNotification* WMRetainNotification(WMNotification *notification);

void* WMGetNotificationClientData(WMNotification *notification);

void* WMGetNotificationObject(WMNotification *notification);

const char* WMGetNotificationName(WMNotification *notification);


void WMAddNotificationObserver(WMNotificationObserverAction *observerAction, 
			       void *observer, const char *name, void *object);

void WMPostNotification(WMNotification *notification);

void WMRemoveNotificationObserver(void *observer);

void WMRemoveNotificationObserverWithName(void *observer, const char *name, 
					  void *object);

void WMPostNotificationName(const char *name, void *object, void *clientData);

WMNotificationQueue* WMGetDefaultNotificationQueue(void);

WMNotificationQueue* WMCreateNotificationQueue(void);

void WMDequeueNotificationMatching(WMNotificationQueue *queue, 
				   WMNotification *notification, 
				   unsigned mask);

void WMEnqueueNotification(WMNotificationQueue *queue,
			   WMNotification *notification,
			   WMPostingStyle postingStyle);

void WMEnqueueCoalesceNotification(WMNotificationQueue *queue, 
				   WMNotification *notification,
				   WMPostingStyle postingStyle,
				   unsigned coalesceMask);


/*......................................................................*/

WMUserDefaults* WMGetStandardUserDefaults(void);

WMUserDefaults* WMGetDefaultsFromPath(char *path);

void WMSynchronizeUserDefaults(WMUserDefaults *database);

void WMSaveUserDefaults(WMUserDefaults *database);

void WMEnableUDPeriodicSynchronization(WMUserDefaults *database, Bool enable);

/* Returns a PLArray with all keys in the user defaults database.
 * Free the returned array with PLRelease() when no longer needed,
 * but do not free the elements of the array! They're just references. */
proplist_t WMGetUDAllKeys(WMUserDefaults *database);

proplist_t WMGetUDObjectForKey(WMUserDefaults *database, char *defaultName);

void WMSetUDObjectForKey(WMUserDefaults *database, proplist_t object,
			 char *defaultName);

void WMRemoveUDObjectForKey(WMUserDefaults *database, char *defaultName);

char* WMGetUDStringForKey(WMUserDefaults *database, char *defaultName);

int WMGetUDIntegerForKey(WMUserDefaults *database, char *defaultName);

float WMGetUDFloatForKey(WMUserDefaults *database, char *defaultName);

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

extern char *WMUserDefaultsDidChangeNotification;


/*-------------------------------------------------------------------------*/

/* WMHost: host handling */

WMHost* WMGetCurrentHost();

WMHost* WMGetHostWithName(char* name);

WMHost* WMGetHostWithAddress(char* address);

WMHost* WMRetainHost(WMHost *hPtr);

void WMReleaseHost(WMHost *hPtr);

/*
 * Host cache management
 * If enabled, only one object representing each host will be created, and
 * a shared instance will be returned by all methods that return a host.
 * Enabled by default.
 */
void WMSetHostCacheEnabled(Bool flag);

Bool WMIsHostCacheEnabled();

void WMFlushHostCache();

/*
 * Compare hosts: Hosts are equal if they share at least one address
 */
Bool WMIsHostEqualToHost(WMHost* hPtr, WMHost* anotherHost);

/*
 * Host names.
 * WMGetHostName() will return first name (official) if a host has several.
 * WMGetHostNames() will return a R/O WMArray with all the names of the host.
 */
char* WMGetHostName(WMHost* hPtr);

/* The returned array is R/O. Do not modify it, and do not free it! */
WMArray* WMGetHostNames(WMHost* hPtr);

/*
 * Host addresses.
 * Addresses are represented as "Dotted Decimal" strings, e.g. "192.42.172.1"
 * WMGetHostAddress() will return an arbitrary address if a host has several.
 * WMGetHostAddresses() will return a R/O WMArray with all addresses of the host.
 */
char* WMGetHostAddress(WMHost* hPtr);

/* The returned array is R/O. Do not modify it, and do not free it! */
WMArray* WMGetHostAddresses(WMHost* hPtr);


/*-------------------------------------------------------------------------*/

/* WMConnection functions */

WMConnection* WMCreateConnectionAsServerAtAddress(char *host, char *service,
                                                  char *protocol);

WMConnection* WMCreateConnectionToAddress(char *host, char *service,
                                          char *protocol);

WMConnection* WMCreateConnectionToAddressAndNotify(char *host, char *service,
                                                   char *protocol);

void WMCloseConnection(WMConnection *cPtr);

void WMDestroyConnection(WMConnection *cPtr);

WMConnection* WMAcceptConnection(WMConnection *listener);

/* Release the returned data! */
WMData* WMGetConnectionAvailableData(WMConnection *cPtr);

int WMSendConnectionData(WMConnection *cPtr, WMData *data);

Bool WMEnqueueConnectionData(WMConnection *cPtr, WMData *data);

#define WMFlushConnection(cPtr)  WMSendConnectionData((cPtr), NULL)

void WMSetConnectionDelegate(WMConnection *cPtr, ConnectionDelegate *delegate);

/* Connection info */

char* WMGetConnectionAddress(WMConnection *cPtr);

char* WMGetConnectionService(WMConnection *cPtr);

char* WMGetConnectionProtocol(WMConnection *cPtr);

Bool WMSetConnectionNonBlocking(WMConnection *cPtr, Bool flag);

Bool WMSetConnectionCloseOnExec(WMConnection *cPtr, Bool flag);

void* WMGetConnectionClientData(WMConnection *cPtr);

void WMSetConnectionClientData(WMConnection *cPtr, void *data);

unsigned int WMGetConnectionFlags(WMConnection *cPtr);

void WMSetConnectionFlags(WMConnection *cPtr, unsigned int flags);

int WMGetConnectionSocket(WMConnection *cPtr);

WMConnectionState WMGetConnectionState(WMConnection *cPtr);

WMConnectionTimeoutState WMGetConnectionTimeoutState(WMConnection *cPtr);

WMArray* WMGetConnectionUnsentData(WMConnection *cPtr);

#define WMGetConnectionQueuedData(cPtr) WMGetConnectionUnsentData(cPtr)


/*
 * Passing timeout==0 in the SetTimeout functions below, will reset that
 * timeout to its default value.
 */

/* The default timeout inherited by all WMConnection operations, if none set */
void WMSetConnectionDefaultTimeout(unsigned int timeout);

/* Global timeout for all WMConnection objects, for opening a new connection */
void WMSetConnectionOpenTimeout(unsigned int timeout);

/* Connection specific timeout for sending out data */
void WMSetConnectionSendTimeout(WMConnection *cPtr, unsigned int timeout);


/* Global variables */

extern int WCErrorCode;


/*-------------------------------------------------------------------------*/



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
