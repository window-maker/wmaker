/*
 *  WINGs WMData function library
 * 
 *  Copyright (c) 1999-2000 Dan Pascu
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
    WMFreeDataProc *destructor;
    int	       format;      /* 0, 8, 16 or 32 */
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
    aData->format = 0;
    aData->destructor = wfree;

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
WMCreateDataWithBytesNoCopy(void *bytes, unsigned length, /*FOLD00*/
                            WMFreeDataProc *destructor)
{
    WMData *aData;

    aData = (WMData*)wmalloc(sizeof(WMData));
    aData->length = length;
    aData->capacity = length;
    aData->growth = length/2 > 0 ? length/2 : 1;
    aData->bytes = bytes;
    aData->retainCount = 1;
    aData->format = 0;
    aData->destructor = destructor;

    return aData;
}


WMData*
WMCreateDataWithData(WMData *aData) /*FOLD00*/
{
    WMData *newData;

    if (aData->length > 0) {
        newData = WMCreateDataWithBytes(aData->bytes, aData->length);
    } else {
        newData = WMCreateDataWithCapacity(0);
    }
    newData->format = aData->format;

    return newData;
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
    if (aData->bytes!=NULL && aData->destructor!=NULL) {
        aData->destructor(aData->bytes);
    }
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
	unsigned char	*dataBytes = (unsigned char *)aData->bytes;

        memset(dataBytes + aData->length, 0, length - aData->length);
    }
    aData->length = length;
}


void
WMSetDataFormat(WMData *aData, unsigned format)
{
    aData->format = format;
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


unsigned
WMGetDataFormat(WMData *aData)
{
    return aData->format;
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
    unsigned char *dataBytes = (unsigned char *)aData->bytes;

    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memcpy(buffer,dataBytes + aRange.position, aRange.count);
}


WMData*
WMGetSubdataWithRange(WMData *aData, WMRange aRange) /*FOLD00*/
{
    void *buffer;
    WMData *newData;

    /* return an empty subdata instead if aRange.count is 0 ? */
    wassertrv(aRange.count > 0, NULL);

    buffer = wmalloc(aRange.count);
    WMGetDataBytesWithRange(aData, buffer, aRange);
    newData = WMCreateDataWithBytesNoCopy(buffer, aRange.count, wfree);
    newData->format = aData->format;
    
    return newData;
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


/* Adding data */
void
WMAppendDataBytes(WMData *aData, void *bytes, unsigned length) /*FOLD00*/
{
    unsigned oldLength = aData->length;
    unsigned newLength = oldLength + length;
    unsigned char *dataBytes = (unsigned char *)aData->bytes;

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
    memcpy(dataBytes + oldLength, bytes, length);
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
    unsigned char *dataBytes = (unsigned char *)aData->bytes;

    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memcpy(dataBytes + aRange.position, bytes, aRange.count);
}


void
WMResetDataBytesInRange(WMData *aData, WMRange aRange) /*FOLD00*/
{
    unsigned char *dataBytes = (unsigned char *)aData->bytes;

    wassertr(aRange.position < aData->length);
    wassertr(aRange.count <= aData->length-aRange.position);

    memset(dataBytes + aRange.position, 0, aRange.count);
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


