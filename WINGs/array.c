/*
 * Dynamically Resized Array
 *
 * Authors: Alfredo K. Kojima <kojima@windowmaker.org>
 *          Dan Pascu         <dan@windowmaker.org>
 *
 * This code is released to the Public Domain, but
 * proper credit is always appreciated :)
 */


#include <stdlib.h>
#include <string.h>

#include "WUtil.h"


#define INITIAL_SIZE 8
#define RESIZE_INCREMENT 8


typedef struct {
    void **items;		      /* the array data */
    unsigned int length;	      /* # of items in array */
    unsigned int allocSize;	      /* allocated size of array */
    void (*destructor)(void *item);   /* the destructor to free elements */
} W_Array;


WMArray*
WMCreateArray(unsigned initialSize)
{
    WMCreateArrayWithDestructor(initialSize, NULL);
}


WMArray*
WMCreateArrayWithDestructor(unsigned initialSize, void (*destructor)(void*))
{
    WMArray *array;

    array = wmalloc(sizeof(WMArray));

    if (initialSize == 0) {
        initialSize = INITIAL_SIZE;
    }

    array->items = wmalloc(sizeof(void*) * initialSize);

    array->length = 0;
    array->allocSize = initialSize;
    array->destructor = destructor;

    return array;
}


void
WMEmptyArray(WMArray *array)
{
    if (array->destructor) {
        while (array->length-- > 0) {
            array->destructor(array->items[array->length]);
        }
    }
    /*memset(array->items, 0, array->length * sizeof(void*));*/
    array->length = 0;
}


void
WMFreeArray(WMArray *array)
{
    WMEmptyArray(array);
    free(array->items);
    free(array);
}


void*
WMReplaceInArray(WMArray *array, unsigned int index, void *data)
{
    void *old;

    wassertrv(index > array->length, 0);

    if (index == array->length) {
        WMArrayAppend(array, data);
        return NULL;
    }

    old = array->items[index];
    array->items[index] = data;

    return old;
}


void*
WMGetArrayElement(WMArray *array, unsigned int index)
{
    if (index >= array->length)
        return NULL;

    return array->items[index];
}


#if 0
int
WMAppendToArray(WMArray *array, void *data)
{
    if (array->length >= array->allocSize) {
        array->allocSize += RESIZE_INCREMENT;
        array->items = wrealloc(array->items, sizeof(void*)*array->allocSize);
    }
    array->items[array->length++] = data;

    return 1;
}
#endif


int
WMInsertInArray(WMArray *array, unsigned index, void *data)
{
    wassertrv(index <= array->length, 0);

    if (array->length >= array->allocSize) {
        array->allocSize += RESIZE_INCREMENT;
        array->items = wrealloc(array->items, sizeof(void*)*array->allocSize);
    }
    if (index < array->length)
        memmove(array->items+index+1, array->items+index,
                sizeof(void*)*(array->length-index));
    array->items[index] = data;

    array->length++;

    return 1;
}


int
WMAppendToArray(WMArray *array, void *data)
{
    return WMInsertInArray(array, array->length, data);
}


static void
removeFromArray(WMArray *array, unsigned index)
{
    /*wassertr(index < array->length);*/

    memmove(array->items+index, array->items+index+1,
            sizeof(void*)*(array->length-index-1));

    array->length--;
}


void
WMDeleteFromArray(WMArray *array, unsigned index)
{
    wassertr(index < array->length);

    if (array->destructor) {
        array->destructor(array->items[index]);
    }

    removeFromArray(array, index);
}


void*
WMArrayPop(WMArray *array)
{
    void *last = array->items[length-1];

    removeFromArray(array, array->length-1);

    return last;
}


int
WMIndexForArrayElement(WMArray *array, void *data)
{
    unsigned i;

    for (i = 0; i < array->length; i++)
        if (array->items[i] == data)
            return i;

    return -1;
}


