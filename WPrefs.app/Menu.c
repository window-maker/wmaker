/* Menu.c- menu definition
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 2000 Alfredo K. Kojima
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
#include <ctype.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>


#include "editmenu.h"


typedef enum {
    NoInfo,
    ExecInfo,
    CommandInfo,
    ExternalInfo,
    PipeInfo,
    DirectoryInfo,
    WSMenuInfo,
    LastInfo
} InfoType;

#define MAX_SECTION_SIZE 4

typedef struct _Panel {
    WMFrame *frame;
    char *sectionName;

    char *description;

    CallbackRec callbacks;
    WMWindow *win;

    
    WMFont *boldFont;
    WMFont *normalFont;
    WMColor *white;
    WMColor *gray;
    WMColor *black;

    WMPixmap *markerPix[LastInfo];

    WMPopUpButton *typeP;
    
    WMWidget *itemPad[3];
    int currentPad;

    WEditMenu *menu;
    char *menuPath;

    WMFrame *optionsF;

    WMFrame *commandF;
    WMTextField *commandT;	       /* command to run */
    WMButton *xtermC;		       /* inside xterm? */
    
    WMFrame *pathF;
    WMTextField *pathT;

    WMFrame *pipeF;
    WMTextField *pipeT;

    WMFrame *dpathF;
    WMTextField *dpathT;

    WMFrame *dcommandF;
    WMTextField *dcommandT;

    WMButton *dstripB;

    WMFrame *shortF;
    WMTextField *shortT;
    WMButton *sgrabB;
    WMButton *sclearB;

    WMList *icommandL;

    WMFrame *paramF;
    WMTextField *paramT;
    
    WMButton *quickB;
    
    Bool dontAsk; 		       /* whether to comfirm submenu remove */
    Bool dontSave;
    
    Bool capturing;

        
    /* about the currently selected item */
    WEditMenuItem *currentItem;
    InfoType currentType;
    WMWidget *sections[LastInfo][MAX_SECTION_SIZE];
} _Panel;


typedef struct {
    InfoType type;
    union {
	struct {
	    int command;
	    char *parameter;
	    char *shortcut;
	} command;
	struct {
	    char *command;
	    char *shortcut;
	} exec;
	struct {
	    char *path;
	} external;
	struct {
	    char *command;
	} pipe;
	struct {
	    char *directory;
	    char *command;
	    unsigned stripExt:1;
	} directory;
    } param;
} ItemData;



static char *commandNames[] = {
    "ARRANGE_ICONS",
	"HIDE_OTHERS",
	"SHOW_ALL",
	"EXIT",
	"SHUTDOWN",
	"RESTART",
	"RESTART",
	"SAVE_SESSION",
	"CLEAR_SESSION",
	"REFRESH",
	"INFO_PANEL",
	"LEGAL_PANEL"
};



#define NEW(type) memset(wmalloc(sizeof(type)), 0, sizeof(type))


#define ICON_FILE	"menus"



static void showData(_Panel *panel);


static void updateMenuItem(_Panel *panel, WEditMenuItem *item, 
			   WMWidget *changedWidget);

static void menuItemSelected(struct WEditMenuDelegate *delegate, 
			     WEditMenu *menu, WEditMenuItem *item);

static void menuItemDeselected(struct WEditMenuDelegate *delegate,
			       WEditMenu *menu, WEditMenuItem *item);

static void menuItemCloned(struct WEditMenuDelegate *delegate, WEditMenu *menu,
			   WEditMenuItem *origItem, WEditMenuItem *newItem);

static void menuItemEdited(struct WEditMenuDelegate *delegate, WEditMenu *menu,
			   WEditMenuItem *item);

static Bool shouldRemoveItem(struct WEditMenuDelegate *delegate, 
			     WEditMenu *menu, WEditMenuItem *item);


static void freeItemData(ItemData *data);



static WEditMenuDelegate menuDelegate = {
    NULL,
	menuItemCloned,
	menuItemEdited,
	menuItemSelected,
	menuItemDeselected,
	shouldRemoveItem
};


static void
dataChanged(void *self, WMNotification *notif)
{
    _Panel *panel = (_Panel*)self;
    WEditMenuItem *item = panel->currentItem;
    WMWidget *w = (WMWidget*)WMGetNotificationObject(notif);
    
    updateMenuItem(panel, item, w);
}


static void
buttonClicked(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WEditMenuItem *item = panel->currentItem;
    
    updateMenuItem(panel, item, w);
}


static void
icommandLClicked(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int cmd;

    cmd = WMGetListSelectedItemRow(w);
    if (cmd == 3 || cmd == 4) {
	WMMapWidget(panel->quickB);
    } else {
	WMUnmapWidget(panel->quickB);
    }
    if (cmd == 6) {
	WMMapWidget(panel->paramF);
    } else {
	WMUnmapWidget(panel->paramF);
    }
}









static char*
captureShortcut(Display *dpy, _Panel *panel)
{
    XEvent ev;
    KeySym ksym;
    char buffer[64];
    char *key = NULL;

    while (panel->capturing) {
    	XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
	WMNextEvent(dpy, &ev);
	if (ev.type==KeyPress && ev.xkey.keycode!=0) {
	    ksym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);
	    if (!IsModifierKey(ksym)) {
		key=XKeysymToString(ksym);
		panel->capturing = 0;
		break;
	    }
	}
	WMHandleEvent(&ev);
    }
    
    if (!key)
	return NULL;
    
    buffer[0] = 0;
    
    if (ev.xkey.state & ControlMask) {
	strcat(buffer, "Control+");
    } 
    if (ev.xkey.state & ShiftMask) {
	strcat(buffer, "Shift+");
    } 
    if (ev.xkey.state & Mod1Mask) {
	strcat(buffer, "Mod1+");
    } 
    if (ev.xkey.state & Mod2Mask) {
	strcat(buffer, "Mod2+");
    } 
    if (ev.xkey.state & Mod3Mask) {
	strcat(buffer, "Mod3+");
    } 
    if (ev.xkey.state & Mod4Mask) {
	strcat(buffer, "Mod4+");
    } 
    if (ev.xkey.state & Mod5Mask) {
	strcat(buffer, "Mod5+");
    }
    strcat(buffer, key);
    
    return wstrdup(buffer);
}



static void
sgrabClicked(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->win));
    char *shortcut;

    
    if (w == panel->sclearB) {
	WMSetTextFieldText(panel->shortT, "");
	updateMenuItem(panel, panel->currentItem, panel->shortT);
	return;
    }
    
    if (!panel->capturing) {
	panel->capturing = 1;
	WMSetButtonText(w, _("Cancel"));
	XGrabKeyboard(dpy, WMWidgetXID(panel->win), True, GrabModeAsync,
		      GrabModeAsync, CurrentTime);
	shortcut = captureShortcut(dpy, panel);
	if (shortcut) {
	    WMSetTextFieldText(panel->shortT, shortcut);
	    updateMenuItem(panel, panel->currentItem, panel->shortT);
	    free(shortcut);
	}
    }
    panel->capturing = 0;
    WMSetButtonText(w, _("Capture"));
    XUngrabKeyboard(dpy, CurrentTime);
}


static void
changedItemPad(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int padn = WMGetPopUpButtonSelectedItem(w);
    
    WMUnmapWidget(panel->itemPad[panel->currentPad]);
    WMMapWidget(panel->itemPad[padn]);
    
    panel->currentPad = padn;
}


static WEditMenu*
putNewSubmenu(WEditMenu *menu, char *title)
{
    WEditMenu *tmp;
    WEditMenuItem *item;

    item = WAddMenuItemWithTitle(menu, title);

    tmp = WCreateEditMenu(WMWidgetScreen(menu), title);
    WSetEditMenuAcceptsDrop(tmp, True);
    WSetEditMenuDelegate(tmp, &menuDelegate);
    WSetEditMenuSubmenu(menu, item, tmp);

    return tmp;
}


static ItemData*
putNewItem(_Panel *panel, WEditMenu *menu, int type, char *title)
{
    WEditMenuItem *item;
    ItemData *data;

    item = WAddMenuItemWithTitle(menu, title);

    data = NEW(ItemData);
    data->type = type;
    WSetEditMenuItemData(item, data, (WMCallback*)freeItemData); 
    WSetEditMenuItemImage(item, panel->markerPix[type]);
   
    return data;
}


static WEditMenu*
makeFactoryMenu(WMWidget *parent, int width)
{
    WEditMenu *pad;
    
    pad = WCreateEditMenuPad(parent);
    WMResizeWidget(pad, width, 10);
    WSetEditMenuMinSize(pad, wmksize(width, 0));
    WSetEditMenuMaxSize(pad, wmksize(width, 0));
    WSetEditMenuSelectable(pad, False);
    WSetEditMenuEditable(pad, False);
    WSetEditMenuIsFactory(pad, True);
    WSetEditMenuDelegate(pad, &menuDelegate);
    
    return pad;
}


static void
createPanel(_Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMColor *black = WMBlackColor(scr);
    WMColor *white = WMWhiteColor(scr);
    WMColor *gray = WMGrayColor(scr);
    WMFont *bold = WMBoldSystemFontOfSize(scr, 12);
    WMFont *font = WMSystemFontOfSize(scr, 12);
    WMLabel *label;
    int width;


    menuDelegate.data = panel;

    
    panel->boldFont = bold;
    panel->normalFont = font;

    panel->black = black;
    panel->white = white;
    panel->gray = gray;

    {
	Pixmap pix;
	Display *dpy = WMScreenDisplay(scr);
	GC gc;
	WMPixmap *pixm;

	pixm = WMCreatePixmap(scr, 7, 7, WMScreenDepth(scr), True);

	pix = WMGetPixmapXID(pixm);

	XDrawLine(dpy, pix, WMColorGC(black), 0, 3, 6, 3);
	XDrawLine(dpy, pix, WMColorGC(black), 3, 0, 3, 6);
	/*
	XDrawLine(dpy, pix, WMColorGC(black), 1, 0, 3, 3);
	XDrawLine(dpy, pix, WMColorGC(black), 1, 6, 3, 3);
	XDrawLine(dpy, pix, WMColorGC(black), 0, 0, 0, 6);
	 */

	pix = WMGetPixmapMaskXID(pixm);

	gc = XCreateGC(dpy, pix, 0, NULL);

	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, pix, gc, 0, 0, 7, 7);

	XSetForeground(dpy, gc, 1);
	XDrawLine(dpy, pix, gc, 0, 3, 6, 3);
	XDrawLine(dpy, pix, gc, 3, 0, 3, 6);

	panel->markerPix[ExternalInfo] = pixm;
	panel->markerPix[PipeInfo] = pixm;
	panel->markerPix[DirectoryInfo] = pixm;
	panel->markerPix[WSMenuInfo] = pixm;

	XFreeGC(dpy, gc);
    }


    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    panel->typeP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->typeP, 150, 20);
    WMMoveWidget(panel->typeP, 10, 10);

    WMAddPopUpButtonItem(panel->typeP, _("New Items"));
    WMAddPopUpButtonItem(panel->typeP, _("Sample Commands"));
    WMAddPopUpButtonItem(panel->typeP, _("Sample Submenus"));
    
    WMSetPopUpButtonAction(panel->typeP, changedItemPad, panel);

    WMSetPopUpButtonSelectedItem(panel->typeP, 0);

    {
	WEditMenu *pad;
	WEditMenu *smenu;
	ItemData *data;

	pad = makeFactoryMenu(panel->frame, 150);
	WMMoveWidget(pad, 10, 40);
	
	data = putNewItem(panel, pad, ExecInfo, "Run Program");
	data = putNewItem(panel, pad, CommandInfo, "Internal Command");
	smenu = putNewSubmenu(pad, "Submenu");
	data = putNewItem(panel, pad, ExternalInfo, "External Submenu");
	data = putNewItem(panel, pad, PipeInfo, "Generated Submenu");
	data = putNewItem(panel, pad, DirectoryInfo, "Directory Contents");
	data = putNewItem(panel, pad, WSMenuInfo, "Workspace Menu");

	panel->itemPad[0] = pad;
    }

    {
	WEditMenu *pad;
	ItemData *data;
	WMScrollView *sview;

	sview = WMCreateScrollView(panel->frame);
	WMResizeWidget(sview, 150, 180);
	WMMoveWidget(sview, 10, 40);
	WMSetScrollViewHasVerticalScroller(sview, True);

	pad = makeFactoryMenu(panel->frame, 130);

	WMSetScrollViewContentView(sview, WMWidgetView(pad));

	data = putNewItem(panel, pad, ExecInfo, _("XTerm"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg black -fg white";

	data = putNewItem(panel, pad, ExecInfo, _("rxvt"));
	data->param.exec.command = "rxvt";

	data = putNewItem(panel, pad, ExecInfo, _("ETerm"));
	data->param.exec.command = "eterm";

	data = putNewItem(panel, pad, ExecInfo, _("Run..."));
	data->param.exec.command = "%a(Run,Type command to run)";

	data = putNewItem(panel, pad, ExecInfo, _("Netscape"));
	data->param.exec.command = "netscape";

	data = putNewItem(panel, pad, ExecInfo, _("gimp"));
	data->param.exec.command = "gimp";

	data = putNewItem(panel, pad, ExecInfo, _("epic"));
	data->param.exec.command = "xterm -e epic";

	data = putNewItem(panel, pad, ExecInfo, _("ee"));
	data->param.exec.command = "ee";

	data = putNewItem(panel, pad, ExecInfo, _("xv"));
	data->param.exec.command = "xv";

	data = putNewItem(panel, pad, ExecInfo, _("Acrobat Reader"));
	data->param.exec.command = "acroread || /usr/local/Acrobat4/bin/acroread";

	data = putNewItem(panel, pad, ExecInfo, _("ghostview"));
	data->param.exec.command = "gv";

	data = putNewItem(panel, pad, CommandInfo, _("Exit Window Maker"));
	data->param.command.command = 3;

	WMMapWidget(pad);
	
	panel->itemPad[1] = sview;
    }

    
    {
	WEditMenu *pad, *smenu;
	ItemData *data;
	WMScrollView *sview;

	sview = WMCreateScrollView(panel->frame);
	WMResizeWidget(sview, 150, 180);
	WMMoveWidget(sview, 10, 40);
	WMSetScrollViewHasVerticalScroller(sview, True);

	pad = makeFactoryMenu(panel->frame, 130);

	WMSetScrollViewContentView(sview, WMWidgetView(pad));

	data = putNewItem(panel, pad, ExternalInfo, _("Debian Menu"));
	data->param.pipe.command = "/etc/X11/WindowMaker/menu.hook";
 
	data = putNewItem(panel, pad, PipeInfo, _("RedHat Menu"));
	data->param.pipe.command = "wmconfig --output wmaker";

	data = putNewItem(panel, pad, PipeInfo, _("Menu Conectiva"));
	data->param.pipe.command = "wmconfig --output wmaker";

	data = putNewItem(panel, pad, DirectoryInfo, _("Themes"));
	data->param.directory.command = "setstyle";
	data->param.directory.directory = "/usr/share/WindowMaker/Themes /usr/local/share/WindowMaker/Themes $HOME/GNUstep/Library/WindowMaker/Themes";
	data->param.directory.stripExt = 1;
	
	data = putNewItem(panel, pad, DirectoryInfo, _("Bg Images (scale)"));
	data->param.directory.command = "wmsetbg -u -s";
	data->param.directory.directory = "/opt/kde2/share/wallpapers /usr/share/WindowMaker/Backgrounds $HOME/GNUstep/Library/WindowMaker/Backgrounds";
	data->param.directory.stripExt = 1;

	data = putNewItem(panel, pad, DirectoryInfo, _("Bg Images (tile)"));
	data->param.directory.command = "wmsetbg -u -t";
	data->param.directory.directory = "/opt/kde2/share/wallpapers /usr/share/WindowMaker/Backgrounds $HOME/GNUstep/Library/WindowMaker/Backgrounds";
	data->param.directory.stripExt = 1;
	
	smenu = putNewSubmenu(pad, _("Assorted XTerms"));

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm Yellow on Blue"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg midnightblue -fg yellow";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm White on Black"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg black -fg white";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm Black on White"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg white -fg black";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm Black on Beige"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg '#bbbb99' -fg black";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm White on Green"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg '#228822' -fg white";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm White on Olive"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg '#335533' -fg white";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm Blue on Blue"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg '#112244' -fg '#88aabb'";

	data = putNewItem(panel, smenu, ExecInfo, _("XTerm BIG FONTS"));
	data->param.exec.command = "xterm -sb -sl 2000 -bg black -fg white -fn 10x20";

	WMMapWidget(pad);
	
	panel->itemPad[2] = sview;
    }


    width = FRAME_WIDTH - 20 - 150 - 10;

    panel->optionsF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->optionsF, width, FRAME_HEIGHT - 15);
    WMMoveWidget(panel->optionsF, 10 + 150 + 10, 5);
    
    width -= 20;

    /* command */
    
    panel->commandF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->commandF, width, 50);
    WMMoveWidget(panel->commandF, 10, 20);
    WMSetFrameTitle(panel->commandF, _("Program to Run"));

    panel->commandT = WMCreateTextField(panel->commandF);
    WMResizeWidget(panel->commandT, width - 20, 20);
    WMMoveWidget(panel->commandT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->commandT);

#if 0
    panel->xtermC = WMCreateSwitchButton(panel->commandF);
    WMResizeWidget(panel->xtermC, width - 20, 20);
    WMMoveWidget(panel->xtermC, 10, 50);
    WMSetButtonText(panel->xtermC, _("Run the program inside a Xterm"));
#endif
    WMMapSubwidgets(panel->commandF);


    /* path */

    panel->pathF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->pathF, width, 150);
    WMMoveWidget(panel->pathF, 10, 40);
    WMSetFrameTitle(panel->pathF, _("Path for Menu"));

    panel->pathT = WMCreateTextField(panel->pathF);
    WMResizeWidget(panel->pathT, width - 20, 20);
    WMMoveWidget(panel->pathT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->pathT);
    
    label = WMCreateLabel(panel->pathF);
    WMResizeWidget(label, width - 20, 80);
    WMMoveWidget(label, 10, 50);
    WMSetLabelText(label, _("Enter the path for a file containing a menu\n"
			    "or a list of directories with the programs you\n"
			    "want to have listed in the menu. Ex:\n"
			    "~/GNUstep/Library/WindowMaker/menu\n"
			    "or\n"
			    "/usr/X11R6/bin ~/xbin"));

    WMMapSubwidgets(panel->pathF);


    /* pipe */

    panel->pipeF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->pipeF, width, 100);
    WMMoveWidget(panel->pipeF, 10, 50);
    WMSetFrameTitle(panel->pipeF, _("Command"));

    panel->pipeT = WMCreateTextField(panel->pipeF);
    WMResizeWidget(panel->pipeT, width - 20, 20);
    WMMoveWidget(panel->pipeT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->pipeT);


    label = WMCreateLabel(panel->pipeF);
    WMResizeWidget(label, width - 20, 40);
    WMMoveWidget(label, 10, 50);
    WMSetLabelText(label, _("Enter a command that outputs a menu\n"
			    "definition to stdout when invoked."));

    WMMapSubwidgets(panel->pipeF);


    /* directory menu */

    panel->dcommandF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->dcommandF, width, 90);
    WMMoveWidget(panel->dcommandF, 10, 25);
    WMSetFrameTitle(panel->dcommandF, _("Command to Open Files"));

    panel->dcommandT = WMCreateTextField(panel->dcommandF);
    WMResizeWidget(panel->dcommandT, width - 20, 20);
    WMMoveWidget(panel->dcommandT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->dcommandT);


    label = WMCreateLabel(panel->dcommandF);
    WMResizeWidget(label, width - 20, 45);
    WMMoveWidget(label, 10, 40);
    WMSetLabelText(label, _("Enter the command you want to use to open the\n"
			    "files in the directories listed below."));

    WMMapSubwidgets(panel->dcommandF);


    panel->dpathF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->dpathF, width, 80);
    WMMoveWidget(panel->dpathF, 10, 125);
    WMSetFrameTitle(panel->dpathF, _("Directories with Files"));

    panel->dpathT = WMCreateTextField(panel->dpathF);
    WMResizeWidget(panel->dpathT, width - 20, 20);
    WMMoveWidget(panel->dpathT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->dpathT);

    panel->dstripB = WMCreateSwitchButton(panel->dpathF);
    WMResizeWidget(panel->dstripB, width - 20, 20);
    WMMoveWidget(panel->dstripB, 10, 50);
    WMSetButtonText(panel->dstripB, _("Strip extensions from file names"));

    WMSetButtonAction(panel->dstripB, buttonClicked, panel);
    
    WMMapSubwidgets(panel->dpathF);


    /* shortcut */

    panel->shortF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->shortF, width, 50);
    WMMoveWidget(panel->shortF, 10, 160);
    WMSetFrameTitle(panel->shortF, _("Keyboard Shortcut"));

    panel->shortT = WMCreateTextField(panel->shortF);
    WMResizeWidget(panel->shortT, width - 20 - 170, 20);
    WMMoveWidget(panel->shortT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->shortT);

    panel->sgrabB = WMCreateCommandButton(panel->shortF);
    WMResizeWidget(panel->sgrabB, 80, 24);
    WMMoveWidget(panel->sgrabB, width - 90, 18);
    WMSetButtonText(panel->sgrabB, _("Capture"));
    WMSetButtonAction(panel->sgrabB, sgrabClicked, panel);

    panel->sclearB = WMCreateCommandButton(panel->shortF);
    WMResizeWidget(panel->sclearB, 80, 24);
    WMMoveWidget(panel->sclearB, width - 175, 18);
    WMSetButtonText(panel->sclearB, _("Clear"));
    WMSetButtonAction(panel->sclearB, sgrabClicked, panel);

    WMMapSubwidgets(panel->shortF);

    /* internal command */

    panel->icommandL = WMCreateList(panel->optionsF);
    WMResizeWidget(panel->icommandL, width, 80);
    WMMoveWidget(panel->icommandL, 10, 20);
    
    WMSetListAction(panel->icommandL, icommandLClicked, panel);

    WMAddNotificationObserver(dataChanged, panel,
			      WMListSelectionDidChangeNotification,
			      panel->icommandL);

    WMInsertListItem(panel->icommandL, 0, _("Arrange Icons"));
    WMInsertListItem(panel->icommandL, 1, _("Hide All Windows Except For The Focused One"));
    WMInsertListItem(panel->icommandL, 2, _("Show All Windows"));

    WMInsertListItem(panel->icommandL, 3, _("Exit Window Maker"));
    WMInsertListItem(panel->icommandL, 4, _("Exit X Session"));
    WMInsertListItem(panel->icommandL, 5, _("Restart Window Maker"));
    WMInsertListItem(panel->icommandL, 6, _("Start Another Window Manager   : ("));

    WMInsertListItem(panel->icommandL, 7, _("Save Current Session"));
    WMInsertListItem(panel->icommandL, 8, _("Clear Saved Session"));
    WMInsertListItem(panel->icommandL, 9, _("Refresh Screen"));
    WMInsertListItem(panel->icommandL, 10, _("Open Info Panel"));
    WMInsertListItem(panel->icommandL, 11, _("Open Copyright Panel"));


    panel->paramF = WMCreateFrame(panel->optionsF);
    WMResizeWidget(panel->paramF, width, 50);
    WMMoveWidget(panel->paramF, 10, 105);
    WMSetFrameTitle(panel->paramF, _("Window Manager to Start"));

    panel->paramT = WMCreateTextField(panel->paramF);
    WMResizeWidget(panel->paramT, width - 20, 20);
    WMMoveWidget(panel->paramT, 10, 20);

    WMAddNotificationObserver(dataChanged, panel,
			      WMTextDidChangeNotification,
			      panel->paramT);

    WMMapSubwidgets(panel->paramF);


    panel->quickB = WMCreateSwitchButton(panel->optionsF);
    WMResizeWidget(panel->quickB, width, 20);
    WMMoveWidget(panel->quickB, 10, 120);
    WMSetButtonText(panel->quickB, _("Do not confirm action."));
    WMSetButtonAction(panel->quickB, buttonClicked, panel);
    
    
    
    
    label = WMCreateLabel(panel->optionsF);
    WMResizeWidget(label, width - 20, FRAME_HEIGHT - 50);
    WMMoveWidget(label, 10, 20);
    WMSetLabelText(label, 
		   _("Instructions:\n\n"
		     " - drag items from the left to the menu to add new items\n"
		     " - drag items out of the menu to remove items\n"
		     " - drag items in menu to change their position\n"
		     " - double click in a menu item to change the label\n"
		     " - click on a menu item to change related information"));
    WMMapWidget(label);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    WMMapWidget(panel->frame);
    

    {
	int i;
	for (i = 0; i < 3; i++)
	    WMUnmapWidget(panel->itemPad[i]);
    }
    changedItemPad(panel->typeP, panel);

    panel->sections[NoInfo][0] = label;

    panel->sections[ExecInfo][0] = panel->commandF;
    panel->sections[ExecInfo][1] = panel->shortF;

    panel->sections[CommandInfo][0] = panel->icommandL;
    panel->sections[CommandInfo][1] = panel->shortF;

    panel->sections[ExternalInfo][0] = panel->pathF;

    panel->sections[PipeInfo][0] = panel->pipeF;

    panel->sections[DirectoryInfo][0] = panel->dpathF;
    panel->sections[DirectoryInfo][1] = panel->dcommandF;

    panel->currentType = NoInfo;

    showData(panel);
    
    {
	WMPoint pos;

	pos = WMGetViewScreenPosition(WMWidgetView(panel->frame));

	if (pos.x < 200) {
	    pos.x += FRAME_WIDTH + 20;
	} else {
	    pos.x = 10;
	}
	
	pos.y = WMAX(pos.y - 100, 0);
	
	WEditMenuShowAt(panel->menu, pos.x, pos.y);
    }
}




static void
freeItemData(ItemData *data)
{
#define CFREE(d) if (d) free(d)

    /* TODO */
    switch (data->type) {
     case CommandInfo:
	CFREE(data->param.command.parameter);
	CFREE(data->param.command.shortcut);
	break;
	
     case ExecInfo:
	CFREE(data->param.exec.command);
	CFREE(data->param.exec.shortcut);
	break;
	
     case PipeInfo:
	CFREE(data->param.pipe.command);
	break;
	
     case ExternalInfo:
	CFREE(data->param.external.path);
	break;
	
     case DirectoryInfo:
	CFREE(data->param.directory.command);
	CFREE(data->param.directory.directory);
	break;

     default:
	break;
    }

    free(data);
#undef CFREE
}


static ItemData*
parseCommand(proplist_t item)
{
    ItemData *data = NEW(ItemData);
    proplist_t p;
    char *command = NULL;
    char *parameter = NULL;
    char *shortcut = NULL;
    int i = 1;
    
    
    p = PLGetArrayElement(item, i++);
    command = PLGetString(p);
    if (strcmp(command, "SHORTCUT") == 0) {
	p = PLGetArrayElement(item, i++);
	shortcut = PLGetString(p);
	p = PLGetArrayElement(item, i++);
	command = PLGetString(p);
    }
    p = PLGetArrayElement(item, i++);
    if (p)
	parameter = PLGetString(p);

    if (strcmp(command, "EXEC") == 0 || strcmp(command, "SHEXEC") == 0) {
	
	data->type = ExecInfo;
	
	data->param.exec.command = wstrdup(parameter);
	if (shortcut)
	    data->param.exec.shortcut = wstrdup(shortcut);
	
    } else if (strcmp(command, "OPEN_MENU") == 0) {
	char *p;
	/*
	 * dir menu, menu file
	 * dir WITH
	 * |pipe (TODO: ||pipe)
	 */
	p = parameter;
	while (isspace(*p) && *p) p++;
	if (*p == '|') {
	    data->type = PipeInfo;
	    data->param.pipe.command = wtrimspace(p+1);
	} else {
	    char *s;

	    p = wstrdup(p);
	    
	    s = strstr(p, "WITH");
	    if (s) {
		char **tokens;
		char **ctokens;
		int tokn;
		int i, j;

		data->type = DirectoryInfo;
		
		*s = '\0';
		s += 5;
		while (*s && isspace(*s)) s++;
		data->param.directory.command = wstrdup(s);
		
		wtokensplit(p, &tokens, &tokn);
		free(p);
		
		ctokens = wmalloc(sizeof(char*)*tokn);

		for (i = 0, j = 0; i < tokn; i++) {
		    if (strcmp(tokens[i], "-noext") == 0) {
			free(tokens[i]);
			data->param.directory.stripExt = 1;
		    } else {
			ctokens[j++] = tokens[i];
		    }
		}
		data->param.directory.directory = wtokenjoin(ctokens, j);
		free(ctokens);

		wtokenfree(tokens, tokn);
	    } else {
		data->type = ExternalInfo;
		data->param.external.path = p;
	    }
	}
    } else if (strcmp(command, "WORKSPACE_MENU") == 0) {
	data->type = WSMenuInfo;
    } else {
	int cmd;

	if (strcmp(command, "ARRANGE_ICONS") == 0) {
	    cmd = 0;
	} else if (strcmp(command, "HIDE_OTHERS") == 0) {
	    cmd = 1;
	} else if (strcmp(command, "SHOW_ALL") == 0) {
	    cmd = 2;
	} else if (strcmp(command, "EXIT") == 0) {
	    cmd = 3;
	} else if (strcmp(command, "SHUTDOWN") == 0) {
	    cmd = 4;
	} else if (strcmp(command, "RESTART") == 0) {
	    if (parameter) {
		cmd = 6;
	    } else {
		cmd = 5;
	    }
	} else if (strcmp(command, "SAVE_SESSION") == 0) {
	    cmd = 7;
	} else if (strcmp(command, "CLEAR_SESSION") == 0) {
	    cmd = 8;
	} else if (strcmp(command, "REFRESH") == 0) {
	    cmd = 9;
	} else if (strcmp(command, "INFO_PANEL") == 0) {
	    cmd = 10;
	} else if (strcmp(command, "LEGAL_PANEL") == 0) {
	    cmd = 11;
	} else {
	    wwarning(_("unknown command '%s' in menu"), command);
	    goto error;
	}

	data->type = CommandInfo;

	data->param.command.command = cmd;
	if (shortcut)
	    data->param.command.shortcut = wstrdup(shortcut);
	if (parameter)
	    data->param.command.parameter = wstrdup(parameter);
    }

    return data;

error:
    free(data);

    return NULL;
}




static void
updateFrameTitle(_Panel *panel, char *title, InfoType type)
{
    if (type != NoInfo) {
	char *tmp;

	switch (type) {
	 case ExecInfo:
	    tmp = wstrappend(title, _(": Execute Program"));
	    break;

	 case CommandInfo:
	    tmp = wstrappend(title, _(": Perform Internal Command"));
	    break;

	 case ExternalInfo:
	    tmp = wstrappend(title, _(": Open a Submenu"));
	    break;

	 case PipeInfo:
	    tmp = wstrappend(title, _(": Program Generated Submenu"));
	    break;

	 case DirectoryInfo:
	    tmp = wstrappend(title, _(": Directory Contents Menu"));
	    break;

	 case WSMenuInfo:
	    tmp = wstrappend(title, _(": Open Workspaces Submenu"));
	    break;

	 default:
	    tmp = NULL;
	    break;
	}
	WMSetFrameTitle(panel->optionsF, tmp);
	free(tmp);
    } else {
	WMSetFrameTitle(panel->optionsF, NULL);
    }
}



static void
changeInfoType(_Panel *panel, char *title, InfoType type)
{
    WMWidget **w;
    
    if (panel->currentType != type) {

	w = panel->sections[panel->currentType];

	while (*w) {
	    WMUnmapWidget(*w);
	    w++;
	}
	WMUnmapWidget(panel->paramF);
	WMUnmapWidget(panel->quickB);


	w = panel->sections[type];
	
	while (*w) {
	    WMMapWidget(*w);
	    w++;
	}
    }

    updateFrameTitle(panel, title, type);

    panel->currentType = type;
}




static void 
updateMenuItem(_Panel *panel, WEditMenuItem *item, WMWidget *changedWidget)
{
    ItemData *data = WGetEditMenuItemData(item);    
    
    assert(data != NULL);

#define REPLACE(v, d) if (v) free(v); v = d

    switch (data->type) {
     case ExecInfo:
	if (changedWidget == panel->commandT) {
	    REPLACE(data->param.exec.command, 
		    WMGetTextFieldText(panel->commandT));
	}
	if (changedWidget == panel->shortT) {
	    REPLACE(data->param.exec.shortcut,
		    WMGetTextFieldText(panel->shortT));
	}
	break;

     case CommandInfo:
	if (changedWidget == panel->icommandL) {
	    data->param.command.command = 
		WMGetListSelectedItemRow(panel->icommandL);
	}
	switch (data->param.command.command) {
	 case 3:
	 case 4:
	    if (changedWidget == panel->quickB) {
		REPLACE(data->param.command.parameter,
			WMGetButtonSelected(panel->quickB) 
			? wstrdup("QUICK") : NULL);
	    }
	    break;

	 case 6:
	    if (changedWidget == panel->paramT) {
		REPLACE(data->param.command.parameter,
			WMGetTextFieldText(panel->paramT));
	    }
	    break;
	}
	if (changedWidget == panel->shortT) {
	    REPLACE(data->param.command.shortcut,
		    WMGetTextFieldText(panel->shortT));
	}
	
	
	break;
	
     case PipeInfo:
	if (changedWidget == panel->pipeT) {
	    REPLACE(data->param.pipe.command,
		    WMGetTextFieldText(panel->pipeT));
	}
	break;
	
     case ExternalInfo:
	if (changedWidget == panel->pathT) {
	    REPLACE(data->param.external.path, 
		    WMGetTextFieldText(panel->pathT));
	}
	break;
	
     case DirectoryInfo:
	if (changedWidget == panel->dpathT) {
	    REPLACE(data->param.directory.directory,
		    WMGetTextFieldText(panel->dpathT));
	}
	if (changedWidget == panel->dcommandT) {
	    REPLACE(data->param.directory.command,
		    WMGetTextFieldText(panel->dcommandT));
	}
	if (changedWidget == panel->dstripB) {
	    data->param.directory.stripExt = 
		WMGetButtonSelected(panel->dstripB);
	}
	break;

     default:
	assert(0);
	break;
    }

#undef REPLACE
}



static void
menuItemCloned(WEditMenuDelegate *delegate, WEditMenu *menu,
	       WEditMenuItem *origItem, WEditMenuItem *newItem)
{
    ItemData *data = WGetEditMenuItemData(origItem);
    ItemData *newData;
    
    if (!data)
	return;
    
#define DUP(s) (s) ? wstrdup(s) : NULL

    newData = NEW(ItemData);

    newData->type = data->type;
    
    switch (data->type) {
     case ExecInfo:
	newData->param.exec.command = DUP(data->param.exec.command);
	newData->param.exec.shortcut = DUP(data->param.exec.shortcut);
	break;

     case CommandInfo:
	newData->param.command.command = data->param.command.command;
	newData->param.command.parameter = DUP(data->param.command.parameter);
	newData->param.command.shortcut = DUP(data->param.command.shortcut);
	break;

     case PipeInfo:
	newData->param.pipe.command = DUP(data->param.pipe.command);
	break;

     case ExternalInfo:
	newData->param.external.path = DUP(data->param.external.path);
	break;

     case DirectoryInfo:
	newData->param.directory.directory = DUP(data->param.directory.directory);
	newData->param.directory.command = DUP(data->param.directory.command);
	newData->param.directory.stripExt = data->param.directory.stripExt;
	break;
	
     default:
	break;
    }

#undef DUP

    WSetEditMenuItemData(newItem, newData, (WMCallback*)freeItemData);
}




static void menuItemEdited(struct WEditMenuDelegate *delegate, WEditMenu *menu,
			   WEditMenuItem *item)
{
    _Panel *panel = (_Panel*)delegate->data;
    WEditMenu *submenu;
    
    updateFrameTitle(panel, WGetEditMenuItemTitle(item), panel->currentType);

    submenu = WGetEditMenuSubmenu(menu, item);
    if (submenu) {
	WSetEditMenuTitle(submenu, WGetEditMenuItemTitle(item));
    }
}





static Bool shouldRemoveItem(struct WEditMenuDelegate *delegate,
			     WEditMenu *menu, WEditMenuItem *item)
{
    _Panel *panel = (_Panel*)delegate->data;

    if (panel->dontAsk)
	return True;
    
    if (WGetEditMenuSubmenu(menu, item)) {
	int res;
	
	res = WMRunAlertPanel(WMWidgetScreen(menu), NULL,
			      _("Remove Submenu"),
			      _("Removing this item will destroy all items inside\n"
				"the submenu. Do you really want to do that?"),
			      _("Yes"), _("No"), 
			      _("Yes, don't ask again."));
	switch (res) {
	 case WAPRDefault:
	    return True;
	 case WAPRAlternate:
	    return False;
	 case WAPROther:
	    panel->dontAsk = True;
	    return True;
	}
    }
    return True;
}


static void 
menuItemDeselected(WEditMenuDelegate *delegate, WEditMenu *menu,
		   WEditMenuItem *item)
{
    _Panel *panel = (_Panel*)delegate->data;

    changeInfoType(panel, NULL, NoInfo);
}


static void 
menuItemSelected(WEditMenuDelegate *delegate, WEditMenu *menu,
		 WEditMenuItem *item)
{
    ItemData *data = WGetEditMenuItemData(item);
    _Panel *panel = (_Panel*)delegate->data;
    
    panel->currentItem = item;
    
    if (data) {
	changeInfoType(panel, WGetEditMenuItemTitle(item), data->type);
	
	switch (data->type) {
	 case NoInfo:
	    break;

	 case ExecInfo:
	    WMSetTextFieldText(panel->commandT, data->param.exec.command);
	    WMSetTextFieldText(panel->shortT, data->param.exec.shortcut);
	    break;

	 case CommandInfo:
	    WMSelectListItem(panel->icommandL, 
			     data->param.command.command);
	    WMSetListPosition(panel->icommandL,
			      data->param.command.command - 2);
	    WMSetTextFieldText(panel->shortT, data->param.command.shortcut);
	    
	    switch (data->param.command.command) {
	     case 3:
	     case 4:
		WMSetButtonSelected(panel->quickB,
				    data->param.command.parameter!=NULL);
		break;
	     case 6:
		WMSetTextFieldText(panel->paramT, 
				   data->param.command.parameter);
		break;
	    }

	    icommandLClicked(panel->icommandL, panel);
	    break;

	 case PipeInfo:
	    WMSetTextFieldText(panel->pipeT, data->param.pipe.command);
	    break;

	 case ExternalInfo:
	    WMSetTextFieldText(panel->pathT, data->param.external.path);
	    break;

	 case DirectoryInfo:
	    WMSetTextFieldText(panel->dpathT, data->param.directory.directory);
	    WMSetTextFieldText(panel->dcommandT, data->param.directory.command);
	    WMSetButtonSelected(panel->dstripB, data->param.directory.stripExt);
	    break;
	    
	 case WSMenuInfo:
	    break;
	    
	 default:
	    break;
	}
    }
}



static WEditMenu*
buildSubmenu(_Panel *panel, proplist_t pl)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    WEditMenu *menu;
    WEditMenuItem *item;
    char *title;
    proplist_t tp, bp;
    int i;

    tp = PLGetArrayElement(pl, 0);
    title = PLGetString(tp);

    menu = WCreateEditMenu(scr, title);
    
    for (i = 1; i < PLGetNumberOfElements(pl); i++) {
	proplist_t pi;
	
	pi = PLGetArrayElement(pl, i);

	tp = PLGetArrayElement(pi, 0);
	bp = PLGetArrayElement(pi, 1);
	
	title = PLGetString(tp);

	if (PLIsArray(bp)) {	       /* it's a submenu */
	    WEditMenu *submenu;
	    
	    submenu = buildSubmenu(panel, pi);
	    
	    item = WAddMenuItemWithTitle(menu, title);

	    WSetEditMenuSubmenu(menu, item, submenu);
	} else {
	    ItemData *data;
	    
	    item = WAddMenuItemWithTitle(menu, title);
	    
	    data = parseCommand(pi);
	    
	    if (panel->markerPix[data->type])
		WSetEditMenuItemImage(item, panel->markerPix[data->type]);
	    WSetEditMenuItemData(item, data, (WMCallback*)freeItemData);
	}
    }

    WSetEditMenuAcceptsDrop(menu, True);
    WSetEditMenuDelegate(menu, &menuDelegate);

    WMRealizeWidget(menu);

    return menu;
}



static void
buildMenuFromPL(_Panel *panel, proplist_t pl)
{
    panel->menu = buildSubmenu(panel, pl);
}



static proplist_t
getDefaultMenu(_Panel *panel)
{
    proplist_t menu, pmenu;
    char *menuPath, *gspath;

    gspath = wusergnusteppath();

    menuPath = wmalloc(strlen(gspath)+128);
    /* if there is a localized plmenu for the tongue put it's filename here */
    sprintf(menuPath, _("%s/Library/WindowMaker/plmenu"), gspath);

    menu = PLGetProplistWithPath(menuPath);
    if (!menu) {
	wwarning("%s:could not read property list menu", menuPath);

	if (strcmp("%s/Library/WindowMaker/plmenu",
		   _("%s/Library/WindowMaker/plmenu"))!=0) {

	    sprintf(menuPath, "%s/Library/WindowMaker/plmenu", gspath);
	    menu = PLGetProplistWithPath(menuPath);
	    wwarning("%s:could not read property list menu", menuPath);
	}
	if (!menu) {
	    char buffer[512];

	    sprintf(buffer, _("Could not open default menu from '%s'"),
		    menuPath);
	    WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win,
			    _("Error"), buffer,	_("OK"), NULL, NULL);
	}
    }

    free(menuPath);

    if (menu) {
	pmenu = menu;
    } else {
	pmenu = NULL;
    }

    return pmenu;
}


static void
showData(_Panel *panel)
{
    char *gspath;
    char *menuPath;
    proplist_t pmenu;

    gspath = wusergnusteppath();

    menuPath = wmalloc(strlen(gspath)+32);
    strcpy(menuPath, gspath);
    strcat(menuPath, "/Defaults/WMRootMenu");

    pmenu = PLGetProplistWithPath(menuPath);
    
    if (!pmenu || !PLIsArray(pmenu)) {
	int res;
	
	res = WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win,
			      _("Warning"),
			      _("The menu file format currently in use is not supported\n"
				"by this tool. Do you want to discard the current menu\n"
				"to use this tool?"),
			      _("Yes, Discard and Update"),
			      _("No, Keep Current Menu"), NULL);

	if (res == WAPRDefault) {
	    pmenu = getDefaultMenu(panel);

	    if (!pmenu) {
		pmenu = PLMakeArrayFromElements(PLMakeString("Applications"),
						NULL);
	    }
	} else {
	    panel->dontSave = True;
	    return;
	}
    }

    panel->menuPath = menuPath;

    buildMenuFromPL(panel, pmenu);

    PLRelease(pmenu);
}


static Bool
notblank(char *s)
{
    if (s) {
	while (*s++) {
	    if (!isspace(*s))
		return True;
	}
    }
    return False;
}


static proplist_t
processData(char *title, ItemData *data)
{
    proplist_t item;
    char *s1;
    static char *pscut = NULL;
    static char *pomenu = NULL;
    int i;

    if (!pscut) {
	pscut = PLMakeString("SHORTCUT");
	pomenu = PLMakeString("OPEN_MENU");
    }

    item = PLMakeArrayFromElements(PLMakeString(title), NULL);
    

    switch (data->type) {
     case ExecInfo:
#if 1
	if (strpbrk(data->param.exec.command, "&$*|><?`=")) {
	    s1 = "SHEXEC";
	} else {
	    s1 = "EXEC";
	}
#else	 
	s1 = "SHEXEC";
#endif

	if (notblank(data->param.exec.shortcut)) {
	    PLAppendArrayElement(item, pscut);
	    PLAppendArrayElement(item, 
				 PLMakeString(data->param.exec.shortcut));
	}

	PLAppendArrayElement(item, PLMakeString(s1));
	PLAppendArrayElement(item, PLMakeString(data->param.exec.command));
	break;

     case CommandInfo:
	if (notblank(data->param.command.shortcut)) {
	    PLAppendArrayElement(item, pscut);
	    PLAppendArrayElement(item, 
				 PLMakeString(data->param.command.shortcut));
	}
	
	i = data->param.command.command;
	
	PLAppendArrayElement(item, PLMakeString(commandNames[i]));
	
	switch (i) {
	 case 3:
	 case 4:
	    if (data->param.command.parameter) {
		PLAppendArrayElement(item, 
				     PLMakeString(data->param.command.parameter));
	    }
	    break;

	 case 6: /* restart */
	    if (data->param.command.parameter) {
		PLAppendArrayElement(item, 
				     PLMakeString(data->param.command.parameter));
	    }
	    break;
	}
	
	break;
	
     case PipeInfo:
	PLAppendArrayElement(item, pomenu);
	s1 = wstrappend("| ", data->param.pipe.command);
	PLAppendArrayElement(item, PLMakeString(s1));
	free(s1);
	break;
	
     case ExternalInfo:
	PLAppendArrayElement(item, pomenu);
	PLAppendArrayElement(item, PLMakeString(data->param.external.path));
	break;
	
     case DirectoryInfo:
	{
	    int l;
	    char *tmp;
	    
	    l = strlen(data->param.directory.directory);
	    l += strlen(data->param.directory.command);
	    l += 32;

	    PLAppendArrayElement(item, pomenu);
	    
	    tmp = wmalloc(l);
	    sprintf(tmp, "%s%s WITH %s",
		    data->param.directory.stripExt ? "-noext " : "",
		    data->param.directory.directory,
		    data->param.directory.command);
	    
	    PLAppendArrayElement(item, PLMakeString(tmp));
	    free(tmp);
	}
	break;
	
     case WSMenuInfo:
	PLAppendArrayElement(item, PLMakeString("WORKSPACE_MENU"));
	break;

     default:
	assert(0);
	break;
    }
    
    return item;
}


static proplist_t
processSubmenu(WEditMenu *menu)
{
    WEditMenuItem *item;
    proplist_t pmenu;
    proplist_t pl;
    char *s;
    int i;
    
    
    s = WGetEditMenuTitle(menu);
    pl = PLMakeString(s);

    pmenu = PLMakeArrayFromElements(pl, NULL);

    i = 0;
    while ((item = WGetEditMenuItem(menu, i++))) {
	WEditMenu *submenu;

	s = WGetEditMenuItemTitle(item);
	
	submenu = WGetEditMenuSubmenu(menu, item);
	if (submenu) {
	    pl = processSubmenu(submenu); 
	} else {
	    pl = processData(s, WGetEditMenuItemData(item));
	}

	PLAppendArrayElement(pmenu, pl);
    }
    
    return pmenu;
}
	       


static proplist_t
buildPLFromMenu(_Panel *panel)
{
    proplist_t menu;
    
    menu = processSubmenu(panel->menu);
    
    return menu;
}




static void
storeData(_Panel *panel)
{
    proplist_t menu;

    if (panel->dontSave)
	return;
    
    menu = buildPLFromMenu(panel);

    PLSetFilename(menu, PLMakeString(panel->menuPath));
    
    PLSave(menu, YES);

    PLRelease(menu);
}



static void
showMenus(_Panel *panel)
{
    WEditMenuUnhide(panel->menu);
}


static void
hideMenus(_Panel *panel)
{
    WEditMenuHide(panel->menu);
}




Panel*
InitMenu(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;
    
    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Applications Menu Definition");

    panel->description = _("Edit the menu for launching applications.");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.showPanel = showMenus;   
    panel->callbacks.hidePanel = hideMenus;

    
    AddSection(panel, ICON_FILE);

    return panel;
}

