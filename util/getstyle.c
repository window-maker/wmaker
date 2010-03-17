/* getstyle.c - outputs style related options from WindowMaker to stdout
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

#define PROG_VERSION "getstyle (Window Maker) 0.6"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <assert.h>
#include <libgen.h>
#include <WINGs/WUtil.h>

#ifndef PATH_MAX
#define PATH_MAX  1024
#endif

#include "../src/wconfig.h"

#ifndef GLOBAL_DEFAULTS_SUBDIR
#define GLOBAL_DEFAULTS_SUBDIR "WindowMaker"
#endif

/* table of style related options */
static char *options[] = {
	"TitleJustify",
	"ClipTitleFont",
	"WindowTitleFont",
	"MenuTitleFont",
	"MenuTextFont",
	"IconTitleFont",
	"DisplayFont",
	"LargeDisplayFont",
	"WindowTitleExtendSpace",
	"MenuTitleExtendSpace",
	"MenuTextExtendSpace",
	"HighlightColor",
	"HighlightTextColor",
	"ClipTitleColor",
	"CClipTitleColor",
	"FTitleColor",
	"PTitleColor",
	"UTitleColor",
	"FTitleBack",
	"PTitleBack",
	"UTitleBack",
	"ResizebarBack",
	"MenuTitleColor",
	"MenuTextColor",
	"MenuDisabledColor",
	"MenuTitleBack",
	"MenuTextBack",
	"IconBack",
	"IconTitleColor",
	"IconTitleBack",
	"MenuStyle",
	"WindowTitleExtendSpace",
	"MenuTitleExtendSpace",
	"MenuTextExtendSpace",
	NULL
};

/* table of theme related options */
static char *theme_options[] = {
	"WorkspaceBack",
	"NormalCursor",
	"ArrowCursor",
	"MoveCursor",
	"ResizeCursor",
	"TopLeftResizeCursor",
	"TopRightResizeCursor",
	"BottomLeftResizeCursor",
	"BottomRightResizeCursor",
	"VerticalResizeCursor",
	"HorizontalResizeCursor",
	"WaitCursor",
	"QuestionCursor",
	"TextCursor",
	"SelectCursor",
	NULL
};

/* table of style related fonts */

static char *font_options[] = {
	"ClipTitleFont",
	"WindowTitleFont",
	"MenuTitleFont",
	"MenuTextFont",
	"IconTitleFont",
	"DisplayFont",
	"LargeDisplayFont",
	NULL
};

char *ProgName;

WMPropList *PixmapPath = NULL;

char *ThemePath = NULL;

extern char *convertFont(char *font, Bool keepXLFD);

void print_help()
{
	printf("Usage: %s [OPTIONS] [FILE]\n", ProgName);
	puts("Retrieves style/theme configuration and output to FILE or to stdout");
	puts("");
	puts("  -t, --theme-options	output theme related options when producing a style file");
	puts("  -p, --pack		produce output as a theme pack");
	puts("  --help		display this help and exit");
	puts("  --version		output version information and exit");
}

char *globalDefaultsPathForDomain(char *domain)
{
	static char path[1024];

	sprintf(path, "%s/%s/%s", SYSCONFDIR, GLOBAL_DEFAULTS_SUBDIR, domain);

	return path;
}

char *defaultsPathForDomain(char *domain)
{
	static char path[1024];
	char *gspath;

	gspath = getenv("GNUSTEP_USER_ROOT");
	if (gspath) {
		strcpy(path, gspath);
		strcat(path, "/");
	} else {
		char *home;

		home = getenv("HOME");
		if (!home) {
			printf("%s:could not get HOME environment variable!\n", ProgName);
			exit(0);
		}
		strcpy(path, home);
		strcat(path, "/GNUstep/");
	}
	strcat(path, DEFAULTS_DIR);
	strcat(path, "/");
	strcat(path, domain);

	return path;
}

void abortar(char *reason)
{
	printf("%s: %s\n", ProgName, reason);
	if (ThemePath) {
		printf("Removing unfinished theme pack\n");
		(void)wrmdirhier(ThemePath);
	}
	exit(1);
}

static Bool isFontOption(char *option)
{
	int i;

	for (i = 0; font_options[i] != NULL; i++) {
		if (strcasecmp(option, font_options[i]) == 0) {
			return True;
		}
	}

	return False;
}

/*
 * it is more or less assumed that this function will only
 * copy reasonably-sized files
 */
void copyFile(char *dir, char *file)
{
	int from_fd, to_fd;
	size_t block, len;
	char buf[4096];
	struct stat st;
	char *dst;

	/* only to a directory */
	if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode))
		return;
	/* only copy files */
	if (stat(file, &st) != 0 || !S_ISREG(st.st_mode))
		return;

	len = strlen(dir) + 1 /* / */ + strlen(file) + 1 /* '\0' */;
	dst = wmalloc(len);
	snprintf(dst, len, "%s/%s", dir, basename(file));
	buf[len] = '\0';

	if ((to_fd = open(dst, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1) {
		wfree(dst);
		return;
	}
	wfree(dst);
	if ((from_fd = open(file, O_RDONLY)) == -1) {
		(void)close(to_fd);
		return;
	}

	/* XXX: signal handling */
	while ((block = read(from_fd, &buf, sizeof(buf))) > 0)
		write(to_fd, &buf, block);

	(void)fsync(to_fd);
	(void)fchmod(to_fd, st.st_mode);
	(void)close(to_fd);
	(void)close(from_fd);
}

void findCopyFile(char *dir, char *file)
{
	char *fullPath;

	fullPath = wfindfileinarray(PixmapPath, file);
	if (!fullPath) {
		char buffer[4000];

		sprintf(buffer, "could not find file %s", file);
		abortar(buffer);
	}
	copyFile(dir, fullPath);
	free(fullPath);
}

void makeThemePack(WMPropList * style, char *themeName)
{
	WMPropList *keys;
	WMPropList *key;
	WMPropList *value;
	int i;
	size_t themeNameLen;
	char *themeDir, *t;

	if ((t = wusergnusteppath()) == NULL)
		return;
	themeNameLen = strlen(t) + 1 /* / */ + strlen(themeName) + 8 /* ".themed/" */ + 1 /* '\0' */;
	themeDir = wmalloc(themeNameLen);
	snprintf(themeDir, themeNameLen, "%s/%s.themed/", t, themeName);
	ThemePath = themeDir;
	if (!wmkdirhier(themeDir))
		return;

	keys = WMGetPLDictionaryKeys(style);

	for (i = 0; i < WMGetPropListItemCount(keys); i++) {
		key = WMGetFromPLArray(keys, i);

		value = WMGetFromPLDictionary(style, key);
		if (value && WMIsPLArray(value) && WMGetPropListItemCount(value) > 2) {
			WMPropList *type;
			char *t;

			type = WMGetFromPLArray(value, 0);
			t = WMGetFromPLString(type);
			if (t == NULL)
				continue;

			if (strcasecmp(t, "tpixmap") == 0
			    || strcasecmp(t, "spixmap") == 0
			    || strcasecmp(t, "cpixmap") == 0
			    || strcasecmp(t, "mpixmap") == 0
			    || strcasecmp(t, "tdgradient") == 0
			    || strcasecmp(t, "tvgradient") == 0 || strcasecmp(t, "thgradient") == 0) {
				WMPropList *file;
				char *p;
				char *newPath;

				file = WMGetFromPLArray(value, 1);

				p = strrchr(WMGetFromPLString(file), '/');
				if (p) {
					copyFile(themeDir, WMGetFromPLString(file));

					newPath = wstrdup(p + 1);
					WMDeleteFromPLArray(value, 1);
					WMInsertInPLArray(value, 1, WMCreatePLString(newPath));
					free(newPath);
				} else {
					findCopyFile(themeDir, WMGetFromPLString(file));
				}
			} else if (strcasecmp(t, "bitmap") == 0) {
				WMPropList *file;
				char *p;
				char *newPath;

				file = WMGetFromPLArray(value, 1);

				p = strrchr(WMGetFromPLString(file), '/');
				if (p) {
					copyFile(themeDir, WMGetFromPLString(file));

					newPath = wstrdup(p + 1);
					WMDeleteFromPLArray(value, 1);
					WMInsertInPLArray(value, 1, WMCreatePLString(newPath));
					free(newPath);
				} else {
					findCopyFile(themeDir, WMGetFromPLString(file));
				}

				file = WMGetFromPLArray(value, 2);

				p = strrchr(WMGetFromPLString(file), '/');
				if (p) {
					copyFile(themeDir, WMGetFromPLString(file));

					newPath = wstrdup(p + 1);
					WMDeleteFromPLArray(value, 2);
					WMInsertInPLArray(value, 2, WMCreatePLString(newPath));
					free(newPath);
				} else {
					findCopyFile(themeDir, WMGetFromPLString(file));
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	WMPropList *prop, *style, *key, *val;
	char *path, *p;
	int i, theme_too = 0, make_pack = 0;
	char *style_file = NULL;

	ProgName = argv[0];

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pack") == 0) {
				make_pack = 1;
				theme_too = 1;
			} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--theme-options") == 0) {
				theme_too++;
			} else if (strcmp(argv[i], "--help") == 0) {
				print_help();
				exit(0);
			} else if (strcmp(argv[i], "--version") == 0) {
				puts(PROG_VERSION);
				exit(0);
			} else {
				if (style_file != NULL) {
					printf("%s: invalid argument '%s'\n", argv[0],
					       style_file[0] == '-' ? style_file : argv[i]);
					printf("Try '%s --help' for more information\n", argv[0]);
					exit(1);
				}
				style_file = argv[i];
				while ((p = strchr(style_file, '/')) != NULL)
					*p = '_';
			}
		}
	}

	if (make_pack && !style_file) {
		printf("%s: you must supply a name for the theme pack\n", ProgName);
		exit(1);
	}

	WMPLSetCaseSensitive(False);

	path = defaultsPathForDomain("WindowMaker");

	prop = WMReadPropListFromFile(path);
	if (!prop) {
		printf("%s:could not load WindowMaker configuration file \"%s\".\n", ProgName, path);
		exit(1);
	}

	/* get global value */
	path = globalDefaultsPathForDomain("WindowMaker");
	val = WMReadPropListFromFile(path);
	if (val) {
		WMMergePLDictionaries(val, prop, True);
		WMReleasePropList(prop);
		prop = val;
	}

	style = WMCreatePLDictionary(NULL, NULL);

	for (i = 0; options[i] != NULL; i++) {
		key = WMCreatePLString(options[i]);

		val = WMGetFromPLDictionary(prop, key);
		if (val) {
			WMRetainPropList(val);
			if (isFontOption(options[i])) {
				char *newfont, *oldfont;

				oldfont = WMGetFromPLString(val);
				newfont = convertFont(oldfont, False);
				/* newfont is a reference to old if conversion is not needed */
				if (newfont != oldfont) {
					WMReleasePropList(val);
					val = WMCreatePLString(newfont);
					wfree(newfont);
				}
			}
			WMPutInPLDictionary(style, key, val);
			WMReleasePropList(val);
		}
		WMReleasePropList(key);
	}

	val = WMGetFromPLDictionary(prop, WMCreatePLString("PixmapPath"));
	if (val)
		PixmapPath = val;

	if (theme_too) {
		for (i = 0; theme_options[i] != NULL; i++) {
			key = WMCreatePLString(theme_options[i]);

			val = WMGetFromPLDictionary(prop, key);
			if (val)
				WMPutInPLDictionary(style, key, val);
		}
	}

	if (make_pack) {
		char *path;

		makeThemePack(style, style_file);

		path = wmalloc(strlen(ThemePath) + 32);
		strcpy(path, ThemePath);
		strcat(path, "/style");
		WMWritePropListToFile(style, path);
		wfree(path);
	} else {
		if (style_file) {
			WMWritePropListToFile(style, style_file);
		} else {
			puts(WMGetPropListDescription(style, True));
		}
	}
	exit(0);
}
