/* seticons.c - sets icon configuration in WindowMaker
 *
 *  WindowMaker window manager
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

#define PROG_VERSION "seticons (Window Maker) 0.1"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <WINGs/WUtil.h>

#include "../src/wconfig.h"

extern char *__progname;

void print_help()
{
	printf("Usage: %s [OPTIONS] FILE\n", __progname);
	puts("Reads icon configuration from FILE and updates Window Maker.");
	puts("");
	puts("  -h, --help      display this help and exit");
	puts("  -v, --version   output version information and exit");
}

int main(int argc, char **argv)
{
	WMPropList *window_name, *icon_key, *window_attrs, *icon_value;
	WMPropList *all_windows, *iconset, *keylist;
	int i;
	char *path = NULL;

	if (argc < 2) {
		printf("%s: missing argument\n", __progname);
		printf("Try '%s --help' for more information\n", __progname);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help();
			exit(0);
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			puts(PROG_VERSION);
			exit(0);
		} else {
			if (path) {
				printf("%s: invalid argument '%s'\n", __progname, argv[i]);
				printf("Try '%s --help' for more information\n", __progname);
				exit(1);
			}
			path = argv[i];
		}
	}

	path = wdefaultspathfordomain("WMWindowAttributes");

	all_windows = WMReadPropListFromFile(path);
	if (!all_windows) {
		printf("%s: could not load WindowMaker configuration file \"%s\".\n", __progname, path);
		exit(1);
	}

	iconset = WMReadPropListFromFile(argv[1]);
	if (!iconset) {
		printf("%s: could not load icon set file \"%s\".\n", __progname, argv[1]);
		exit(1);
	}

	keylist = WMGetPLDictionaryKeys(iconset);
	icon_key = WMCreatePLString("Icon");

	for (i = 0; i < WMGetPropListItemCount(keylist); i++) {
		window_name = WMGetFromPLArray(keylist, i);
		if (!WMIsPLString(window_name))
			continue;

		icon_value = WMGetFromPLDictionary(iconset, window_name);
		if (!icon_value || !WMIsPLDictionary(icon_value))
			continue;

		window_attrs = WMGetFromPLDictionary(all_windows, window_name);
		if (window_attrs) {
			if (WMIsPLDictionary(window_attrs)) {
				WMMergePLDictionaries(window_attrs, icon_value, True);
			}
		} else {
			WMPutInPLDictionary(all_windows, window_name, icon_value);
		}
	}

	WMWritePropListToFile(all_windows, path);

	exit(0);
}
