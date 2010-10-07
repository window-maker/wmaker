/*
 * wmmenugen - Window Maker PropList menu generator
 *
 * miscellaneous functions
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

#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include <WINGs/WUtil.h>

static char *terminals[] = {
	"x-terminal-emulator", "aterm","eterm", "gnome-terminal", "konsole",
	"kterm", "mlterm", "rxvt", "mrxvt", "pterm", "xterm", "dtterm",
	NULL
};

/* pick a terminal emulator by finding the first existing entry of `terminals'
 * in $PATH. the returned pointer should be wfreed later.
 * if $WMMENU_TERMINAL exists in the environment, it's value overrides this
 * detection.
 */
char *find_terminal_emulator(void)
{
	char *path, *t, *ret;
	int i;

	path = t = ret = NULL;

	t = getenv("WMMENU_TERMINAL");
	if (t) {
		ret = wstrdup(t);
		return ret;
	}

	path = getenv("PATH");
	if (!path)
		return NULL;

	for (i = 0; terminals[i]; i++) {
		t = wfindfile(path, terminals[i]);
		if (t)
			break;
	}

	if (t)
		ret = wstrdup(basename(t));
	else
		ret = wstrdup(t);

	wfree(t);

	return ret;
}

/* tokenize `what' (LC_MESSAGES or LANG if `what' is NULL) in the form of
 * `language[_territory][.codeset][@modifier]' into separate language, country,
 * encoding, modifier components, which are allocated on demand and should be
 * wfreed later. components that do not exist in `what' are set to NULL.
 */
void parse_locale(const char *what, char **language, char **country, char **encoding, char **modifier)
{
	char *e, *p;

	*language = *country = *encoding = *modifier = NULL;

	if (what == NULL) {
		e = getenv("LC_MESSAGES");
		if (e == NULL) {
			e = getenv("LANG");	/* this violates the spec */
			if (e == NULL)
				return;
		}
		e = wstrdup(e);
	} else {
		e = wstrdup(what);
	}

	if (strlen(e) == 0 ||
	    strcmp(e, "POSIX") == 0 ||
	    strcmp(e, "C") == 0)
		goto out;

	p = strchr(e, '@');
	if (p) {
		*modifier = wstrdup(p + 1);
		*p = '\0';
	}

	p = strchr(e, '.');
	if (p) {
		*encoding = wstrdup(p + 1);
		*p = '\0';
	}

	p = strchr(e, '_');
	if (p) {
		*country = wstrdup(p + 1);
		*p = '\0';
	}

	if (strlen(e) > 0)
		*language = wstrdup(e);

out:
	free(e);
	return;

}
