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
    void **items;		       /* the array data */
    unsigned length;		       /* # of items in array */
    unsigned allocSize;		       /* allocated size of array */
} W_Array;


WMArray*
WMCreateArray(unsigned initialSize)
{
    Array *a;

    a = malloc(sizeof(Array));
    sassertrv(a!=NULL, NULL);

    if (initialSize == 0) {
        initialSize = INITIAL_SIZE;
    }

    a->items = malloc(sizeof(void*)*initialSize);
    sassertdo(a->items!=NULL,
              free(a);
              return NULL;
             );
    a->length = 0;
    a->allocSize = initialSize;

    return a;
}


void ArrayFree(Array *array)
{
    free(array->items);
    free(array);
}


void ArrayClear(Array *array)
{
    memset(array->items, 0, array->length*sizeof(void*));
    array->length = 0;
}


int ArraySet(Array *array, unsigned index, void *data)
{
    sassertrv(index > array->length, 0);

    if (index == array->length)
        return ArrayAppend(array, data);

    array->items[index] = data;

    return 1;
}


#if 0
void *ArrayGet(Array *array, unsigned index)
{
    return array->items[index];
}
#endif


int ArrayAppend(Array *array, void *data)
{
    if (array->length >= array->allocSize) {
        array->allocSize += RESIZE_INCREMENT;
        array->items = realloc(array->items, sizeof(void*)*array->allocSize);
        sassertrv(array->items != NULL, 0);
    }
    array->items[array->length++] = data;

    return 1;
}


int ArrayInsert(Array *array, unsigned index, void *data)
{
    sassertrv(index < array->length, 0);

    if (array->length >= array->allocSize) {
        array->allocSize += RESIZE_INCREMENT;
        array->items = realloc(array->items, sizeof(void*)*array->allocSize);
        sassertrv(array->items != NULL, 0);
    }
    if (index < array->length-1)
        memmove(array->items+index+1, array->items+index,
                sizeof(void*)*(array->length-index));
    array->items[index] = data;

    array->length++;

    return 1;
}


void ArrayDelete(Array *array, unsigned index)
{
    sassertr(index < array->length);

    memmove(array->items+index, array->items+index+1,
            sizeof(void*)*(array->length-index-1));

    array->length--;
}



void *ArrayPop(Array *array)
{
    void *d = ArrayGet(array, array->length-1);

    ArrayDelete(array, array->length-1);

    return d;
}


int ArrayIndex(Array *array, void *data)
{
    unsigned i;

    for (i = 0; i < array->length; i++)
        if (array->items[i] == data)
            return i;

    return -1;
}
