/* dialog.c - dialog windows for internal use
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include "WindowMaker.h"
#include "GNUstep.h"
#include "screen.h"
#include "dialog.h"
#include "funcs.h"
#include "stacking.h"
#include "framewin.h"
#include "window.h"
#include "actions.h"


extern WPreferences wPreferences;



int
wMessageDialog(WScreen *scr, char *title, char *message, 
	       char *defBtn, char *altBtn, char *othBtn)
{
    WMAlertPanel *panel;
    Window parent;
    WWindow *wwin;
    int result;

    panel = WMCreateAlertPanel(scr->wmscreen, NULL, title, message,
			       defBtn, altBtn, othBtn);    

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 400, 180, 0, 0, 0);
    
    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);
    
    wwin = wManageInternalWindow(scr, parent, None, NULL, 
				 (scr->scr_width - 400)/2,
				 (scr->scr_height - 180)/2, 400, 180);
    wwin->client_leader = WMWidgetXID(panel->win);

    WMMapWidget(panel->win);
    
    wWindowMap(wwin);

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }

    result = panel->result;

    WMUnmapWidget(panel->win);

    wUnmanageWindow(wwin, False);

    WMDestroyAlertPanel(panel);

    XDestroyWindow(dpy, parent);

    return result;
}



int
wInputDialog(WScreen *scr, char *title, char *message, char **text)
{
    WWindow *wwin;
    Window parent;
    WMInputPanel *panel;
    char *result;

    
    panel = WMCreateInputPanel(scr->wmscreen, NULL, title, message, *text,
			       _("OK"), _("Cancel"));


    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 320, 160, 0, 0, 0);
    XSelectInput(dpy, parent, KeyPressMask|KeyReleaseMask);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, NULL, 
				 (scr->scr_width - 320)/2,
				 (scr->scr_height - 160)/2, 320, 160);

    wwin->client_leader = WMWidgetXID(panel->win);

    WMMapWidget(panel->win);
    
    wWindowMap(wwin);
    
    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
    
    if (panel->result == WAPRDefault)
	result = WMGetTextFieldText(panel->text);
    else
	result = NULL;
    
    wUnmanageWindow(wwin, False);
    
    WMDestroyInputPanel(panel);

    XDestroyWindow(dpy, parent);

    if (result==NULL)
	return WDB_CANCEL;
    else {
        if (*text)
            free(*text);
        *text = result;

        return WDB_OK;
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
} IconPanel;



static void
listPixmaps(WScreen *scr, WMList *lPtr, char *path)
{
    struct dirent *dentry;
    DIR *dir;
    char pbuf[PATH_MAX+16];
    char *apath;
    
    apath = wexpandpath(path);
    dir = opendir(apath);
    
    if (!dir) {
	char *msg;
	char *tmp;
	tmp = _("Could not open directory ");
	msg = wmalloc(strlen(tmp)+strlen(path)+6);
	strcpy(msg, tmp);
	strcat(msg, path);
	
	wMessageDialog(scr, _("Error"), msg, _("OK"), NULL, NULL);
	free(msg);
	free(apath);
	return;
    }

    /* list contents in the column */
    while ((dentry = readdir(dir))) {
	struct stat statb;
	
	if (strcmp(dentry->d_name, ".")==0 ||
	    strcmp(dentry->d_name, "..")==0)
	    continue;

	strcpy(pbuf, apath);
	strcat(pbuf, "/");
	strcat(pbuf, dentry->d_name);
	
	if (stat(pbuf, &statb)<0)
	    continue;
	
	if (statb.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)
	    && statb.st_mode & (S_IFREG|S_IFLNK)) {
	    WMAddSortedListItem(lPtr, dentry->d_name);
	}
    }

    closedir(dir);
    free(apath);
}



static void
setViewedImage(IconPanel *panel, char *file)
{
    WMPixmap *pixmap;
    RColor color;
		
    color.red = 0xae;
    color.green = 0xaa;
    color.blue = 0xae;
    color.alpha = 0;
    pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
					   file, &color);
    if (!pixmap) {
	char *msg;
	char *tmp;

	WMSetButtonEnabled(panel->okButton, False);
	    
	tmp = _("Could not load image file ");
	msg = wmalloc(strlen(tmp)+strlen(file)+6);
	strcpy(msg, tmp);
	strcat(msg, file);
	
	wMessageDialog(panel->scr, _("Error"), msg, _("OK"), NULL, NULL);
	free(msg);
	
	WMSetLabelImage(panel->iconView, NULL);
    } else {
	WMSetButtonEnabled(panel->okButton, True);
	
	WMSetLabelImage(panel->iconView, pixmap);
	WMReleasePixmap(pixmap);
    }
}


static void
listCallback(void *self, void *data)
{
    WMList *lPtr = (WMList*)self;
    IconPanel *panel = (IconPanel*)data;
    char *path;

    if (lPtr==panel->dirList) {
	path = WMGetListSelectedItem(lPtr)->text;
	
	WMSetTextFieldText(panel->fileField, path);

	WMSetLabelImage(panel->iconView, NULL);

	WMSetButtonEnabled(panel->okButton, False);

	WMClearList(panel->iconList);
	listPixmaps(panel->scr, panel->iconList, path);	
    } else {
	char *tmp, *iconFile;
	
	path = WMGetListSelectedItem(panel->dirList)->text;
	tmp = wexpandpath(path);

	iconFile = WMGetListSelectedItem(panel->iconList)->text;

	path = wmalloc(strlen(tmp)+strlen(iconFile)+4);
	strcpy(path, tmp);
	strcat(path, "/");
	strcat(path, iconFile);
	free(tmp);
	WMSetTextFieldText(panel->fileField, path);
	setViewedImage(panel, path);
	free(path);
    }
}


static void
listIconPaths(WMList *lPtr)
{
    int i;
    
    for (i=0; wPreferences.icon_path[i]!=NULL; i++) {
	char *tmp;
	tmp = wexpandpath(wPreferences.icon_path[i]);
	/* do not sort, because the order implies the order of
	 * directories searched */
	if (access(tmp, X_OK)==0)
	    WMAddListItem(lPtr, wPreferences.icon_path[i]);
	free(tmp);
    }
}



static void
buttonCallback(void *self, void *clientData)
{
    WMButton *bPtr = (WMButton*)self;
    IconPanel *panel = (IconPanel*)clientData;
    
    
    if (bPtr==panel->okButton) {
	panel->done = True;
	panel->result = True;
    } else if (bPtr==panel->cancelButton) {
	panel->done = True;
	panel->result = False;
    }
#if 0
    else if (bPtr==panel->chooseButton) {
	WMOpenPanel *op;
	
	op = WMCreateOpenPanel(WMWidgetScreen(bPtr));
	
	if (WMRunModalOpenPanelForDirectory(op, NULL, "/usr/local", NULL, NULL)) {
	    char *path;
	    path = WMGetFilePanelFile(op);
	    WMSetTextFieldText(panel->fileField, path);
	    setViewedImage(panel, path);
	    free(path);
	}
	WMDestroyFilePanel(op);
    }
#endif
}


Bool
wIconChooserDialog(WScreen *scr, char **file)
{
    WWindow *wwin;
    Window parent;
    IconPanel *panel;
    WMColor *color;
    WMFont *boldFont;
    
    panel = wmalloc(sizeof(IconPanel));
    memset(panel, 0, sizeof(IconPanel));

    panel->scr = scr;
    
    panel->win = WMCreateWindow(scr->wmscreen, "iconChooser");
    WMResizeWidget(panel->win, 450, 280);
    
    boldFont = WMBoldSystemFontOfSize(scr->wmscreen, 12);
    
    panel->dirLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirLabel, 200, 20);
    WMMoveWidget(panel->dirLabel, 10, 7);
    WMSetLabelText(panel->dirLabel, _("Directories"));
    WMSetLabelFont(panel->dirLabel, boldFont);
    WMSetLabelTextAlignment(panel->dirLabel, WACenter);
        
    WMSetLabelRelief(panel->dirLabel, WRSunken);

    panel->iconLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconLabel, 140, 20);
    WMMoveWidget(panel->iconLabel, 215, 7);
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
    WMResizeWidget(panel->dirList, 200, 170);
    WMMoveWidget(panel->dirList, 10, 30);
    WMSetListAction(panel->dirList, listCallback, panel);
    
    panel->iconList = WMCreateList(panel->win);
    WMResizeWidget(panel->iconList, 140, 170);
    WMMoveWidget(panel->iconList, 215, 30); 
    WMSetListAction(panel->iconList, listCallback, panel);
   
    panel->iconView = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconView, 75, 75);
    WMMoveWidget(panel->iconView, 365, 60);
    WMSetLabelImagePosition(panel->iconView, WIPImageOnly);
    WMSetLabelRelief(panel->iconView, WRSunken);
    
    panel->fileLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->fileLabel, 80, 20);
    WMMoveWidget(panel->fileLabel, 10, 210);
    WMSetLabelText(panel->fileLabel, _("File Name:"));
    
    panel->fileField = WMCreateTextField(panel->win);
    WMResizeWidget(panel->fileField, 345, 20);
    WMMoveWidget(panel->fileField, 95, 210);
    WMSetTextFieldEnabled(panel->fileField, False);
    
    panel->okButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->okButton, 80, 26);
    WMMoveWidget(panel->okButton, 360, 240);
    WMSetButtonText(panel->okButton, _("OK"));
    WMSetButtonEnabled(panel->okButton, False);
    WMSetButtonAction(panel->okButton, buttonCallback, panel);
    
    panel->cancelButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancelButton, 80, 26);
    WMMoveWidget(panel->cancelButton, 270, 240);
    WMSetButtonText(panel->cancelButton, _("Cancel"));
    WMSetButtonAction(panel->cancelButton, buttonCallback, panel);
#if 0
    panel->chooseButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->chooseButton, 110, 26);
    WMMoveWidget(panel->chooseButton, 150, 240);
    WMSetButtonText(panel->chooseButton, _("Choose File"));
    WMSetButtonAction(panel->chooseButton, buttonCallback, panel);
#endif
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 450, 280, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, _("Icon Chooser"),
				 (scr->scr_width - 450)/2,
				 (scr->scr_height - 280)/2, 450, 280);

    
    /* put icon paths in the list */
    listIconPaths(panel->dirList);
    
    WMMapWidget(panel->win);
    
    wWindowMap(wwin);

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
	if ((*file)[0]==0) {
	    free(*file);
	    *file = NULL;
	} else {
	    defaultPath = FindImage(wPreferences.icon_path, *file);
	    wantedPath = WMGetTextFieldText(panel->fileField);
	    /* if the file is not the default, use full path */
	    if (strcmp(wantedPath, defaultPath)!=0) {
		*file = wantedPath;
	    } else {
		*file = wstrdup(*file);
		free(wantedPath);
	    }
	    free(defaultPath);
	}
    } else {
	*file = NULL;
    }

    WMUnmapWidget(panel->win);

    WMDestroyWidget(panel->win);    

    wUnmanageWindow(wwin, False);

    free(panel);

    XDestroyWindow(dpy, parent);
    
    return panel->result;
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
    WMLabel *name2L;

    WMLabel *versionL;

    WMLabel *infoL;

    WMLabel *copyrL;
} InfoPanel;



#define COPYRIGHT_TEXT  \
     "Copyright \xa9 1997, 1998 Alfredo K. Kojima <kojima@windowmaker.org>\n"\
     "Copyright \xa9 1998 Dan Pascu <dan@windowmaker.org>"

 

static InfoPanel *thePanel = NULL;

static void
destroyInfoPanel(WCoreWindow *foo, void *data, XEvent *event)
{
    WMUnmapWidget(thePanel);

    WMDestroyWidget(thePanel->win);

    wUnmanageWindow(thePanel->wwin, False);

    free(thePanel);

    thePanel = NULL;
}


WMPixmap*
renderText(WMScreen *scr, char *text, char *font, RColor *from, RColor *to)
{
    WMPixmap *wpix = NULL;
    RImage *gradient = NULL;
    Pixmap grad = None;
    Pixmap mask = None;
    RContext *rc = WMScreenRContext(scr);
    XFontStruct *f = NULL;
    int w, h;
    GC gc = None;
    
    f = XLoadQueryFont(dpy, font);
    if (!f)
	return NULL;

    w = XTextWidth(f, text, strlen(text));
    h = f->ascent+f->descent;

    gradient = RRenderGradient(w, h, from, to, RVerticalGradient);
    if (!gradient) {
	wwarning("error doing image processing:%s",
		RMessageForError(RErrorCode));
	goto bye;
    }
    if (!RConvertImage(rc, gradient, &grad)) {
	wwarning("error doing image processing:%s",
		RMessageForError(RErrorCode));
	goto bye;
    }

    mask = XCreatePixmap(dpy, rc->drawable, w, h, 1);
    gc = XCreateGC(dpy, mask, 0, NULL);
    XSetForeground(dpy, gc, rc->black);
    XSetFont(dpy, gc, f->fid);
    XFillRectangle(dpy, mask, gc, 0, 0, w, h);

    XSetForeground(dpy, gc, rc->white);
    XDrawString(dpy, mask, gc, 0, f->ascent, text, strlen(text));

    XSetLineAttributes(dpy, gc, 3, LineSolid, CapRound, JoinMiter);
    
    XDrawLine(dpy, mask, gc, 0, h-2, w, h-2);
    
    wpix = WMCreatePixmapFromXPixmaps(scr, grad, mask, w, h, rc->depth);

 bye:
    if (gc)
	XFreeGC(dpy, gc);
    XFreeFont(dpy, f);
    if (gradient)
	RDestroyImage(gradient);

    return wpix;
}


void
wShowInfoPanel(WScreen *scr)
{
    InfoPanel *panel;
    WMPixmap *logo;
    WMSize size;
    WMFont *font;
    char version[32];
    char buffer[512];
    Window parent;
    WWindow *wwin;
    RColor color1, color2;
    char **strl;
    int i;
    char *visuals[] = {
	"StaticGray",
	    "GrayScale",
	    "StaticColor",
	    "PseudoColor",
	    "TrueColor",
	    "DirectColor"
    };

    if (thePanel) {
	wRaiseFrame(thePanel->wwin->frame->core);
	wSetFocusTo(scr, thePanel->wwin);
	return;
    }
    
    panel = wmalloc(sizeof(InfoPanel));

    panel->scr = scr;

    panel->win = WMCreateWindow(scr->wmscreen, "info");
    WMResizeWidget(panel->win, 382, 230);
    
    logo = WMGetApplicationIconImage(scr->wmscreen);
    if (logo) {
	size = WMGetPixmapSize(logo);
	panel->logoL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->logoL, 64, 64);
	WMMoveWidget(panel->logoL, 30, 20);
	WMSetLabelImagePosition(panel->logoL, WIPImageOnly);
	WMSetLabelImage(panel->logoL, logo);
    }

    panel->name1L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name1L, 200, 30);
    WMMoveWidget(panel->name1L, 120, 30);
    color1.red = 0;
    color1.green = 0;
    color1.blue = 0;
    color2.red = 0x50;
    color2.green = 0x50;
    color2.blue = 0x60;
    logo = renderText(scr->wmscreen, "    Window Maker    ",
		      "-*-times-bold-r-*-*-24-*", &color1, &color2);
    if (logo) {
	WMSetLabelImagePosition(panel->name1L, WIPImageOnly);
	WMSetLabelImage(panel->name1L, logo);
	WMReleasePixmap(logo);
    } else {
	font = WMBoldSystemFontOfSize(scr->wmscreen, 24);
	if (font) {
	    WMSetLabelFont(panel->name1L, font);
	    WMReleaseFont(font);
	}
	WMSetLabelText(panel->name1L, "Window Maker");
    }

    panel->name2L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name2L, 200, 24);
    WMMoveWidget(panel->name2L, 120, 60);
    font = WMBoldSystemFontOfSize(scr->wmscreen, 18);
    if (font) {
	WMSetLabelFont(panel->name2L, font);
	WMReleaseFont(font);
	font = NULL;
    }
    WMSetLabelText(panel->name2L, "X11 Window Manager");

    
    sprintf(version, "Version %s", VERSION);
    panel->versionL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->versionL, 150, 16);
    WMMoveWidget(panel->versionL, 190, 95);
    WMSetLabelTextAlignment(panel->versionL, WARight);
    WMSetLabelText(panel->versionL, version);
    

    panel->copyrL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->copyrL, 340, 40);
    WMMoveWidget(panel->copyrL, 15, 185);
    WMSetLabelTextAlignment(panel->copyrL, WALeft);
    WMSetLabelText(panel->copyrL, COPYRIGHT_TEXT);
    /* we want the (c) character in the helvetica font */
    font = WMCreateFontInDefaultEncoding(scr->wmscreen, HELVETICA10_FONT);
    if (font) {
	WMSetLabelFont(panel->copyrL, font);
    }

    switch (scr->w_depth) {
     case 15:
	strcpy(version, "32 thousand");
	break;
     case 16:
	strcpy(version, "64 thousand");
	break;
     case 24:
     case 32:
	strcpy(version, "16 million");
	break;
     default:
	sprintf(version, "%d", 1<<scr->w_depth);
	break;
    }

    sprintf(buffer, "Using visual 0x%x: %s %ibpp (%s colors)\n",
	    (unsigned)scr->w_visual->visualid,
	    visuals[scr->w_visual->class], scr->w_depth, version);

    strcat(buffer, "Supported image formats: ");
    strl = RSupportedFileFormats();
    for (i=0; strl[i]!=NULL; i++) {
	strcat(buffer, strl[i]);
	strcat(buffer, " ");
    }
#ifdef WMSOUND
    strcat(buffer, "\nSound support compiled in");
#else
    strcat(buffer, "\nSound support not available");
#endif

    panel->infoL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->infoL, 350, 75);
    WMMoveWidget(panel->infoL, 15, 115);
    WMSetLabelText(panel->infoL, buffer);
    if (font) {
	WMSetLabelFont(panel->infoL, font);
	WMReleaseFont(font);
    }

    
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 382, 230, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    WMMapWidget(panel->win);

    wwin = wManageInternalWindow(scr, parent, None, "Info",
				 (scr->scr_width - 382)/2,
				 (scr->scr_height - 230)/2, 382, 230);

    wwin->window_flags.no_closable = 0;
    wwin->window_flags.no_close_button = 0;
    wWindowUpdateButtonImages(wwin);
    wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
    wwin->frame->on_click_right = destroyInfoPanel;
    
    wWindowMap(wwin);

    panel->wwin = wwin;

    thePanel = panel;
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
    
    WMLabel *licenseL;
} LegalPanel;



#define LICENSE_TEXT \
	"    Window Maker is free software; you can redistribute it and/or modify "\
	"it under the terms of the GNU General Public License as published "\
	"by the Free Software Foundation; either version 2 of the License, "\
	"or (at your option) any later version.\n\n\n"\
	"    Window Maker is distributed in the hope that it will be useful, but "\
	"WITHOUT ANY WARRANTY; without even the implied warranty of "\
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "\
	"General Public License for more details.\n\n\n"\
	"    You should have received a copy of the GNU General Public License "\
	"along with this program; if not, write to the Free Software "\
	"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "\
	"02111-1307, USA."
 

static LegalPanel *legalPanel = NULL;

static void
destroyLegalPanel(WCoreWindow *foo, void *data, XEvent *event)
{
    WMUnmapWidget(legalPanel->win);

    WMDestroyWidget(legalPanel->win);    

    wUnmanageWindow(legalPanel->wwin, False);

    free(legalPanel);
    
    legalPanel = NULL;
}


void
wShowLegalPanel(WScreen *scr)
{
    LegalPanel *panel;
    Window parent;
    WWindow *wwin;

    if (legalPanel) {
	wRaiseFrame(legalPanel->wwin->frame->core);
	wSetFocusTo(scr, legalPanel->wwin);
	return;
    }

    panel = wmalloc(sizeof(LegalPanel));

    panel->scr = scr;
    
    panel->win = WMCreateWindow(scr->wmscreen, "legal");
    WMResizeWidget(panel->win, 420, 250);

    
    panel->licenseL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->licenseL, 400, 230);
    WMMoveWidget(panel->licenseL, 10, 10);
    WMSetLabelTextAlignment(panel->licenseL, WALeft);
    WMSetLabelText(panel->licenseL, LICENSE_TEXT);
    WMSetLabelRelief(panel->licenseL, WRGroove);

    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 420, 250, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, "Legal",
				 (scr->scr_width - 420)/2,
				 (scr->scr_height - 250)/2, 420, 250);

    wwin->window_flags.no_closable = 0;
    wwin->window_flags.no_close_button = 0;
    wWindowUpdateButtonImages(wwin);
    wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
    wwin->frame->on_click_right = destroyLegalPanel;
    
    panel->wwin = wwin;
    
    WMMapWidget(panel->win);

    wWindowMap(wwin);

    legalPanel = panel;
}

