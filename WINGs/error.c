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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#include "wconfig.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <WUtil.h>

extern char *_WINGS_progname;

void __wmessage(const char *func, const char *file, int line, int type, const char *msg, ...)
{
	va_list args;
	char *buf;
	static int linemax = 0;
	int truncated = 0;

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

	/* message format: <wings_progname>(function(file:line): <type?>: <message>"\n" */
	strncat(buf, _WINGS_progname ? _WINGS_progname : "WINGs", linemax - 1);
	snprintf(buf + strlen(buf), linemax - strlen(buf), "(%s(%s:%d))", func, file, line);
	strncat(buf, ": ", linemax - 1 - strlen(buf));

	switch (type) {
		case WMESSAGE_TYPE_FATAL:
			strncat(buf, _("fatal error: "), linemax - 1 - strlen(buf));
		break;
		case WMESSAGE_TYPE_ERROR:
			strncat(buf, _("error: "), linemax - 1 - strlen(buf));
		break;
		case WMESSAGE_TYPE_WARNING:
			strncat(buf, _("warning: "), linemax - 1 - strlen(buf));
		break;
		case WMESSAGE_TYPE_MESSAGE:
			/* FALLTHROUGH */
		default:	/* should not happen, but doesn't hurt either */
		break;
	}

	va_start(args, msg);
	if (vsnprintf(buf + strlen(buf), linemax - strlen(buf), msg, args) >= linemax - strlen(buf))
		truncated = 1;

	va_end(args);

	fputs(buf, stderr);

	if (truncated)
		fputs("*** message truncated ***", stderr);

	fputs("\n", stderr);

	wfree(buf);
}

