/* WPrefs.c- main window and other basic stuff
 *
 *  WPrefs - Window Maker Preferences Program
 *
 *  Copyright (c) 1998-2003 Alfredo K. Kojima
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


#include "WPrefs.h"
#include <assert.h>


extern Panel *InitWindowHandling(WMScreen *scr, WMWidget *parent);

extern Panel *InitKeyboardSettings(WMScreen *scr, WMWidget *parent);

extern Panel *InitMouseSettings(WMScreen *scr, WMWidget *parent);

extern Panel *InitKeyboardShortcuts(WMScreen *scr, WMWidget *parent);

extern Panel *InitWorkspace(WMScreen *scr, WMWidget *parent);

extern Panel *InitFocus(WMScreen *scr, WMWidget *parent);

extern Panel *InitPreferences(WMScreen *scr, WMWidget *parent);

extern Panel *InitFont(WMScreen *scr, WMWidget *parent);

extern Panel *InitConfigurations(WMScreen *scr, WMWidget *parent);

extern Panel *InitPaths(WMScreen *scr, WMWidget *parent);

extern Panel *InitMenu(WMScreen *scr, WMWidget *parent);

extern Panel *InitExpert(WMScreen *scr, WMWidget *parent);

extern Panel *InitMenuPreferences(WMScreen *scr, WMWidget *parent);

extern Panel *InitIcons(WMScreen *scr, WMWidget *parent);

extern Panel *InitThemes(WMScreen *scr, WMWidget *parent);

extern Panel *InitAppearance(WMScreen *scr, WMWidget *parent);




#define ICON_TITLE_FONT "-adobe-helvetica-bold-r-*-*-10-*"
#define ICON_TITLE_VFONT "-adobe-helvetica-bold-r-*-*-10-[]-*"


#define MAX_SECTIONS 16


typedef struct _WPrefs {
    WMWindow *win;

    WMScrollView *scrollV;
    WMFrame *buttonF;
    WMButton *sectionB[MAX_SECTIONS];

    int sectionCount;

    WMButton *saveBtn;
    WMButton *closeBtn;
    WMButton *undoBtn;
    WMButton *undosBtn;

    WMButton *balloonBtn;

    WMFrame *banner;
    WMLabel *nameL;
    WMLabel *versionL;
    WMLabel *creditsL;
    WMLabel *statusL;

    Panel *currentPanel;
} _WPrefs;


static _WPrefs WPrefs;

/* system wide defaults dictionary. Read-only */
static WMPropList *GlobalDB = NULL;
/* user defaults dictionary */
static WMPropList *WindowMakerDB = NULL;
static char *WindowMakerDBPath = NULL;


static Bool TIFFOK = False;


#define INITIALIZED_PANEL	(1<<0)




static void loadConfigurations(WMScreen *scr, WMWindow *mainw);

static void savePanelData(Panel *panel);

static void prepareForClose();

void
quit(WMWidget *w, void *data)
{
    prepareForClose();

    exit(0);
}


static void
save(WMWidget *w, void *data)
{
    int i;
    WMPropList *p1, *p2;
    WMPropList *keyList;
    WMPropList *key;
    char *msg = "Reconfigure";
    XEvent ev;


    /*    puts("gathering data");*/
    for (i=0; i<WPrefs.sectionCount; i++) {
        PanelRec *rec = WMGetHangedData(WPrefs.sectionB[i]);
        if ((rec->callbacks.flags & INITIALIZED_PANEL))
            savePanelData((Panel*)rec);
    }
    /*    puts("compressing data");*/
    /* compare the user dictionary with the global and remove redundant data */
    keyList = WMGetPLDictionaryKeys(GlobalDB);
    /*    puts(WMGetPropListDescription(WindowMakerDB, False));*/
    for (i=0; i<WMGetPropListItemCount(keyList); i++) {
        key = WMGetFromPLArray(keyList, i);

        /* We don't have this value anyway, so no problem.
         * Probably  a new option */
        p1 = WMGetFromPLDictionary(WindowMakerDB, key);
        if (!p1)
            continue;
        /* The global doesn't have it, so no problem either. */
        p2 = WMGetFromPLDictionary(GlobalDB, key);
        if (!p2)
            continue;
        /* If both values are the same, don't save. */
        if (WMIsPropListEqualTo(p1, p2))
            WMRemoveFromPLDictionary(WindowMakerDB, key);
    }
    /*    puts(WMGetPropListDescription(WindowMakerDB, False));*/
    WMReleasePropList(keyList);
    /*    puts("storing data");*/

    WMWritePropListToFile(WindowMakerDB, WindowMakerDBPath, True);


    memset(&ev, 0, sizeof(XEvent));

    ev.xclient.type = ClientMessage;
    ev.xclient.message_type = XInternAtom(WMScreenDisplay(WMWidgetScreen(w)),
                                          "_WINDOWMAKER_COMMAND", False);
    ev.xclient.window = DefaultRootWindow(WMScreenDisplay(WMWidgetScreen(w)));
    ev.xclient.format = 8;

    for (i = 0; i <= strlen(msg); i++) {
        ev.xclient.data.b[i] = msg[i];
    }
    XSendEvent(WMScreenDisplay(WMWidgetScreen(w)),
               DefaultRootWindow(WMScreenDisplay(WMWidgetScreen(w))),
               False, SubstructureRedirectMask, &ev);
    XFlush(WMScreenDisplay(WMWidgetScreen(w)));
}



static void
undo(WMWidget *w, void *data)
{
    PanelRec *rec = (PanelRec*)WPrefs.currentPanel;

    if (!rec)
        return;

    if (rec->callbacks.undoChanges
        && (rec->callbacks.flags & INITIALIZED_PANEL)) {
        (*rec->callbacks.undoChanges)(WPrefs.currentPanel);
    }
}


static void
undoAll(WMWidget *w, void *data)
{
    int i;

    for (i=0; i<WPrefs.sectionCount; i++) {
        PanelRec *rec = WMGetHangedData(WPrefs.sectionB[i]);

        if (rec->callbacks.undoChanges
            && (rec->callbacks.flags & INITIALIZED_PANEL))
            (*rec->callbacks.undoChanges)((Panel*)rec);
    }
}



static void
prepareForClose()
{
    int i;

    for (i=0; i<WPrefs.sectionCount; i++) {
        PanelRec *rec = WMGetHangedData(WPrefs.sectionB[i]);

        if (rec->callbacks.prepareForClose
            && (rec->callbacks.flags & INITIALIZED_PANEL))
            (*rec->callbacks.prepareForClose)((Panel*)rec);
    }
}


void
toggleBalloons(WMWidget *w, void *data)
{
    WMUserDefaults *udb = WMGetStandardUserDefaults();
    Bool flag;

    flag = WMGetButtonSelected(WPrefs.balloonBtn);

    WMSetBalloonEnabled(WMWidgetScreen(WPrefs.win), flag);

    WMSetUDBoolForKey(udb, flag, "BalloonHelp");
}


static void
createMainWindow(WMScreen *scr)
{
    WMScroller *scroller;
    WMFont *font;
    char buffer[128];

    WPrefs.win = WMCreateWindow(scr, "wprefs");
    WMResizeWidget(WPrefs.win, 520, 390);
    WMSetWindowTitle(WPrefs.win, _("Window Maker Preferences"));
    WMSetWindowCloseAction(WPrefs.win, quit, NULL);
    WMSetWindowMaxSize(WPrefs.win, 520, 390);
    WMSetWindowMinSize(WPrefs.win, 520, 390);
    WMSetWindowMiniwindowTitle(WPrefs.win, "Preferences");
    WMSetWindowMiniwindowPixmap(WPrefs.win, WMGetApplicationIconPixmap(scr));

    WPrefs.scrollV = WMCreateScrollView(WPrefs.win);
    WMResizeWidget(WPrefs.scrollV, 500, 87);
    WMMoveWidget(WPrefs.scrollV, 10, 10);
    WMSetScrollViewRelief(WPrefs.scrollV, WRSunken);
    WMSetScrollViewHasHorizontalScroller(WPrefs.scrollV, True);
    WMSetScrollViewHasVerticalScroller(WPrefs.scrollV, False);
    scroller = WMGetScrollViewHorizontalScroller(WPrefs.scrollV);
    WMSetScrollerArrowsPosition(scroller, WSANone);

    WPrefs.buttonF = WMCreateFrame(WPrefs.win);
    WMSetFrameRelief(WPrefs.buttonF, WRFlat);

    WMSetScrollViewContentView(WPrefs.scrollV, WMWidgetView(WPrefs.buttonF));

    WPrefs.undosBtn = WMCreateCommandButton(WPrefs.win);
    WMResizeWidget(WPrefs.undosBtn, 90, 28);
    WMMoveWidget(WPrefs.undosBtn, 135, 350);
    WMSetButtonText(WPrefs.undosBtn, _("Revert Page"));
    WMSetButtonAction(WPrefs.undosBtn, undo, NULL);

    WPrefs.undoBtn = WMCreateCommandButton(WPrefs.win);
    WMResizeWidget(WPrefs.undoBtn, 90, 28);
    WMMoveWidget(WPrefs.undoBtn, 235, 350);
    WMSetButtonText(WPrefs.undoBtn, _("Revert All"));
    WMSetButtonAction(WPrefs.undoBtn, undoAll, NULL);

    WPrefs.saveBtn = WMCreateCommandButton(WPrefs.win);
    WMResizeWidget(WPrefs.saveBtn, 80, 28);
    WMMoveWidget(WPrefs.saveBtn, 335, 350);
    WMSetButtonText(WPrefs.saveBtn, _("Save"));
    WMSetButtonAction(WPrefs.saveBtn, save, NULL);

    WPrefs.closeBtn = WMCreateCommandButton(WPrefs.win);
    WMResizeWidget(WPrefs.closeBtn, 80, 28);
    WMMoveWidget(WPrefs.closeBtn, 425, 350);
    WMSetButtonText(WPrefs.closeBtn, _("Close"));
    WMSetButtonAction(WPrefs.closeBtn, quit, NULL);


    WPrefs.balloonBtn = WMCreateSwitchButton(WPrefs.win);
    WMResizeWidget(WPrefs.balloonBtn, 200, 28);
    WMMoveWidget(WPrefs.balloonBtn, 15, 350);
    WMSetButtonText(WPrefs.balloonBtn, _("Balloon Help"));
    WMSetButtonAction(WPrefs.balloonBtn, toggleBalloons, NULL);
    {
        WMUserDefaults *udb = WMGetStandardUserDefaults();
        Bool flag = WMGetUDBoolForKey(udb, "BalloonHelp");

        WMSetButtonSelected(WPrefs.balloonBtn, flag);
        WMSetBalloonEnabled(scr, flag);
    }

    /* banner */
    WPrefs.banner = WMCreateFrame(WPrefs.win);
    WMResizeWidget(WPrefs.banner, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(WPrefs.banner, FRAME_LEFT, FRAME_TOP);
    WMSetFrameRelief(WPrefs.banner, WRFlat);

    font = WMCreateFont(scr, "-*-times-bold-r-*-*-24-*-*-*-*-*-*-*,"
                        "-*-fixed-medium-r-normal-*-24-*");
    if (!font)
        font = WMBoldSystemFontOfSize(scr, 24);
    WPrefs.nameL = WMCreateLabel(WPrefs.banner);
    WMSetLabelTextAlignment(WPrefs.nameL, WACenter);
    WMResizeWidget(WPrefs.nameL, FRAME_WIDTH-20, 30);
    WMMoveWidget(WPrefs.nameL, 10, 25);
    WMSetLabelFont(WPrefs.nameL, font);
    WMSetLabelText(WPrefs.nameL, _("Window Maker Preferences Utility"));
    WMReleaseFont(font);

    WPrefs.versionL = WMCreateLabel(WPrefs.banner);
    WMResizeWidget(WPrefs.versionL, FRAME_WIDTH-20, 20);
    WMMoveWidget(WPrefs.versionL, 10, 65);
    WMSetLabelTextAlignment(WPrefs.versionL, WACenter);
    sprintf(buffer, _("Version %s for Window Maker %s or newer"), WVERSION,
            WMVERSION);
    WMSetLabelText(WPrefs.versionL, buffer);

    WPrefs.statusL = WMCreateLabel(WPrefs.banner);
    WMResizeWidget(WPrefs.statusL, FRAME_WIDTH-20, 60);
    WMMoveWidget(WPrefs.statusL, 10, 100);
    WMSetLabelTextAlignment(WPrefs.statusL, WACenter);
    WMSetLabelText(WPrefs.statusL, _("Starting..."));

    WPrefs.creditsL = WMCreateLabel(WPrefs.banner);
    WMResizeWidget(WPrefs.creditsL, FRAME_WIDTH-20, 60);
    WMMoveWidget(WPrefs.creditsL, 10, FRAME_HEIGHT-60);
    WMSetLabelTextAlignment(WPrefs.creditsL, WACenter);
    WMSetLabelText(WPrefs.creditsL, _("Programming/Design: Alfredo K. Kojima\n"
                                      "Artwork: Marco van Hylckama Vlieg, Largo et al\n"
                                      "More Programming: James Thompson et al"));


    WMMapSubwidgets(WPrefs.win);

    WMUnmapWidget(WPrefs.undosBtn);
    WMUnmapWidget(WPrefs.undoBtn);
    WMUnmapWidget(WPrefs.saveBtn);
}


static void
showPanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;

    if (!(rec->callbacks.flags & INITIALIZED_PANEL)) {
        (*rec->callbacks.createWidgets)(panel);
        rec->callbacks.flags |= INITIALIZED_PANEL;
    }

    WMSetWindowTitle(WPrefs.win, rec->sectionName);

    if (rec->callbacks.showPanel)
        (*rec->callbacks.showPanel)(panel);

    WMMapWidget(rec->box);
}



static void
hidePanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;

    WMUnmapWidget(rec->box);

    if (rec->callbacks.hidePanel)
        (*rec->callbacks.hidePanel)(panel);
}


static void
savePanelData(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;

    if (rec->callbacks.updateDomain) {
        (*rec->callbacks.updateDomain)(panel);
    }
}


static void
changeSection(WMWidget *self, void *data)
{
    if (WPrefs.currentPanel == data)
        return;

    if (WPrefs.currentPanel == NULL) {
        WMDestroyWidget(WPrefs.nameL);
        WMDestroyWidget(WPrefs.creditsL);
        WMDestroyWidget(WPrefs.versionL);
        WMDestroyWidget(WPrefs.statusL);

        WMSetFrameRelief(WPrefs.banner, WRGroove);

        /*	WMMapWidget(WPrefs.undosBtn);
         WMMapWidget(WPrefs.undoBtn);
         */
        WMMapWidget(WPrefs.saveBtn);
    }

    showPanel(data);

    if (WPrefs.currentPanel)
        hidePanel(WPrefs.currentPanel);
    WPrefs.currentPanel = data;
}



char*
LocateImage(char *name)
{
    char *path;
    char *tmp = wmalloc(strlen(name)+8);

    if (TIFFOK) {
        sprintf(tmp, "%s.tiff", name);
        path = WMPathForResourceOfType(tmp, "tiff");
    } else {
        sprintf(tmp, "%s.xpm", name);
        path = WMPathForResourceOfType(tmp, "xpm");
    }
    wfree(tmp);
    if (!path) {
        wwarning(_("could not locate image file %s\n"), name);
    }

    return path;
}



static WMPixmap*
makeTitledIcon(WMScreen *scr, WMPixmap *icon, char *title1, char *title2)
{
    return WMRetainPixmap(icon);

#if 0
    static GC gc = NULL;
    static XFontStruct *hfont = NULL;
    static XFontStruct *vfont = NULL;
    WMPixmap *tmp;
    Pixmap pix, mask;
    Display *dpy = WMScreenDisplay(scr);
    WMColor *black = WMBlackColor(scr);
    GC fgc;
    WMSize size = WMGetPixmapSize(icon);


    tmp = WMCreatePixmap(scr, 60, 60, WMScreenDepth(scr), True);

    pix = WMGetPixmapXID(tmp);
    mask = WMGetPixmapMaskXID(tmp);

    if (gc == NULL) {
        gc = XCreateGC(dpy, mask, 0, NULL);

        hfont = XLoadQueryFont(dpy, ICON_TITLE_FONT);
        vfont = XLoadQueryFont(dpy, ICON_TITLE_VFONT);
    }

    if (hfont == NULL) {
        return WMRetainPixmap(icon);
    }

    XSetForeground(dpy, gc, 0);
    XFillRectangle(dpy, mask, gc, 0, 0, 60, 60);

    fgc = WMColorGC(black);

    XSetForeground(dpy, gc, 1);

    XCopyArea(dpy, WMGetPixmapXID(icon), pix, fgc, 0, 0,
              size.width, size.height, 12, 12);

    if (WMGetPixmapMaskXID(icon) != None)
        XCopyPlane(dpy, WMGetPixmapMaskXID(icon), mask, gc, 0, 0,
                   size.width, size.height, 12, 12, 1);
    else
        XFillRectangle(dpy, mask, gc, 12, 12, 48, 48);


    if (title1) {
        XSetFont(dpy, fgc, vfont->fid);
        XSetFont(dpy, gc, vfont->fid);

        XDrawString(dpy, pix, fgc, 0, vfont->ascent,
                    title1, strlen(title1));

        XDrawString(dpy, mask, gc, 0, vfont->ascent,
                    title1, strlen(title1));
    }

    if (title2) {
        XSetFont(dpy, fgc, hfont->fid);
        XSetFont(dpy, gc, hfont->fid);

        XDrawString(dpy, pix, fgc, (title1 ? 12 : 0), hfont->ascent,
                    title2, strlen(title2));

        XDrawString(dpy, mask, gc, (title1 ? 12 : 0), hfont->ascent,
                    title2, strlen(title2));
    }

    return tmp;
#endif
}


void
SetButtonAlphaImage(WMScreen *scr, WMButton *bPtr, char *file,
                    char *title1, char *title2)
{
    WMPixmap *icon;
    WMPixmap *icon2;
    RColor color;
    char *iconPath;

    iconPath = LocateImage(file);

    color.red = 0xae;
    color.green = 0xaa;
    color.blue = 0xae;
    color.alpha = 0;
    if (iconPath) {
        icon = WMCreateBlendedPixmapFromFile(scr, iconPath, &color);
        if (!icon)
            wwarning(_("could not load icon file %s"), iconPath);
    } else {
        icon = NULL;
    }

    if (icon) {
        icon2 = makeTitledIcon(scr, icon, title1, title2);
        if (icon)
            WMReleasePixmap(icon);
    } else {
        icon2 = NULL;
    }

    WMSetButtonImage(bPtr, icon2);

    if (icon2)
        WMReleasePixmap(icon2);

    color.red = 0xff;
    color.green = 0xff;
    color.blue = 0xff;
    color.alpha = 0;
    if (iconPath) {
        icon = WMCreateBlendedPixmapFromFile(scr, iconPath, &color);
        if (!icon)
            wwarning(_("could not load icon file %s"), iconPath);
    } else {
        icon = NULL;
    }

    WMSetButtonAltImage(bPtr, icon);

    if (icon)
        WMReleasePixmap(icon);

    if (iconPath)
        wfree(iconPath);
}


void
AddSection(Panel *panel, char *iconFile)
{
    WMButton *bPtr;

    assert(WPrefs.sectionCount < MAX_SECTIONS);


    bPtr = WMCreateCustomButton(WPrefs.buttonF,	WBBStateLightMask
                                |WBBStateChangeMask);
    WMResizeWidget(bPtr, 64, 64);
    WMMoveWidget(bPtr, WPrefs.sectionCount*64, 0);
    WMSetButtonImagePosition(bPtr, WIPImageOnly);
    WMSetButtonAction(bPtr, changeSection, panel);
    WMHangData(bPtr, panel);

    WMSetBalloonTextForView(((PanelRec*)panel)->description,
                            WMWidgetView(bPtr));

    {
        char *t1, *t2;

        t1 = wstrdup(((PanelRec*)panel)->sectionName);
        t2 = strchr(t1, ' ');
        if (t2) {
            *t2 = 0;
            t2++;
        }
        SetButtonAlphaImage(WMWidgetScreen(bPtr), bPtr, iconFile,
                            t1, t2);
        wfree(t1);
    }
    WMMapWidget(bPtr);

    WPrefs.sectionB[WPrefs.sectionCount] = bPtr;

    if (WPrefs.sectionCount > 0) {
        WMGroupButtons(WPrefs.sectionB[0], bPtr);
    }

    WPrefs.sectionCount++;

    WMResizeWidget(WPrefs.buttonF, WPrefs.sectionCount*64, 64);
}


void
Initialize(WMScreen *scr)
{
    char **list;
    int i;
    char *path;
    WMPixmap *icon;


    list = RSupportedFileFormats();
    for (i=0; list[i]!=NULL; i++) {
        if (strcmp(list[i], "TIFF")==0) {
            TIFFOK = True;
            break;
        }
    }

    if (TIFFOK)
        path = WMPathForResourceOfType("WPrefs.tiff", NULL);
    else
        path = WMPathForResourceOfType("WPrefs.xpm", NULL);
    if (path) {
        RImage *tmp;

        tmp = RLoadImage(WMScreenRContext(scr), path, 0);
        if (!tmp) {
            wwarning(_("could not load image file %s:%s"), path,
                     RMessageForError(RErrorCode));
        } else {
            icon = WMCreatePixmapFromRImage(scr, tmp, 0);
            RReleaseImage(tmp);
            if (icon) {
                WMSetApplicationIconPixmap(scr, icon);
                WMReleasePixmap(icon);
            }
        }
        wfree(path);
    }

    memset(&WPrefs, 0, sizeof(_WPrefs));
    createMainWindow(scr);

    WMRealizeWidget(WPrefs.win);
    WMMapWidget(WPrefs.win);
    XFlush(WMScreenDisplay(scr));
    WMSetLabelText(WPrefs.statusL, _("Loading Window Maker configuration files..."));
    XFlush(WMScreenDisplay(scr));
    loadConfigurations(scr, WPrefs.win);

    WMSetLabelText(WPrefs.statusL, _("Initializing configuration panels..."));

    InitFocus(scr, WPrefs.banner);
    InitWindowHandling(scr, WPrefs.banner);

    InitMenuPreferences(scr, WPrefs.banner);
    InitIcons(scr, WPrefs.banner);
    InitPreferences(scr, WPrefs.banner);

    InitPaths(scr, WPrefs.banner);
    InitWorkspace(scr, WPrefs.banner);
    InitConfigurations(scr, WPrefs.banner);

    InitMenu(scr, WPrefs.banner);

#ifdef not_yet_fully_implemented
    InitKeyboardSettings(scr, WPrefs.banner);
#endif
    InitKeyboardShortcuts(scr, WPrefs.banner);
    InitMouseSettings(scr, WPrefs.banner);

    InitAppearance(scr, WPrefs.banner);

#ifdef finished_checking
    InitFont(scr, WPrefs.banner);
#endif

#ifdef not_yet_fully_implemented
    InitThemes(scr, WPrefs.banner);
#endif
    InitExpert(scr, WPrefs.banner);

    WMRealizeWidget(WPrefs.scrollV);

    WMSetLabelText(WPrefs.statusL,
                   _("WPrefs is free software and is distributed WITHOUT ANY\n"
                     "WARRANTY under the terms of the GNU General Public License."));
}


WMWindow*
GetWindow(Panel *panel)
{
    return WPrefs.win;
}


static void
loadConfigurations(WMScreen *scr, WMWindow *mainw)
{
    WMPropList *db, *gdb;
    char *path;
    FILE *file;
    char buffer[1024];
    char mbuf[1024];
    int v1, v2, v3;

    path = wdefaultspathfordomain("WindowMaker");
    WindowMakerDBPath = path;

    db = WMReadPropListFromFile(path);
    if (db) {
        if (!WMIsPLDictionary(db)) {
            WMReleasePropList(db);
            db = NULL;
            sprintf(mbuf, _("Window Maker domain (%s) is corrupted!"), path);
            WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
        }
    } else {
        sprintf(mbuf, _("Could not load Window Maker domain (%s) from defaults database."),
                path);
        WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
    }

    path = getenv("WMAKER_BIN_NAME");
    if (!path)
        path = "wmaker";
    {
        char *command;

        command = wstrconcat(path, " --version");
        file = popen(command, "r");
        wfree(command);
    }
    if (!file || !fgets(buffer, 1023, file)) {
        wsyserror(_("could not extract version information from Window Maker"));
        wfatal(_("Make sure wmaker is in your search path."));

        WMRunAlertPanel(scr, mainw, _("Error"),
                        _("Could not extract version from Window Maker. Make sure it is correctly installed and is in your PATH environment variable."),
                        _("OK"), NULL, NULL);
        exit(1);
    }
    if (file)
        pclose(file);

    if (sscanf(buffer, "Window Maker %i.%i.%i",&v1,&v2,&v3)!=3
        && sscanf(buffer, "WindowMaker %i.%i.%i",&v1,&v2,&v3)!=3) {
        WMRunAlertPanel(scr, mainw, _("Error"),
                        _("Could not extract version from Window Maker. "
                          "Make sure it is correctly installed and the path "
                          "where it installed is in the PATH environment "
                          "variable."), _("OK"), NULL, NULL);
        exit(1);
    }
    if (v1 == 0 && (v2 < 18 || v3 < 0)) {
        sprintf(mbuf, _("WPrefs only supports Window Maker 0.18.0 or newer.\n"
                        "The version installed is %i.%i.%i\n"), v1, v2, v3);
        WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
        exit(1);

    }
    if (v1 > 1 || (v1 == 1 && (v2 > 0))) {
        sprintf(mbuf, _("Window Maker %i.%i.%i, which is installed in your system, is not fully supported by this version of WPrefs."),
                v1, v2, v3);
        WMRunAlertPanel(scr, mainw, _("Warning"), mbuf, _("OK"), NULL, NULL);
    }

    {
        char *command;

        command = wstrconcat(path, " --global_defaults_path");
        file = popen(command, "r");
        wfree(command);
    }
    if (!file || !fgets(buffer, 1023, file)) {
        wsyserror(_("could not run \"%s --global_defaults_path\"."), path);
        exit(1);
    } else {
        char *ptr;
        ptr = strchr(buffer, '\n');
        if (ptr)
            *ptr = 0;
        strcat(buffer, "/WindowMaker");
    }

    if (file)
        pclose(file);

    gdb = WMReadPropListFromFile(buffer);

    if (gdb) {
        if (!WMIsPLDictionary(gdb)) {
            WMReleasePropList(gdb);
            gdb = NULL;
            sprintf(mbuf, _("Window Maker domain (%s) is corrupted!"), buffer);
            WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
        }
    } else {
        sprintf(mbuf, _("Could not load global Window Maker domain (%s)."),
                buffer);
        WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
    }

    if (!db) {
        db = WMCreatePLDictionary(NULL, NULL);
    }
    if (!gdb) {
        gdb = WMCreatePLDictionary(NULL, NULL);
    }

    GlobalDB = gdb;

    WindowMakerDB = db;
}


WMPropList*
GetObjectForKey(char *defaultName)
{
    WMPropList *object = NULL;
    WMPropList *key = WMCreatePLString(defaultName);

    object = WMGetFromPLDictionary(WindowMakerDB, key);
    if (!object)
        object = WMGetFromPLDictionary(GlobalDB, key);

    WMReleasePropList(key);

    return object;
}


void
SetObjectForKey(WMPropList *object, char *defaultName)
{
    WMPropList *key = WMCreatePLString(defaultName);

    WMPutInPLDictionary(WindowMakerDB, key, object);
    WMReleasePropList(key);
}


void
RemoveObjectForKey(char *defaultName)
{
    WMPropList *key = WMCreatePLString(defaultName);

    WMRemoveFromPLDictionary(WindowMakerDB, key);

    WMReleasePropList(key);
}


char*
GetStringForKey(char *defaultName)
{
    WMPropList *val;

    val = GetObjectForKey(defaultName);

    if (!val)
        return NULL;

    if (!WMIsPLString(val))
        return NULL;

    return WMGetFromPLString(val);
}



WMPropList*
GetArrayForKey(char *defaultName)
{
    WMPropList *val;

    val = GetObjectForKey(defaultName);

    if (!val)
        return NULL;

    if (!WMIsPLArray(val))
        return NULL;

    return val;
}


WMPropList*
GetDictionaryForKey(char *defaultName)
{
    WMPropList *val;

    val = GetObjectForKey(defaultName);

    if (!val)
        return NULL;

    if (!WMIsPLDictionary(val))
        return NULL;

    return val;
}


int
GetIntegerForKey(char *defaultName)
{
    WMPropList *val;
    char *str;
    int value;

    val = GetObjectForKey(defaultName);

    if (!val)
        return 0;

    if (!WMIsPLString(val))
        return 0;

    str = WMGetFromPLString(val);
    if (!str)
        return 0;

    if (sscanf(str, "%i", &value)!=1)
        return 0;

    return value;
}


Bool
GetBoolForKey(char *defaultName)
{
    int value;
    char *str;

    str = GetStringForKey(defaultName);

    if (!str)
        return False;

    if (sscanf(str, "%i", &value)==1 && value!=0)
        return True;

    if (strcasecmp(str, "YES")==0)
        return True;

    if (strcasecmp(str, "Y")==0)
        return True;

    return False;
}


void
SetIntegerForKey(int value, char *defaultName)
{
    WMPropList *object;
    char buffer[128];

    sprintf(buffer, "%i", value);
    object = WMCreatePLString(buffer);

    SetObjectForKey(object, defaultName);
    WMReleasePropList(object);
}



void
SetStringForKey(char *value, char *defaultName)
{
    WMPropList *object;

    object = WMCreatePLString(value);

    SetObjectForKey(object, defaultName);
    WMReleasePropList(object);
}


void
SetBoolForKey(Bool value, char *defaultName)
{
    static WMPropList *yes = NULL, *no = NULL;

    if (!yes) {
        yes = WMCreatePLString("YES");
        no = WMCreatePLString("NO");
    }

    SetObjectForKey(value ? yes : no, defaultName);
}


void
SetSpeedForKey(int speed, char *defaultName)
{
    char *str;

    switch (speed) {
    case 0:
        str = "ultraslow";
        break;
    case 1:
        str = "slow";
        break;
    case 2:
        str = "medium";
        break;
    case 3:
        str = "fast";
        break;
    case 4:
        str = "ultrafast";
        break;
    default:
        str = NULL;
    }

    if (str)
        SetStringForKey(str, defaultName);
}


int
GetSpeedForKey(char *defaultName)
{
    char *str;
    int i;

    str = GetStringForKey(defaultName);
    if (!str)
        return 2;

    if (strcasecmp(str, "ultraslow")==0)
        i = 0;
    else if (strcasecmp(str, "slow")==0)
        i = 1;
    else if (strcasecmp(str, "medium")==0)
        i = 2;
    else if (strcasecmp(str, "fast")==0)
        i = 3;
    else if (strcasecmp(str, "ultrafast")==0)
        i = 4;
    else {
        wwarning(_("bad speed value for option %s\n. Using default Medium"),
                 defaultName);
        i = 2;
    }
    return i;
}


