
#include <WINGs/WUtil.h>

#include "wconfig.h"

#ifndef STUB_HINT
#define STUB_HINT	"(unknown)"
#endif

Bool GetCommandForPid(int pid, char ***argv, int *argc)
{
	*argv = NULL;
	*argc = 0;
	static int notified = 0;

	if (!notified) {
		wwarning(_("%s is not implemented on this platform; "
		    "tell wmaker-dev@lists.windowmaker.info you are running \"%s\""),
		    __FUNCTION__, STUB_HINT);
		notified = 1;
	}

	return False;
}
