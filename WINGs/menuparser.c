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

#include "menuparser.h"

static WMenuParser menu_parser_create_new(const char *file_name, void *file);
static void separateline(char *line, char **title, char **command, char **parameter, char **shortcut);


/***** Constructor and Destructor for the Menu Parser object *****/
WMenuParser WMenuParserCreate(const char *file_name, void *file)
{
	WMenuParser parser;

	parser = menu_parser_create_new(file_name, file);
	return parser;
}

void WMenuParserDelete(WMenuParser parser)
{
	wfree(parser);
}

static WMenuParser menu_parser_create_new(const char *file_name, void *file)
{
	WMenuParser parser;

	parser = wmalloc(sizeof(*parser));
	parser->file_name = file_name;
	parser->file_handle = file;
	parser->rd = parser->line_buffer;

	return parser;
}

/***** To report helpfull messages to user *****/
const char *WMenuParserGetFilename(WMenuParser parser)
{
	return parser->file_name;
}


/***** Read one line from file and split content *****/
Bool WMenuParserGetLine(WMenuParser top_parser, char **title, char **command, char **parameter, char **shortcut)
{
	WMenuParser cur_parser;
	char *line = NULL, *result = NULL;
	size_t len;
	int done;

	*title = NULL;
	*command = NULL;
	*parameter = NULL;
	*shortcut = NULL;

	cur_parser = top_parser;

again:
	done = 0;
	while (!done && fgets(cur_parser->line_buffer, sizeof(cur_parser->line_buffer), cur_parser->file_handle) != NULL) {
		line = wtrimspace(cur_parser->line_buffer);
		len = strlen(line);
		cur_parser->line_number++;

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
	if (!done || ferror(cur_parser->file_handle)) {
		wfree(result);
		result = NULL;
	} else if (result != NULL && (result[0] == 0 || result[0] == '#' ||
		   (result[0] == '/' && result[1] == '/'))) {
		wfree(result);
		result = NULL;
		goto again;
	} else if (result != NULL && strlen(result) >= MAXLINE) {
		wwarning(_("%s:maximal line size exceeded in menu config: %s"),
			 cur_parser->file_name, line);
		wfree(result);
		result = NULL;
		goto again;
	}

	if (result == NULL)
	  return False;

	separateline(line, title, command, parameter, shortcut);
	wfree(line);
	return True;
}

static void separateline(char *line, char **title, char **command, char **parameter, char **shortcut)
{
	char *suffix, *next = line;

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
