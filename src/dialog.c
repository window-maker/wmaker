/* dialog.c - dialog windows for internal use
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
 *  Copyright (c) 1999       Dan Pascu
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
#include "dialog.h"
#include "funcs.h"
#include "stacking.h"
#include "framewin.h"
#include "window.h"
#include "actions.h"
#include "defaults.h"


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

    WMRunModalLoop(WMWidgetScreen(panel->win), WMWidgetView(panel->win));

    result = panel->result;

    WMUnmapWidget(panel->win);

    wUnmanageWindow(wwin, False, False);

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

    WMRunModalLoop(WMWidgetScreen(panel->win), WMWidgetView(panel->win));

    if (panel->result == WAPRDefault)
	result = WMGetTextFieldText(panel->text);
    else
	result = NULL;
    
    wUnmanageWindow(wwin, False, False);
    
    WMDestroyInputPanel(panel);

    XDestroyWindow(dpy, parent);

    if (result==NULL)
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



static void
listPixmaps(WScreen *scr, WMList *lPtr, char *path)
{
    struct dirent *dentry;
    DIR *dir;
    char pbuf[PATH_MAX+16];
    char *apath;
    IconPanel *panel = WMGetHangedData(lPtr);

    panel->preview = False;
    
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
	wfree(msg);
	wfree(apath);
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
	    WMAddListItem(lPtr, dentry->d_name);
	}
    }
    WMSortListItems(lPtr);

    closedir(dir);
    wfree(apath);
    panel->preview = True;
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
	wfree(tmp);
	WMSetTextFieldText(panel->fileField, path);
	setViewedImage(panel, path);
	wfree(path);
    }
}


static void
listIconPaths(WMList *lPtr)
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
	if (access(tmp, X_OK)==0)
	    WMAddListItem(lPtr, path);
	wfree(tmp);
    } while ((path=strtok(NULL, ":"))!=NULL);

    wfree(paths);
}


void
drawIconProc(WMList *lPtr, int index, Drawable d, char *text,
	     int state, WMRect *rect) 
{
    IconPanel *panel = WMGetHangedData(lPtr);
    GC gc = panel->scr->draw_gc;
    GC copygc = panel->scr->copy_gc;
    char *buffer, *dirfile;
    WMPixmap *pixmap;
    WMColor *blackcolor;
    WMColor *whitecolor;
    WMSize size;
    WMScreen *wmscr=WMWidgetScreen(panel->win);
    int width;

    if(!panel->preview) return;

    width = rect->size.width;

    blackcolor = WMBlackColor(wmscr);
    whitecolor = WMWhiteColor(wmscr);

    dirfile = wexpandpath(WMGetListSelectedItem(panel->dirList)->text);
    buffer = wmalloc(strlen(dirfile)+strlen(text)+4);
    sprintf(buffer, "%s/%s", dirfile, text);
    wfree(dirfile);

    pixmap = WMCreatePixmapFromFile(WMWidgetScreen(panel->win), buffer);
    wfree(buffer);
    if (!pixmap) {
        WMRemoveListItem(lPtr, index);
        return;
    }

    XClearArea(dpy, d, rect->pos.x, rect->pos.y, width, rect->size.height, 
	       False);
    XSetClipMask(dpy, gc, None);
    /*
    XDrawRectangle(dpy, d, WMColorGC(whitecolor), rect->pos.x + 5, 
		   rect->pos.y +5, width - 10, 54);
     */
    XDrawLine(dpy, d, WMColorGC(whitecolor), rect->pos.x,
	      rect->pos.y+rect->size.height-1, rect->pos.x+width, 
	      rect->pos.y+rect->size.height-1);
   
    
    if (state&WLDSSelected) {
        XFillRectangle(dpy, d, WMColorGC(whitecolor), rect->pos.x,
		       rect->pos.y, width, rect->size.height);
    }

    size = WMGetPixmapSize(pixmap);

    XSetClipMask(dpy, copygc, WMGetPixmapMaskXID(pixmap));
    XSetClipOrigin(dpy, copygc, rect->pos.x + (width-size.width)/2,
		   rect->pos.y+2);
    XCopyArea(dpy, WMGetPixmapXID(pixmap), d, copygc, 0, 0,
	      size.width>100?100:size.width, size.height>64?64:size.height, 
	      rect->pos.x + (width-size.width)/2, rect->pos.y+2);

    {
        int i,j;
	int fheight = WMFontHeight(panel->normalfont);
	int tlen = strlen(text);
	int twidth = WMWidthOfString(panel->normalfont, text, tlen);
	int ofx, ofy;
	
	ofx = rect->pos.x + (width - twidth)/2;
	ofy = rect->pos.y + 64 - fheight;
	
        for(i=-1;i<2;i++)
	    for(j=-1;j<2;j++)
		WMDrawString(wmscr, d, WMColorGC(whitecolor), 
			     panel->normalfont, ofx+i, ofy+j,
			     text, tlen);

	WMDrawString(wmscr, d, WMColorGC(blackcolor), panel->normalfont, 
		     ofx, ofy, text, tlen);
    }

    WMReleasePixmap(pixmap);
    /* I hope it is better to do not use cache / on my box it is fast nuff */
    XFlush(dpy);

    WMReleaseColor(blackcolor);
    WMReleaseColor(whitecolor);
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
    } else if (bPtr==panel->previewButton) {
        /**** Previewer ****/
        WMSetButtonEnabled(bPtr, False);
        WMSetListUserDrawItemHeight(panel->iconList, 68);
        WMSetListUserDrawProc(panel->iconList, drawIconProc);
        WMRedisplayWidget(panel->iconList);
        /* for draw proc to access screen/gc */
        /*** end preview ***/
    }
#if 0
    else if (bPtr==panel->chooseButton) {
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


Bool
wIconChooserDialog(WScreen *scr, char **file, char *instance, char *class)
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
    panel->normalfont = WMSystemFontOfSize(WMWidgetScreen(panel->win), 12);
    
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

    WMHangData(panel->iconList,panel);

    panel->previewButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->previewButton, 75, 26);
    WMMoveWidget(panel->previewButton, 365, 130);
    WMSetButtonText(panel->previewButton, _("Preview"));
    WMSetButtonAction(panel->previewButton, buttonCallback, panel);

    panel->iconView = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconView, 75, 75);
    WMMoveWidget(panel->iconView, 365, 40);
    WMSetLabelImagePosition(panel->iconView, WIPOverlaps);
    WMSetLabelRelief(panel->iconView, WRSunken);
    WMSetLabelTextAlignment(panel->iconView, WACenter);

    panel->fileLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->fileLabel, 80, 20);
    WMMoveWidget(panel->fileLabel, 10, 210);
    WMSetLabelText(panel->fileLabel, _("File Name:"));
    
    panel->fileField = WMCreateTextField(panel->win);
    WMResizeWidget(panel->fileField, 345, 20);
    WMMoveWidget(panel->fileField, 95, 210);
    WMSetTextFieldEditable(panel->fileField, False);
    
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

    {
	char *tmp;

	tmp = wmalloc((instance ? strlen(instance) : 0)
		     + (class ? strlen(class) : 0) + 32);

	if (tmp && (instance || class))
	    sprintf(tmp, "%s [%s.%s]", _("Icon Chooser"), instance, class);
	else
	    strcpy(tmp, _("Icon Chooser"));

	wwin = wManageInternalWindow(scr, parent, None, tmp,
				     (scr->scr_width - 450)/2,
				     (scr->scr_height - 280)/2, 450, 280);
	wfree(tmp);
    }
    
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
	    wfree(*file);
	    *file = NULL;
	} else {
	    defaultPath = FindImage(wPreferences.icon_path, *file);
	    wantedPath = WMGetTextFieldText(panel->fileField);
	    /* if the file is not the default, use full path */
	    if (strcmp(wantedPath, defaultPath)!=0) {
		*file = wantedPath;
	    } else {
		*file = wstrdup(*file);
		wfree(wantedPath);
	    }
	    wfree(defaultPath);
	}
    } else {
	*file = NULL;
    }

    WMReleaseFont(panel->normalfont);

    WMUnmapWidget(panel->win);

    WMDestroyWidget(panel->win);    

    wUnmanageWindow(wwin, False, False);

    wfree(panel);

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

#ifdef SILLYNESS
    WMHandlerID timer;
    int cycle;
    RImage *icon;
    RImage *pic;
    WMPixmap *oldPix;
    char *str;
    int x;
#endif
} InfoPanel;



#define COPYRIGHT_TEXT  \
     "Copyright \xa9 1997~2000 Alfredo K. Kojima <kojima@windowmaker.org>\n"\
     "Copyright \xa9 1998~2000 Dan Pascu <dan@windowmaker.org>"

 

static InfoPanel *thePanel = NULL;

static void
destroyInfoPanel(WCoreWindow *foo, void *data, XEvent *event)
{
#ifdef SILLYNESS
    if (thePanel->timer) {
	WMDeleteTimerHandler(thePanel->timer);
    }
    if (thePanel->oldPix) {
	WMReleasePixmap(thePanel->oldPix);
    }
    if (thePanel->icon) {
	RDestroyImage(thePanel->icon);
    }
    if (thePanel->pic) {
	RDestroyImage(thePanel->pic);
    }
#endif /* SILLYNESS */
    WMUnmapWidget(thePanel);

    wUnmanageWindow(thePanel->wwin, False, False);

    WMDestroyWidget(thePanel->win);

    wfree(thePanel);
    
    thePanel = NULL;
}


WMPixmap*
renderText(WMScreen *scr, char *text, char *font, RColor *from, RColor *to)
{
    WMPixmap *wpix = NULL;
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

    mask = XCreatePixmap(dpy, rc->drawable, w, h, 1);
    gc = XCreateGC(dpy, mask, 0, NULL);
    XSetForeground(dpy, gc, 0);
    XSetFont(dpy, gc, f->fid);
    XFillRectangle(dpy, mask, gc, 0, 0, w, h);

    XSetForeground(dpy, gc, 1);
    XDrawString(dpy, mask, gc, 0, f->ascent, text, strlen(text));
    XSetLineAttributes(dpy, gc, 3, LineSolid, CapRound, JoinMiter);
    XDrawLine(dpy, mask, gc, 0, h-2, w, h-2);

    grad = XCreatePixmap(dpy, rc->drawable, w, h, rc->depth);
    {
	WMColor *color;

	color = WMBlackColor(scr);
	XFillRectangle(dpy, grad, WMColorGC(color), 0, 0, w, h);
	WMReleaseColor(color);
    }

    wpix = WMCreatePixmapFromXPixmaps(scr, grad, mask, w, h, rc->depth);

    if (gc)
	XFreeGC(dpy, gc);
    XFreeFont(dpy, f);

    return wpix;
}

#ifdef SILLYNESS

extern WMPixmap *DoXThing();
extern Bool InitXThing();

static void
logoPushCallback(void *data)
{
    InfoPanel *panel = (InfoPanel*)data;
    char buffer[512];
    int i;
    int len;
    static int jingobeu[] = {
	329, 150, -1, 100, 329, 150, -1, 100, 329, 300, -1, 250,
	    329, 150, -1, 100, 329, 150, -1, 100,  329, 300, -1, 250,
	    329, 150, 392, 150, 261, 150, 293, 150, 329, 400, -1, 400, 0
    };
    static int c = 0;

    if (panel->x) {
	XKeyboardControl kc;
    
	if (panel->x > 0) {
	    if(jingobeu[panel->x-1]==0){panel->x=-1;}else if(jingobeu[panel->x
-1]<0){panel->x++;c=jingobeu[panel->x-1]/50;panel->x++;}else if(c==0){
    kc.bell_pitch=jingobeu[panel->x-1];panel->x++;kc.bell_percent=50;c=
	jingobeu[panel->x-1]/50;kc.bell_duration=jingobeu[panel->x-1];panel->x++;
XChangeKeyboardControl(dpy,KBBellPitch|KBBellDuration|KBBellPercent,&kc);
    XBell(dpy,50);XFlush(dpy);}else{c--;}}
	if (!(panel->cycle % 4)) {
	    WMPixmap *p;

	    p = DoXThing(panel->wwin);
	    WMSetLabelImage(panel->logoL, p);
	}
    } else if (panel->cycle < 30) {
	RImage *image;
	WMPixmap *pix;

	image = RCloneImage(panel->icon);
	RCombineImagesWithOpaqueness(image, panel->pic, panel->cycle*255/30);
	pix = WMCreatePixmapFromRImage(panel->scr->wmscreen, image, 128);
	RDestroyImage(image);
	WMSetLabelImage(panel->logoL, pix);
	WMReleasePixmap(pix);
    }

    i = panel->cycle%200;

    len = strlen(panel->str);

    strncpy(buffer, panel->str, i<len ? i : len);
    if (i >= len)
	memset(&buffer[len], ' ', i-len);

    strncpy(buffer, panel->str, i<len ? i : len);
    if (i >= len)
	memset(&buffer[len], ' ', i-len);
    buffer[i]=0;
    WMSetLabelText(panel->versionL, buffer);

    panel->timer = WMAddTimerHandler(50, logoPushCallback, panel);
    panel->cycle++;
}


static void
handleLogoPush(XEvent *event, void *data)
{
    InfoPanel *panel = (InfoPanel*)data;
    static int broken = 0;
    static int clicks = 0;
    static char *pic_data[] = {
"45 45 57 1",
" 	c None",
".	c #000000",
"X	c #383C00",
"o	c #515500",
"O	c #616100",
"+	c #616900",
"@	c #696D00",
"#	c #697100",
"$	c #495100",
"%	c #202800",
"&	c #969600",
"*	c #CFCF00",
"=	c #D7DB00",
"-	c #D7D700",
";	c #C7CB00",
":	c #A6AA00",
">	c #494900",
",	c #8E8E00",
"<	c #DFE700",
"1	c #F7FF00",
"2	c #FFFF00",
"3	c #E7EB00",
"4	c #B6B600",
"5	c #595900",
"6	c #717500",
"7	c #AEB200",
"8	c #CFD300",
"9	c #E7EF00",
"0	c #EFF300",
"q	c #9EA200",
"w	c #F7FB00",
"e	c #F7F700",
"r	c #BEBE00",
"t	c #8E9200",
"y	c #EFF700",
"u	c #969A00",
"i	c #414500",
"p	c #595D00",
"a	c #E7E700",
"s	c #C7C700",
"d	c #797D00",
"f	c #BEC300",
"g	c #DFE300",
"h	c #868600",
"j	c #EFEF00",
"k	c #9E9E00",
"l	c #616500",
"z	c #DFDF00",
"x	c #868A00",
"c	c #969200",
"v	c #B6BA00",
"b	c #A6A600",
"n	c #8E8A00",
"m	c #717100",
"M	c #AEAE00",
"N	c #AEAA00",
"B	c #868200",
"               ...............               ",
"             ....XoO+@##+O$%....             ",
"           ...%X&*========-;;:o...           ",
"         ...>.>,<122222222222134@...         ",
"        ..>5678912222222222222220q%..        ",
"       ..$.&-w2222222222222222222er>..       ",
"      ..O.t31222222222222222222222y4>..      ",
"    ...O5u3222222222222222222222222yri...    ",
"    ..>p&a22222222222222222222222222wso..    ",
"   ..ids91222222222222222222222222222wfi..   ",
"  ..X.7w222222wgs-w2222222213=g0222222<hi..  ",
"  ..Xuj2222222<@X5=222222229k@l:022222y4i..  ",
"  .Xdz22222222*X%.s22222222axo%$-222222<c>.. ",
" ..o7y22222222v...r222222223hX.i82222221si.. ",
"..io*222222222&...u22222222yt..%*22222220:%. ",
"..>k02222222227...f222222222v..X=222222229t. ",
"..dz12222222220ui:y2222222223d%qw222222221g. ",
".%vw222222222221y2222222222219*y2222222222wd.",
".X;2222222222222222222222222222222222222222b.",
".i*2222222222222222222222222222222222222222v.",
".i*2222222222222222222222222222222222222222;.",
".i*22222222222222222222222222222222222222228.",
".>*2222222222222222222222222222222222222222=.",
".i*22222222222222222222222222222222222222228.",
".i*2222222222222222222222222222222222222222;.",
".X*222222222222222222222222222222we12222222r.",
".Xs12222222w3aw22222222222222222y8s0222222wk.",
".Xq02222222a,na22222222222222222zm6zwy2222gi.",
"..>*22222y<:Xcj22222222222222222-o$k;;02228..",
"..i7y2220rhX.:y22222222222222222jtiXd,a220,..",
" .X@z222a,do%kj2222222222222222wMX5q;gw228%..",
" ..58222wagsh6ry222222222222221;>Of0w222y:...",
" ...:e2222218mdz22222222222222a&$vw222220@...",
" ...O-122222y:.u02222222222229q$uj222221r... ",
"  ..%&a1222223&573w2222222219NOxz122221z>... ",
"   ...t3222221-l$nr8ay1222yzbo,=12222w-5...  ",
"    ..X:022222w-k+>o,7s**s7xOn=12221<f5...   ",
"     ..o:9222221j8:&Bl>>>>ihv<12221=dX...    ",
"      ..Xb9122222109g-****;<y22221zn%...     ",
"       ..X&801222222222222222222w-h....      ",
"        ...o:=022222222222222221=lX...       ",
"          ..X@:;3w2222222222210fO...         ",
"           ...XX&v8<30000003-N@...           ",
"             .....XmnbN:q&Bo....             ",
"                 ............                "};
    static char *msgs[] = {
	    "Have a nice day!"
    };


    clicks++;
    if (!panel->timer && !broken && clicks > 0) {
	char *file;
	char *path;

	panel->x = 0;
	clicks = 0;
	if (!panel->icon) {
	    file = wDefaultGetIconFile(panel->scr, "Logo", "WMPanel", False);
	    if (!file) {
		broken = 1;
		return;
	    }
    
	    path = FindImage(wPreferences.icon_path, file);
	    if (!path) {
		broken = 1;
		return;
	    }

	    panel->icon = RLoadImage(panel->scr->rcontext, path, 0);
	    wfree(path);
	    if (!panel->icon) {
		broken = 1;
		return;
	    }
	}
	if (!panel->pic) {
	    panel->pic = RGetImageFromXPMData(panel->scr->rcontext, pic_data);
	    if (!panel->pic || panel->icon->width!=panel->pic->width
		|| panel->icon->height!=panel->pic->height) {
		broken = 1;
		RDestroyImage(panel->icon);
		panel->icon = NULL;
		if (panel->pic) {
		    RDestroyImage(panel->pic);
		    panel->pic = NULL;
		}
		return;
	    }

	    {
		RColor color;
		color.red = 0xae;
		color.green = 0xaa;
		color.blue = 0xae;
		color.alpha = 255;
		RCombineImageWithColor(panel->icon, &color);
		RCombineImageWithColor(panel->pic, &color);
	    }
	}

	panel->str = msgs[rand()%(sizeof(msgs)/sizeof(char*))];

	panel->timer = WMAddTimerHandler(50, logoPushCallback, panel);
	panel->cycle = 0;
	panel->oldPix = WMRetainPixmap(WMGetLabelImage(panel->logoL));
    } else if (panel->timer) {
	char version[20];

	panel->x = 0;
	clicks = 0;
	WMSetLabelImage(panel->logoL, panel->oldPix);
	WMReleasePixmap(panel->oldPix);
	panel->oldPix = NULL;
	
	WMDeleteTimerHandler(panel->timer);
	panel->timer = NULL;

	sprintf(version, "Version %s", VERSION);
	WMSetLabelText(panel->versionL, version);
    }

    {
	XEvent ev;
	while (XCheckTypedWindowEvent(dpy, WMWidgetXID(panel->versionL),
				      ButtonPress, &ev));
    }
}
#endif /* SILLYNESS */


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
	if (thePanel->scr == scr) {
	    wRaiseFrame(thePanel->wwin->frame->core);
	    wSetFocusTo(scr, thePanel->wwin);
	}
	return;
    }
    
    panel = wmalloc(sizeof(InfoPanel));
    memset(panel, 0, sizeof(InfoPanel));

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
#ifdef SILLYNESS
	WMCreateEventHandler(WMWidgetView(panel->logoL), ButtonPressMask,
			     handleLogoPush, panel);
#endif
    }

    panel->name1L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name1L, 240, 30);
    WMMoveWidget(panel->name1L, 100, 30);
    color1.red = 0;
    color1.green = 0;
    color1.blue = 0;
    color2.red = 0x50;
    color2.green = 0x50;
    color2.blue = 0x70;
    logo = renderText(scr->wmscreen, "GNU Window Maker",
		      "-*-utopia-*-r-*-*-25-*", &color1, &color2);
    if (logo) {
	WMSetLabelImagePosition(panel->name1L, WIPImageOnly);
	WMSetLabelImage(panel->name1L, logo);
	WMReleasePixmap(logo);
    } else {
	font = WMBoldSystemFontOfSize(scr->wmscreen, 20);
	if (font) {
	    WMSetLabelFont(panel->name1L, font);
	    WMReleaseFont(font);
	}
	WMSetLabelTextAlignment(panel->name1L, WACenter);
	WMSetLabelText(panel->name1L, "GNU Window Maker");
    }

    panel->name2L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name2L, 240, 24);
    WMMoveWidget(panel->name2L, 100, 60);
    font = WMBoldSystemFontOfSize(scr->wmscreen, 18);
    if (font) {
	WMSetLabelFont(panel->name2L, font);
	WMReleaseFont(font);
	font = NULL;
    }
    WMSetLabelTextAlignment(panel->name2L, WACenter);
    WMSetLabelText(panel->name2L, "Window Manager for X");

    
    sprintf(version, "Version %s", VERSION);
    panel->versionL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->versionL, 310, 16);
    WMMoveWidget(panel->versionL, 30, 95);
    WMSetLabelTextAlignment(panel->versionL, WARight);
    WMSetLabelText(panel->versionL, version);
    WMSetLabelWraps(panel->versionL, False);

    panel->copyrL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->copyrL, 340, 40);
    WMMoveWidget(panel->copyrL, 15, 185);
    WMSetLabelTextAlignment(panel->copyrL, WALeft);
    WMSetLabelText(panel->copyrL, COPYRIGHT_TEXT);
    /* we want the (c) character in the helvetica font */
    font = WMCreateNormalFont(scr->wmscreen, HELVETICA10_FONT);
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

#if defined(HAVE_MALLOC_H) && defined(HAVE_MALLINFO)
    {
	struct mallinfo ma = mallinfo();
	sprintf(buffer+strlen(buffer), 
		"Total allocated memory: %i kB. Total memory in use: %i kB.\n",
		(ma.arena+ma.hblkhd)/1024, (ma.uordblks+ma.hblkhd)/1024);
	
    }
#endif
    
    strcat(buffer, "Supported image formats: ");
    strl = RSupportedFileFormats();
    for (i=0; strl[i]!=NULL; i++) {
	strcat(buffer, strl[i]);
	strcat(buffer, " ");
    }

    strcat(buffer, "\nAdditional Support For: ");
    {
	char *list[8];
	char buf[80];
	int j = 0;

#ifdef MWM_HINTS
	list[j++] = "MWM";
#endif
#ifdef KWM_HINTS
	list[j++] = "KDE";
#endif
#ifdef GNOME_STUFF
	list[j++] = "GNOME";
#endif
#ifdef OLWM_HINTS
	list[j++] = "OLWM";
#endif
#ifdef WSOUND
	list[j++] = "Sound";
#endif

	buf[0] = 0;
	for (i = 0; i < j; i++) {
	    if (i > 0) {
		if (i == j - 1)
		    strcat(buf, " and ");
		else
		    strcat(buf, ", ");
	    }
	    strcat(buf, list[i]);
	}
	strcat(buffer, buf);
    }


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

    thePanel = panel;
#ifdef SILLYNESS
    if (InitXThing(panel->scr)) {	
	panel->timer = WMAddTimerHandler(100, logoPushCallback, panel);
	panel->cycle = 0;
	panel->x = 1;
	panel->str = "Merry Christmas!";
	panel->oldPix = WMRetainPixmap(WMGetLabelImage(panel->logoL));
    } 
#endif
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

    wUnmanageWindow(legalPanel->wwin, False, False);

    wfree(legalPanel);
    
    legalPanel = NULL;
}


void
wShowLegalPanel(WScreen *scr)
{
    LegalPanel *panel;
    Window parent;
    WWindow *wwin;

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
    WMResizeWidget(panel->win, 420, 250);

    
    panel->licenseL = WMCreateLabel(panel->win);
    WMSetLabelWraps(panel->licenseL, True);
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

extern WDDomain *WDWindowAttributes;


typedef struct _CrashPanel {
    WMWindow *win;            /* main window */

    WMLabel *iconL;           /* application icon */
    WMLabel *nameL;           /* title of panel */

    WMFrame *sepF;            /* separator frame */

    WMLabel *noteL;           /* Title of note */
    WMLabel *note2L;          /* body of note with what happened */

    WMFrame *whatF;           /* "what to do next" frame */
    WMPopUpButton *whatP;     /* action selection popup button */

    WMButton *okB;            /* ok button */

    Bool done;                /* if finished with this dialog */
    int action;               /* what to do after */

    KeyCode retKey;

} CrashPanel;


static void
handleKeyPress(XEvent *event, void *clientData)
{
    CrashPanel *panel = (CrashPanel*)clientData;

    if (event->xkey.keycode == panel->retKey) {
	WMPerformButtonClick(panel->okB);
    }
}


static void
okButtonCallback(void *self, void *clientData)
{
    CrashPanel *panel = (CrashPanel*)clientData;

    panel->done = True;
}


static void
setCrashAction(void *self, void *clientData)
{
    WMPopUpButton *pop = (WMPopUpButton*)self;
    CrashPanel *panel = (CrashPanel*)clientData;

    panel->action = WMGetPopUpButtonSelectedItem(pop);
}


static WMPixmap*
getWindowMakerIconImage(WMScreen *scr)
{
    proplist_t dict, key, option, value=NULL;
    WMPixmap *pix=NULL;
    char *path;

    PLSetStringCmpHook(NULL);

    key = PLMakeString("Logo.WMPanel");
    option = PLMakeString("Icon");

    dict = PLGetDictionaryEntry(WDWindowAttributes->dictionary, key);

    if (dict) {
        value = PLGetDictionaryEntry(dict, option);
    }

    PLRelease(key);
    PLRelease(option);

    PLSetStringCmpHook(StringCompareHook);

    if (value && PLIsString(value)) {
        path = FindImage(wPreferences.icon_path, PLGetString(value));

        if (path) {
            RImage *image;

            image = RLoadImage(WMScreenRContext(scr), path, 0);
            if (image) {
                pix = WMCreatePixmapFromRImage(scr, image, 0);
                RDestroyImage(image);
            }
            wfree(path);
        }
    }

    return pix;
}


#define PWIDTH	295
#define PHEIGHT	345


int
wShowCrashingDialogPanel(int whatSig)
{
    CrashPanel *panel;
    WMScreen *scr;
    WMFont *font;
    WMPixmap *logo;
    int screen_no, scr_width, scr_height;
    int action;
    char buf[256];

    panel = wmalloc(sizeof(CrashPanel));
    memset(panel, 0, sizeof(CrashPanel));

    screen_no = DefaultScreen(dpy);
    scr_width = WidthOfScreen(ScreenOfDisplay(dpy, screen_no));
    scr_height = HeightOfScreen(ScreenOfDisplay(dpy, screen_no));

    scr = WMCreateScreen(dpy, screen_no);
    if (!scr) {
        wsyserror(_("cannot open connection for crashing dialog panel. Aborting."));
	return WMAbort;
    }

    panel->retKey = XKeysymToKeycode(dpy, XK_Return);

    panel->win = WMCreateWindow(scr, "crashingDialog");
    WMResizeWidget(panel->win, PWIDTH, PHEIGHT);
    WMMoveWidget(panel->win, (scr_width - PWIDTH)/2, (scr_height - PHEIGHT)/2);

    logo = getWindowMakerIconImage(scr);
    if (logo) {
        panel->iconL = WMCreateLabel(panel->win);
        WMResizeWidget(panel->iconL, 64, 64);
        WMMoveWidget(panel->iconL, 10, 10);
        WMSetLabelImagePosition(panel->iconL, WIPImageOnly);
        WMSetLabelImage(panel->iconL, logo);
    }

    panel->nameL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->nameL, 190, 18);
    WMMoveWidget(panel->nameL, 80, 35);
    WMSetLabelTextAlignment(panel->nameL, WALeft);
    font = WMBoldSystemFontOfSize(scr, 18);
    WMSetLabelFont(panel->nameL, font);
    WMReleaseFont(font);
    WMSetLabelText(panel->nameL, _("Fatal error"));

    panel->sepF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->sepF, PWIDTH+4, 2);
    WMMoveWidget(panel->sepF, -2, 80);

    panel->noteL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->noteL, PWIDTH-20, 40);
    WMMoveWidget(panel->noteL, 10, 90);
    WMSetLabelTextAlignment(panel->noteL, WAJustified);
#ifdef SYS_SIGLIST_DECLARED
    sprintf(buf, _("Window Maker received signal %i\n(%s)."),
            whatSig, sys_siglist[whatSig]);
#else
    sprintf(buf, _("Window Maker received signal %i."), whatSig);
#endif
    WMSetLabelText(panel->noteL, buf);

    panel->note2L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->note2L, PWIDTH-20, 100);
    WMMoveWidget(panel->note2L, 10, 130);
    WMSetLabelTextAlignment(panel->note2L, WALeft);
    WMSetLabelText(panel->note2L,
                   _(" This fatal error occured probably due to a bug."
                   " Please fill the included BUGFORM and "
                   "report it to bugs@windowmaker.org."));


    panel->whatF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->whatF, PWIDTH-20, 50);
    WMMoveWidget(panel->whatF, 10, 240);
    WMSetFrameTitle(panel->whatF, _("What do you want to do now?"));

    panel->whatP = WMCreatePopUpButton(panel->whatF);
    WMResizeWidget(panel->whatP, PWIDTH-20-70, 20);
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

    WMCreateEventHandler(WMWidgetView(panel->win), KeyPressMask,
                         handleKeyPress, panel);

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




/*****************************************************************************
 *			About GNUstep Panel
 *****************************************************************************/


static void
drawGNUstepLogo(Display *dpy, Drawable d, int width, int height,
		unsigned long blackPixel, unsigned long whitePixel)
{
    GC gc;
    XGCValues gcv;
    XRectangle rects[3];

    gcv.foreground = blackPixel;
    gc = XCreateGC(dpy, d, GCForeground, &gcv);

    XFillArc(dpy, d, gc, width/45, height/45, 
	     width - 2*width/45, height - 2*height/45, 0, 360*64);

    rects[0].x = 0;
    rects[0].y = 37*height/45;
    rects[0].width = width/3;
    rects[0].height = height - rects[0].y;

    rects[1].x = rects[0].width;
    rects[1].y = height/2;
    rects[1].width = width - 2*width/3;
    rects[1].height = height - rects[1].y;

    rects[2].x = 2*width/3;
    rects[2].y = height - 37*height/45;
    rects[2].width = width/3;
    rects[2].height = height - rects[2].y;

    XSetClipRectangles(dpy, gc, 0, 0, rects, 3, Unsorted);
    XFillRectangle(dpy, d, gc, 0, 0, width, height);

    XSetForeground(dpy, gc, whitePixel);
    XFillArc(dpy, d, gc, width/45, height/45, 
	     width - 2*width/45, height - 2*height/45, 0, 360*64);

    XFreeGC(dpy, gc);
}



#define GNUSTEP_TEXT \
	"Window Maker is part of the GNUstep project.\n"\
	"The GNUstep project aims to create a free\n"\
	"implementation of the OpenStep(tm) specification\n"\
	"which is a object-oriented framework for\n"\
	"creating advanced graphical, multi-platform\n"\
	"applications. Additionally, a development and\n"\
	"user desktop enviroment will be created on top\n"\
	"of the framework. For more information about\n"\
	"GNUstep, please visit: www.gnustep.org"


typedef struct {
    WScreen *scr;
    
    WWindow *wwin;

    WMWindow *win;

    WMLabel *gstepL;
    WMLabel *textL;
} GNUstepPanel;


static GNUstepPanel *gnustepPanel = NULL;

static void
destroyGNUstepPanel(WCoreWindow *foo, void *data, XEvent *event)
{
    WMUnmapWidget(gnustepPanel->win);

    WMDestroyWidget(gnustepPanel->win);    

    wUnmanageWindow(gnustepPanel->wwin, False, False);

    wfree(gnustepPanel);

    gnustepPanel = NULL;
}


void
wShowGNUstepPanel(WScreen *scr)
{
    GNUstepPanel *panel;
    Window parent;
    WWindow *wwin;
    WMPixmap *pixmap;
    WMColor *color;

    if (gnustepPanel) {
	if (gnustepPanel->scr == scr) {
	    wRaiseFrame(gnustepPanel->wwin->frame->core);
	    wSetFocusTo(scr, gnustepPanel->wwin);
	}
	return;
    }

    panel = wmalloc(sizeof(GNUstepPanel));

    panel->scr = scr;

    panel->win = WMCreateWindow(scr->wmscreen, "About GNUstep");
    WMResizeWidget(panel->win, 325, 200);

    pixmap = WMCreatePixmap(scr->wmscreen, 130, 130, 
			    WMScreenDepth(scr->wmscreen), True);

    color = WMCreateNamedColor(scr->wmscreen, "gray50", True);

    drawGNUstepLogo(dpy, WMGetPixmapXID(pixmap), 130, 130,
		    WMColorPixel(color), scr->white_pixel);

    WMReleaseColor(color);

    XSetForeground(dpy, scr->mono_gc, 0);
    XFillRectangle(dpy, WMGetPixmapMaskXID(pixmap), scr->mono_gc, 0, 0, 
		   130, 130);
    drawGNUstepLogo(dpy, WMGetPixmapMaskXID(pixmap), 130, 130, 1, 1);

    panel->gstepL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->gstepL, 285, 64);
    WMMoveWidget(panel->gstepL, 20, 0);
    WMSetLabelTextAlignment(panel->gstepL, WARight);
    WMSetLabelText(panel->gstepL, "GNUstep");
    {
	WMFont *font = WMBoldSystemFontOfSize(scr->wmscreen, 24);

	WMSetLabelFont(panel->gstepL, font);
	WMReleaseFont(font);
    }

    panel->textL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->textL, 275, 130);
    WMMoveWidget(panel->textL, 30, 50);
    WMSetLabelTextAlignment(panel->textL, WARight);
    WMSetLabelImagePosition(panel->textL, WIPOverlaps);
    WMSetLabelText(panel->textL, GNUSTEP_TEXT);
    WMSetLabelImage(panel->textL, pixmap);

    WMReleasePixmap(pixmap);

    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 325, 200, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, "About GNUstep",
				 (scr->scr_width - 325)/2,
				 (scr->scr_height - 200)/2, 325, 200);

    WSETUFLAG(wwin, no_closable, 0);
    WSETUFLAG(wwin, no_close_button, 0);
    wWindowUpdateButtonImages(wwin);
    wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
#ifdef XKB_BUTTON_HINT
    wFrameWindowHideButton(wwin->frame, WFF_LANGUAGE_BUTTON);
#endif
    wwin->frame->on_click_right = destroyGNUstepPanel;
    
    panel->wwin = wwin;
    
    WMMapWidget(panel->win);

    wWindowMap(wwin);

    gnustepPanel = panel;
}
