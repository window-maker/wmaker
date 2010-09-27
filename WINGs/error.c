/*
 *  Window Maker miscelaneous function library
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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

#include "wconfig.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <WUtil.h>

extern char *_WINGS_progname;

void __wmessage(int type, const char *msg, ...)
{
	va_list args;
	char *buf;
	static int linemax = 0;

	if (linemax == 0) {
#ifdef HAVE_SYSCONF
		linemax = sysconf(_SC_LINE_MAX);
		if (linemax == -1) {
			/* I'd like to know of this ever fires */
			fprintf(stderr, "%s %d: sysconf(_SC_LINE_MAX) returned error\n",
				__FILE__, __LINE__);
			linemax = 512;
		}
#else /* !HAVE_SYSCONF */
		fprintf(stderr, "%s %d: Your system does not have sysconf(3); "
			"let wmaker-dev@windowmaker.org know.\n", __FILE__, __LINE__);
		linemax = 512;
#endif /* HAVE_SYSCONF */
	}

	buf = wmalloc(linemax);

	fflush(stdout);
	/* message format: <wings_progname>: <qualifier>: <message>"\n" */
	snprintf(buf, linemax, "%s: ", _WINGS_progname ? _WINGS_progname : "WINGs");
	switch (type) {
		case WMESSAGE_TYPE_FATAL:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1, _("fatal error: "));
		break;
		case WMESSAGE_TYPE_ERROR:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1, _("error: "));
		break;
		case WMESSAGE_TYPE_WARNING:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1, _("warning: "));
		break;
		break;
		case WMESSAGE_TYPE_MESSAGE:
			/* FALLTHROUGH */
		default:	/* should not happen, but doesn't hurt either */
			strncat(buf, ": ", linemax - strlen(buf));
		break;
	}

	va_start(args, msg);
	vsnprintf(buf + strlen(buf), linemax - strlen(buf) - 1, msg, args);
	va_end(args);

	strncat(buf, "\n", linemax - strlen(buf));
	fputs(buf, stderr);

	wfree(buf);
}

