
/* Miscelaneous helper functions */

#include "WINGsP.h"


WMRange
wmkrange(int start, int count)
{
    WMRange range;

    range.position = start;
    range.count = count;

    return range;
}


