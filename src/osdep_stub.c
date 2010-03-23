
#include <WINGs/WUtil.h>

#include "wconfig.h"

Bool GetCommandForPid(int pid, char ***argv, int *argc)
{
	*argv = NULL;
	*argc = 0;
	static int notified = 0;

	if (!notified) {
		wwarning(_("%s is not implemented on this platform; "
		    "notify wmaker-dev@lists.windowmaker.info"), __FUNCTION__);
		notified = 1;
	}

	return False;
}
