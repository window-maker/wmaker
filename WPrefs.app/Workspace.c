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

    char *description;

    CallbackRec callbacks;

    WMWindow *win;

    WMFrame *navF;

    WMButton *linkB;
    WMButton *cyclB;
    WMButton *newB;
    WMLabel *linkL;
    WMLabel *cyclL;
    WMLabel *newL;

    WMLabel *posiL;
    WMLabel *posL;
    WMPopUpButton *posP;

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


static char *WSNamePositions[] = {
    "none",
	"center",
	"top",
	"bottom",
	"topleft",
	"topright",
	"bottomleft",
	"bottomright"
};


static void
createImages(WMScreen *scr, RContext *rc, RImage *xis, char *file, 
	     WMPixmap **icon1,  WMPixmap **icon2)
{
    RImage *icon;
    RColor gray = {0xae,0xaa,0xae};
    
    *icon1 = WMCreatePixmapFromFile(scr, file);
    if (!*icon1) {
	wwarning(_("could not load icon %s"), file);
	if (icon2)
	    *icon2 = NULL;
	return;
    }

    if (!icon2)
	return;
    
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
    int i, idx;
    char *str;

    WMSetButtonSelected(panel->linkB, !GetBoolForKey("DontLinkWorkspaces"));
    
    WMSetButtonSelected(panel->cyclB, GetBoolForKey("CycleWorkspaces"));
    
    WMSetButtonSelected(panel->newB, GetBoolForKey("AdvanceToNewWorkspace"));
    
    WMSetButtonSelected(panel->dockB, !GetBoolForKey("DisableDock"));
    
    WMSetButtonSelected(panel->clipB, !GetBoolForKey("DisableClip"));

    str = GetStringForKey("WorkspaceNameDisplayPosition");
    if (!str)
	str = "center";

    idx = 1; /* center */
    for (i = 0; i < sizeof(WSNamePositions); i++) {
	if (strcmp(WSNamePositions[i], str) == 0) {
	    idx = i;
	    break;
	}
    }
    WMSetPopUpButtonSelectedItem(panel->posP, idx);
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
    WMResizeWidget(panel->navF, 365, 210);
    WMMoveWidget(panel->navF, 15, 10);
    WMSetFrameTitle(panel->navF, _("Workspace Navigation"));

    
    panel->cyclB = WMCreateSwitchButton(panel->navF);
    WMResizeWidget(panel->cyclB, 280, 34);
    WMMoveWidget(panel->cyclB, 75, 30);
    WMSetButtonText(panel->cyclB,
		   _("wrap to the first workspace after the last workspace."));

    panel->cyclL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->cyclL, 60, 60);
    WMMoveWidget(panel->cyclL, 10, 15);
    WMSetLabelImagePosition(panel->cyclL, WIPImageOnly);
    path = LocateImage(CYCLE_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, NULL);
	if (icon1) {
	    WMSetLabelImage(panel->cyclL, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }

    /**/

    panel->linkB = WMCreateSwitchButton(panel->navF);
    WMResizeWidget(panel->linkB, 280, 34);
    WMMoveWidget(panel->linkB, 75, 75);
    WMSetButtonText(panel->linkB,
		   _("switch workspaces while dragging windows."));

    panel->linkL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->linkL, 60, 40);
    WMMoveWidget(panel->linkL, 10, 80);
    WMSetLabelImagePosition(panel->linkL, WIPImageOnly);
    path = LocateImage(DONT_LINK_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, NULL);
	if (icon1) {
	    WMSetLabelImage(panel->linkL, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }

    /**/
    
    panel->newB = WMCreateSwitchButton(panel->navF);
    WMResizeWidget(panel->newB, 280, 34);
    WMMoveWidget(panel->newB, 75, 120);
    WMSetButtonText(panel->newB,
		   _("automatically create new workspaces."));

    panel->newL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->newL, 60, 20);
    WMMoveWidget(panel->newL, 10, 130);
    WMSetLabelImagePosition(panel->newL, WIPImageOnly);
    path = LocateImage(ADVANCE_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, NULL);
	if (icon1) {
	    WMSetLabelImage(panel->newL, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }

    /**/
    
    panel->posL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->posL, 140, 30);
    WMMoveWidget(panel->posL, 75, 165);
    WMSetLabelTextAlignment(panel->posL, WARight);
    WMSetLabelText(panel->posL, 
		   _("Position of workspace name display"));
    
#if 0
    panel->posiL = WMCreateLabel(panel->navF);
    WMResizeWidget(panel->posiL, 60, 40);
    WMMoveWidget(panel->posiL, 10, 160);
    WMSetLabelImagePosition(panel->posiL, WIPImageOnly);
    path = LocateImage(ADVANCE_FILE);
    if (path) {
	createImages(scr, rc, xis, path, &icon1, NULL);
	if (icon1) {
	    WMSetLabelImage(panel->posiL, icon1);
	    WMReleasePixmap(icon1);
	}
	free(path);
    }
#endif
    
    panel->posP = WMCreatePopUpButton(panel->navF);
    WMResizeWidget(panel->posP, 125, 20);
    WMMoveWidget(panel->posP, 225, 175);
    WMAddPopUpButtonItem(panel->posP, _("Disable"));
    WMAddPopUpButtonItem(panel->posP, _("Center"));
    WMAddPopUpButtonItem(panel->posP, _("Top"));
    WMAddPopUpButtonItem(panel->posP, _("Bottom"));
    WMAddPopUpButtonItem(panel->posP, _("Top/left"));
    WMAddPopUpButtonItem(panel->posP, _("Top/right"));
    WMAddPopUpButtonItem(panel->posP, _("Bottom/left"));
    WMAddPopUpButtonItem(panel->posP, _("Bottom/right"));
    
    WMMapSubwidgets(panel->navF);

    /***************** Dock/Clip *****************/
    panel->dockF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->dockF, 115, 210);
    WMMoveWidget(panel->dockF, 390, 10);
    WMSetFrameTitle(panel->dockF, _("Dock/Clip"));

    panel->dockB = WMCreateButton(panel->dockF, WBTToggle);
    WMResizeWidget(panel->dockB, 64, 64);
    WMMoveWidget(panel->dockB, 25, 35);
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
    WMSetBalloonTextForView(_("Disable/enable the application Dock (the\n"
			      "vertical icon bar in the side of the screen)."),
			    WMWidgetView(panel->dockB));

    panel->clipB = WMCreateButton(panel->dockF, WBTToggle);
    WMResizeWidget(panel->clipB, 64, 64);
    WMMoveWidget(panel->clipB, 25, 120);
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
    WMSetBalloonTextForView(_("Disable/enable the Clip (that thing with\n"
			      "a paper clip icon)."),
			    WMWidgetView(panel->clipB));

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
    
    SetStringForKey(WSNamePositions[WMGetPopUpButtonSelectedItem(panel->posP)],
		    "WorkspaceNameDisplayPosition");
}



Panel*
InitWorkspace(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Workspace Preferences");

    panel->description = _("Workspace navigation features.\n"
			   "You can also enable/disable the Dock and Clip here.");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
