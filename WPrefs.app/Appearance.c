/* Apperance.c- color/texture for titlebar etc.
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1999 Alfredo K. Kojima
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

    WMButton *prevB;

    WMPopUpButton *secP;

    /* texture list */
    WMLabel *texL;
    WMList *texLs;

    WMButton *newB;
    WMButton *editB;
    WMButton *ripB;
    WMButton *delB;


    proplist_t ftitleTex;
    proplist_t utitleTex;
    proplist_t ptitleTex;
    proplist_t iconTex;
    proplist_t menuTex;
    proplist_t mtitleTex;
    proplist_t backTex;
    
    int ftitleIndex;
    int utitleIndex;
    int ptitleIndex;
    int iconIndex;
    int menuIndex;
    int mtitleIndex;
    int backIndex;

    WMFont *normalFont;
    WMFont *selectedFont;

    /* for preview shit */
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

#define TNEW_FILE 	"tnew"
#define TDEL_FILE	"tdel"
#define TEDIT_FILE	"tedit"
#define TEXTR_FILE 	"textr"


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

#if 0
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



static char*
getStrArrayForKey(char *key)
{
    proplist_t v;

    v = GetObjectForKey(key);
    if (!v)
	return NULL;

    return PLGetDescription(v);
}
#endif



static void
newTexture(WMButton *bPtr, void *data)
{
    _Panel *panel = (_Panel*)data;

    
}


static void
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->secP);

    WMSelectListItem(panel->texLs, section);

}


static void
selectTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;

    section = WMGetListSelectedItemRow(panel->secP);
    

}

static void
fillTextureList(WMList *lPtr)
{
    proplist_t textures;
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    textures = WMGetUDObjectForKey(udb, "Textures");

    if (!textures)
	return;

    
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMColor *color;
    WMFont *font;
    WMScreen *scr = WMWidgetScreen(panel->win);

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    /* preview box */
    panel->prevB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->prevB, 260, 165);
    WMMoveWidget(panel->prevB, 15, 10);
    WMSetButtonImagePosition(panel->prevB, WIPImageOnly);

    panel->secP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->secP, 260, 20);
    WMMoveWidget(panel->secP, 15, 180);
    WMSetPopUpButtonSelectedItem(panel->secP, 0);
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Unfocused Windows"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window's Owner"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Menus"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Items"));
    WMAddPopUpButtonItem(panel->secP, _("Icon Background"));
    WMAddPopUpButtonItem(panel->secP, _("Workspace Backgrounds"));
    WMSetPopUpButtonAction(panel->secP, changePage, panel);

    /* texture list */
    font = WMBoldSystemFontOfSize(scr, 12);

    panel->texL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->texL, 225, 18);
    WMMoveWidget(panel->texL, 285, 10);
    WMSetLabelFont(panel->texL, font);
    WMSetLabelText(panel->texL, _("Textures"));
    WMSetLabelRelief(panel->texL, WRSunken);
    WMSetLabelTextAlignment(panel->texL, WACenter);
    color = WMDarkGrayColor(scr);
    WMSetWidgetBackgroundColor(panel->texL, color);
    WMReleaseColor(color);
    color = WMWhiteColor(scr);
    WMSetLabelTextColor(panel->texL, color);
    WMReleaseColor(color);

    WMReleaseFont(font);

    panel->texLs = WMCreateList(panel->frame);
    WMResizeWidget(panel->texLs, 225, 144);
    WMMoveWidget(panel->texLs, 285, 30);
    
    /* command buttons */

    font = WMSystemFontOfSize(scr, 10);


    panel->newB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->newB, 56, 48);
    WMMoveWidget(panel->newB, 285, 180);
    WMSetButtonFont(panel->newB, font);
    WMSetButtonImagePosition(panel->newB, WIPAbove);
    WMSetButtonText(panel->newB, _("New"));
    SetButtonAlphaImage(scr, panel->newB, TNEW_FILE);

    panel->ripB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->ripB, 56, 48);
    WMMoveWidget(panel->ripB, 341, 180);
    WMSetButtonFont(panel->ripB, font);
    WMSetButtonImagePosition(panel->ripB, WIPAbove);
    WMSetButtonText(panel->ripB, _("Extract..."));
    SetButtonAlphaImage(scr, panel->ripB, TEXTR_FILE);

    panel->editB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->editB, 56, 48);
    WMMoveWidget(panel->editB, 397, 180);
    WMSetButtonFont(panel->editB, font);
    WMSetButtonImagePosition(panel->editB, WIPAbove);
    WMSetButtonText(panel->editB, _("Edit"));
    SetButtonAlphaImage(scr, panel->editB, TEDIT_FILE);
    WMSetButtonEnabled(panel->editB, False);

    panel->delB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->delB, 56, 48);
    WMMoveWidget(panel->delB, 453, 180);
    WMSetButtonFont(panel->delB, font);
    WMSetButtonImagePosition(panel->delB, WIPAbove);
    WMSetButtonText(panel->delB, _("Delete"));
    SetButtonAlphaImage(scr, panel->delB, TDEL_FILE);
    WMSetButtonEnabled(panel->delB, False);

    WMReleaseFont(font);

    /**/

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    WMSetPopUpButtonSelectedItem(panel->secP, 0);

    showData(panel);

    fillTextureList(panel->texLs);

}

static proplist_t
setupTextureFor(WMList *list, char *key, char *defValue, char *title)
{
    WMListItem *item;
    char *tex, *str;
    proplist_t prop;

    prop = GetObjectForKey(key);
    if (!prop) {
	prop = PLMakeString(defValue);
    }
    tex = PLGetDescription(prop);
    str = wstrappend(title, tex);
    free(tex);
    item = WMAddListItem(list, str);
    free(str);
    item->clientData = prop;

    return prop;
}


static void
showData(_Panel *panel)
{
    panel->ftitleTex = setupTextureFor(panel->texLs, "FTitleBack", 
				       "(solid, black)", "[Focused]:");
    panel->ftitleIndex = 0;

    panel->utitleTex = setupTextureFor(panel->texLs, "UTitleBack",
				       "(solid, gray)", "[Unfocused]:");
    panel->utitleIndex = 1;

    panel->ptitleTex = setupTextureFor(panel->texLs, "PTitleBack",
				       "(solid, \"#616161\")",
				       "[Owner of Focused]:");
    panel->ptitleIndex = 2;

    panel->mtitleTex = setupTextureFor(panel->texLs, "MenuTitleBack", 
				       "(solid, black)", "[Menu Title]:");
    panel->mtitleIndex = 3;

    panel->menuTex = setupTextureFor(panel->texLs, "MenuTextBack", 
				     "(solid, gray)", "[Menu Item]:");
    panel->menuIndex = 4;

    panel->iconTex = setupTextureFor(panel->texLs, "IconBack",
				     "(solid, gray)", "[Icon]:");
    panel->iconIndex = 5;

    panel->backTex = setupTextureFor(panel->texLs, "WorkspaceBack",
				     "(solid, black)", "[Workspace]:");
    panel->backIndex = 6;
}



Panel*
InitAppearance(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));
    
    panel->sectionName = _("Appearance Preferences");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;

    AddSection(panel, ICON_FILE);

    return panel;
}
