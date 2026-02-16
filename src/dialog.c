/* dialog.c - dialog windows for internal use
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
 *  Copyright (c) 1998-2003 Dan Pascu
 *  Copyright (c) 2014-2023 Window Maker Team
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/utsname.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <signal.h>
#ifdef __FreeBSD__
#include <sys/signal.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include "WindowMaker.h"
#include "GNUstep.h"
#include "screen.h"
#include "window.h"
#include "dialog.h"
#include "misc.h"
#include "stacking.h"
#include "framewin.h"
#include "window.h"
#include "actions.h"
#include "xinerama.h"


static WMPoint getCenter(WScreen * scr, int width, int height)
{
	return wGetPointToCenterRectInHead(scr, wGetHeadForPointerLocation(scr), width, height);
}

int wMessageDialog(WScreen *scr, const char *title, const char *message, const char *defBtn, const char *altBtn, const char *othBtn)
{
	WMAlertPanel *panel;
	Window parent;
	WWindow *wwin;
	int result;
	WMPoint center;
	int pwidth, pheight;

	panel = WMCreateScaledAlertPanel(scr->wmscreen, NULL, title, message, defBtn, altBtn, othBtn);
	pwidth = WMWidgetWidth(panel->win);
	pheight = WMWidgetHeight(panel->win);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, NULL, center.x, center.y, pwidth, pheight);
	wwin->client_leader = WMWidgetXID(panel->win);

	WMMapWidget(panel->win);

	wWindowMap(wwin);

	WMRunModalLoop(WMWidgetScreen(panel->win), WMWidgetView(panel->win));

	result = panel->result;

	WMUnmapWidget(panel->win);

	wUnmanageWindow(wwin, False, False);

	WMDestroyAlertPanel(panel);

	XDestroyWindow(dpy, parent);

	return result;
}

static void toggleSaveSession(WMWidget *w, void *data)
{
	/* Parameter not used, but tell the compiler that it is ok */
	(void) data;

	wPreferences.save_session_on_exit = WMGetButtonSelected((WMButton *) w);
}

int wExitDialog(WScreen *scr, const char *title, const char *message, const char *defBtn, const char *altBtn, const char *othBtn)
{
	WMAlertPanel *panel;
	WMButton *saveSessionBtn;
	Window parent;
	WWindow *wwin;
	WMPoint center;
	int result;
	int pwidth, pheight;

	panel = WMCreateScaledAlertPanel(scr->wmscreen, NULL, title, message, defBtn, altBtn, othBtn);
	pwidth = WMWidgetWidth(panel->win);
	pheight = WMWidgetHeight(panel->win);

	/* add save session button */
	saveSessionBtn = WMCreateSwitchButton(panel->hbox);
	WMSetButtonAction(saveSessionBtn, toggleSaveSession, NULL);
	WMAddBoxSubview(panel->hbox, WMWidgetView(saveSessionBtn), False, True, pwidth / 2, 0, 0);
	WMSetButtonText(saveSessionBtn, _("Save workspace state"));
	WMSetButtonSelected(saveSessionBtn, wPreferences.save_session_on_exit);
	WMRealizeWidget(saveSessionBtn);
	WMMapWidget(saveSessionBtn);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, NULL, center.x, center.y, pwidth, pheight);

	wwin->client_leader = WMWidgetXID(panel->win);

	WMMapWidget(panel->win);

	wWindowMap(wwin);

	WMRunModalLoop(WMWidgetScreen(panel->win), WMWidgetView(panel->win));

	result = panel->result;

	WMUnmapWidget(panel->win);

	wUnmanageWindow(wwin, False, False);

	WMDestroyAlertPanel(panel);

	XDestroyWindow(dpy, parent);

	return result;
}

typedef struct _WMInputPanelWithHistory {
	WMInputPanel *panel;
	WMArray *history;
	int histpos;
	char *prefix;
	char *suffix;
	char *rest;
	WMArray *variants;
	int varpos;
} WMInputPanelWithHistory;

static char *HistoryFileName(const char *name)
{
	char *filename = NULL;

	filename = getenv("XDG_STATE_HOME");
	if (filename)
		filename = wstrappend(wexpandpath(filename), "/" PACKAGE_TARNAME "/History");
	else
		filename = wstrconcat(wusergnusteppath(), "/.AppInfo/" PACKAGE_TARNAME "/History");

	if (name && strlen(name)) {
		filename = wstrappend(filename, ".");
		filename = wstrappend(filename, name);
	}
	return filename;
}

static int strmatch(const void *str1, const void *str2)
{
	return !strcmp((const char *)str1, (const char *)str2);
}

static WMArray *LoadHistory(const char *filename, int max)
{
	WMPropList *plhistory;
	WMPropList *plitem;
	WMArray *history;
	int i, num;
	char *str;

	history = WMCreateArrayWithDestructor(1, wfree);
	WMAddToArray(history, wstrdup(""));

	plhistory = WMReadPropListFromFile(filename);

	if (plhistory) {
		if (WMIsPLArray(plhistory)) {
			num = WMGetPropListItemCount(plhistory);

			for (i = 0; i < num; ++i) {
				plitem = WMGetFromPLArray(plhistory, i);
				if (WMIsPLString(plitem)) {
					str = WMGetFromPLString(plitem);
					if (WMFindInArray(history, strmatch, str) == WANotFound) {
						/*
						 * The string here is duplicated because it will be freed
						 * automatically when the array is deleted. This is not really
						 * great because it is already an allocated string,
						 * unfortunately we cannot re-use it because it will be freed
						 * when we discard the PL (and we don't want to waste the PL's
						 * memory either)
						 */
						WMAddToArray(history, wstrdup(str));
						if (--max <= 0)
							break;
					}
				}
			}
		}
		WMReleasePropList(plhistory);
	}

	return history;
}

static void SaveHistory(WMArray * history, const char *filename)
{
	int i;
	WMPropList *plhistory;

	plhistory = WMCreatePLArray(NULL);

	for (i = 0; i < WMGetArrayItemCount(history); ++i)
		WMAddToPLArray(plhistory, WMCreatePLString(WMGetFromArray(history, i)));

	WMWritePropListToFile(plhistory, filename);
	WMReleasePropList(plhistory);
}

static int pstrcmp(const char **str1, const char **str2)
{
	return strcmp(*str1, *str2);
}

static void
ScanFiles(const char *dir, const char *prefix, unsigned acceptmask, unsigned declinemask, WMArray * result)
{
	int prefixlen;
	DIR *d;
	struct dirent *de;
	struct stat sb;
	char *fullfilename, *suffix;

	prefixlen = strlen(prefix);
	d = opendir(dir);
	if (d != NULL) {
		while ((de = readdir(d)) != NULL) {
			if (strlen(de->d_name) > prefixlen &&
			    !strncmp(prefix, de->d_name, prefixlen) &&
			    strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..")) {
				fullfilename = wstrconcat((char *)dir, "/");
				fullfilename = wstrappend(fullfilename, de->d_name);

				if (stat(fullfilename, &sb) == 0 &&
				    (sb.st_mode & acceptmask) &&
				    !(sb.st_mode & declinemask) &&
				    WMFindInArray(result, (WMMatchDataProc *) strmatch,
						  de->d_name + prefixlen) == WANotFound) {
					suffix = wstrdup(de->d_name + prefixlen);
					if (sb.st_mode & S_IFDIR)
						suffix = wstrappend(suffix, "/");
					WMAddToArray(result, suffix);
				}
				wfree(fullfilename);
			}
		}
		closedir(d);
	}
}

static WMArray *GenerateVariants(const char *complete)
{
	Bool firstWord = True;
	WMArray *variants = NULL;
	char *pos = NULL, *path = NULL, *tmp = NULL, *dir = NULL, *prefix = NULL;

	variants = WMCreateArrayWithDestructor(0, wfree);

	while (*complete == ' ')
		++complete;

	pos = strrchr(complete, ' ');
	if (pos != NULL) {
		complete = pos + 1;
		firstWord = False;
	}

	pos = strrchr(complete, '/');
	if (pos != NULL) {
		tmp = wstrndup((char *)complete, pos - complete + 1);
		if (*tmp == '~' && *(tmp + 1) == '/' && getenv("HOME")) {
			dir = wstrdup(getenv("HOME"));
			dir = wstrappend(dir, tmp + 1);
			wfree(tmp);
		} else {
			dir = tmp;
		}
		prefix = wstrdup(pos + 1);
		ScanFiles(dir, prefix, (unsigned)-1, 0, variants);
		wfree(dir);
		wfree(prefix);
	} else if (*complete == '~') {
		WMAddToArray(variants, wstrdup("/"));
	} else if (firstWord) {
		path = getenv("PATH");
		while (path) {
			pos = strchr(path, ':');
			if (pos) {
				tmp = wstrndup(path, pos - path);
				path = pos + 1;
			} else if (*path != '\0') {
				tmp = wstrdup(path);
				path = NULL;
			} else
				break;
			ScanFiles(tmp, complete, S_IXOTH | S_IXGRP | S_IXUSR, S_IFDIR, variants);
			wfree(tmp);
		}
	}

	WMSortArray(variants, (WMCompareDataProc *) pstrcmp);
	return variants;
}

static void handleHistoryKeyPress(XEvent * event, void *clientData)
{
	char *text;
	unsigned pos;
	WMInputPanelWithHistory *p = (WMInputPanelWithHistory *) clientData;
	KeySym ksym;

	ksym = XLookupKeysym(&event->xkey, 0);

	switch (ksym) {
	case XK_Up:
		if (p->histpos < WMGetArrayItemCount(p->history) - 1) {
			if (p->histpos == 0)
				wfree(WMReplaceInArray(p->history, 0, WMGetTextFieldText(p->panel->text)));
			p->histpos++;
			WMSetTextFieldText(p->panel->text, WMGetFromArray(p->history, p->histpos));
		}
		break;
	case XK_Down:
		if (p->histpos > 0) {
			p->histpos--;
			WMSetTextFieldText(p->panel->text, WMGetFromArray(p->history, p->histpos));
		}
		break;
	case XK_Tab:
		if (!p->variants) {
			text = WMGetTextFieldText(p->panel->text);
			pos = WMGetTextFieldCursorPosition(p->panel->text);
			p->prefix = wstrndup(text, pos);
			p->suffix = wstrdup(text + pos);
			wfree(text);
			p->variants = GenerateVariants(p->prefix);
			p->varpos = 0;
			if (!p->variants) {
				wfree(p->prefix);
				wfree(p->suffix);
				p->prefix = NULL;
				p->suffix = NULL;
			}
		}
		if (p->variants && p->prefix && p->suffix) {
			p->varpos++;
			if (p->varpos > WMGetArrayItemCount(p->variants))
				p->varpos = 0;
			if (p->varpos > 0)
				text = wstrconcat(p->prefix, WMGetFromArray(p->variants, p->varpos - 1));
			else
				text = wstrdup(p->prefix);
			pos = strlen(text);
			text = wstrappend(text, p->suffix);
			WMSetTextFieldText(p->panel->text, text);
			WMSetTextFieldCursorPosition(p->panel->text, pos);
			wfree(text);
		}
		break;
	}
	if (ksym != XK_Tab) {
		if (p->prefix) {
			wfree(p->prefix);
			p->prefix = NULL;
		}
		if (p->suffix) {
			wfree(p->suffix);
			p->suffix = NULL;
		}
		if (p->variants) {
			WMFreeArray(p->variants);
			p->variants = NULL;
		}
	}
}

int wAdvancedInputDialog(WScreen *scr, const char *title, const char *message, const char *name, char **text)
{
	WWindow *wwin;
	Window parent;
	char *result;
	WMPoint center;
	WMInputPanelWithHistory *p;
	char *filename;
	int pwidth, pheight;

	filename = HistoryFileName(name);
	p = wmalloc(sizeof(WMInputPanelWithHistory));
	p->panel = WMCreateScaledInputPanel(scr->wmscreen, NULL, title, message, *text, _("OK"), _("Cancel"));
	p->history = LoadHistory(filename, wPreferences.history_lines);
	p->histpos = 0;
	p->prefix = NULL;
	p->suffix = NULL;
	p->rest = NULL;
	p->variants = NULL;
	p->varpos = 0;
	WMCreateEventHandler(WMWidgetView(p->panel->text), KeyPressMask, handleHistoryKeyPress, p);
	pwidth = WMWidgetWidth(p->panel->win);
	pheight = WMWidgetHeight(p->panel->win);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);
	XSelectInput(dpy, parent, KeyPressMask | KeyReleaseMask);

	XReparentWindow(dpy, WMWidgetXID(p->panel->win), parent, 0, 0);

	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, NULL, center.x, center.y, pwidth, pheight);

	wwin->client_leader = WMWidgetXID(p->panel->win);

	WMMapWidget(p->panel->win);

	wWindowMap(wwin);

	WMRunModalLoop(WMWidgetScreen(p->panel->win), WMWidgetView(p->panel->win));

	if (p->panel->result == WAPRDefault) {
		result = WMGetTextFieldText(p->panel->text);
		wfree(WMReplaceInArray(p->history, 0, wstrdup(result)));
		SaveHistory(p->history, filename);
	} else
		result = NULL;

	wUnmanageWindow(wwin, False, False);

	WMDestroyInputPanel(p->panel);
	WMFreeArray(p->history);
	wfree(p);
	wfree(filename);

	XDestroyWindow(dpy, parent);

	if (result == NULL)
		return False;
	else {
		if (*text)
			wfree(*text);
		*text = result;

		return True;
	}
}

int wInputDialog(WScreen *scr, const char *title, const char *message, char **text)
{
	WWindow *wwin;
	Window parent;
	WMInputPanel *panel;
	char *result;
	WMPoint center;
	int pwidth, pheight;

	panel = WMCreateScaledInputPanel(scr->wmscreen, NULL, title, message, *text, _("OK"), _("Cancel"));
	pwidth = WMWidgetWidth(panel->win);
	pheight = WMWidgetHeight(panel->win);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);
	XSelectInput(dpy, parent, KeyPressMask | KeyReleaseMask);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, NULL, center.x, center.y, pwidth, pheight);

	wwin->client_leader = WMWidgetXID(panel->win);

	WMMapWidget(panel->win);

	wWindowMap(wwin);

	WMRunModalLoop(WMWidgetScreen(panel->win), WMWidgetView(panel->win));

	if (panel->result == WAPRDefault)
		result = WMGetTextFieldText(panel->text);
	else
		result = NULL;

	wUnmanageWindow(wwin, False, False);

	WMDestroyInputPanel(panel);

	XDestroyWindow(dpy, parent);

	if (result == NULL)
		return False;
	else {
		if (*text)
			wfree(*text);
		*text = result;

		return True;
	}
}

/*
 *****************************************************************
 * Icon Selection Panel
 *****************************************************************
 */

typedef struct IconPanel {

	WScreen *scr;

	WMWindow *win;

	WMLabel *dirLabel;
	WMLabel *iconLabel;

	WMList *dirList;
	WMList *iconList;
	WMFont *normalfont;

	WMButton *previewButton;

	WMLabel *iconView;

	WMLabel *fileLabel;
	WMTextField *fileField;

	WMButton *okButton;
	WMButton *cancelButton;
#if 0
	WMButton *chooseButton;
#endif
	short done;
	short result;
	short preview;
} IconPanel;

static void listPixmaps(WScreen *scr, WMList *lPtr, const char *path)
{
	struct dirent *dentry;
	DIR *dir;
	char pbuf[PATH_MAX + 16];
	char *apath;
	IconPanel *panel = WMGetHangedData(lPtr);

	panel->preview = False;

	apath = wexpandpath(path);
	dir = opendir(apath);

	if (!dir) {
		wfree(apath);
		snprintf(pbuf, sizeof(pbuf),
		         _("Could not open directory \"%s\":\n%s"),
		         path, strerror(errno));
		wMessageDialog(scr, _("Error"), pbuf, _("OK"), NULL, NULL);
		return;
	}

	/* list contents in the column */
	while ((dentry = readdir(dir))) {
		struct stat statb;

		if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
			continue;

		if (wstrlcpy(pbuf, apath, sizeof(pbuf)) >= sizeof(pbuf) ||
		    wstrlcat(pbuf, "/", sizeof(pbuf)) >= sizeof(pbuf) ||
		    wstrlcat(pbuf, dentry->d_name, sizeof(pbuf)) >= sizeof(pbuf)) {
			wwarning(_("full path for file \"%s\" in \"%s\" is longer than %d bytes, skipped"),
			         dentry->d_name, path, (int) (sizeof(pbuf) - 1) );
			continue;
		}

		if (stat(pbuf, &statb) < 0)
			continue;

		if (statb.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)
		    && statb.st_mode & (S_IFREG | S_IFLNK)) {
			WMAddListItem(lPtr, dentry->d_name);
		}
	}
	WMSortListItems(lPtr);

	closedir(dir);
	wfree(apath);
	panel->preview = True;
}

static void setViewedImage(IconPanel *panel, const char *file)
{
	WMPixmap *pixmap;
	RColor color;
	int iwidth, iheight;

	color.red = 0xae;
	color.green = 0xaa;
	color.blue = 0xae;
	color.alpha = 0;
	iwidth = WMWidgetWidth(panel->iconView);
	iheight = WMWidgetHeight(panel->iconView);
	pixmap = WMCreateScaledBlendedPixmapFromFile(WMWidgetScreen(panel->win), file, &color, iwidth, iheight);

	if (!pixmap) {
		WMSetButtonEnabled(panel->okButton, False);

		WMSetLabelText(panel->iconView, _("Could not load image file "));

		WMSetLabelImage(panel->iconView, NULL);
	} else {
		WMSetButtonEnabled(panel->okButton, True);

		WMSetLabelText(panel->iconView, NULL);
		WMSetLabelImage(panel->iconView, pixmap);
		WMReleasePixmap(pixmap);
	}
}

static void listCallback(void *self, void *data)
{
	WMList *lPtr = (WMList *) self;
	IconPanel *panel = (IconPanel *) data;
	char *path;

	if (lPtr == panel->dirList) {
		WMListItem *item = WMGetListSelectedItem(lPtr);

		if (item == NULL)
			return;
		path = item->text;

		WMSetTextFieldText(panel->fileField, path);

		WMSetLabelImage(panel->iconView, NULL);

		WMSetButtonEnabled(panel->okButton, False);

		WMClearList(panel->iconList);
		listPixmaps(panel->scr, panel->iconList, path);
	} else {
		char *tmp, *iconFile;
		WMListItem *item = WMGetListSelectedItem(panel->dirList);

		if (item == NULL)
			return;
		path = item->text;

		item = WMGetListSelectedItem(panel->iconList);
		if (item == NULL)
			return;
		iconFile = item->text;

		tmp = wexpandpath(path);
		path = wmalloc(strlen(tmp) + strlen(iconFile) + 4);
		strcpy(path, tmp);
		strcat(path, "/");
		strcat(path, iconFile);
		wfree(tmp);
		WMSetTextFieldText(panel->fileField, path);
		setViewedImage(panel, path);
		wfree(path);
	}
}

static void listIconPaths(WMList * lPtr)
{
	char *paths;
	char *path;

	paths = wstrdup(wPreferences.icon_path);

	path = strtok(paths, ":");

	do {
		char *tmp;

		tmp = wexpandpath(path);
		/* do not sort, because the order implies the order of
		 * directories searched */
		if (access(tmp, X_OK) == 0)
			WMAddListItem(lPtr, path);
		wfree(tmp);
	} while ((path = strtok(NULL, ":")) != NULL);

	wfree(paths);
}

static void drawIconProc(WMList * lPtr, int index, Drawable d, char *text, int state, WMRect * rect)
{
	IconPanel *panel = WMGetHangedData(lPtr);
	WScreen *scr = panel->scr;
	GC gc = scr->draw_gc;
	GC copygc = scr->copy_gc;
	char *file, *dirfile;
	WMPixmap *pixmap;
	WMColor *back;
	WMSize size;
	WMScreen *wmscr = WMWidgetScreen(panel->win);
	RColor color;
	int x, y, width, height, len;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) index;

	if (!panel->preview)
		return;

	x = rect->pos.x;
	y = rect->pos.y;
	width = rect->size.width;
	height = rect->size.height;

	back = (state & WLDSSelected) ? scr->white : scr->gray;

	dirfile = wexpandpath(WMGetListSelectedItem(panel->dirList)->text);
	len = strlen(dirfile) + strlen(text) + 4;
	file = wmalloc(len);
	snprintf(file, len, "%s/%s", dirfile, text);
	wfree(dirfile);

	color.red = WMRedComponentOfColor(back) >> 8;
	color.green = WMGreenComponentOfColor(back) >> 8;
	color.blue = WMBlueComponentOfColor(back) >> 8;
	color.alpha = WMGetColorAlpha(back) >> 8;

	pixmap = WMCreateScaledBlendedPixmapFromFile(wmscr, file, &color, width - 2, height - 2);
	wfree(file);

	if (!pixmap) {
		/*WMRemoveListItem(lPtr, index); */
		return;
	}

	XFillRectangle(dpy, d, WMColorGC(back), x, y, width, height);

	XSetClipMask(dpy, gc, None);
	/*XDrawRectangle(dpy, d, WMColorGC(white), x+5, y+5, width-10, 54); */
	XDrawLine(dpy, d, WMColorGC(scr->white), x, y + height - 1, x + width, y + height - 1);

	size = WMGetPixmapSize(pixmap);

	XSetClipMask(dpy, copygc, WMGetPixmapMaskXID(pixmap));
	XSetClipOrigin(dpy, copygc, x + (width - size.width) / 2, y + 2);
	XCopyArea(dpy, WMGetPixmapXID(pixmap), d, copygc, 0, 0,
		  size.width > 100 ? 100 : size.width, size.height > 64 ? 64 : size.height,
		  x + (width - size.width) / 2, y + 2);

	{
		int i, j;
		int fheight = WMFontHeight(panel->normalfont);
		int tlen = strlen(text);
		int twidth = WMWidthOfString(panel->normalfont, text, tlen);
		int ofx, ofy;

		ofx = x + (width - twidth) / 2;
		ofy = y + 64 - fheight;

		for (i = -1; i < 2; i++)
			for (j = -1; j < 2; j++)
				WMDrawString(wmscr, d, scr->white, panel->normalfont,
					     ofx + i, ofy + j, text, tlen);

		WMDrawString(wmscr, d, scr->black, panel->normalfont, ofx, ofy, text, tlen);
	}

	WMReleasePixmap(pixmap);
	/* I hope it is better to do not use cache / on my box it is fast nuff */
	XFlush(dpy);
}

static void buttonCallback(void *self, void *clientData)
{
	WMButton *bPtr = (WMButton *) self;
	IconPanel *panel = (IconPanel *) clientData;

	if (bPtr == panel->okButton) {
		panel->done = True;
		panel->result = True;
	} else if (bPtr == panel->cancelButton) {
		panel->done = True;
		panel->result = False;
	} else if (bPtr == panel->previewButton) {
	/**** Previewer ****/
		WMSetButtonEnabled(bPtr, False);
		WMSetListUserDrawItemHeight(panel->iconList, 68);
		WMSetListUserDrawProc(panel->iconList, drawIconProc);
		WMRedisplayWidget(panel->iconList);
		/* for draw proc to access screen/gc */
	/*** end preview ***/
	}
#if 0
	else if (bPtr == panel->chooseButton) {
		WMOpenPanel *op;

		op = WMCreateOpenPanel(WMWidgetScreen(bPtr));

		if (WMRunModalFilePanelForDirectory(op, NULL, "/usr/local", NULL, NULL)) {
			char *path;
			path = WMGetFilePanelFile(op);
			WMSetTextFieldText(panel->fileField, path);
			setViewedImage(panel, path);
			wfree(path);
		}
		WMDestroyFilePanel(op);
	}
#endif
}

static void keyPressHandler(XEvent * event, void *data)
{
	IconPanel *panel = (IconPanel *) data;
	char buffer[32];
	KeySym ksym;
	int iidx;
	int didx;
	int item = 0;
	WMList *list = NULL;

	if (event->type == KeyRelease)
		return;

	buffer[0] = 0;
	XLookupString(&event->xkey, buffer, sizeof(buffer), &ksym, NULL);

	iidx = WMGetListSelectedItemRow(panel->iconList);
	didx = WMGetListSelectedItemRow(panel->dirList);

	switch (ksym) {
	case XK_Up:
		if (iidx > 0)
			item = iidx - 1;
		else
			item = iidx;
		list = panel->iconList;
		break;
	case XK_Down:
		if (iidx < WMGetListNumberOfRows(panel->iconList) - 1)
			item = iidx + 1;
		else
			item = iidx;
		list = panel->iconList;
		break;
	case XK_Home:
		item = 0;
		list = panel->iconList;
		break;
	case XK_End:
		item = WMGetListNumberOfRows(panel->iconList) - 1;
		list = panel->iconList;
		break;
	case XK_Next:
		if (didx < WMGetListNumberOfRows(panel->dirList) - 1)
			item = didx + 1;
		else
			item = didx;
		list = panel->dirList;
		break;
	case XK_Prior:
		if (didx > 0)
			item = didx - 1;
		else
			item = 0;
		list = panel->dirList;
		break;
	case XK_Return:
		WMPerformButtonClick(panel->okButton);
		break;
	case XK_Escape:
		WMPerformButtonClick(panel->cancelButton);
		break;
	}

	if (list) {
		WMSelectListItem(list, item);
		WMSetListPosition(list, item - 5);
		listCallback(list, panel);
	}
}

Bool wIconChooserDialog(WScreen *scr, char **file, const char *instance, const char *class)
{
	WWindow *wwin;
	Window parent;
	IconPanel *panel;
	WMColor *color;
	WMFont *boldFont;
	Bool result;
	int wmScaleWidth, wmScaleHeight;
	int pwidth, pheight;

	panel = wmalloc(sizeof(IconPanel));

	panel->scr = scr;

	panel->win = WMCreateWindow(scr->wmscreen, "iconChooser");
	WMGetScaleBaseFromSystemFont(scr->wmscreen, &wmScaleWidth, &wmScaleHeight);
	pwidth = WMScaleX(450);
	pheight = WMScaleY(280);
	WMResizeWidget(panel->win, pwidth, pheight);

	WMCreateEventHandler(WMWidgetView(panel->win), KeyPressMask | KeyReleaseMask, keyPressHandler, panel);

	boldFont = WMBoldSystemFontOfSize(scr->wmscreen, WMScaleY(12));
	panel->normalfont = WMSystemFontOfSize(WMWidgetScreen(panel->win), WMScaleY(12));

	panel->dirLabel = WMCreateLabel(panel->win);
	WMResizeWidget(panel->dirLabel, WMScaleX(200), WMScaleY(20));
	WMMoveWidget(panel->dirLabel, WMScaleX(10), WMScaleY(7));
	WMSetLabelText(panel->dirLabel, _("Directories"));
	WMSetLabelFont(panel->dirLabel, boldFont);
	WMSetLabelTextAlignment(panel->dirLabel, WACenter);

	WMSetLabelRelief(panel->dirLabel, WRSunken);

	panel->iconLabel = WMCreateLabel(panel->win);
	WMResizeWidget(panel->iconLabel, WMScaleX(140), WMScaleY(20));
	WMMoveWidget(panel->iconLabel, WMScaleX(215), WMScaleY(7));
	WMSetLabelText(panel->iconLabel, _("Icons"));
	WMSetLabelFont(panel->iconLabel, boldFont);
	WMSetLabelTextAlignment(panel->iconLabel, WACenter);

	WMReleaseFont(boldFont);

	color = WMWhiteColor(scr->wmscreen);
	WMSetLabelTextColor(panel->dirLabel, color);
	WMSetLabelTextColor(panel->iconLabel, color);
	WMReleaseColor(color);

	color = WMDarkGrayColor(scr->wmscreen);
	WMSetWidgetBackgroundColor(panel->iconLabel, color);
	WMSetWidgetBackgroundColor(panel->dirLabel, color);
	WMReleaseColor(color);

	WMSetLabelRelief(panel->iconLabel, WRSunken);

	panel->dirList = WMCreateList(panel->win);
	WMResizeWidget(panel->dirList, WMScaleX(200), WMScaleY(170));
	WMMoveWidget(panel->dirList, WMScaleX(10), WMScaleY(30));
	WMSetListAction(panel->dirList, listCallback, panel);

	panel->iconList = WMCreateList(panel->win);
	WMResizeWidget(panel->iconList, WMScaleX(140), WMScaleY(170));
	WMMoveWidget(panel->iconList, WMScaleX(215), WMScaleY(30));
	WMSetListAction(panel->iconList, listCallback, panel);

	WMHangData(panel->iconList, panel);

	panel->previewButton = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->previewButton, WMScaleX(75), WMScaleY(26));
	WMMoveWidget(panel->previewButton, WMScaleX(365), WMScaleY(130));
	WMSetButtonText(panel->previewButton, _("Preview"));
	WMSetButtonAction(panel->previewButton, buttonCallback, panel);

	panel->iconView = WMCreateLabel(panel->win);
	WMResizeWidget(panel->iconView, WMScaleX(75), WMScaleY(75));
	WMMoveWidget(panel->iconView, WMScaleX(365), WMScaleY(40));
	WMSetLabelImagePosition(panel->iconView, WIPOverlaps);
	WMSetLabelRelief(panel->iconView, WRSunken);
	WMSetLabelTextAlignment(panel->iconView, WACenter);

	panel->fileLabel = WMCreateLabel(panel->win);
	WMResizeWidget(panel->fileLabel, WMScaleX(80), WMScaleY(20));
	WMMoveWidget(panel->fileLabel, WMScaleX(10), WMScaleY(210));
	WMSetLabelText(panel->fileLabel, _("File Name:"));

	panel->fileField = WMCreateTextField(panel->win);
	WMSetViewNextResponder(WMWidgetView(panel->fileField), WMWidgetView(panel->win));
	WMResizeWidget(panel->fileField, WMScaleX(345), WMScaleY(20));
	WMMoveWidget(panel->fileField, WMScaleX(95), WMScaleY(210));
	WMSetTextFieldEditable(panel->fileField, False);

	panel->okButton = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->okButton, WMScaleX(80), WMScaleY(26));
	WMMoveWidget(panel->okButton, WMScaleX(360), WMScaleY(242));
	WMSetButtonText(panel->okButton, _("OK"));
	WMSetButtonEnabled(panel->okButton, False);
	WMSetButtonAction(panel->okButton, buttonCallback, panel);

	panel->cancelButton = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->cancelButton, WMScaleX(80), WMScaleY(26));
	WMMoveWidget(panel->cancelButton, WMScaleX(270), WMScaleY(242));
	WMSetButtonText(panel->cancelButton, _("Cancel"));
	WMSetButtonAction(panel->cancelButton, buttonCallback, panel);
#if 0
	panel->chooseButton = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->chooseButton, WMScaleX(110), WMScaleY(26));
	WMMoveWidget(panel->chooseButton, WMScaleX(150), WMScaleY(242));
	WMSetButtonText(panel->chooseButton, _("Choose File"));
	WMSetButtonAction(panel->chooseButton, buttonCallback, panel);
#endif
	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	{
		static const char *prefix = NULL;
		char *title;
		int len;
		WMPoint center;

		if (prefix == NULL)
			prefix = _("Icon Chooser");

		len = strlen(prefix)
			+ 2	// " ["
			+ (instance ? strlen(instance) : 0)
			+ 1	// "."
			+ (class ? strlen(class) : 0)
			+ 1	// "]"
			+ 1;	// final NUL

		title = wmalloc(len);
		strcpy(title, prefix);

		if (instance || class) {
			strcat(title, " [");
			if (instance != NULL)
				strcat(title, instance);
			if (instance && class)
				strcat(title, ".");
			if (class != NULL)
				strcat(title, class);
			strcat(title, "]");
		}

		center = getCenter(scr, pwidth, pheight);

		wwin = wManageInternalWindow(scr, parent, None, title, center.x, center.y, pwidth, pheight);
		wfree(title);
	}

	/* put icon paths in the list */
	listIconPaths(panel->dirList);

	WMMapWidget(panel->win);

	wWindowMap(wwin);
	wSetFocusTo(scr, wwin);

	while (!panel->done) {
		XEvent event;

		WMNextEvent(dpy, &event);
		WMHandleEvent(&event);
	}

	if (panel->result) {
		char *defaultPath, *wantedPath;

		/* check if the file the user selected is not the one that
		 * would be loaded by default with the current search path */
		*file = WMGetListSelectedItem(panel->iconList)->text;
		if (**file == 0) {
			wfree(*file);
			*file = NULL;
		} else {
			defaultPath = FindImage(wPreferences.icon_path, *file);
			wantedPath = WMGetTextFieldText(panel->fileField);
			if (defaultPath) {
				/* if the file is not the default, use full path */
				if (strcmp(wantedPath, defaultPath) != 0) {
					*file = wstrdup(wantedPath);
				} else {
					*file = wstrdup(*file);
				}
				wfree(defaultPath);
			}
			wfree(wantedPath);
		}
	} else {
		*file = NULL;
	}

	result = panel->result;

	WMReleaseFont(panel->normalfont);

	WMUnmapWidget(panel->win);

	WMDestroyWidget(panel->win);

	wUnmanageWindow(wwin, False, False);

	wfree(panel);

	XDestroyWindow(dpy, parent);

	return result;
}

/*
 ***********************************************************************
 * Info Panel
 ***********************************************************************
 */

typedef struct {
	WScreen *scr;
	WWindow *wwin;
	WMWindow *win;
	WMLabel *logoL;
	WMLabel *name1L;
	WMFrame *lineF;
	WMLabel *name2L;
	WMLabel *versionL;
	WMLabel *infoL;
	WMLabel *copyrL;
} InfoPanel;

#define COPYRIGHT_TEXT  \
    "Copyright \xc2\xa9 1997-2006 Alfredo K. Kojima\n"\
    "Copyright \xc2\xa9 1998-2006 Dan Pascu\n"\
    "Copyright \xc2\xa9 2013-%hu Window Maker Developers Team"

static InfoPanel *infoPanel = NULL;

static void destroyInfoPanel(WCoreWindow *foo, void *data, XEvent *event)
{
	/* Parameter not used, but tell the compiler that it is ok */
	(void) foo;
	(void) data;
	(void) event;

	WMUnmapWidget(infoPanel);
	wUnmanageWindow(infoPanel->wwin, False, False);
	WMDestroyWidget(infoPanel->win);
	wfree(infoPanel);
	infoPanel = NULL;
}

char *getPrettyOSName(void)
{
	char line[200];
	char *token;
	char *posn = NULL;
	const char s[2] = "=";
	FILE *fp;

	fp = fopen("/etc/os-release", "r");
	if (!fp) {
		fp = fopen("/usr/lib/os-release", "r");
		if (!fp) {
			wwarning(_("no os-release file on the system"));
			return NULL;
		}
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "PRETTY_NAME", strlen("PRETTY_NAME")) == 0) {
			line[strcspn(line, "\r\n")] = 0;
			token = strtok(line, s);
			if (token) {
				//get straight to the second part of the line
				token = strtok(NULL, s);
				if (token) {
					//removing potential quotes
					char *dst = token;
					char *src = token;
					char c;

					while ((c = *src++) != '\0') {
						if (c == '\\') {
							*dst++ = c;
							if ((c = *src++) == '\0')
								break;
							*dst++ = c;
						} else if (c != '"' && c != '\'')
							*dst++ = c;
					}
					*dst = '\0';
					posn = wmalloc(strlen(token) + 1);
					strcpy(posn, token);
				}
			}
			break;
		}
	}
	fclose(fp);
	return posn;
}

void wShowInfoPanel(WScreen *scr)
{
	InfoPanel *panel;
	WMPixmap *logo;
	WMFont *font;
	char *strbuf = NULL;
	const char *separator;
	char buffer[256];
	Window parent;
	WWindow *wwin;
	WMPoint center;
	char **strl;
	int i, width = 50, sepHeight;
	char *visuals[] = {
		"StaticGray",
		"GrayScale",
		"StaticColor",
		"PseudoColor",
		"TrueColor",
		"DirectColor"
	};
	int wmScaleWidth, wmScaleHeight;
	int pwidth, pheight;
	int current_year = 2020;
	time_t s;
	struct tm *current_time;
	struct utsname uts;

	s = time(NULL);
	if (s) {
		current_time = localtime(&s);
		if (current_time->tm_year > (current_year - 1900))
			current_year = current_time->tm_year + 1900;
	}

	if (infoPanel) {
		if (infoPanel->scr == scr) {
			wRaiseFrame(infoPanel->wwin->frame->core);
			wSetFocusTo(scr, infoPanel->wwin);
		}
		return;
	}

	panel = wmalloc(sizeof(InfoPanel));

	panel->scr = scr;

	panel->win = WMCreateWindow(scr->wmscreen, "info");
	WMGetScaleBaseFromSystemFont(scr->wmscreen, &wmScaleWidth, &wmScaleHeight);
#if defined(HAVE_MALLOC_H) && defined(HAVE_MALLINFO2) && defined(DEBUG)
	pwidth = WMScaleX(432);
#else
	pwidth = WMScaleX(392);
#endif
	pheight = WMScaleY(270);
	WMResizeWidget(panel->win, pwidth, pheight);

	logo = WMCreateApplicationIconBlendedPixmap(scr->wmscreen, (RColor *) NULL);
	if (!logo) {
		logo = WMRetainPixmap(WMGetApplicationIconPixmap(scr->wmscreen));
	}
	if (logo) {
		panel->logoL = WMCreateLabel(panel->win);
		WMResizeWidget(panel->logoL, WMScaleX(64), WMScaleY(64));
		WMMoveWidget(panel->logoL, WMScaleX(30), WMScaleY(20));
		WMSetLabelImagePosition(panel->logoL, WIPImageOnly);
		WMSetLabelImage(panel->logoL, logo);
		WMReleasePixmap(logo);
	}

	sepHeight = WMScaleY(3);
	panel->name1L = WMCreateLabel(panel->win);
	WMResizeWidget(panel->name1L, WMScaleX(240), WMScaleY(30) + WMScaleY(2));
	WMMoveWidget(panel->name1L, WMScaleX(100), WMScaleY(30) - WMScaleY(2) - sepHeight);

	snprintf(buffer,
		sizeof(buffer),
		"Lucida Sans,Comic Sans MS,URW Gothic L,Trebuchet MS:italic:pixelsize=%d:antialias=true",
		WMScaleY(24));
	font = WMCreateFont(scr->wmscreen, buffer);
	strbuf = "Window Maker";
	if (font) {
		width = WMWidthOfString(font, strbuf, strlen(strbuf));
		WMSetLabelFont(panel->name1L, font);
		WMReleaseFont(font);
	}
	WMSetLabelTextAlignment(panel->name1L, WACenter);
	WMSetLabelText(panel->name1L, strbuf);

	panel->lineF = WMCreateFrame(panel->win);
	WMResizeWidget(panel->lineF, width, sepHeight);
	WMMoveWidget(panel->lineF, WMScaleX(100) + (WMScaleX(240) - width) / 2, WMScaleY(60) - sepHeight);
	WMSetFrameRelief(panel->lineF, WRSimple);
	WMSetWidgetBackgroundColor(panel->lineF, scr->black);

	panel->name2L = WMCreateLabel(panel->win);
	WMResizeWidget(panel->name2L, WMScaleX(240), WMScaleY(24));
	WMMoveWidget(panel->name2L, WMScaleX(100), WMScaleY(60));
	snprintf(buffer, sizeof(buffer), "URW Gothic L,Nimbus Sans L:pixelsize=%d:antialias=true", WMScaleY(16));
	font = WMCreateFont(scr->wmscreen, buffer);
	if (font) {
		WMSetLabelFont(panel->name2L, font);
		WMReleaseFont(font);
		font = NULL;
	}
	WMSetLabelTextAlignment(panel->name2L, WACenter);
	WMSetLabelText(panel->name2L, _("Window Manager for X"));

	snprintf(buffer, sizeof(buffer), _("Version %s"), VERSION);
	panel->versionL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->versionL, WMScaleX(310), WMScaleY(16));
	WMMoveWidget(panel->versionL, WMScaleX(30), WMScaleY(95));
	WMSetLabelTextAlignment(panel->versionL, WARight);
	WMSetLabelText(panel->versionL, buffer);
	WMSetLabelWraps(panel->versionL, False);

	panel->copyrL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->copyrL, WMScaleX(360), WMScaleY(60));
	WMMoveWidget(panel->copyrL, WMScaleX(15), WMScaleY(215));
	WMSetLabelTextAlignment(panel->copyrL, WALeft);

	snprintf(buffer, sizeof(buffer), COPYRIGHT_TEXT, current_year);
	WMSetLabelText(panel->copyrL, buffer);
	font = WMSystemFontOfSize(scr->wmscreen, WMScaleY(11));
	if (font) {
		WMSetLabelFont(panel->copyrL, font);
		WMReleaseFont(font);
		font = NULL;
	}

	strbuf = NULL;
	if (uname(&uts) != -1) {
		char *posn = getPrettyOSName();
		if (posn) {
			snprintf(buffer, sizeof(buffer), _("Running on: %s (%s)\n"), posn, uts.machine);
			free(posn);
		}
		else
			snprintf(buffer, sizeof(buffer), _("Running on: %s (%s)\n"), uts.sysname, uts.machine);
		strbuf = wstrappend(strbuf, buffer);
	}

	snprintf(buffer, sizeof(buffer), _("Using visual 0x%x: %s %ibpp "),
		 (unsigned)scr->w_visual->visualid, visuals[scr->w_visual->class], scr->w_depth);
	strbuf = wstrappend(strbuf, buffer);

	switch (scr->w_depth) {
	case 15:
		strbuf = wstrappend(strbuf, _("(32 thousand colors)\n"));
		break;
	case 16:
		strbuf = wstrappend(strbuf, _("(64 thousand colors)\n"));
		break;
	case 24:
	case 32:
		strbuf = wstrappend(strbuf, _("(16 million colors)\n"));
		break;
	case 30:
	case 40:
		strbuf = wstrappend(strbuf, _("(1 billion colors)\n"));
		break;
	default:
		snprintf(buffer, sizeof(buffer), _("(%d colors)\n"), 1 << scr->w_depth);
		strbuf = wstrappend(strbuf, buffer);
		break;
	}

#if defined(HAVE_MALLOC_H) && defined(HAVE_MALLINFO2)
	{
		struct mallinfo2 ma = mallinfo2();
		snprintf(buffer, sizeof(buffer),
#ifdef DEBUG
					_("Total memory allocated: %lu kB (in use: %lu kB, %lu free chunks)\n"),
#else
					_("Total memory allocated: %lu kB (in use: %lu kB)\n"),
#endif
					(ma.arena + ma.hblkhd) / 1024,
					(ma.uordblks + ma.hblkhd) / 1024
#ifdef DEBUG
					/*
					 * This information is representative of the memory
					 * fragmentation. In ideal case it should be 1, but
					 * that is never possible
					 */
					, ma.ordblks
#endif
					);

		strbuf = wstrappend(strbuf, buffer);
	}
#endif

	strbuf = wstrappend(strbuf, _("Image formats: "));
	strl = RSupportedFileFormats();
	separator = NULL;
	for (i = 0; strl[i] != NULL; i++) {
		if (separator != NULL)
			strbuf = wstrappend(strbuf, separator);
		strbuf = wstrappend(strbuf, strl[i]);
		separator = ", ";
	}

	strbuf = wstrappend(strbuf, _("\nAdditional support for: "));
	strbuf = wstrappend(strbuf, "WMSPEC");

#ifdef USE_MWM_HINTS
	strbuf = wstrappend(strbuf, ", MWM");
#endif

#ifdef USE_DOCK_XDND
	strbuf = wstrappend(strbuf, ", XDnD");
#endif

#ifdef USE_XRES
	strbuf = wstrappend(strbuf, ", XRes");
#endif

#ifdef USE_MAGICK
	strbuf = wstrappend(strbuf, ", ImageMagick");
#endif

#ifdef USE_XINERAMA
	strbuf = wstrappend(strbuf, _("\n"));
#ifdef SOLARIS_XINERAMA
	strbuf = wstrappend(strbuf, _("Solaris "));
#endif
	strbuf = wstrappend(strbuf, _("Xinerama: "));
	{
		char tmp[128];
		snprintf(tmp, sizeof(tmp) - 1, scr->xine_info.count > 1 ? _("%d head(s) found") : _("%d head found"), scr->xine_info.count);
		strbuf = wstrappend(strbuf, tmp);
	}
#endif

#ifdef USE_RANDR
	strbuf = wstrappend(strbuf, _("\n"));
	strbuf = wstrappend(strbuf, "RandR: ");
	if (w_global.xext.randr.supported)
		strbuf = wstrappend(strbuf, _("supported"));
	else
		strbuf = wstrappend(strbuf, _("unsupported"));
#endif

	panel->infoL = WMCreateLabel(panel->win);
#if defined(HAVE_MALLOC_H) && defined(HAVE_MALLINFO2) && defined(DEBUG)
	WMResizeWidget(panel->infoL, WMScaleX(430), WMScaleY(105));
#else
	WMResizeWidget(panel->infoL, WMScaleX(370), WMScaleY(105));
#endif
	WMMoveWidget(panel->infoL, WMScaleX(15), WMScaleY(115));
	WMSetLabelText(panel->infoL, strbuf);
	font = WMSystemFontOfSize(scr->wmscreen, WMScaleY(11));
	if (font) {
		WMSetLabelFont(panel->infoL, font);
		WMReleaseFont(font);
		font = NULL;
	}
	wfree(strbuf);

	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	WMMapWidget(panel->win);

	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, _("Info"), center.x, center.y, pwidth, pheight);

	WSETUFLAG(wwin, no_closable, 0);
	WSETUFLAG(wwin, no_close_button, 0);
#ifdef XKB_BUTTON_HINT
	wFrameWindowHideButton(wwin->frame, WFF_LANGUAGE_BUTTON);
#endif
	wWindowUpdateButtonImages(wwin);
	wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
	wwin->frame->on_click_right = destroyInfoPanel;

	wWindowMap(wwin);

	panel->wwin = wwin;
	infoPanel = panel;
}

/*
 ***********************************************************************
 * Legal Panel
 ***********************************************************************
 */

typedef struct {
	WScreen *scr;
	WWindow *wwin;
	WMWindow *win;
	WMFrame *frame;
	WMLabel *licenseL;
} LegalPanel;

static LegalPanel *legalPanel = NULL;

static void destroyLegalPanel(WCoreWindow * foo, void *data, XEvent * event)
{
	/* Parameter not used, but tell the compiler that it is ok */
	(void) foo;
	(void) data;
	(void) event;

	WMUnmapWidget(legalPanel->win);
	WMDestroyWidget(legalPanel->win);
	wUnmanageWindow(legalPanel->wwin, False, False);
	wfree(legalPanel);
	legalPanel = NULL;
}

void wShowLegalPanel(WScreen *scr)
{
	LegalPanel *panel;
	Window parent;
	WWindow *wwin;
	WMPoint center;
	int wmScaleWidth, wmScaleHeight;
	int pwidth, pheight;

	if (legalPanel) {
		if (legalPanel->scr == scr) {
			wRaiseFrame(legalPanel->wwin->frame->core);
			wSetFocusTo(scr, legalPanel->wwin);
		}
		return;
	}

	panel = wmalloc(sizeof(LegalPanel));
	panel->scr = scr;
	panel->win = WMCreateWindow(scr->wmscreen, "legal");
	WMGetScaleBaseFromSystemFont(scr->wmscreen, &wmScaleWidth, &wmScaleHeight);
	pwidth = WMScaleX(440);
	pheight = WMScaleY(270);
	WMResizeWidget(panel->win, pwidth, pheight);

	panel->frame = WMCreateFrame(panel->win);
	WMResizeWidget(panel->frame, pwidth - (2 * WMScaleX(10)), pheight - (2 * WMScaleY(10)));
	WMMoveWidget(panel->frame, WMScaleX(10), WMScaleY(10));
	WMSetFrameTitle(panel->frame, NULL);

	panel->licenseL = WMCreateLabel(panel->frame);
	WMSetLabelWraps(panel->licenseL, True);
	WMResizeWidget(panel->licenseL, pwidth - (4 * WMScaleX(10)), pheight - (4 * WMScaleY(10)));
	WMMoveWidget(panel->licenseL, WMScaleX(8), WMScaleY(8));
	WMSetLabelTextAlignment(panel->licenseL, WALeft);
	WMSetLabelText(panel->licenseL,
		       _("    Window Maker is free software; you can redistribute it and/or "
			 "modify it under the terms of the GNU General Public License as "
			 "published by the Free Software Foundation; either version 2 of the "
			 "License, or (at your option) any later version.\n\n"
			 "    Window Maker is distributed in the hope that it will be useful, "
			 "but WITHOUT ANY WARRANTY; without even the implied warranty "
			 "of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
			 "See the GNU General Public License for more details.\n\n"
			 "    You should have received a copy of the GNU General Public "
			 "License along with this program; if not, write to the Free Software "
			 "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA"
			 "02110-1301 USA."));

	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);
	WMMapSubwidgets(panel->frame);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);
	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);
	center = getCenter(scr, pwidth, pheight);
	wwin = wManageInternalWindow(scr, parent, None, _("Legal"), center.x, center.y, pwidth, pheight);

	WSETUFLAG(wwin, no_closable, 0);
	WSETUFLAG(wwin, no_close_button, 0);
	wWindowUpdateButtonImages(wwin);
	wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
#ifdef XKB_BUTTON_HINT
	wFrameWindowHideButton(wwin->frame, WFF_LANGUAGE_BUTTON);
#endif
	wwin->frame->on_click_right = destroyLegalPanel;
	panel->wwin = wwin;

	WMMapWidget(panel->win);
	wWindowMap(wwin);
	legalPanel = panel;
}

/*
 ***********************************************************************
 * Crashing Dialog Panel
 ***********************************************************************
 */

typedef struct _CrashPanel {
	WMWindow *win;		/* main window */

	WMLabel *iconL;		/* application icon */
	WMLabel *nameL;		/* title of panel */

	WMFrame *sepF;		/* separator frame */

	WMLabel *noteL;		/* Title of note */
	WMLabel *note2L;	/* body of note with what happened */

	WMFrame *whatF;		/* "what to do next" frame */
	WMPopUpButton *whatP;	/* action selection popup button */

	WMButton *okB;		/* ok button */

	Bool done;		/* if finished with this dialog */
	int action;		/* what to do after */

	KeyCode retKey;
	KeyCode upKey;
	KeyCode downKey;

} CrashPanel;

static void handleKeyPress(XEvent * event, void *clientData)
{
	CrashPanel *panel = (CrashPanel *) clientData;

	if (event->xkey.keycode == panel->retKey) {
		WMPerformButtonClick(panel->okB);
		return;
	}

	if (event->xkey.keycode == panel->upKey) {
		WMSelectPopUpButtonPreviousItem(panel->whatP);
		return;
	}

	if (event->xkey.keycode == panel->downKey) {
		WMSelectPopUpButtonNextItem(panel->whatP);
		return;
	}
}

static void okButtonCallback(void *self, void *clientData)
{
	CrashPanel *panel = (CrashPanel *) clientData;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) self;

	panel->done = True;
}

static void setCrashAction(void *self, void *clientData)
{
	WMPopUpButton *pop = (WMPopUpButton *) self;
	CrashPanel *panel = (CrashPanel *) clientData;

	panel->action = WMGetPopUpButtonSelectedItem(pop);
}

/* Make this read the logo from a compiled in pixmap -Dan */
static WMPixmap *getWindowMakerIconImage(WMScreen *scr)
{
	WMPixmap *pix = NULL;
	char *path = NULL;

	/* Get the Logo icon, without the default icon */
	path = get_icon_filename("Logo", "WMPanel", NULL, False);

	if (path) {
		RColor gray;

		gray.red = 0xae;
		gray.green = 0xaa;
		gray.blue = 0xae;
		gray.alpha = 0;

		pix = WMCreateBlendedPixmapFromFile(scr, path, &gray);
		wfree(path);
	}

	return pix;
}

#define PWIDTH	295
#define PHEIGHT	345

int wShowCrashingDialogPanel(int whatSig)
{
	CrashPanel *panel;
	WMScreen *scr;
	WMFont *font;
	WMPixmap *logo;
	int screen_no, scr_width, scr_height;
	int action;
	char buf[256];

	screen_no = DefaultScreen(dpy);
	scr_width = WidthOfScreen(ScreenOfDisplay(dpy, screen_no));
	scr_height = HeightOfScreen(ScreenOfDisplay(dpy, screen_no));

	scr = WMCreateScreen(dpy, screen_no);
	if (!scr) {
		werror(_("cannot open connection for crashing dialog panel. Aborting."));
		return WMAbort;
	}

	panel = wmalloc(sizeof(CrashPanel));

	panel->retKey = XKeysymToKeycode(dpy, XK_Return);
	panel->upKey = XKeysymToKeycode(dpy, XK_Up);
	panel->downKey = XKeysymToKeycode(dpy, XK_Down);

	panel->win = WMCreateWindow(scr, "crashingDialog");
	WMResizeWidget(panel->win, PWIDTH, PHEIGHT);
	WMMoveWidget(panel->win, (scr_width - PWIDTH) / 2, (scr_height - PHEIGHT) / 2);

	logo = getWindowMakerIconImage(scr);
	if (logo) {
		panel->iconL = WMCreateLabel(panel->win);
		WMResizeWidget(panel->iconL, 64, 64);
		WMMoveWidget(panel->iconL, 10, 10);
		WMSetLabelImagePosition(panel->iconL, WIPImageOnly);
		WMSetLabelImage(panel->iconL, logo);
	}

	panel->nameL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->nameL, 200, 30);
	WMMoveWidget(panel->nameL, 80, 25);
	WMSetLabelTextAlignment(panel->nameL, WALeft);
	font = WMBoldSystemFontOfSize(scr, 24);
	WMSetLabelFont(panel->nameL, font);
	WMReleaseFont(font);
	WMSetLabelText(panel->nameL, _("Fatal error"));

	panel->sepF = WMCreateFrame(panel->win);
	WMResizeWidget(panel->sepF, PWIDTH + 4, 2);
	WMMoveWidget(panel->sepF, -2, 80);

	panel->noteL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->noteL, PWIDTH - 20, 40);
	WMMoveWidget(panel->noteL, 10, 90);
	WMSetLabelTextAlignment(panel->noteL, WAJustified);
	snprintf(buf, sizeof(buf), _("Window Maker received signal %i."), whatSig);
	WMSetLabelText(panel->noteL, buf);

	panel->note2L = WMCreateLabel(panel->win);
	WMResizeWidget(panel->note2L, PWIDTH - 20, 100);
	WMMoveWidget(panel->note2L, 10, 130);
	WMSetLabelTextAlignment(panel->note2L, WALeft);
	snprintf(buf, sizeof(buf), /* Comment for the PO file: the %s is an email address */
	         _(" This fatal error occurred probably due to a bug."
	           " Please fill the included BUGFORM and report it to %s."),
	         PACKAGE_BUGREPORT);
	WMSetLabelText(panel->note2L, buf);
	WMSetLabelWraps(panel->note2L, True);

	panel->whatF = WMCreateFrame(panel->win);
	WMResizeWidget(panel->whatF, PWIDTH - 20, 50);
	WMMoveWidget(panel->whatF, 10, 240);
	WMSetFrameTitle(panel->whatF, _("What do you want to do now?"));

	panel->whatP = WMCreatePopUpButton(panel->whatF);
	WMResizeWidget(panel->whatP, PWIDTH - 20 - 70, 20);
	WMMoveWidget(panel->whatP, 35, 20);
	WMSetPopUpButtonPullsDown(panel->whatP, False);
	WMSetPopUpButtonText(panel->whatP, _("Select action"));
	WMAddPopUpButtonItem(panel->whatP, _("Abort and leave a core file"));
	WMAddPopUpButtonItem(panel->whatP, _("Restart Window Maker"));
	WMAddPopUpButtonItem(panel->whatP, _("Start alternate window manager"));
	WMSetPopUpButtonAction(panel->whatP, setCrashAction, panel);
	WMSetPopUpButtonSelectedItem(panel->whatP, WMRestart);
	panel->action = WMRestart;

	WMMapSubwidgets(panel->whatF);

	panel->okB = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->okB, 80, 26);
	WMMoveWidget(panel->okB, 205, 309);
	WMSetButtonText(panel->okB, _("OK"));
	WMSetButtonImage(panel->okB, WMGetSystemPixmap(scr, WSIReturnArrow));
	WMSetButtonAltImage(panel->okB, WMGetSystemPixmap(scr, WSIHighlightedReturnArrow));
	WMSetButtonImagePosition(panel->okB, WIPRight);
	WMSetButtonAction(panel->okB, okButtonCallback, panel);

	panel->done = 0;

	WMCreateEventHandler(WMWidgetView(panel->win), KeyPressMask, handleKeyPress, panel);

	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);

	WMMapWidget(panel->win);

	XSetInputFocus(dpy, WMWidgetXID(panel->win), RevertToParent, CurrentTime);

	while (!panel->done) {
		XEvent event;

		WMNextEvent(dpy, &event);
		WMHandleEvent(&event);
	}

	action = panel->action;

	WMUnmapWidget(panel->win);
	WMDestroyWidget(panel->win);
	wfree(panel);

	return action;
}
