



#include <stdlib.h>
#include <string.h>

#include "WUtil.h"

#if 0

typedef struct W_ArrayBag {
    void **items;
    int size;
    int count;

    int base;
    int first;
    int last;
} W_ArrayBag;


static int   getItemCount(WMBag *bag);
static int   appendBag(WMBag *bag, WMBag *appendedBag);
static int   putInBag(WMBag *bag, void *item);
static int   insertInBag(WMBag *bag, int index, void *item);
static int   removeFromBag(WMBag *bag, void *item);
static int   deleteFromBag(WMBag *bag, int index);
static void* getFromBag(WMBag *bag, int index);
static int   firstInBag(WMBag *bag, void *item);
static int   countInBag(WMBag *bag, void *item);
static void* replaceInBag(WMBag *bag, int index, void *item);
static int   sortBag(WMBag *bag, int (*comparer)(const void*, const void*));
static void  emptyBag(WMBag *bag);
static void  freeBag(WMBag *bag);
static void  mapBag(WMBag *bag, void (*function)(void*, void*), void *data);
static int   findInBag(WMBag *bag, int (*match)(void*));
static void* first(WMBag *bag, void **ptr);
static void* last(WMBag *bag, void **ptr);
static void* next(WMBag *bag, void **ptr);
static void* previous(WMBag *bag, void **ptr);
static void* iteratorAtIndex(WMBag *bag, int index, WMBagIterator *ptr);
static int   indexForIterator(WMBag *bag, WMBagIterator ptr);


static W_BagFunctions arrayFunctions = {
    getItemCount,
    appendBag,
    putInBag,
    insertInBag,
    removeFromBag,
    deleteFromBag,
    deleteFromBag,
    getFromBag,
    firstInBag,
    countInBag,
    replaceInBag,
    sortBag,
    emptyBag,
    freeBag,
    mapBag,
    findInBag,
    first,
    last,
    next,
    previous,
    iteratorAtIndex,
    indexForIterator
};


#define ARRAY ((W_ArrayBag*)(bag->data))


#define I2O(a, i) ((a)->base + i)


WMBag*
WMCreateArrayBagWithDestructor(int initialSize, void (*destructor)(void*))
{
    WMBag *bag;
    W_ArrayBag *array;
    int size;

    assert(0&&"array bag is not working");
    bag = wmalloc(sizeof(WMBag));
    
    array = wmalloc(sizeof(W_ArrayBag));
    
    bag->data = array;

    array->items = wmalloc(sizeof(void*) * size);
    array->size = size;
    array->count = 0;
    array->first = 0;
    array->last = 0;
    array->base = 0;

    bag->func = arrayFunctions;

    bag->destructor = destructor;

    return bag;
}


WMBag*
WMCreateArrayBag(int initialSize)
{
    return WMCreateArrayBagWithDestructor(initialSize, NULL);
}


static int
getItemCount(WMBag *bag)
{
    return ARRAY->count;
}


static int
appendBag(WMBag *bag, WMBag *appendedBag)
{
    W_ArrayBag *array = (W_ArrayBag*)appendedBag->data;
    int ok;
    int i;

    for (i = array->first; i <= array->last; i++) {
	if (array->items[array->base+i]) {
	    ok = putInBag(bag, array->items[array->base+i]);
	    if (!ok)
		return 0;
	}
    }

    return 1;
}



static int
putInBag(WMBag *bag, void *item)
{
    return insertInBag(bag, ARRAY->last+1, item);
}



static int
insertInBag(WMBag *bag, int index, void *item)
{
    W_ArrayBag *array = ARRAY;
    
    if (I2O(array, index) >= array->size) {
	array->size = WMAX(array->size + 16, I2O(array, index));
	array->items = wrealloc(array->items, sizeof(void*) * array->size);
	memset(array->items + I2O(array, array->last), 0,
	       sizeof(void*) * (array->size - I2O(array, array->last)));
    }
    
    if (index > array->last) {
	array->last = index;
    } else if (index >= array->first) {
	memmove(array->items + I2O(array, index),
		array->items + (I2O(array, index) + 1), sizeof(void*));
	array->last++;
    } else {
	memmove(array->items,
		array->items + (abs(index) - array->base),
		sizeof(void*) * (abs(index) - array->base));
	memset(array->items, 0, sizeof(void*) * (abs(index) - array->base));
	array->first = index;
	array->base = abs(index);
    }
    
    array->items[array->base + index] = item;
    array->count++;
    
    return 1;
}


static int
removeFromBag(WMBag *bag, void *item)
{
    int i;
    W_ArrayBag *array = ARRAY;

    for (i = 0; i < array->count; i++) {
	if (array->items[i] == item) {
	    if (bag->destructor)
		bag->destructor(array->items[i]);
	    
	    memmove(&array->items[i], &array->items[i + 1],
		    (array->count - i - 1) * sizeof(void*));
	    array->count--;

	    return 1;
	}
    }
    
    return 0;
}



static int
deleteFromBag(WMBag *bag, int index)
{
    W_ArrayBag *array = ARRAY;
    
    if (index < 0 || index >= array->count)
	return 0;
    
    if (index < array->count-1) {
	if (bag->destructor)
	    bag->destructor(array->items[index]);

	memmove(&array->items[index], &array->items[index + 1],
		(array->count - index - 1) * sizeof(void*));
    }
    array->count--;
    
    return 1;
}


static void*
getFromBag(WMBag *bag, int index)
{
    if (index < 0 || index >= ARRAY->count)
	return NULL;

    return ARRAY->items[index];
}


static int
firstInBag(WMBag *bag, void *item)
{
    int j;
    int count = ARRAY->count;
    W_ArrayBag *array = ARRAY;
    
    for (j = 0; j < count; j++) {
	if (array->items[j] == item)
	    return j;
    }
    return -1;
}




static int
countInBag(WMBag *bag, void *item)
{
    int i, j;
    int count = ARRAY->count;
    W_ArrayBag *array = ARRAY;
    
    for (j = 0, i = 0; j < count; j++) {
	if (array->items[j] == item)
	    i++;
    }
    return i;
}


static void*
replaceInBag(WMBag *bag, int index, void *item)
{
    void *old;
    
    if (index < 0 || index >= ARRAY->count)
	return NULL;
    
    old = ARRAY->items[index];

    ARRAY->items[index] = item;

    return old;
}


static int
sortBag(WMBag *bag, int (*comparer)(const void*, const void*))
{
    qsort(ARRAY->items, ARRAY->count, sizeof(void*), comparer);
    return 1;
}



static void
emptyBag(WMBag *bag)
{
    W_ArrayBag *array = ARRAY;
    
    if (bag->destructor) {
	while (--array->count) {
	    bag->destructor(array->items[array->count]);
	}
    }
    ARRAY->count=0;
}


static void
freeBag(WMBag *bag)
{
    emptyBag(bag);

    wfree(ARRAY->items);
    wfree(ARRAY);
    wfree(bag);
}


static void
mapBag(WMBag *bag, void (*function)(void *, void *), void *clientData)
{
    int i;
    
    for (i = 0; i < ARRAY->last; i++) {
	if (ARRAY->items[i])
	    (*function)(ARRAY->items[i], clientData);
    }
}


static int 
findInBag(WMBag *bag, int (*match)(void*))
{
    int i;
    
    for (i = 0; i < ARRAY->last; i++) {
	if ((*match)(ARRAY->items[i]))
	    return i;
    }
    
    return -1;
}


static void*
first(WMBag *bag, void **ptr)
{
    *ptr = NULL;

    if (ARRAY->count == 0)
	return NULL;
    
    *(int**)ptr = 0;
    return ARRAY->items[0];
}


static void*
last(WMBag *bag, void **ptr)
{
    *ptr = NULL;
    
    if (ARRAY->count == 0)
	return NULL;

    *(int*)ptr = ARRAY->count-1;

    return ARRAY->items[ARRAY->count-1];
}


static void*
next(WMBag *bag, void **ptr)
{
    if (!*ptr)
	return NULL;
    
    (*(int*)ptr)++;
    if (*(int*)ptr >= ARRAY->count) {
	*ptr = NULL;
	return NULL;
    }
    
    return ARRAY->items[*(int*)ptr];
}


static void*
previous(WMBag *bag, void **ptr)
{
    if (!*ptr)
	return NULL;
    
    (*(int*)ptr)--;
    if (*(int*)ptr < 0) {
	*ptr = NULL;
	return NULL;
    }
    
    return ARRAY->items[*(int*)ptr];
}



static void* 
iteratorAtIndex(WMBag *bag, int index, WMBagIterator *ptr)
{
}


static int
indexForIterator(WMBag *bag, WMBagIterator ptr)
{
    
}

#endif
