

#include <string.h>
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



static WMCompareDataProc *strCmp = (WMCompareDataProc*) strcmp;



void
WMSetPropListStringComparer(WMCompareDataProc *comparer)
{
    if (!comparer)
        strCmp = (WMCompareDataProc*) strcmp;
    else
        strCmp = comparer;
}


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


WMPropList*
WMCreatePropListString(char *str)
{
    WMPropList *plist;

    wassertrv(str!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(WMPropList));

    plist->type = WPLString;
    plist->d.string = wstrdup(str);
    plist->retainCount = 1;

    return plist;
}


WMPropList*
WMCreatePropListDataWithBytes(unsigned char *bytes, unsigned int length)
{
    WMPropList *plist;

    wassertrv(length!=0 && bytes!=NULL, NULL);

    plist = (WMPropList*)wmalloc(sizeof(WMPropList));

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

    plist = (WMPropList*)wmalloc(sizeof(WMPropList));

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

    plist = (WMPropList*)wmalloc(sizeof(WMPropList));

    plist->type = WPLData;
    plist->d.data = WMRetainData(data);
    plist->retainCount = 1;

    return plist;
}


Bool
WMIsPropListEqualWith(WMPropList *plist, WMPropList *other)
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
            if (!WMIsPropListEqualWith(item1, item2))
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
            if (!item2 || !item1 || !WMIsPropListEqualWith(item1, item2))
                return False;
        }
        return True;
    default:
        wwarning(_("Used proplist functions on non-WMPropLists objects"));
        wassertrv(False, False);
    }

    return False;
}



typedef unsigned (*hashFunc)(const void*);
typedef Bool (*isEqualFunc)(const void*, const void*);
typedef void* (*retainFunc)(const void*);
typedef void (*releaseFunc)(const void*);


const WMHashTableCallbacks WMPropListHashCallbacks = {
    (hashFunc)hashPropList,
    (isEqualFunc)WMIsPropListEqualWith,
    (retainFunc)NULL,
    (releaseFunc)NULL
};



