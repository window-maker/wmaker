
#include <sys/utsname.h>

#include <WINGs/WUtil.h>

#include "wconfig.h"

Bool GetCommandForPid(int pid, char ***argv, int *argc)
{
	static int notified = 0;

	if (!notified) {
		struct utsname un;

		if (uname(&un) != -1) {
			wwarning(_("%s is not implemented on this platform; "
				"tell wmaker-dev@windowmaker.org you are running "
				"%s release %s version %s"), __FUNCTION__,
				un.sysname, un.release, un.version);
			notified = 1;
		}

	}

	*argv = NULL;
	*argc = 0;

	return False;
}
