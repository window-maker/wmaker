/* Workspace.c- workspace options
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


typedef struct _Panel {
    WMFrame *frame;

    char *sectionName;

    CallbackRec callbacks;
    
    WMWindow *win;
    
    WMFrame *navF;
    WMButton *linkB;
    WMButton *cyclB;
    WMButton *newB;
    WMLabel *linkL;
    WMLabel *cyclL;
    WMLabel *newL;
    
    WMFrame *dockF;
    WMButton *dockB;
    WMButton *clipB;
} _Panel;



#define ICON_FILE	"workspace"

#define ARQUIVO_XIS	"xis"
#define DONT_LINK_FILE	"dontlinkworkspaces"
#define CYCLE_FILE	"cycleworkspaces"
#define ADVANCE_FILE	"advancetonewworkspace"
#define DOCK_FILE	"dock"
#define CLIP_FILE	"clip"



static void
createImages(WMScreen *scr, RContext *rc, RImage *xis, char *file, 
	     WMPixmap **icon1,  WMPixmap **icon2)
{
    RImage *icon;
    RColor gray = {0xae,0xaa,0xae};
    
    *icon1 = WMCreatePixmapFromFile(scr, file);
    if (!*icon1) {
	wwarning(_("could not load icon %s"), file);
	*icon2 = NULL;
	return;
    }
    icon = RLoadImage(rc, file, 0);
    if (!icon) {
	wwarning(_("could not load icon %s"), file);
	*icon2 = NULL;
	return;
    }
    RCombineImageWithColor(icon, &gray);
    if (xis) {
	RCombineImagesWithOpaqueness(icon, xis, 180);
	if (!(*icon2 = WMCreatePixmapFromRImage(scr, icon, 127))) {
	    wwarning(_("could not process icon %s:"), file, RMessageForError(RErrorCode));
	    *icon2 = NULL;
	}
    }
    RDestroyImage(icon);
}



static void
showData(_Panel *panel)
{
    WMSetButtonSelected(panel->linkB, !GetBoolForKey("DontLinkWorkspaces"));
    
    WMSetButtonSelected(panel->cyclB, GetBoolForKey("CycleWorkspaces"));
    
    WMSetButtonSelected(panel->newB, GetBoolForKey("AdvanceToNewWorkspace"));
    
    WMSetButtonSelected(panel->dockB, !GetBoolForKey("DisableDock"));
    
    WMSetButtonSelected(panel->clipB, !GetBoolForKey("DisableClip"));
}



static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMPixmap *icon1, *icon2;
    RImage *xis = NULL;
    RContext *rc = WMScreenRContext(scr);
    char *path;
    
    path = LocateImage(ARQUIVO_XIS);
    if (path) {
	xis = RLoadImage(rc, path, 0);
	if (!xis) {
	    wwarning(_("could not load image file %s"), path);
	}
	free(path);
    }

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    /***************** Workspace Navigation *****************/
    panel->navF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->navF, 365, 200);
    WMMoveWidget(panel->navF, 20, 15);
    WMSetFrameTitle(panel->navF, _("Workspace Navigation"));

    panel->linkB = WMCreateButton(panel->navF, WBTToggle);
    WMResizeWidget(panel->linkB, 60, 60);
    WMMoveWidget(panel->linkB, 20, 25);
    WMSetButtonImagePosition(panel->linkB, WIPImageOnly);
    path = LocateImage(DONT_LINK_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, &icon2);
	if (icon2) {
	    WMSetButtonImage(panel->linkB, icon2);
	    WMReleasePixmap(icon2);
	}
	if (icon1) {
	    WMSetButtonAltImage(panel->linkB, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }
    panel->linkL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->linkL, 260, 38);
    WMMoveWidget(panel->linkL, 85, 25);
    WMSetLabelTextAlignment(panel->linkL, WALeft);
    WMSetLabelText(panel->linkL,		   
		   _("drag windows between workspaces."));

    
    panel->cyclB = WMCreateButton(panel->navF, WBTToggle);
    WMResizeWidget(panel->cyclB, 60, 60);
    WMMoveWidget(panel->cyclB, 285, 75);
    WMSetButtonImagePosition(panel->cyclB, WIPImageOnly);
    path = LocateImage(CYCLE_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, &icon2);
	if (icon2) {
	    WMSetButtonImage(panel->cyclB, icon2);
	    WMReleasePixmap(icon2);
	}
	if (icon1) {
	    WMSetButtonAltImage(panel->cyclB, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }
    panel->cyclL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->cyclL, 260, 38);
    WMMoveWidget(panel->cyclL, 20, 85);
    WMSetLabelTextAlignment(panel->cyclL, WARight);
    WMSetLabelText(panel->cyclL, 
		   _("switch to first workspace when switching past the last workspace and vice-versa"));
    
    panel->newB = WMCreateButton(panel->navF, WBTToggle);
    WMResizeWidget(panel->newB, 60, 60);
    WMMoveWidget(panel->newB, 20, 125);
    WMSetButtonImagePosition(panel->newB, WIPImageOnly);
    path = LocateImage(ADVANCE_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, &icon2);
	if (icon2) {
	    WMSetButtonImage(panel->newB, icon2);
	    WMReleasePixmap(icon2);
	}
	if (icon1) {
	    WMSetButtonAltImage(panel->newB, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }
    panel->newL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->newL, 260, 38);
    WMMoveWidget(panel->newL, 85, 140);
    WMSetLabelTextAlignment(panel->newL, WALeft);
    WMSetLabelText(panel->newL, 
		   _("create a new workspace when switching past the last workspace."));
    
    WMMapSubwidgets(panel->navF);

    /***************** Dock/Clip *****************/
    panel->dockF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->dockF, 105, 200);
    WMMoveWidget(panel->dockF, 400, 15);
    WMSetFrameTitle(panel->dockF, _("Dock/Clip"));

    panel->dockB = WMCreateButton(panel->dockF, WBTToggle);
    WMResizeWidget(panel->dockB, 64, 64);
    WMMoveWidget(panel->dockB, 20, 30);
    WMSetButtonImagePosition(panel->dockB, WIPImageOnly);
    path = LocateImage(DOCK_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, &icon2);
	if (icon2) {
	    WMSetButtonImage(panel->dockB, icon2); 
	    WMReleasePixmap(icon2);
	}
	if (icon1) {
	    WMSetButtonAltImage(panel->dockB, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }
    
    panel->clipB = WMCreateButton(panel->dockF, WBTToggle);
    WMResizeWidget(panel->clipB, 64, 64);
    WMMoveWidget(panel->clipB, 20, 110);
    WMSetButtonImagePosition(panel->clipB, WIPImageOnly);
    path = LocateImage(CLIP_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, &icon2);
	if (icon2) {
	    WMSetButtonImage(panel->clipB, icon2); 
	    WMReleasePixmap(icon2);
	}
	if (icon1) {
	    WMSetButtonAltImage(panel->clipB, icon1);
	    WMReleasePixmap(icon1);
	}
    }
    WMMapSubwidgets(panel->dockF);

    if (xis)
	RDestroyImage(xis);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    showData(panel);
}


static void
storeData(_Panel *panel)
{
    SetBoolForKey(!WMGetButtonSelected(panel->linkB), "DontLinkWorkspaces");
    SetBoolForKey(WMGetButtonSelected(panel->cyclB), "CycleWorkspaces");
    SetBoolForKey(WMGetButtonSelected(panel->newB), "AdvanceToNewWorkspace");
    
    SetBoolForKey(!WMGetButtonSelected(panel->dockB), "DisableDock");
    SetBoolForKey(!WMGetButtonSelected(panel->clipB), "DisableClip");
}



Panel*
InitWorkspace(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Workspace Preferences");
    
    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
