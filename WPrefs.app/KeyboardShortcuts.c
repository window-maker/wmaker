/* KeyboardShortcuts.c- keyboard shortcut bindings
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
#include <ctype.h>

#include <X11/keysym.h>


typedef struct _Panel {
    WMFrame *frame;

    char *sectionName;
    
    CallbackRec callbacks;

    WMWindow *win;

    WMLabel *actL;
    WMList *actLs;
    
    WMFrame *shoF;
    WMTextField *shoT;
    WMButton *cleB;
    WMButton *defB;

    WMLabel *instructionsL;

    WMColor *white;
    WMColor *black;
    WMFont *font;

    /**/
    char capturing;
    char **shortcuts;
    int actionCount;
} _Panel;



#define ICON_FILE	"keyshortcuts"


/* must be in the same order as the corresponding items in actions list */
static char *keyOptions[] = {
    "RootMenuKey",
	"WindowListKey",
	"WindowMenuKey",
	"HideKey",
	"MiniaturizeKey",
	"CloseKey",
	"MaximizeKey",
	"VMaximizeKey",
	"RaiseKey",
	"LowerKey",
	"RaiseLowerKey",
	"ShadeKey",
	"MoveResizeKey",
	"SelectKey",
	"FocusNextKey",
	"FocusPrevKey",
	"NextWorkspaceKey",
	"PrevWorkspaceKey",
	"NextWorkspaceLayerKey",
	"PrevWorkspaceLayerKey",
	"Workspace1Key",
	"Workspace2Key",
	"Workspace3Key",
	"Workspace4Key",
	"Workspace5Key",
	"Workspace6Key",
	"Workspace7Key",
	"Workspace8Key",
	"Workspace9Key",
	"Workspace10Key",
	"WindowShortcut1Key",
	"WindowShortcut2Key",
	"WindowShortcut3Key",
	"WindowShortcut4Key",
#ifdef EXTEND_WINDOWSHORTCUT
	"WindowShortcut5Key",
	"WindowShortcut6Key",
	"WindowShortcut7Key",
	"WindowShortcut8Key",
	"WindowShortcut9Key",
	"WindowShortcut10Key",
#endif
	"ClipRaiseKey",
	"ClipLowerKey",
#ifndef XKB_MODELOCK
	"ClipRaiseLowerKey"
#else
	"ClipRaiseLowerKey",
	"ToggleKbdModeKey"
#endif /* XKB_MODELOCK */
};



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
captureClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->win));
    char *shortcut;
    
    if (!panel->capturing) {
	panel->capturing = 1;
	WMSetButtonText(w, _("Cancel"));
	WMSetLabelText(panel->instructionsL, _("Press the desired shortcut key(s) or click Cancel to stop capturing."));
	XGrabKeyboard(dpy, WMWidgetXID(panel->win), True, GrabModeAsync,
		      GrabModeAsync, CurrentTime);
	shortcut = captureShortcut(dpy, panel);
	if (shortcut) {
	    int row = WMGetListSelectedItemRow(panel->actLs);
	    
	    WMSetTextFieldText(panel->shoT, shortcut);
	    if (row>=0) {
		if (panel->shortcuts[row])
		    free(panel->shortcuts[row]);
		panel->shortcuts[row] = shortcut;

		WMRedisplayWidget(panel->actLs);
	    } else {
		free(shortcut);
	    }
	}
    }
    panel->capturing = 0;
    WMSetButtonText(w, _("Capture"));
    WMSetLabelText(panel->instructionsL, _("Click Capture to interactively define the shortcut key."));
    XUngrabKeyboard(dpy, CurrentTime);
}



static void
clearShortcut(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int row = WMGetListSelectedItemRow(panel->actLs);

    WMSetTextFieldText(panel->shoT, NULL);

    if (row>=0) {
	if (panel->shortcuts[row])
	    free(panel->shortcuts[row]);
	panel->shortcuts[row]=NULL;
	WMRedisplayWidget(panel->actLs);
    }
}



static void
typedKeys(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;    
    int row = WMGetListSelectedItemRow(panel->actLs);

    if (row<0)
	return;

    if (panel->shortcuts[row])
	free(panel->shortcuts[row]);
    panel->shortcuts[row] = WMGetTextFieldText(panel->shoT);
    if (strlen(panel->shortcuts[row])==0) {
	free(panel->shortcuts[row]);
	panel->shortcuts[row] = NULL;
    }
    WMRedisplayWidget(panel->actLs);
}



static void
listClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int row = WMGetListSelectedItemRow(w);
    
    WMSetTextFieldText(panel->shoT, panel->shortcuts[row]);
}


static char*
trimstr(char *str)
{
    char *p = str;
    int i;
    
    while (isspace(*p)) p++;
    p = wstrdup(p);
    i = strlen(p);
    while (isspace(p[i]) && i>0) {
	p[i]=0;
	i--;
    }

    return p;
}


static void
showData(_Panel *panel)
{
    char *str;
    int i;

    for (i=0; i<panel->actionCount; i++) {

	str = GetStringForKey(keyOptions[i]);
	if (panel->shortcuts[i])
	    free(panel->shortcuts[i]);
	if (str)
	    panel->shortcuts[i] = trimstr(str);
	else
	    panel->shortcuts[i] = NULL;

	if (panel->shortcuts[i] &&
	    (strcasecmp(panel->shortcuts[i], "none")==0 
	     || strlen(panel->shortcuts[i])==0)) {
	    free(panel->shortcuts[i]);
	    panel->shortcuts[i] = NULL;
	}	    
    }
    WMRedisplayWidget(panel->actLs);
}


static void
paintItem(WMList *lPtr, int index, Drawable d, char *text, int state,
	  WMRect *rect)
{
    int width, height, x, y;
    _Panel *panel = (_Panel*)WMGetHangedData(lPtr);
    WMScreen *scr = WMWidgetScreen(lPtr);
    Display *dpy = WMScreenDisplay(scr);

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
	XFillRectangle(dpy, d, WMColorGC(panel->white), x, y, width, 
		       height);
    else 
	XClearArea(dpy, d, x, y, width, height, False);

    if (panel->shortcuts[index]) {
	WMPixmap *pix = WMGetSystemPixmap(scr, WSICheckMark);
	WMSize size = WMGetPixmapSize(pix);
	
	WMDrawPixmap(pix, d, x+(20-size.width)/2, (height-size.height)/2+y);
	WMReleasePixmap(pix);
    }

    WMDrawString(scr, d, WMColorGC(panel->black), panel->font, x+20, y,
		 text, strlen(text));
}


static void
createPanel(Panel *p)
{   
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMColor *color;
    WMFont *boldFont;

    panel->capturing = 0;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    boldFont = WMBoldSystemFontOfSize(scr, 12);

    /* **************** Actions **************** */
    panel->actL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->actL, 280, 20);
    WMMoveWidget(panel->actL, 20, 10);
    WMSetLabelFont(panel->actL, boldFont);
    WMSetLabelText(panel->actL, _("Actions"));
    WMSetLabelRelief(panel->actL, WRSunken);
    WMSetLabelTextAlignment(panel->actL, WACenter);
    color = WMDarkGrayColor(scr);
    WMSetWidgetBackgroundColor(panel->actL, color); 
    WMReleaseColor(color);
    color = WMWhiteColor(scr);
    WMSetLabelTextColor(panel->actL, color);
    WMReleaseColor(color);
    
    panel->actLs = WMCreateList(panel->frame);
    WMResizeWidget(panel->actLs, 280, 190);
    WMMoveWidget(panel->actLs, 20, 32);
    WMSetListUserDrawProc(panel->actLs, paintItem);
    WMHangData(panel->actLs, panel);
    
    WMAddListItem(panel->actLs, _("Open applications menu"));
    WMAddListItem(panel->actLs, _("Open window list menu"));
    WMAddListItem(panel->actLs, _("Open window commands menu"));
    WMAddListItem(panel->actLs, _("Hide active application"));
    WMAddListItem(panel->actLs, _("Miniaturize active window"));
    WMAddListItem(panel->actLs, _("Close active window"));
    WMAddListItem(panel->actLs, _("Maximize active window"));
    WMAddListItem(panel->actLs, _("Maximize active window vertically"));
    WMAddListItem(panel->actLs, _("Raise active window"));
    WMAddListItem(panel->actLs, _("Lower active window"));
    WMAddListItem(panel->actLs, _("Raise/Lower window under mouse pointer"));
    WMAddListItem(panel->actLs, _("Shade active window"));
    WMAddListItem(panel->actLs, _("Move/Resize active window"));
    WMAddListItem(panel->actLs, _("Select active window"));
    WMAddListItem(panel->actLs, _("Focus next window"));
    WMAddListItem(panel->actLs, _("Focus previous window"));
    WMAddListItem(panel->actLs, _("Switch to next workspace"));
    WMAddListItem(panel->actLs, _("Switch to previous workspace"));
    WMAddListItem(panel->actLs, _("Switch to next ten workspaces"));
    WMAddListItem(panel->actLs, _("Switch to previous ten workspaces"));
    WMAddListItem(panel->actLs, _("Switch to workspace 1"));
    WMAddListItem(panel->actLs, _("Switch to workspace 2"));
    WMAddListItem(panel->actLs, _("Switch to workspace 3"));
    WMAddListItem(panel->actLs, _("Switch to workspace 4"));
    WMAddListItem(panel->actLs, _("Switch to workspace 5"));
    WMAddListItem(panel->actLs, _("Switch to workspace 6"));
    WMAddListItem(panel->actLs, _("Switch to workspace 7"));
    WMAddListItem(panel->actLs, _("Switch to workspace 8"));
    WMAddListItem(panel->actLs, _("Switch to workspace 9"));
    WMAddListItem(panel->actLs, _("Switch to workspace 10"));
    WMAddListItem(panel->actLs, _("Shortcut for window 1"));
    WMAddListItem(panel->actLs, _("Shortcut for window 2"));
    WMAddListItem(panel->actLs, _("Shortcut for window 3"));
    WMAddListItem(panel->actLs, _("Shortcut for window 4"));
    WMAddListItem(panel->actLs, _("Raise Clip"));
    WMAddListItem(panel->actLs, _("Lower Clip"));
    WMAddListItem(panel->actLs, _("Raise/Lower Clip"));
#ifdef XKB_MODELOCK
    WMAddListItem(panel->actLs, _("Toggle keyboard language"));
#endif /* XKB_MODELOCK */

    WMSetListAction(panel->actLs, listClick, panel);

    panel->actionCount = WMGetListNumberOfRows(panel->actLs);
    panel->shortcuts = wmalloc(sizeof(char*)*panel->actionCount);
    memset(panel->shortcuts, 0, sizeof(char*)*panel->actionCount);

    /***************** Shortcut ****************/

    panel->shoF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->shoF, 190, 210);
    WMMoveWidget(panel->shoF, 315, 10);
    WMSetFrameTitle(panel->shoF, _("Shortcut"));
    
    panel->shoT = WMCreateTextField(panel->shoF);
    WMResizeWidget(panel->shoT, 160, 20);
    WMMoveWidget(panel->shoT, 15, 65);
    WMAddNotificationObserver(typedKeys, panel,
			      WMTextDidChangeNotification, panel->shoT);
    
    panel->cleB = WMCreateCommandButton(panel->shoF);
    WMResizeWidget(panel->cleB, 75, 24);
    WMMoveWidget(panel->cleB, 15, 95);
    WMSetButtonText(panel->cleB, _("Clear"));
    WMSetButtonAction(panel->cleB, clearShortcut, panel);

    panel->defB = WMCreateCommandButton(panel->shoF);
    WMResizeWidget(panel->defB, 75, 24);
    WMMoveWidget(panel->defB, 100, 95);
    WMSetButtonText(panel->defB, _("Capture"));
    WMSetButtonAction(panel->defB, captureClick, panel);
    
    panel->instructionsL = WMCreateLabel(panel->shoF);
    WMResizeWidget(panel->instructionsL, 160, 55);
    WMMoveWidget(panel->instructionsL, 15, 140);
    WMSetLabelTextAlignment(panel->instructionsL, WACenter);
    WMSetLabelText(panel->instructionsL, _("Click Capture to interactively define the shortcut key."));

    WMMapSubwidgets(panel->shoF);
    
    WMReleaseFont(boldFont);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    
    showData(panel);
}


static void
storeData(_Panel *panel)
{
    int i;
    char *str;
    
    for (i=0; i<panel->actionCount; i++) {
	str = NULL;
	if (panel->shortcuts[i]) {
	    str = trimstr(panel->shortcuts[i]);
	    if (strlen(str)==0) {
		free(str);
		str = NULL;
	    }
	}
	if (str) {
	    SetStringForKey(str, keyOptions[i]);
	    free(str);
	} else {
	    SetStringForKey("None", keyOptions[i]);
	}
    }
}



Panel*
InitKeyboardShortcuts(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Keyboard Shortcut Preferences");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    panel->white = WMWhiteColor(scr);
    panel->black = WMBlackColor(scr);
    panel->font = WMSystemFontOfSize(scr, 12);

    AddSection(panel, ICON_FILE);
    
    return panel;
}
