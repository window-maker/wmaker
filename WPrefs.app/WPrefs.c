/* WPrefs.c- main window and other basic stuff
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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


extern Panel *InitWindowHandling(WMScreen *scr, WMWindow *win);

extern Panel *InitKeyboardSettings(WMScreen *scr, WMWindow *win);

extern Panel *InitMouseSettings(WMScreen *scr, WMWindow *win);

extern Panel *InitKeyboardShortcuts(WMScreen *scr, WMWindow *win);

extern Panel *InitWorkspace(WMScreen *scr, WMWindow *win);

extern Panel *InitFocus(WMScreen *scr, WMWindow *win);

extern Panel *InitPreferences(WMScreen *scr, WMWindow *win);

extern Panel *InitText(WMScreen *scr, WMWindow *win);

extern Panel *InitConfigurations(WMScreen *scr, WMWindow *win);

extern Panel *InitPaths(WMScreen *scr, WMWindow *win);

extern Panel *InitMenu(WMScreen *scr, WMWindow *win);

extern Panel *InitExpert(WMScreen *scr, WMWindow *win);

extern Panel *InitMenuPreferences(WMScreen *scr, WMWindow *win);

extern Panel *InitIcons(WMScreen *scr, WMWindow *win);

extern Panel *InitThemes(WMScreen *scr, WMWindow *win);

extern Panel *InitAppearance(WMScreen *scr, WMWindow *win);



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
static proplist_t GlobalDB = NULL;
/* user defaults dictionary */
static proplist_t WindowMakerDB = NULL;


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
    proplist_t p1, p2;
    proplist_t keyList;
    proplist_t key;
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
    keyList = PLGetAllDictionaryKeys(GlobalDB);
/*    puts(PLGetDescription(WindowMakerDB));*/
    for (i=0; i<PLGetNumberOfElements(keyList); i++) {
	key = PLGetArrayElement(keyList, i);
	
	/* We don't have this value anyway, so no problem. 
	 * Probably  a new option */
	p1 = PLGetDictionaryEntry(WindowMakerDB, key);
	if (!p1)
	    continue;
	/* The global doesn't have it, so no problem either. */
	p2 = PLGetDictionaryEntry(GlobalDB, key);
	if (!p2)
	    continue;
	/* If both values are the same, don't save. */
	if (PLIsEqual(p1, p2))
	    PLRemoveDictionaryEntry(WindowMakerDB, key);
    }
/*    puts(PLGetDescription(WindowMakerDB));*/
    PLRelease(keyList);
/*    puts("storing data");*/

    PLSave(WindowMakerDB, YES);
    

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
    WMSetWindowMiniwindowImage(WPrefs.win, WMGetApplicationIconImage(scr));
    
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
		   		"Artwork: Marco van Hylckama Vlieg\n"
				      "More Programming: James Thompson"));
   
    
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
    
    WMMapWidget(rec->frame);
}



static void
hidePanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;    
    
    WMUnmapWidget(rec->frame);
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
    if (WPrefs.banner) {
	WMDestroyWidget(WPrefs.banner);
	WPrefs.banner = NULL;
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
    free(tmp);
    if (!path) {
	wwarning(_("could not locate image file %s\n"), name);
    }
    
    return path;
}


void
SetButtonAlphaImage(WMScreen *scr, WMButton *bPtr, char *file)
{
    WMPixmap *icon;
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

    WMSetButtonImage(bPtr, icon);

    if (icon)
	WMReleasePixmap(icon);

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
	free(iconPath);
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

    SetButtonAlphaImage(WMWidgetScreen(bPtr), bPtr, iconFile);

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
	    RDestroyImage(tmp);
	    if (icon) {
	        WMSetApplicationIconImage(scr, icon);
	        WMReleasePixmap(icon);
	    }
	}
	free(path);
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

    InitWindowHandling(scr, WPrefs.win);
    InitFocus(scr, WPrefs.win);
    InitMenuPreferences(scr, WPrefs.win);
    InitIcons(scr, WPrefs.win);
    InitPreferences(scr, WPrefs.win);

    InitPaths(scr, WPrefs.win);    
    InitWorkspace(scr, WPrefs.win);
    InitConfigurations(scr, WPrefs.win);

    InitMenu(scr, WPrefs.win);

#ifdef not_yet_fully_implemented
    InitKeyboardSettings(scr, WPrefs.win);
#endif
    InitKeyboardShortcuts(scr, WPrefs.win);
    InitMouseSettings(scr, WPrefs.win);

    InitAppearance(scr, WPrefs.win);
#ifdef not_yet_fully_implemented

    InitText(scr, WPrefs.win);
    InitThemes(scr, WPrefs.win);
#endif
    InitExpert(scr, WPrefs.win);

    WMRealizeWidget(WPrefs.scrollV);

    WMSetLabelText(WPrefs.statusL, 
		   _("WPrefs is free software and is distributed WITHOUT ANY\n"
		   "WARRANTY under the terms of the GNU General Public License.\n"
		   "The icons in this program are licensed through the\n"
		   "OpenContent License."));
}


WMWindow*
GetWindow(Panel *panel)
{
    return WPrefs.win;
}


static void
loadConfigurations(WMScreen *scr, WMWindow *mainw)
{
    proplist_t db, gdb;
    char *path;
    FILE *file;
    char buffer[1024];
    char mbuf[1024];
    int v1, v2, v3;
    
    path = wdefaultspathfordomain("WindowMaker");
    
    db = PLGetProplistWithPath(path);
    if (db) {
	if (!PLIsDictionary(db)) {
	    PLRelease(db);
	    db = NULL;
	    sprintf(mbuf, _("Window Maker domain (%s) is corrupted!"), path);
	    WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
	}
    } else {
	sprintf(mbuf, _("Could not load Window Maker domain (%s) from defaults database."),
		path);
	WMRunAlertPanel(scr, mainw, _("Error"), mbuf, _("OK"), NULL, NULL);
    }
    free(path);

    path = getenv("WMAKER_BIN_NAME");
    if (!path)
	path = "wmaker";
    path = wstrappend(path, " --version");

    file = popen(path, "r");
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

    file = popen("wmaker --global_defaults_path", "r");
    if (!file || !fgets(buffer, 1023, file)) {
	wsyserror(_("could not run \"wmaker --global_defaults_path\"."));
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

    gdb = PLGetProplistWithPath(buffer);
    if (gdb) {
	if (!PLIsDictionary(gdb)) {
	    PLRelease(gdb);
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
	db = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
    }
    if (!gdb) {
	gdb = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
    }

    GlobalDB = gdb;

    WindowMakerDB = db;
}


proplist_t
GetObjectForKey(char *defaultName)
{
    proplist_t object = NULL;
    proplist_t key = PLMakeString(defaultName);

    object = PLGetDictionaryEntry(WindowMakerDB, key);
    if (!object)
	object = PLGetDictionaryEntry(GlobalDB, key);

    PLRelease(key);

    return object;
}


void
SetObjectForKey(proplist_t object, char *defaultName)
{
    proplist_t key = PLMakeString(defaultName);

    PLInsertDictionaryEntry(WindowMakerDB, key, object);
    PLRelease(key);
}


void
RemoveObjectForKey(char *defaultName)
{
    proplist_t key = PLMakeString(defaultName);
    
    PLRemoveDictionaryEntry(WindowMakerDB, key);
    
    PLRelease(key);
}


char*
GetStringForKey(char *defaultName)
{
    proplist_t val;
    
    val = GetObjectForKey(defaultName);

    if (!val)
	return NULL;

    if (!PLIsString(val))
	return NULL;

    return PLGetString(val);
}



proplist_t
GetArrayForKey(char *defaultName)
{
    proplist_t val;
    
    val = GetObjectForKey(defaultName);
    
    if (!val)
	return NULL;

    if (!PLIsArray(val))
	return NULL;

    return val;
}


proplist_t
GetDictionaryForKey(char *defaultName)
{
    proplist_t val;

    val = GetObjectForKey(defaultName);
    
    if (!val)
	return NULL;

    if (!PLIsDictionary(val))
	return NULL;

    return val;
}


int
GetIntegerForKey(char *defaultName)
{
    proplist_t val;
    char *str;
    int value;

    val = GetObjectForKey(defaultName);
    
    if (!val)
	return 0;

    if (!PLIsString(val))
	return 0;
    
    str = PLGetString(val);
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
    proplist_t object;
    char buffer[128];

    sprintf(buffer, "%i", value);
    object = PLMakeString(buffer);
 
    SetObjectForKey(object, defaultName);
    PLRelease(object);
}



void
SetStringForKey(char *value, char *defaultName)
{
    proplist_t object;

    object = PLMakeString(value);
 
    SetObjectForKey(object, defaultName);
    PLRelease(object);
}


void
SetBoolForKey(Bool value, char *defaultName)
{
    static proplist_t yes = NULL, no = NULL;

    if (!yes) {
	yes = PLMakeString("YES");
	no = PLMakeString("NO");
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


