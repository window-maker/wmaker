/*
 *  Window Maker window manager
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "wconfig.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <WINGs/WUtil.h>

#define MAXLINE  1024


char *getLine(void * file, const char *file_name)
{
	char linebuf[MAXLINE];
	char *line = NULL, *result = NULL;
	size_t len;
	int done;

again:
	done = 0;
	while (!done && fgets(linebuf, sizeof(linebuf), file) != NULL) {
		line = wtrimspace(linebuf);
		len = strlen(line);

		/* allow line wrapping */
		if (len > 0 && line[len - 1] == '\\') {
			line[len - 1] = '\0';
		} else {
			done = 1;
		}

		if (result == NULL) {
			result = line;
		} else {
			if (strlen(result) < MAXLINE) {
				result = wstrappend(result, line);
			}
			wfree(line);
		}
	}
	if (!done || ferror(file)) {
		wfree(result);
		result = NULL;
	} else if (result != NULL && (result[0] == 0 || result[0] == '#' ||
		   (result[0] == '/' && result[1] == '/'))) {
		wfree(result);
		result = NULL;
		goto again;
	} else if (result != NULL && strlen(result) >= MAXLINE) {
		wwarning(_("%s:maximal line size exceeded in menu config: %s"),
			 file_name, line);
		wfree(result);
		result = NULL;
		goto again;
	}

	return result;
}

void separateline(char *line, char **title, char **command, char **parameter, char **shortcut)
{
	char *suffix, *next = line;

	*title = NULL;
	*command = NULL;
	*parameter = NULL;
	*shortcut = NULL;

	/* get the title */
	*title = wtokennext(line, &next);
	if (next == NULL)
		return;
	line = next;

	/* get the command or shortcut keyword */
	*command = wtokennext(line, &next);
	if (next == NULL)
		return;
	line = next;

	if (*command != NULL && strcmp(*command, "SHORTCUT") == 0) {
		/* get the shortcut */
		*shortcut = wtokennext(line, &next);
		if (next == NULL)
			return;
		line = next;

		/* get the command */
		*command = wtokennext(line, &next);
		if (next == NULL)
			return;
		line = next;
	}

	/* get the parameters */
	suffix = wtrimspace(line);

	/* should we keep this weird old behavior? */
	if (suffix[0] == '"') {
		*parameter = wtokennext(suffix, &next);
		wfree(suffix);
	} else {
		*parameter = suffix;
	}
}
