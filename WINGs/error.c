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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <WUtil.h>

extern char *_WINGS_progname;

#define MAXLINE	1024

void __wmessage(int type, void *extra, const char *msg, ...)
{
	va_list args;
	char buf[MAXLINE];

	fflush(stdout);
	/* message format: <wings_progname>: <qualifier>: <message>[: <extra info>]"\n" */
	snprintf(buf, sizeof(buf), "%s: ", _WINGS_progname ? _WINGS_progname : "WINGs");
	va_start(args, msg);
	switch (type) {
		case WMESSAGE_TYPE_WARNING:
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				_("warning: "));
			vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				msg, args);
		break;
		case WMESSAGE_TYPE_FATAL:
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				_("fatal error: "));
			vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				msg, args);
		break;
		case WMESSAGE_TYPE_WSYSERROR:
		case WMESSAGE_TYPE_WSYSERRORWITHCODE:
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				_("error: "));
			vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				msg, args);
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				": %s", type == WMESSAGE_TYPE_WSYSERROR ?
				    strerror(errno) : strerror(*(int *)extra));
		break;
		case WMESSAGE_TYPE_MESSAGE:
			/* FALLTHROUGH */
		default:
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ": ");
			vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), msg, args);
		break;
	}
	va_end(args);
	strncat(buf + strlen(buf), "\n", sizeof(buf) - strlen(buf) - 1);
	fputs(buf, stderr);
}

