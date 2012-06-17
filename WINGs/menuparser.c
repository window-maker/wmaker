/*
 *  Window Maker window manager
 *
 *  Copyright (c) 2012 Christophe Curis
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
#include <ctype.h>
#include <stdarg.h>

#include <WINGs/WUtil.h>

#include "menuparser.h"

static WMenuParser menu_parser_create_new(const char *file_name, void *file);
static char *menu_parser_isolate_token(WMenuParser parser);


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

void WMenuParserError(WMenuParser parser, const char *msg, ...)
{
	char buf[MAXLINE];
	va_list args;

	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	__wmessage("WMenuParser", parser->file_name, parser->line_number, WMESSAGE_TYPE_WARNING, buf);
}

/***** Read one line from file and split content *****/
/* The function returns False when the end of file is reached */
Bool WMenuParserGetLine(WMenuParser top_parser, char **title, char **command, char **parameter, char **shortcut)
{
	WMenuParser cur_parser;
	enum { GET_TITLE, GET_COMMAND, GET_PARAMETERS, GET_SHORTCUT } scanmode;
	char *token;
	char  lineparam[MAXLINE];
	char *params = NULL;

	lineparam[0] = '\0';
	*title = NULL;
	*command = NULL;
	*parameter = NULL;
	*shortcut = NULL;
	scanmode = GET_TITLE;

	cur_parser = top_parser;

 read_next_line:
	if (fgets(cur_parser->line_buffer, sizeof(cur_parser->line_buffer), cur_parser->file_handle) == NULL) {
		return False;
	}
	cur_parser->line_number++;
	cur_parser->rd = cur_parser->line_buffer;

	for (;;) {
		if (!menu_parser_skip_spaces_and_comments(cur_parser)) {
			/* We reached the end of line */
			if (scanmode == GET_TITLE)
				goto read_next_line; // Empty line -> skip
			else
				break; // Finished reading current line -> return it to caller
		}

		/* Found a word */
		token = menu_parser_isolate_token(cur_parser);
		switch (scanmode) {
		case GET_TITLE:
			*title = token;
			scanmode = GET_COMMAND;
			break;

		case GET_COMMAND:
			if (strcmp(token, "SHORTCUT") == 0) {
				scanmode = GET_SHORTCUT;
				wfree(token);
			} else {
				*command = token;
				scanmode = GET_PARAMETERS;
			}
			break;

		case GET_SHORTCUT:
			if (*shortcut != NULL) {
				WMenuParserError(top_parser, _("multiple SHORTCUT definition not valid") );
				wfree(*shortcut);
			}
			*shortcut = token;
			scanmode = GET_COMMAND;
			break;

		case GET_PARAMETERS:
			{
				char *src;

				if (params == NULL) {
					params = lineparam;
				} else {
					if ((params - lineparam) < sizeof(lineparam)-1)
						*params++ = ' ';
				}
				src = token;
				while ((params - lineparam) < sizeof(lineparam)-1)
					if ( (*params = *src++) == '\0')
						break;
					else
						params++;
				wfree(token);
			}
			break;
		}
	}

	if (params != NULL) {
		lineparam[sizeof(lineparam) - 1] = '\0';
		*parameter = wstrdup(lineparam);
	}

	return True;
}

/* Return False when there's nothing left on the line,
   otherwise increment parser's pointer to next token */
Bool menu_parser_skip_spaces_and_comments(WMenuParser parser)
{
	for (;;) {
		while (isspace(*parser->rd))
			parser->rd++;

		if (*parser->rd == '\0')
			return False; // Found the end of current line

		else if ((parser->rd[0] == '\\') &&
					(parser->rd[1] == '\n') &&
					(parser->rd[2] == '\0')) {
			// Means that the current line is expected to be continued on next line
			if (fgets(parser->line_buffer, sizeof(parser->line_buffer), parser->file_handle) == NULL) {
				WMenuParserError(parser, _("premature end of file while expecting a new line after '\\'") );
				return False;
			}
			parser->line_number++;
			parser->rd = parser->line_buffer;

		} else if (parser->rd[0] == '/') {
			if (parser->rd[1] == '/') // Single line C comment
				return False; // Won't find anything more on this line
			if (parser->rd[1] == '*') {
				int start_line;

				start_line = parser->line_number;
				parser->rd += 2;
				for (;;) {
					/* Search end-of-comment marker */
					while (*parser->rd != '\0') {
						if (parser->rd[0] == '*')
							if (parser->rd[1] == '/')
								goto found_end_of_comment;
						parser->rd++;
					}

					/* Marker not found -> load next line */
					if (fgets(parser->line_buffer, sizeof(parser->line_buffer), parser->file_handle) == NULL) {
						WMenuParserError(parser, _("reached end of file while searching '*/' for comment started at line %d"), start_line);
						return False;
					}
					parser->line_number++;
					parser->rd = parser->line_buffer;
				}

			found_end_of_comment:
				parser->rd += 2;  // Skip closing mark
				continue; // Because there may be spaces after the comment
			}
			return True; // the '/' was not a comment, treat it as user data
		} else
			return True; // Found some data
	}
}

/* read a token (non-spaces suite of characters)
   the result os wmalloc's, so it needs to be free'd */
static char *menu_parser_isolate_token(WMenuParser parser)
{
	char *start;
	char *token;

	start = parser->rd;

	while (*parser->rd != '\0')
		if (isspace(*parser->rd))
			break;
		else if ((parser->rd[0] == '/') &&
					((parser->rd[1] == '*') || (parser->rd[1] == '/')))
			break;
		else if ((parser->rd[0] == '\\') && (parser->rd[1] == '\n'))
			break;
		else if ((*parser->rd == '"' ) || (*parser->rd == '\'')) {
			char eot = *parser->rd++;
			while ((*parser->rd != '\0') && (*parser->rd != '\n'))
				if (*parser->rd++ == eot)
					goto found_end_quote;
			WMenuParserError(parser, _("missing closing quote or double-quote before end-of-line") );
		found_end_quote:
			;
		} else
			parser->rd++;

	token = wmalloc(parser->rd - start + 1);
	strncpy(token, start, parser->rd - start);
	token[parser->rd - start] = '\0';

	return token;
}
