



#include <stdlib.h>
#include <string.h>

#include "WUtil.h"


typedef struct W_Item {
    struct W_Item *next;
    struct W_Item *prev;
    void *data;
} W_Item;


typedef struct W_ListBag {
    W_Item *first;
    W_Item *last;
    int count;
} W_ListBag;






static int getItemCount(WMBag *self);
static int appendBag(WMBag *self, WMBag *bag);
static int putInBag(WMBag *self, void *item);
static int insertInBag(WMBag *self, int index, void *item);
static int removeFromBag(WMBag *bag, void *item);
static int deleteFromBag(WMBag *bag, int index);
static void *getFromBag(WMBag *bag, int index);
static int countInBag(WMBag *bag, void *item);
static int firstInBag(WMBag *bag, void *item);
static void *replaceInBag(WMBag *bag, int index, void *item);
static void sortBag(WMBag *bag, int (*comparer)(const void*, const void*));
static void emptyBag(WMBag *bag);
static void freeBag(WMBag *bag);
static WMBag *mapBag(WMBag *bag, void * (*function)(void*));
static int findInBag(WMBag *bag, int (*match)(void*));;
static void *first(WMBag *bag, void **ptr);
static void *last(WMBag *bag, void **ptr);
static void *next(WMBag *bag, void **ptr);
static void *previous(WMBag *bag, void **ptr);

static W_BagFunctions bagFunctions = {
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


#define SELF ((W_ListBag*)self->data)

WMBag *WMCreateListBag(void)
{
    return WMCreateListBagWithDestructor(NULL);
}


WMBag *WMCreateListBagWithDestructor(void (*destructor)(void*))
{
    WMBag *bag;
    
    bag = wmalloc(sizeof(WMBag));

    bag->data = wmalloc(sizeof(W_ListBag));
    memset(bag->data, 0, sizeof(W_ListBag));

    bag->destructor = destructor;

    bag->func = bagFunctions;
    
    return bag;
}


static int getItemCount(WMBag *self)
{
    return SELF->count;
}


static int appendBag(WMBag *self, WMBag *bag)
{
    void *ptr;
    void *data;
    
    for (data = first(bag, &ptr); data != NULL; data = next(bag, &ptr)) {
	if (!putInBag(self, data))
	    return 0;
    }
    
    return 1;
}


static int putInBag(WMBag *self, void *item)
{
    W_Item *ptr;
    
    ptr = wmalloc(sizeof(W_Item));
    
    ptr->next = NULL;
    ptr->prev = SELF->last;
    ptr->data = item;
    
    SELF->count++;
    
    if (SELF->last)
	SELF->last->next = ptr;
    else {
	SELF->last = ptr;
	SELF->first = ptr;
    }

    return 1;
}


static int insertInBag(WMBag *self, int index, void *item)
{
    W_Item *tmp;
    W_Item *ptr;

    if (index < 0)
	index = 0;

    if (index >= SELF->count) {
	return putInBag(self, item);
    }
    
    ptr = wmalloc(sizeof(W_Item));

    ptr->next = NULL;
    ptr->prev = NULL;
    ptr->data = item;
    
    SELF->count++;

    if (!SELF->first) {
	SELF->first = ptr;
	SELF->last = ptr;

	return 1;
    }

    tmp = SELF->first;
    while (index-- && tmp->next)
	tmp = tmp->next;
    
    if (tmp->prev) {
	tmp->prev->next = ptr;
	ptr->prev = tmp->prev;
	ptr->next = tmp;
	tmp->prev = ptr;
    } else {
	assert(SELF->first == tmp);
	
	SELF->first->prev = ptr;
	ptr->next = SELF->first;
		
	SELF->first = ptr;
    }

    return 1;
}



static int removeFromBag(WMBag *self, void *item)
{
    W_Item *ptr = SELF->first;
    
    while (ptr) {
	
	if (ptr->data == item) {
	    
	    SELF->count--;
	    
	    if (self->destructor) {
		self->destructor(item);
	    }
	    if (ptr->next)
		ptr->next->prev = ptr->prev;
	    if (ptr->prev)
		ptr->prev->next = ptr->next;

	    if (SELF->first == ptr)
		SELF->first = ptr->next;
	    if (SELF->last == ptr)
		SELF->last = ptr->prev;
		
	    free(ptr);
	    return 1;
	}
	
	ptr = ptr->next;
    }
    
    return -1;
}


static int deleteFromBag(WMBag *self, int index)
{
    W_Item *ptr = SELF->first;
    
    while (ptr && index--) {
	ptr = ptr->next;
    }
    
    if (ptr && index==0) {
	SELF->count--;
	if (self->destructor) {
	    self->destructor(ptr->data);
	}
	if (ptr->next)
	    ptr->next->prev = ptr->prev;
	if (ptr->prev)
	    ptr->prev->next = ptr->next;
	
	if (SELF->first == ptr)
	    SELF->first = ptr->next;
	if (SELF->last == ptr)
	    SELF->last = ptr->prev;

	free(ptr);
	
	return 1;
    }

    return -1;
}


static void *getFromBag(WMBag *self, int index)
{
    W_Item *ptr = SELF->first;
    
    if (index < 0 || index > SELF->count)
	return NULL;
    
    while (ptr && index--)
	ptr = ptr->next;

    return ptr;
}



static int firstInBag(WMBag *self, void *item)
{
    int index = 0;
    W_Item *ptr = SELF->first;
    
    while (ptr) {
	if (ptr->data == item)
	    return index;
	index++;
	ptr = ptr->next;
    }
    
    return -1;
}



static int countInBag(WMBag *self, void *item)
{
    int count = 0;
    W_Item *ptr = SELF->first;
    
    while (ptr) {
	if (ptr->data == item)
	    count++;
	ptr = ptr->next;
    }
    
    return count;
}


static void *replaceInBag(WMBag *self, int index, void *item)
{
    void *old;
    W_Item *ptr = SELF->first;
    
    while (ptr && index--)
	ptr = ptr->next;

    if (!ptr || index!=0)
	return NULL;
    
    old = ptr->data;

    ptr->data = item;

    return old;
}



static void sortBag(WMBag *self, int (*comparer)(const void*, const void*))
{
    assert(0&&"not implemented");
}


static void emptyBag(WMBag *self)
{
    W_Item *ptr = SELF->first;
    W_Item *tmp;
    
    while (ptr) {
	tmp = ptr->next;
	
	if (self->destructor) {
	    self->destructor(ptr->data);
	}
	free(ptr);
	
	ptr = tmp;
    }
    SELF->count = 0;
}


static void freeBag(WMBag *self)
{
    emptyBag(self);
    
    free(self);
}


static WMBag *mapBag(WMBag *self, void * (*function)(void*))
{
    WMBag *bag = WMCreateListBagWithDestructor(self->destructor);
    W_Item *ptr = SELF->first;
    
    while (ptr) {
	if ((*function)(ptr->data))
	    putInBag(bag, ptr->data);
	ptr = ptr->next;
    }
    return bag;
}


static int findInBag(WMBag *self, int (*match)(void*))
{
    W_Item *ptr = SELF->first;
    int index = 0;
    
    while (ptr) {
	if ((*match)(ptr)) {
	    return index; 
	}
	index++;
	ptr = ptr->next;
    }
    
    return -1;
}




static void *first(WMBag *self, void **ptr)
{
    *ptr = SELF->first;

    if (!*ptr)
	return NULL;
    
    return SELF->first->data;
}



static void *last(WMBag *self, void **ptr)
{
    *ptr = SELF->last;
    
    if (!*ptr)
	return NULL;

    return SELF->last->data;
}



static void *next(WMBag *bag, void **ptr)
{
    W_Item *item = *(W_Item**)ptr;

    if (!item)
	return NULL;
    
    *ptr = item->next;

    return item->next->data;
}



static void *previous(WMBag *bag, void **ptr)
{
    W_Item *item = *(W_Item**)ptr;

    if (!item)
	return NULL;
    
    *ptr = item->prev;

    return item->prev->data;
}




