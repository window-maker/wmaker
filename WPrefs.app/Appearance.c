/* TextureAndColor.c- color/texture for titlebar etc.
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

    WMLabel *prevL;

    WMPopUpButton *secP;

    /* texture list */
    WMLabel *texL;
    WMList *texLs;

    WMPopUpButton *cmdP;
    WMTextField *texT;

    WMButton *editB;

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
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMColor *color;
    WMFont *boldFont;
    WMScreen *scr = WMWidgetScreen(panel->win);

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    /* preview box */
    panel->prevL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->prevL, 260, 190);
    WMMoveWidget(panel->prevL, 10, 10);
    WMSetLabelRelief(panel->prevL, WRSunken);

    panel->secP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->secP, 242, 20);
    WMMoveWidget(panel->secP, 10, 207);
/*    WMSetPopUpButtonAction(panel->secP, changePage, panel);
 */


    /* texture list */
    boldFont = WMBoldSystemFontOfSize(scr, 12);

    panel->texL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->texL, 225, 18);
    WMMoveWidget(panel->texL, 285, 10);
    WMSetLabelFont(panel->texL, boldFont);
    WMSetLabelText(panel->texL, _("Textures"));
    WMSetLabelRelief(panel->texL, WRSunken);
    WMSetLabelTextAlignment(panel->texL, WACenter);
    color = WMDarkGrayColor(scr);
    WMSetWidgetBackgroundColor(panel->texL, color);
    WMReleaseColor(color);
    color = WMWhiteColor(scr);
    WMSetLabelTextColor(panel->texL, color);
    WMReleaseColor(color);

    WMReleaseFont(boldFont);


    panel->texLs = WMCreateList(panel->frame);
    WMResizeWidget(panel->texLs, 225, 144);
    WMMoveWidget(panel->texLs, 285, 30);

    panel->cmdP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->cmdP, 225, 20);
    WMMoveWidget(panel->cmdP, 285, 180);
    WMSetPopUpButtonPullsDown(panel->cmdP, True);
    WMSetPopUpButtonText(panel->cmdP, _("Texture Commands"));
    WMAddPopUpButtonItem(panel->cmdP, _("Create New"));
    WMAddPopUpButtonItem(panel->cmdP, _("Add From Text Field"));
    WMAddPopUpButtonItem(panel->cmdP, _("Remove Selected"));
    WMAddPopUpButtonItem(panel->cmdP, _("Extract From File"));

    panel->editB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->editB, 64, 20);
    WMMoveWidget(panel->editB, 260, 207);
    WMSetButtonText(panel->editB, _("Browse..."));

    panel->texT = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->texT, 176, 20);
    WMMoveWidget(panel->texT, 330, 207);

    /**/

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    WMSetPopUpButtonSelectedItem(panel->secP, 0);



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
