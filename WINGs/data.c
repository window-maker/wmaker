/*
 *  WINGs WMData function library
 * 
 *  Copyright (c) 1999 Dan Pascu
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



#include <string.h>
#include "WUtil.h"


typedef struct W_Data {
    unsigned   length;      /* How many bytes we have */
    unsigned   capacity;    /* How many bytes it can hold */
    unsigned   growth;      /* How much to grow */
    void       *bytes;      /* Actual data */
    unsigned   retainCount;
    unsigned   freeData:1;  /* whether the data should be released */
} W_Data;




/* Creating and destroying data objects */

WMData*
WMCreateDataWithCapacity(unsigned capacity) /*FOLD00*/
{
    WMData *aData;

    aData = (WMData*)wmalloc(sizeof(WMData));

    if (capacity>0)
        aData->bytes = wmalloc(capacity);
    else
        aData->bytes = NULL;
    aData->capacity = capacity;
    aData->growth = capacity/2 > 0 ? capacity/2 : 1;
    aData->length = 0;
    aData->retainCount = 1;
    aData->freeData = 1;

    return aData;
}


WMData*
WMCreateDataWithLength(unsigned length) /*FOLD00*/
{
    WMData *aData;

    aData = WMCreateDataWithCapacity(length);
    if (length>0) {
        memset(aData->bytes, 0, length);
        aData->length = length;
    }

    return aData;
}


WMData*
WMCreateDataWithBytes(void *bytes, unsigned length) /*FOLD00*/
{
    WMData *aData;

    aData = WMCreateDataWithCapacity(length);
    aData->length = length;
    memcpy(aData->bytes, bytes, length);

    return aData;
}


WMData*
WMCreateDataWithBytesNoCopy(void *bytes, unsigned length) /*FOLD00*/
{
    WMData *aData;

    aData = (WMData*)wmalloc(sizeof(WMData));
    aData->length = length;
    aData->capacity = length;
    aData->growth = length/2 > 0 ? length/2 : 1;
    aData->bytes = bytes;
    aData->retainCount = 1;
    aData->freeData = 0;

    return aData;
}


WMData*
WMCreateDataWithData(WMData *aData) /*FOLD00*/
{
    if (aData->length > 0)
        return WMCreateDataWithBytes(aData->bytes, aData->length);
    else
        return WMCreateDataWithCapacity(0);
}


WMData*
WMRetainData(WMData *aData) /*FOLD00*/
{
    aData->retainCount++;
    return aData;
}


void
WMReleaseData(WMData *aData) /*FOLD00*/
{
    aData->retainCount--;
    if (aData->retainCount > 0)
        return;
    if (aData->bytes && aData->freeData)
        wfree(aData->bytes);
    wfree(aData);
}



/* Adjusting capacity */

void
WMSetDataCapacity(WMData *aData, unsigned capacity) /*FOLD00*/
{
    if (aData->capacity != capacity) {
        aData->bytes = wrealloc(aData->bytes, capacity);
        aData->capacity = capacity;
        aData->growth = capacity/2 > 0 ? capacity/2 : 1;
    }
    if (aData->length > capacity) {
        aData->length = capacity;
    }
}


void
WMSetDataLength(WMData *aData, unsigned length) /*FOLD00*/
{
    if (length > aData->capacity) {
        WMSetDataCapacity(aData, length);
    }
    if (length > aData->length) {
        memset(aData->bytes + aData->length, 0, length - aData->length);
    }
    aData->length = length;
}


void
WMIncreaseDataLengthBy(WMData *aData, unsigned extraLength) /*FOLD00*/
{
    WMSetDataLength(aData, aData->length + extraLength);
}


/* Accessing data */

const void*
WMDataBytes(WMData *aData) /*FOLD00*/
{
    return aData->bytes;
}


void
WMGetDataBytes(WMData *aData, void *buffer) /*FOLD00*/
{
    wassertr(aData->length > 0);

    memcpy(buffer, aData->bytes, aData->length);
}


void
WMGetDataBytesWithLength(WMData *aData, void *buffer, unsigned length) /*FOLD00*/
{
    wassertr(aData->length > 0);
    wassertr(length <= aData->length);

    memcpy(buffer, aData->bytes, length);
}


void
WMGetDataBytesWithRange(WMData *aData, void *buffer, WMRange aRange) /*FOLD00*/
{
    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memcpy(buffer, aData->bytes + aRange.position, aRange.count);
}


WMData*
WMGetSubdataWithRange(WMData *aData, WMRange aRange) /*FOLD00*/
{
    void *buffer;

    /* return an empty subdata instead if aRange.count is 0 ? */
    wassertrv(aRange.count > 0, NULL);

    buffer = wmalloc(aRange.count);
    WMGetDataBytesWithRange(aData, buffer, aRange);
    return WMCreateDataWithBytesNoCopy(buffer, aRange.count);
}


/* Testing data */

Bool
WMIsDataEqualToData(WMData *aData, WMData *anotherData) /*FOLD00*/
{
    if (aData->length != anotherData->length)
        return False;
    else if (!aData->bytes && !anotherData->bytes) /* both are empty */
        return True;
    else if (!aData->bytes || !anotherData->bytes) /* one of them is empty */
        return False;
    return (memcmp(aData->bytes, anotherData->bytes, aData->length)==0);
}


unsigned
WMGetDataLength(WMData *aData) /*FOLD00*/
{
    return aData->length;
}


unsigned
WMGetDataHash(WMData *aData) /*FOLD00*/
{
    return aData->length;
}


/* Adding data */
void
WMAppendDataBytes(WMData *aData, void *bytes, unsigned length) /*FOLD00*/
{
    unsigned oldLength = aData->length;
    unsigned newLength = oldLength + length;

    if (newLength > aData->capacity) {
        unsigned nextCapacity = aData->capacity + aData->growth;
        unsigned nextGrowth = aData->capacity ? aData->capacity : 1;

        while (nextCapacity < newLength) {
            unsigned tmp = nextCapacity + nextGrowth;

            nextGrowth = nextCapacity;
            nextCapacity = tmp;
        }
        WMSetDataCapacity(aData, nextCapacity);
        aData->growth = nextGrowth;
    }
    memcpy(aData->bytes + oldLength, bytes, length);
    aData->length = newLength;
}


void
WMAppendData(WMData *aData, WMData *anotherData) /*FOLD00*/
{
    if (anotherData->length > 0)
        WMAppendDataBytes(aData, anotherData->bytes, anotherData->length);
}



/* Modifying data */

void
WMReplaceDataBytesInRange(WMData *aData, WMRange aRange, void *bytes) /*FOLD00*/
{
    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memcpy(aData->bytes + aRange.position, bytes, aRange.count);
}


void
WMResetDataBytesInRange(WMData *aData, WMRange aRange) /*FOLD00*/
{
    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memset(aData->bytes + aRange.position, 0, aRange.count);
}


void
WMSetData(WMData *aData, WMData *anotherData) /*FOLD00*/
{
    unsigned length = anotherData->length;

    WMSetDataCapacity(aData, length);
    if (length > 0)
        memcpy(aData->bytes, anotherData->bytes, length);
    aData->length = length;
}


/* Storing data */


