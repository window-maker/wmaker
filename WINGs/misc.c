
/* Miscelaneous helper functions */

#include "WINGsP.h"

#include "error.h"

WMRange wmkrange(int start, int count)
{
	WMRange range;

	range.position = start;
	range.count = count;

	return range;
}

/*
 * wutil_shutdown - cleanup in WUtil when user program wants to exit
 */
void wutil_shutdown(void)
{
#ifdef HAVE_SYSLOG
	w_syslog_close();
#endif
}
