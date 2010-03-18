/* geticonset.c - outputs icon configuration from WindowMaker to stdout
 *
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#define PROG_VERSION "geticonset (Window Maker) 0.1"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <WINGs/WUtil.h>

#include "../src/wconfig.h"

extern char *__progname;

void print_help()
{
	printf("Usage: %s [OPTIONS] [FILE]\n", __progname);
	puts("Retrieves program icon configuration and output to FILE or to stdout");
	puts("");
	puts("  -h, --help      display this help and exit");
	puts("  -v, --version   output version information and exit");
}

int main(int argc, char **argv)
{
	WMPropList *window_name, *icon_key, *window_attrs, *icon_value;
	WMPropList *all_windows, *iconset, *keylist;
	char *path;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help();
			exit(0);
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			puts(PROG_VERSION);
			exit(0);
		}
	}

	path = wdefaultspathfordomain("WMWindowAttributes");

	all_windows = WMReadPropListFromFile(path);
	if (!all_windows) {
		printf("%s: could not load WindowMaker configuration file \"%s\".\n", __progname, path);
		exit(1);
	}

	iconset = WMCreatePLDictionary(NULL, NULL);

	keylist = WMGetPLDictionaryKeys(all_windows);
	icon_key = WMCreatePLString("Icon");

	for (i = 0; i < WMGetPropListItemCount(keylist); i++) {
		WMPropList *icondic;

		window_name = WMGetFromPLArray(keylist, i);
		if (!WMIsPLString(window_name))
			continue;

		window_attrs = WMGetFromPLDictionary(all_windows, window_name);
		if (window_attrs && WMIsPLDictionary(window_attrs)) {
			icon_value = WMGetFromPLDictionary(window_attrs, icon_key);
			if (icon_value) {
				icondic = WMCreatePLDictionary(icon_key, icon_value, NULL);
				WMPutInPLDictionary(iconset, window_name, icondic);
			}
		}
	}

	if (argc == 2) {
		WMWritePropListToFile(iconset, argv[1]);
	} else {
		puts(WMGetPropListDescription(iconset, True));
	}
	exit(0);
}
