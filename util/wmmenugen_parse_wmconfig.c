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
	char	*Restart;
	int	 Flags;
} WMConfigMenuEntry;

static Bool wmc_to_wm(WMConfigMenuEntry **wmc, WMMenuEntry **wm);
static void parse_wmconfig_line(char **label, char **key, char **value, char *line);
static void init_wmconfig_storage(WMConfigMenuEntry **wmc);

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
	wmc->Restart = NULL;
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
			if (wmc_to_wm(&wmc, &wm)) {
				(*addWMMenuEntryCallback)(wm);
				init_wmconfig_storage(&wmc);
			}

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
				wmc->Restart = value;
			else if (strcmp(key, "terminal") == 0)
				wmc->Flags |= F_TERMINAL;
		}

	}

	fclose(fp);

	if (wmc_to_wm(&wmc, &wm)) {
		(*addWMMenuEntryCallback)(wm);
		init_wmconfig_storage(&wmc);
	}
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

/* normalize and convert one wmconfig-format entry to wm format */
static Bool wmc_to_wm(WMConfigMenuEntry **wmc, WMMenuEntry **wm)
{
	char *p;
	size_t slen;

	/* only Exec is mandatory */
	if (!*wmc || !(*wmc)->Exec || !*(*wmc)->Exec)
		return False;

	/* normalize Exec: wmconfig tends to stuck an ampersand
	 * at the end of everything, which we don't need */
	slen = strlen((*wmc)->Exec) - 1;
	p = (*wmc)->Exec;
	while (slen > 0 && (isspace(*(p + slen)) || *(p + slen) == '&'))
		*(p + slen--) = '\0';

	/* if there's no Name, use the first word of Exec; still better
	 * than nothing. i realize it's highly arguable whether `xterm' from
	 * `xterm -e "ssh dev push-to-prod"' is helpful or not, but since
	 * the alternative is to completely lose the entry, i opt for this.
	 * you could just fix the descriptor file to have a label <G> */
	if (!(*wmc)->Name) {
		(*wmc)->Name = wstrdup((*wmc)->Exec);
		p = strchr((*wmc)->Name, ' ');
		if (p)
			*p = '\0';
	}

	/* if there's no Category, use "Applications"; apparently "no category"
	 * can manifest both as no `group' descriptor at all, or a group
	 * descriptor of "" */
	if (!(*wmc)->Category || !*(*wmc)->Category)
		(*wmc)->Category = wstrdup("Applications");

	/* the `restart' type is used for restart, restart other
	 * wm and quit current wm too. separate these cases. */
	if ((*wmc)->Restart) {
		if (strcmp((*wmc)->Restart, "restart") == 0)
			(*wmc)->Flags |= F_RESTART_SELF;
		else if (strcmp((*wmc)->Restart, "quit") == 0)
			(*wmc)->Flags |= F_QUIT;
		else
			(*wmc)->Flags |= F_RESTART_OTHER;
	}

	(*wm)->Name = (*wmc)->Name;
	(*wm)->CmdLine = (*wmc)->Exec;
	(*wm)->SubMenu = (*wmc)->Category;
	(*wm)->Flags = (*wmc)->Flags;

	return True;
}

static void init_wmconfig_storage(WMConfigMenuEntry **wmc)
{
	if ((*wmc)->Category)
		wfree((*wmc)->Category);
	(*wmc)->Category = NULL;
	if ((*wmc)->Name)
		wfree((*wmc)->Name);
	(*wmc)->Name = NULL;
	if ((*wmc)->Restart)
		wfree((*wmc)->Restart);
	(*wmc)->Restart = NULL;
	(*wmc)->Flags = 0;
}
