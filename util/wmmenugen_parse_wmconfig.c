/*
 * wmmenugen - Window Maker PropList menu generator
 *
 * Wmconfig <http://www.arrishq.net/> parser functions
 *
 * Copyright (c) 2010. Tamas Tevesz <ice@extreme.hu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <ctype.h>
#if DEBUG
#include <errno.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wmmenugen.h"

typedef struct {
	char	*Name;
	char	*Exec;
	char	*Category;
	int	 Flags;
} WMConfigMenuEntry;

static Bool wmc_to_wm(WMConfigMenuEntry **wmc, WMMenuEntry **wm);
static void parse_wmconfig_line(char **label, char **key, char **value, char *line);

void parse_wmconfig(const char *file, void (*addWMMenuEntryCallback)(WMMenuEntry *aEntry))
{
	FILE *fp;
	char buf[1024];
	char *p, *lastlabel, *label, *key, *value;
	WMConfigMenuEntry *wmc;
	WMMenuEntry *wm;

	lastlabel = label = key = value = NULL;

	fp = fopen(file, "r");
	if (!fp) {
#if DEBUG
		fprintf(stderr, "Error opening file %s: %s\n", file, strerror(errno));
#endif
		return;
	}

	wmc = (WMConfigMenuEntry *)wmalloc(sizeof(WMConfigMenuEntry));
	wmc->Name = NULL;
	wmc->Exec = NULL;
	wmc->Category = NULL;
	wmc->Flags = 0;

	wm = (WMMenuEntry *)wmalloc(sizeof(WMMenuEntry));

	memset(buf, 0, sizeof(buf));

	while (fgets(buf, sizeof(buf), fp)) {

		p = buf;

		/* skip whitespaces */
		while (isspace(*p))
			p++;
		/* skip comments, empty lines */
		if (*p == '\r' || *p == '\n' || *p == '#') {
			memset(buf, 0, sizeof(buf));
			continue;
		}
		/* trim crlf */
		buf[strcspn(buf, "\r\n")] = '\0';
		if (strlen(buf) == 0)
			continue;

		parse_wmconfig_line(&label, &key, &value, p);

		if (label && strlen(label) == 0)
			continue;
		if (!lastlabel && label)
			lastlabel = wstrdup(label);

		if (strcmp(lastlabel, label) != 0) {
			if (wmc->Name && wmc->Exec && wmc->Category &&
			    wmc_to_wm(&wmc, &wm))
				(*addWMMenuEntryCallback)(wm);

			wfree(wmc->Name);
			wmc->Name = NULL;
			wfree(wmc->Exec);
			wmc->Exec = NULL;
			wfree(wmc->Category);
			wmc->Category = NULL;
			wmc->Flags = 0;
			wfree(lastlabel);
			lastlabel = wstrdup(label);
		}

		if (key && value) {
			if (strcmp(key, "name") == 0)
				wmc->Name = value;
			else if (strcmp(key, "exec") == 0)
				wmc->Exec = value;
			else if (strcmp(key, "group") == 0)
				wmc->Category = value;
			else if (strcmp(key, "restart") == 0)
				wmc->Flags |= F_RESTART;
			else if (strcmp(key, "terminal") == 0)
				wmc->Flags |= F_TERMINAL;
		}

	}

	fclose(fp);

	if (wmc_to_wm(&wmc, &wm))
		(*addWMMenuEntryCallback)(wm);

}

/* get a line allocating label, key and value as necessary */
static void parse_wmconfig_line(char **label, char **key, char **value, char *line)
{
	char *p;
	int kstart, kend;

	p = (char *)line;
	*label = *key = *value = NULL;
	kstart = kend = 0;

	/* skip whitespace */
	while (isspace(*(p + kstart)))
		kstart++;

	kend = kstart;
	/* find end of label */
	while (*(p + kend) && !isspace(*(p + kend)))
		kend++;

	/* label */
	*label = wstrndup(p + kstart, kend - kstart);
	kstart = kend + 1;

	/* skip whitespace */
	while (isspace(*(p + kstart)))
		kstart++;

	kend = kstart;
	/* find end of key */
	while (*(p + kend) && !isspace(*(p + kend)))
		kend++;

	/* key */
	*key = wstrndup(p + kstart, kend - kstart);
	kstart = kend + 1;

	/* skip until after " */
	while (*(p + kstart) && *(p + kstart) != '"')
		kstart++;
	kstart++;

	kend = kstart;
	/* skip until " */
	while (*(p + kend) && *(p + kend) != '"')
		kend++;

	/* value */
	*value = wstrndup(p + kstart, kend - kstart);
}

static Bool wmc_to_wm(WMConfigMenuEntry **wmc, WMMenuEntry **wm)
{
	if (!*wmc || !(*wmc)->Name)
		return False;

	(*wm)->Name = (*wmc)->Name;
	(*wm)->CmdLine = (*wmc)->Exec;
	(*wm)->SubMenu = (*wmc)->Category;
	(*wm)->Flags = (*wmc)->Flags;

	return True;
}
