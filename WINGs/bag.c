

#include <stdlib.h>
#include <string.h>

#include "WUtil.h"



struct W_Bag {
    int size;
    int count;
    
    void **items;
};




WMBag*
WMCreateBag(int size)
{
    WMBag *bag;

    wassertrv(size > 0, NULL);

    bag = wmalloc(sizeof(WMBag));

    bag->items = wmalloc(sizeof(void*) * size);
    bag->size = size;
    bag->count = 0;
    
    return bag;
}



int
WMGetBagItemCount(WMBag *bag)
{
    return bag->count;
}



void
WMAppendBag(WMBag *bag, WMBag *appendedBag)
{
    bag->items = wrealloc(bag->items, 
			  sizeof(void*) * (bag->size+appendedBag->count));

    memcpy(bag->items + bag->count, appendedBag->items, appendedBag->count);

    bag->count += appendedBag->count;
}



void
WMPutInBag(WMBag *bag, void *item)
{
    WMInsertInBag(bag, bag->count, item);
}



void
WMInsertInBag(WMBag *bag, int index, void *item)
{
    if (bag->count == bag->size) {
	bag->size += 16;
	bag->items = wrealloc(bag->items, sizeof(void*) * bag->size);
    }

    bag->items[bag->count++] = item;
}


int
WMGetFirstInBag(WMBag *bag, void *item)
{
    int i;
    
    for (i = 0; i < bag->count; i++) {
	if (bag->items[i] == item) {
	    return i;
	}
    }
    return -1;
}


int
WMGetLastInBag(WMBag *bag, void *item)
{
    int i;
    
    for (i = bag->count-1; i>= 0; i--) {
	if (bag->items[i] == item) {
	    return i;
	}
    }
    return -1;
}


void
WMRemoveFromBag(WMBag *bag, void *item)
{
    int i;

    i = WMGetFirstInBag(bag, item);
    if (i >= 0) {
	WMDeleteFromBag(bag, i);
    }
}



void
WMDeleteFromBag(WMBag *bag, int index)
{
    if (index < 0 || index >= bag->count)
	return;
    
    if (index < bag->count-1) {
	memmove(&bag->items[index], &bag->items[index + 1],
		(bag->count - index - 1) * sizeof(void*));
    }
    bag->count--;
}


void*
WMGetFromBag(WMBag *bag, int index)
{
    if (index < 0 || index >= bag->count) {
	return NULL;
    }

    return bag->items[index];
}


int
WMCountInBag(WMBag *bag, void *item)
{
    int i, j;
    
    for (j = 0, i = 0; j < bag->count; j++) {
	if (bag->items[j] == item)
	    i++;
    }
    return i;
}



void
WMSortBag(WMBag *bag, int (*comparer)(void*, void*))
{
    qsort(bag->items, bag->count, sizeof(void*), comparer);
}



void 
WMFreeBag(WMBag *bag)
{
    free(bag->items);
    free(bag);
}


void
WMEmptyBag(WMBag *bag)
{
    bag->count = 0;
}



WMBag*
WMMapBag(WMBag *bag, void *(*function)(void *))
{
    int i;
    WMBag *new = WMCreateBag(bag->size);
    
    for (i = 0; i < bag->count; i++) {
	WMPutInBag(new, (*function)(bag->items[i]));
    }
    
    return new;
}

