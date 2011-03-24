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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef __GLIBC__
#define _GNU_SOURCE		/* getopt_long */
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <WINGs/WUtil.h>

#define	RETRY( x )	do {				\
				x;			\
			} while (errno == EINTR);

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

extern char *__progname;

WMPropList *PixmapPath = NULL;

char *ThemePath = NULL;

extern char *convertFont(char *font, Bool keepXLFD);

void print_help(int print_usage, int exitval)
{
	printf("Usage: %s [-t] [-p] [-h] [-v] [file]\n", __progname);
	if (print_usage) {
		puts("Retrieves style/theme configuration and output to FILE or to stdout");
		puts("");
		puts("  -h, --help           display this help and exit");
		puts("  -v, --version        output version information and exit");
		puts("  -t, --theme-options  output theme related options when producing a style file");
		puts("  -p, --pack           produce output as a theme pack");
	}
	exit(exitval);
}

void abortar(char *reason)
{
	printf("%s: %s\n", __progname, reason);
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
 * copy a file specified by `file' into `directory'. name stays.
 */
/*
 * it is more or less assumed that this function will only
 * copy reasonably-sized files
 */
/* XXX: is almost like WINGs/wcolodpanel.c:fetchFile() */
void copyFile(char *dir, char *file)
{
	FILE *src = NULL, *dst = NULL;
	size_t nread, nwritten, len;
	char buf[4096];
	struct stat st;
	char *dstpath;

	/* only to a directory */
	if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode))
		return;
	/* only copy files */
	if (stat(file, &st) != 0 || !S_ISREG(st.st_mode))
		return;

	len = strlen(dir) + 1 /* / */ + strlen(file) + 1 /* '\0' */;
	dstpath = wmalloc(len);
	snprintf(dstpath, len, "%s/%s", dir, basename(file));
	buf[len] = '\0';

	RETRY( dst = fopen(dstpath, "wb") )
	if (dst == NULL) {
		werror(_("Could not create %s"), dstpath);
		goto err;
	}

	RETRY( src = fopen(file, "rb") )
	if (src == NULL) {
		werror(_("Could not open %s"), file);
		goto err;
	}

	do {
		RETRY( nread = fread(buf, 1, sizeof(buf), src) )
		if (ferror(src))
			break;

		RETRY( nwritten = fwrite(buf, 1, nread, dst) )
		if (ferror(dst) || feof(src) || nread != nwritten)
			break;

	} while (1);

	if (ferror(src) || ferror(dst))
		unlink(dstpath);

	fchmod(fileno(dst), st.st_mode);
	fsync(fileno(dst));
	RETRY( fclose(dst) )

err:
	if (src) {
		RETRY( fclose(src) )
	}
	wfree(dstpath);
	return;
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

			if (strcasecmp(t, "tpixmap") == 0 ||
			    strcasecmp(t, "spixmap") == 0 ||
			    strcasecmp(t, "cpixmap") == 0 ||
			    strcasecmp(t, "mpixmap") == 0 ||
			    strcasecmp(t, "tdgradient") == 0 ||
			    strcasecmp(t, "tvgradient") == 0 ||
			    strcasecmp(t, "thgradient") == 0) {

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
	int i, ch, theme_too = 0, make_pack = 0;
	char *style_file = NULL;

	struct option longopts[] = {
		{ "pack",		no_argument,	NULL,	'p' },
		{ "theme-options",	no_argument,	NULL,	't' },
		{ "version",		no_argument,	NULL,	'v' },
		{ "help",		no_argument,	NULL,	'h' },
		{ NULL,			0,		NULL,	0 }
	};

	while ((ch = getopt_long(argc, argv, "ptvh", longopts, NULL)) != -1)
		switch(ch) {
			case 'v':
				printf("%s (Window Maker %s)\n", __progname, VERSION);
				return 0;
				/* NOTREACHED */
			case 'h':
				print_help(1, 0);
				/* NOTREACHED */
			case 'p':
				make_pack = 1;
				theme_too = 1;
				break;
			case 't':
				theme_too = 1;
			case 0:
				break;
			default:
				print_help(0, 1);
				/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		print_help(0, 1);

	style_file = argv[0];
	while ((p = strchr(style_file, '/')) != NULL)
		*p = '_';

	if (style_file && !make_pack)	/* what's this? */
		print_help(0, 1);

	if (make_pack && !style_file) {
		printf("%s: you must supply a name for the theme pack\n", __progname);
		return 1;
	}

	WMPLSetCaseSensitive(False);

	path = wdefaultspathfordomain("WindowMaker");

	prop = WMReadPropListFromFile(path);
	if (!prop) {
		printf("%s: could not load WindowMaker configuration file \"%s\".\n", __progname, path);
		return 1;
	}

	/* get global value */
	path = wglobaldefaultspathfordomain("WindowMaker");

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
	return 0;
}
