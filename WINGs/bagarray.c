



#include <stdlib.h>
#include <string.h>

#include "WUtil.h"



typedef struct W_ArrayBag {
    void **items;
    int size;
    int count;
} W_ArrayBag;


static int    getItemCount(WMBag *bag);
static int   appendBag(WMBag *bag, WMBag *appendedBag);
static int   putInBag(WMBag *bag, void *item);
static int   insertInBag(WMBag *bag, int index, void *item);
static int   removeFromBag(WMBag *bag, void *item);
static int   deleteFromBag(WMBag *bag, int index);
static void*  getFromBag(WMBag *bag, int index);
static int    firstInBag(WMBag *bag, void *item);
static int    countInBag(WMBag *bag, void *item);
static void*  replaceInBag(WMBag *bag, int index, void *item);
static void   sortBag(WMBag *bag, int (*comparer)(const void*, const void*));
static void   emptyBag(WMBag *bag);
static void   freeBag(WMBag *bag);
static WMBag* mapBag(WMBag *bag, void *(*function)(void *));
static int findInBag(WMBag *bag, int (*match)(void*));
static void*  first(WMBag *bag, void **ptr);
static void*  last(WMBag *bag, void **ptr);
static void*  next(WMBag *bag, void **ptr);
static void*  previous(WMBag *bag, void **ptr);


static W_BagFunctions arrayFunctions = {
    getItemCount,
    appendBag,
    putInBag,
    insertInBag,
    removeFromBag,
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
    previous
};


#define ARRAY ((W_ArrayBag*)bag->data)


WMBag*
WMCreateArrayBagWithDestructor(int size, void (*destructor)(void*))
{
    WMBag *bag;
    W_ArrayBag *array;

    wassertrv(size > 0, NULL);

    bag = wmalloc(sizeof(WMBag));
    
    array = wmalloc(sizeof(W_ArrayBag));
    
    bag->data = array;

    array->items = wmalloc(sizeof(void*) * size);
    array->size = size;
    array->count = 0;

    bag->func = arrayFunctions;

    bag->destructor = destructor;

    return bag;
}


WMBag*
WMCreateArrayBag(int size)
{
    return WMCreateArrayBagWithDestructor(size, NULL);
}


static int
getItemCount(WMBag *bag)
{
    return ARRAY->count;
}


static int
appendBag(WMBag *bag, WMBag *appendedBag)
{
    W_ArrayBag *array1 = ARRAY;
    W_ArrayBag *array2 = (W_ArrayBag*)appendedBag->data;

    if (array1->count + array2->count >= array1->size) {
	array1->items =
	    wrealloc(array1->items, sizeof(void*) * (array1->size+array2->count));
	array1->size += array2->count;
    }
    
    memcpy(array1->items + array1->count, array2->items,
           sizeof(void*) * array2->count);

    array1->count += array2->count;
    
    return 1;
}



static int
putInBag(WMBag *bag, void *item)
{
    return insertInBag(bag, ARRAY->count, item);
}



static int
insertInBag(WMBag *bag, int index, void *item)
{
    W_ArrayBag *array = ARRAY;
    
    if (array->count == array->size) {
	array->size += 16;
	array->items = wrealloc(array->items, sizeof(void*) * array->size);
    }

    if (index >= 0 && index < array->count) {
        memmove(&array->items[index+1], &array->items[index],
                (array->count - index) * sizeof(void*));
    } else {
        /* If index is invalid, place it at end */
        index = array->count;
    }
    array->items[index] = item;
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


static void
sortBag(WMBag *bag, int (*comparer)(const void*, const void*))
{
    qsort(ARRAY->items, ARRAY->count, sizeof(void*), comparer);
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


static WMBag*
mapBag(WMBag *bag, void *(*function)(void *))
{
    int i;
    void *data;
    WMBag *new = WMCreateArrayBagWithDestructor(ARRAY->size, bag->destructor);
    
    for (i = 0; i < ARRAY->count; i++) {
	data = (*function)(ARRAY->items[i]);
	if (data)
	    putInBag(new, data);
    }
    
    return new;
}


static int 
findInBag(WMBag *bag, int (*match)(void*))
{
    int i;
    
    for (i = 0; i < ARRAY->count; i++) {
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


