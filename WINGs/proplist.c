

#include <string.h>
#include <stdarg.h>

#include "WUtil.h"
#include "wconfig.h"



typedef enum {
    WPLString     = 0x57504c01,
    WPLData       = 0x57504c02,
    WPLArray      = 0x57504c03,
    WPLDictionary = 0x57504c04
} WPLType;


typedef struct W_PropList {
    WPLType type;

    union {
        char *string;
        WMData *data;
        WMArray *array;
        WMHashTable *dict;
    } d;

    int retainCount;
} W_PropList;




static unsigned hashPropList(WMPropList *plist);




typedef unsigned (*hashFunc)(const void*);
typedef Bool (*isEqualFunc)(const void*, const void*);
typedef void* (*retainFunc)(const void*);
typedef void (*releaseFunc)(const void*);

static const WMHashTableCallbacks WMPropListHashCallbacks = {
    (hashFunc)hashPropList,
    (isEqualFunc)WMArePropListsEqual,
    (retainFunc)NULL,
    (releaseFunc)NULL
};

static WMCompareDataProc *strCmp = (WMCompareDataProc*) strcmp;



static unsigned
hashPropList(WMPropList *plist)
{
    unsigned ret = 0;
    unsigned ctr = 0;
    const char *key;
    int i;

    switch (plist->type) {
    case WPLString:
        key = plist->d.string;
        while (*key) {
            ret ^= *key++ << ctr;
            ctr = (ctr + 1) % sizeof (char *);
        }
        return ret;

    case WPLData:
        key = WMDataBytes(plist->d.data);
        for (i=0; i<WMGetDataLength(plist->d.data); i++) {
            ret ^= key[i] << ctr;
            ctr = (ctr + 1) % sizeof (char *);
        }
        return ret;

    default:
        wwarning(_("Only string or data is supported for a proplist dictionary key"));
        wassertrv(False, 0);
        break;
    }
}


void
WMSetPropListStringComparer(WMCompareDataProc *comparer)
{
    if (!comparer)
        strCmp = (WMCompareDataProc*) strcmp;
    else
        strCmp = comparer;
}


WMPropList*
WMCreatePropListString(char *str)
{
    WMPropList *plist;

    wassertrv(str!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(W_PropList));

    plist->type = WPLString;
    plist->d.string = wstrdup(str);
    plist->retainCount = 1;

    return plist;
}


WMPropList*
WMCreatePropListDataWithBytes(unsigned char *bytes, unsigned int length)
{
    WMPropList *plist;

    wassertrv(bytes!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(W_PropList));

    plist->type = WPLData;
    plist->d.data = WMCreateDataWithBytes(bytes, length);
    plist->retainCount = 1;

    return plist;
}


WMPropList*
WMCreatePropListDataWithBytesNoCopy(unsigned char *bytes, unsigned int length,
                                    WMFreeDataProc *destructor)
{
    WMPropList *plist;

    wassertrv(length!=0 && bytes!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(W_PropList));

    plist->type = WPLData;
    plist->d.data = WMCreateDataWithBytesNoCopy(bytes, length, destructor);
    plist->retainCount = 1;

    return plist;
}


WMPropList*
WMCreatePropListDataWithData(WMData *data)
{
    WMPropList *plist;

    wassertrv(data!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(W_PropList));

    plist->type = WPLData;
    plist->d.data = WMRetainData(data);
    plist->retainCount = 1;

    return plist;
}


WMPropList*
WMCreatePropListArrayFromElements(WMPropList *elem, ...)
{
    WMPropList *plist, *nelem;
    va_list ap;

    plist = (WMPropList*)wmalloc(sizeof(W_PropList));
    plist->type = WPLArray;
    plist->d.array = WMCreateArray(4);
    plist->retainCount = 1;

    if (!elem)
        return plist;

    WMAddToArray(plist->d.array, WMRetainPropList(elem));

    va_start(ap, elem);

    while (1) {
        nelem = va_arg(ap, WMPropList*);
        if(!nelem) {
            va_end(ap);
            return plist;
        }
        WMAddToArray(plist->d.array, WMRetainPropList(nelem));
    }
}


WMPropList*
WMCreatePropListDictionaryFromEntries(WMPropList *key, WMPropList *value, ...)
{
}


WMPropList*
WMRetainPropList(WMPropList *plist)
{
    WMPropList *key, *value;
    WMHashEnumerator e;
    int i;

    plist->retainCount++;

    switch(plist->type) {
    case WPLString:
    case WPLData:
        break;
    case WPLArray:
        for (i=0; i<WMGetArrayItemCount(plist->d.array); i++) {
            WMRetainPropList(WMGetFromArray(plist->d.array, i));
        }
        break;
    case WPLDictionary:
        e = WMEnumerateHashTable(plist->d.dict);
        while (WMNextHashEnumeratorItemAndKey(&e, (void**)&value, (void**)&key)) {
            WMRetainPropList(key);
            WMRetainPropList(value);
        }
        break;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertrv(False, NULL);
    }

    return plist;
}


#if 0
void
WMReleasePropList(WMPropList *plist)
{
    WMPropList *key, *value;
    WMHashEnumerator e;
    int i;

    plist->retainCount--;

    switch(plist->type) {
    case WPLString:
        if (plist->retainCount < 1) {
            wfree(plist->d.string);
            wfree(plist);
        }
        break;
    case WPLData:
        if (plist->retainCount < 1) {
            WMReleaseData(plist->d.data);
            wfree(plist);
        }
        break;
    case WPLArray:
        for (i=0; i<WMGetArrayItemCount(plist->d.array); i++) {
            WMReleasePropList(WMGetFromArray(plist->d.array, i));
        }
        if (plist->retainCount < 1) {
            WMFreeArray(plist->d.array);
            wfree(plist);
        }
        break;
    case WPLDictionary:
        e = WMEnumerateHashTable(plist->d.dict);
        while (WMNextHashEnumeratorItemAndKey(&e, (void**)&value, (void**)&key)) {
            WMReleasePropList(key);
            WMReleasePropList(value);
        }
        if (plist->retainCount < 1) {
            WMFreeHashTable(plist->d.dict);
            wfree(plist);
        }
        break;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertr(False);
    }
}
#else

void
WMReleasePropList(WMPropList *plist)
{
    WMPropList *key, *value;
    WMHashEnumerator e;
    int i;

    plist->retainCount--;

    switch(plist->type) {
    case WPLString:
    case WPLData:
        break;
    case WPLArray:
        for (i=0; i<WMGetArrayItemCount(plist->d.array); i++) {
            WMReleasePropList(WMGetFromArray(plist->d.array, i));
        }
        break;
    case WPLDictionary:
        e = WMEnumerateHashTable(plist->d.dict);
        while (WMNextHashEnumeratorItemAndKey(&e, (void**)&value, (void**)&key)) {
            WMReleasePropList(key);
            WMReleasePropList(value);
        }
        break;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertr(False);
    }

    if (plist->retainCount > 0)
        return;

    switch(plist->type) {
    case WPLString:
        wfree(plist->d.string);
        break;
    case WPLData:
        WMReleaseData(plist->d.data);
        break;
    case WPLArray:
        WMFreeArray(plist->d.array);
        break;
    case WPLDictionary:
        WMFreeHashTable(plist->d.dict);
        break;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertr(False);
    }
    wfree(plist);
}
#endif


Bool
WMPropListIsString(WMPropList *plist)
{
    return (plist->type == WPLString);
}


Bool
WMPropListIsData(WMPropList *plist)
{
    return (plist->type == WPLData);
}


Bool
WMPropListIsArray(WMPropList *plist)
{
    return (plist->type == WPLArray);
}


Bool
WMPropListIsDictionary(WMPropList *plist)
{
    return (plist->type == WPLDictionary);
}


Bool
WMPropListIsSimple(WMPropList *plist)
{
    return (plist->type==WPLString || plist->type==WPLData);
}


Bool
WMPropListIsCompound(WMPropList *plist)
{
    return (plist->type==WPLArray || plist->type==WPLDictionary);
}


Bool
WMArePropListsEqual(WMPropList *plist, WMPropList *other)
{
    WMPropList *key1, *key2, *item1, *item2;
    WMHashEnumerator enumerator;
    int n, i;

    if (plist->type != other->type)
        return False;

    switch(plist->type) {
    case WPLString:
        return ((*strCmp)(plist->d.string, other->d.string) == 0);
    case WPLData:
        return WMIsDataEqualToData(plist->d.data, other->d.data);
    case WPLArray:
        n = WMGetArrayItemCount(plist->d.array);
        if (n != WMGetArrayItemCount(other->d.array))
            return False;
        for (i=0; i<n; i++) {
            item1 = WMGetFromArray(plist->d.array, i);
            item2 = WMGetFromArray(other->d.array, i);
            if (!WMArePropListsEqual(item1, item2))
                return False;
        }
        return True;
    case WPLDictionary:
        if (WMCountHashTable(plist->d.dict) != WMCountHashTable(other->d.dict))
            return False;
        enumerator = WMEnumerateHashTable(plist->d.dict);
        while (WMNextHashEnumeratorItemAndKey(&enumerator, (void**)&item1,
                                              (void**)&key1)) {
            item2 = WMHashGet(other->d.dict, key1);
            if (!item2 || !item1 || !WMArePropListsEqual(item1, item2))
                return False;
        }
        return True;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertrv(False, False);
    }

    return False;
}





