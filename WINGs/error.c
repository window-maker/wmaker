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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <WUtil.h>

extern char *_WINGS_progname;

void __wmessage(int type, void *extra, const char *msg, ...)
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
	/* message format: <wings_progname>: <qualifier>: <message>[: <extra info>]"\n" */
	snprintf(buf, linemax, "%s: ", _WINGS_progname ? _WINGS_progname : "WINGs");
	va_start(args, msg);
	switch (type) {
		case WMESSAGE_TYPE_WARNING:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				_("warning: "));
			vsnprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				msg, args);
		break;
		case WMESSAGE_TYPE_FATAL:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				_("fatal error: "));
			vsnprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				msg, args);
		break;
		case WMESSAGE_TYPE_WSYSERROR:
		case WMESSAGE_TYPE_WSYSERRORWITHCODE:
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				_("error: "));
			vsnprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				msg, args);
			snprintf(buf + strlen(buf), linemax - strlen(buf) - 1,
				": %s", type == WMESSAGE_TYPE_WSYSERROR ?
				    strerror(errno) : strerror(*(int *)extra));
		break;
		case WMESSAGE_TYPE_MESSAGE:
			/* FALLTHROUGH */
		default:
			strncat(buf, ": ", linemax - strlen(buf));
			vsnprintf(buf + strlen(buf), linemax - strlen(buf) - 1, msg, args);
		break;
	}

	va_end(args);

	strncat(buf, "\n", linemax - strlen(buf));
	fputs(buf, stderr);

	wfree(buf);
}

