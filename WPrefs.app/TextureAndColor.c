/* TextureAndColor.c- color/texture for titlebar etc.
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

#include "TexturePanel.h"

typedef struct _Panel {
    WMFrame *frame;
    char *sectionName;

    CallbackRec callbacks;

    WMWindow *win;

    WMPopUpButton *secP;

    WMLabel *prevL;

    /* window titlebar */
    WMFrame *focF;
    WMColorWell *focC;
    WMLabel *focL;
    WMTextField *focT;
    WMLabel *foc2L;
    WMButton *focB;

    WMFrame *unfF;
    WMColorWell *unfC;
    WMLabel *unfL;
    WMTextField *unfT;
    WMLabel *unf2L;
    WMButton *unfB;

    WMFrame *ownF;
    WMColorWell *ownC;
    WMLabel *ownL;
    WMTextField *ownT;
    WMLabel *own2L;
    WMButton *ownB;

    /* menu title */
    WMFrame *backF;
    WMTextField *backT;
    WMButton *backB;

    WMFrame *textF;
    WMColorWell *textC;

    /* menu items */
    WMFrame *unsF;
    WMTextField *unsT;
    WMButton *unsB;
    WMLabel *unsL;
    WMColorWell *unsnC;
    WMLabel *unsnL;
    WMColorWell *unsdC;
    WMLabel *unsdL;

    WMFrame *selF;
    WMColorWell *seltC;
    WMLabel *seltL;
    WMColorWell *selbC;
    WMLabel *selbL;

    /* workspace/clip */
    WMFrame *workF;
    WMTextField *workT;
    WMButton *workB;

    WMFrame *clipF;
    WMColorWell *clipnC;
    WMColorWell *clipcC;
    WMLabel *clipnL;
    WMLabel *clipcL;

    /* icon */
    WMFrame *iconF;
    WMTextField *iconT;
    WMButton *iconB;

    Pixmap preview;
    Pixmap ftitle;
    Pixmap utitle;
    Pixmap otitle;
    Pixmap icon;
    Pixmap back;
    Pixmap mtitle;
    Pixmap mitem;
} _Panel;




#define ICON_FILE	"appearance"


#define	FTITLE	(1<<0)
#define UTITLE	(1<<1)
#define OTITLE	(1<<2)
#define ICON	(1<<3)
#define BACK	(1<<4)
#define MTITLE	(1<<5)
#define MITEM	(1<<6)
#define EVERYTHING	0xff


static Pixmap
renderTexture(_Panel *panel, char *texture, int width, int height, 
	      Bool bordered)
{
    return None;
}


static void
updatePreviewBox(_Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    Display *dpy = WMScreenDisplay(scr);
 /*   RContext *rc = WMScreenRContext(scr);*/
    int refresh = 0;
    char *tmp;

    if (!panel->preview) {
	panel->preview = XCreatePixmap(dpy, WMWidgetXID(panel->win),
				       220-4, 185-4, WMScreenDepth(scr));

	refresh = -1;
    }

    if (elements & FTITLE) {
	if (panel->ftitle)
	    XFreePixmap(dpy, panel->ftitle);

	tmp = WMGetTextFieldText(panel->focT);
	panel->ftitle = renderTexture(panel, tmp, 180, 20, True);
	free(tmp);
    }

    /* have to repaint everything to make things simple, eliminating
     * clipping stuff */
    if (refresh) {
	
    }
    
    if (refresh<0) {
	WMPixmap *pix;
	pix = WMCreatePixmapFromXPixmaps(scr, panel->preview, None,
					 220-4, 185-4, WMScreenDepth(scr));

	WMSetLabelImage(panel->prevL, pix);
	WMReleasePixmap(pix);
    }
}



static void
changePage(WMWidget *self, void *data)
{
    int i;
    _Panel *panel = (_Panel*)data;
    
    i = WMGetPopUpButtonSelectedItem(self);
    
    if (i==0) {
	WMMapWidget(panel->focF);
	WMMapWidget(panel->unfF);
	WMMapWidget(panel->ownF);
    } else if (i==1) {
	WMMapWidget(panel->backF);
	WMMapWidget(panel->textF);
    } else if (i==2) {
	WMMapWidget(panel->unsF);
	WMMapWidget(panel->selF);
    } else if (i==3) {
	WMMapWidget(panel->workF);
	WMMapWidget(panel->clipF);
    } else if (i==4) {
	WMMapWidget(panel->iconF);
    }

    if (i!=0) {
	WMUnmapWidget(panel->focF);
	WMUnmapWidget(panel->unfF);
	WMUnmapWidget(panel->ownF);
    } 
    if (i!=1) {
	WMUnmapWidget(panel->backF);
	WMUnmapWidget(panel->textF);
    } 
    if (i!=2) {
	WMUnmapWidget(panel->unsF);
	WMUnmapWidget(panel->selF);
    }
    if (i!=3) {
	WMUnmapWidget(panel->workF);
	WMUnmapWidget(panel->clipF);
    }
    if (i!=4) {
	WMUnmapWidget(panel->iconF);
    }
}


static char*
getStrArrayForKey(char *key)
{
    proplist_t v;
    
    v = GetObjectForKey(key);
    if (!v)
	return NULL;

    return PLGetDescription(v);
}


static void
showData(_Panel *panel)
{
    char *str;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMColor *color;
    
    str = GetStringForKey("FTitleColor");
    if (!str) 
	str = "white";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->focC, color);
    WMReleaseColor(color);

    str = GetStringForKey("PTitleColor");
    if (!str) 
	str = "white";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->ownC, color);
    WMReleaseColor(color);

    str = GetStringForKey("UTitleColor");
    if (!str) 
	str = "black";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->unfC, color);
    WMReleaseColor(color);


    str = getStrArrayForKey("FTitleBack");
    if (!str)
	str = wstrdup("(solid, black)");
    WMSetTextFieldText(panel->focT, str);
    free(str);
    
    str = getStrArrayForKey("PTitleBack");
    if (!str)
	str = wstrdup("(solid, gray40)");
    WMSetTextFieldText(panel->ownT, str);
    free(str);
    
    str = getStrArrayForKey("UTitleBack");
    if (!str)
	str = wstrdup("(solid, grey66)");
    WMSetTextFieldText(panel->unfT, str);
    free(str);
    
    /**/
    
    str = GetStringForKey("MenuTitleColor");
    if (!str)
	str = "white";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->textC, color);
    WMReleaseColor(color);
    
    str = getStrArrayForKey("MenuTitleBack");
    if (!str)
	str = wstrdup("(solid, black)");
    WMSetTextFieldText(panel->backT, str);
    free(str);

    /**/

    str = getStrArrayForKey("MenuTextBack");
    if (!str)
	str = wstrdup("gray66");
    WMSetTextFieldText(panel->unsT, str);
    free(str);

    str = GetStringForKey("MenuTextColor");
    if (!str)
	str = "black";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->unsnC, color);
    WMReleaseColor(color);
    
    str = GetStringForKey("MenuDisabledColor");
    if (!str)
	str = "gray40";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->unsdC, color);
    WMReleaseColor(color);

    str = GetStringForKey("HighlightTextColor");
    if (!str)
	str = "white";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->seltC, color);
    WMReleaseColor(color);

    str = GetStringForKey("HighlightColor");
    if (!str)
	str = "black";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->selbC, color);
    WMReleaseColor(color);

    /**/

    str = getStrArrayForKey("WorkspaceBack");
    WMSetTextFieldText(panel->workT, str);
    if (str)
	free(str);

    
    str = GetStringForKey("ClipTitleColor");
    if (!str)
	str = "black";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->clipnC, color);
    WMReleaseColor(color);

    str = GetStringForKey("CClipTitleColor");
    if (!str)
	str = "grey40";
    color = WMCreateNamedColor(scr, str, True);
    WMSetColorWellColor(panel->clipcC, color);
    WMReleaseColor(color);

    /**/

    str = getStrArrayForKey("IconBack");
    if (!str)
	str = wstrdup("(solid, gray66)");
    WMSetTextFieldText(panel->iconT, str);
    free(str);
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    panel->secP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->secP, 220, 20);
    WMMoveWidget(panel->secP, 15, 10);
    WMSetPopUpButtonAction(panel->secP, changePage, panel);
    
    WMAddPopUpButtonItem(panel->secP, _("Window Title Bar"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Title Bar"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Items"));
    WMAddPopUpButtonItem(panel->secP, _("Workspace/Clip"));
    WMAddPopUpButtonItem(panel->secP, _("Icons"));

    panel->prevL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->prevL, 220, 185);
    WMMoveWidget(panel->prevL, 15, 40);
    WMSetLabelRelief(panel->prevL, WRSunken);
    
    /* window titlebar */
    panel->focF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->focF, 265, 70);
    WMMoveWidget(panel->focF, 245, 5);
    WMSetFrameTitle(panel->focF, _("Focused Window"));
    
    panel->focC = WMCreateColorWell(panel->focF);
    WMResizeWidget(panel->focC, 60, 35);
    WMMoveWidget(panel->focC, 15, 15);
    
    panel->focT = WMCreateTextField(panel->focF);
    WMResizeWidget(panel->focT, 116, 20);
    WMMoveWidget(panel->focT, 85, 25);

    panel->foc2L = WMCreateLabel(panel->focF);
    WMResizeWidget(panel->foc2L, 165, 16);
    WMMoveWidget(panel->foc2L, 90, 50);
    WMSetLabelText(panel->foc2L, _("Texture"));
    WMSetLabelTextAlignment(panel->foc2L, WACenter);

    panel->focL = WMCreateLabel(panel->focF);
    WMResizeWidget(panel->focL, 100, 16);
    WMMoveWidget(panel->focL, 15, 50);
    WMSetLabelText(panel->focL, _("Text Color"));
    
    panel->focB = WMCreateCommandButton(panel->focF);
    WMResizeWidget(panel->focB, 48, 22);
    WMMoveWidget(panel->focB, 205, 24);
    WMSetButtonText(panel->focB, _("Set..."));

    WMMapSubwidgets(panel->focF);
    /**/
    panel->unfF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->unfF, 265, 70);
    WMMoveWidget(panel->unfF, 245, 80);
    WMSetFrameTitle(panel->unfF, _("Unfocused Window"));
    
    panel->unfC = WMCreateColorWell(panel->unfF);
    WMResizeWidget(panel->unfC, 60, 35);
    WMMoveWidget(panel->unfC, 15, 15);
    
    panel->unfT = WMCreateTextField(panel->unfF);
    WMResizeWidget(panel->unfT, 116, 20);
    WMMoveWidget(panel->unfT, 85, 25);

    panel->unf2L = WMCreateLabel(panel->unfF);
    WMResizeWidget(panel->unf2L, 165, 16);
    WMMoveWidget(panel->unf2L, 90, 50);
    WMSetLabelText(panel->unf2L, _("Texture"));
    WMSetLabelTextAlignment(panel->unf2L, WACenter);

    panel->unfL = WMCreateLabel(panel->unfF);
    WMResizeWidget(panel->unfL, 100, 16);
    WMMoveWidget(panel->unfL, 15, 50);
    WMSetLabelText(panel->unfL, _("Text Color"));
    
    panel->unfB = WMCreateCommandButton(panel->unfF);
    WMResizeWidget(panel->unfB, 48, 22);
    WMMoveWidget(panel->unfB, 205, 24);
    WMSetButtonText(panel->unfB, _("Set..."));

    WMMapSubwidgets(panel->unfF);
    /**/
    panel->ownF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->ownF, 265, 70);
    WMMoveWidget(panel->ownF, 245, 155);
    WMSetFrameTitle(panel->ownF, _("Owner of Focused Window"));
    
    panel->ownC = WMCreateColorWell(panel->ownF);
    WMResizeWidget(panel->ownC, 60, 35);
    WMMoveWidget(panel->ownC, 15, 15);
    
    panel->ownT = WMCreateTextField(panel->ownF);
    WMResizeWidget(panel->ownT, 116, 20);
    WMMoveWidget(panel->ownT, 85, 25);

    panel->own2L = WMCreateLabel(panel->ownF);
    WMResizeWidget(panel->own2L, 165, 16);
    WMMoveWidget(panel->own2L, 90, 50);
    WMSetLabelText(panel->own2L, _("Texture"));
    WMSetLabelTextAlignment(panel->own2L, WACenter);

    panel->ownL = WMCreateLabel(panel->ownF);
    WMResizeWidget(panel->ownL, 100, 16);
    WMMoveWidget(panel->ownL, 15, 50);
    WMSetLabelText(panel->ownL, _("Text Color"));
    
    panel->ownB = WMCreateCommandButton(panel->ownF);
    WMResizeWidget(panel->ownB, 48, 22);
    WMMoveWidget(panel->ownB, 205, 24);
    WMSetButtonText(panel->ownB, _("Set..."));

    WMMapSubwidgets(panel->ownF);
    
    /***************** Menu Item *****************/
    
    panel->unsF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->unsF, 260, 140);
    WMMoveWidget(panel->unsF, 250, 5);
    WMSetFrameTitle(panel->unsF, _("Unselected Items"));
    
    panel->unsT = WMCreateTextField(panel->unsF);
    WMResizeWidget(panel->unsT, 175, 20);
    WMMoveWidget(panel->unsT, 15, 25);
    
    panel->unsL = WMCreateLabel(panel->unsF);
    WMResizeWidget(panel->unsL, 175, 16);
    WMMoveWidget(panel->unsL, 15, 50);
    WMSetLabelTextAlignment(panel->unsL, WACenter);
    WMSetLabelText(panel->unsL, _("Background"));
    
    panel->unsB = WMCreateCommandButton(panel->unsF);
    WMResizeWidget(panel->unsB, 48, 22);
    WMMoveWidget(panel->unsB, 200, 24);
    WMSetButtonText(panel->unsB, _("Set..."));
    
    panel->unsnC = WMCreateColorWell(panel->unsF);
    WMResizeWidget(panel->unsnC, 60, 40);
    WMMoveWidget(panel->unsnC, 40, 75);
    
    panel->unsnL = WMCreateLabel(panel->unsF);
    WMResizeWidget(panel->unsnL, 120, 16);
    WMMoveWidget(panel->unsnL, 10, 117);
    WMSetLabelTextAlignment(panel->unsnL, WACenter);
    WMSetLabelText(panel->unsnL, _("Normal Text"));

    panel->unsdC = WMCreateColorWell(panel->unsF);
    WMResizeWidget(panel->unsdC, 60, 40);
    WMMoveWidget(panel->unsdC, 160, 75);

    panel->unsdL = WMCreateLabel(panel->unsF);
    WMResizeWidget(panel->unsdL, 120, 16);
    WMMoveWidget(panel->unsdL, 130, 117);
    WMSetLabelTextAlignment(panel->unsdL, WACenter);
    WMSetLabelText(panel->unsdL, _("Disabled Text"));

    WMMapSubwidgets(panel->unsF);
    
    /**/
    
    panel->selF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->selF, 260, 75);
    WMMoveWidget(panel->selF, 250, 150);
    WMSetFrameTitle(panel->selF, _("Selected Items"));

    panel->seltC = WMCreateColorWell(panel->selF);
    WMResizeWidget(panel->seltC, 60, 36);
    WMMoveWidget(panel->seltC, 40, 20);
    
    panel->seltL = WMCreateLabel(panel->selF);
    WMResizeWidget(panel->seltL, 120, 16);
    WMMoveWidget(panel->seltL, 10, 56);
    WMSetLabelTextAlignment(panel->seltL, WACenter);
    WMSetLabelText(panel->seltL, _("Text"));

    panel->selbC = WMCreateColorWell(panel->selF);
    WMResizeWidget(panel->selbC, 60, 36);
    WMMoveWidget(panel->selbC, 160, 20);

    panel->selbL = WMCreateLabel(panel->selF);
    WMResizeWidget(panel->selbL, 120, 16);
    WMMoveWidget(panel->selbL, 130, 56);
    WMSetLabelTextAlignment(panel->selbL, WACenter);
    WMSetLabelText(panel->selbL, _("Background"));

    WMMapSubwidgets(panel->selF);
    
    /***************** Menu Title *****************/
    panel->backF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->backF, 260, 110);
    WMMoveWidget(panel->backF, 250, 35);
    WMSetFrameTitle(panel->backF, _("Menu Title Background"));
    
    panel->backT = WMCreateTextField(panel->backF);
    WMResizeWidget(panel->backT, 210, 20);
    WMMoveWidget(panel->backT, 25, 35);
    
    panel->backB = WMCreateCommandButton(panel->backF);
    WMResizeWidget(panel->backB, 50, 24);
    WMMoveWidget(panel->backB, 185, 60);
    WMSetButtonText(panel->backB, _("Set..."));
    
    WMMapSubwidgets(panel->backF);
    
    /**/
    
    panel->textF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->textF, 260, 75);
    WMMoveWidget(panel->textF, 250, 150);
    WMSetFrameTitle(panel->textF, _("Menu Title Text"));
    
    panel->textC = WMCreateColorWell(panel->textF);
    WMResizeWidget(panel->textC, 60, 40);
    WMMoveWidget(panel->textC, 100, 20);
    
    WMMapSubwidgets(panel->textF);

    /***************** Workspace ****************/
    panel->workF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->workF, 260, 90);
    WMMoveWidget(panel->workF, 250, 35);
    WMSetFrameTitle(panel->workF, _("Workspace Background"));
    
    panel->workT = WMCreateTextField(panel->workF);
    WMResizeWidget(panel->workT, 220, 20);
    WMMoveWidget(panel->workT, 20, 25);
    
    panel->workB = WMCreateCommandButton(panel->workF);
    WMResizeWidget(panel->workB, 70, 24);
    WMMoveWidget(panel->workB, 170, 55);
    WMSetButtonText(panel->workB, _("Change"));

    /**/
    panel->clipF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->clipF, 260, 90);
    WMMoveWidget(panel->clipF, 250, 135);
    WMSetFrameTitle(panel->clipF, _("Clip Title Text"));

    panel->clipnC = WMCreateColorWell(panel->clipF);
    WMResizeWidget(panel->clipnC, 60, 40);
    WMMoveWidget(panel->clipnC, 40, 25);
    
    panel->clipnL = WMCreateLabel(panel->clipF);
    WMResizeWidget(panel->clipnL, 120, 16);
    WMMoveWidget(panel->clipnL, 10, 70);
    WMSetLabelTextAlignment(panel->clipnL, WACenter);
    WMSetLabelText(panel->clipnL, _("Normal"));

    panel->clipcC = WMCreateColorWell(panel->clipF);
    WMResizeWidget(panel->clipcC, 60, 40);
    WMMoveWidget(panel->clipcC, 160, 25);

    panel->clipcL = WMCreateLabel(panel->clipF);
    WMResizeWidget(panel->clipcL, 120, 16);
    WMMoveWidget(panel->clipcL, 130, 70);
    WMSetLabelTextAlignment(panel->clipcL, WACenter);
    WMSetLabelText(panel->clipcL, _("Collapsed"));

    WMMapSubwidgets(panel->clipF);

    
    
    WMMapSubwidgets(panel->workF);

    /***************** Icon *****************/
    panel->iconF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->iconF, 260, 190);
    WMMoveWidget(panel->iconF, 250, 35);
    WMSetFrameTitle(panel->iconF, _("Icon Background"));

    panel->iconT = WMCreateTextField(panel->iconF);
    WMResizeWidget(panel->iconT, 220, 20);
    WMMoveWidget(panel->iconT, 20, 80);

    panel->iconB = WMCreateCommandButton(panel->iconF);
    WMResizeWidget(panel->iconB, 50, 24);
    WMMoveWidget(panel->iconB, 190, 105);
    WMSetButtonText(panel->iconB, _("Set..."));

    WMMapSubwidgets(panel->iconF);
    /**/
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    WMSetPopUpButtonSelectedItem(panel->secP, 0);
    changePage(panel->secP, panel);
    
    
    showData(panel);
}



Panel*
InitTextureAndColor(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));
    
    panel->sectionName = _("Texture and Color Preferences");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;

    AddSection(panel, ICON_FILE);

    return panel;
}
