/* Font.c- text/font settings
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

#include <proplist.h>

typedef struct _Panel {
    WMFrame *frame;
    char *sectionName;   

    char *description;

    CallbackRec callbacks;
    
    WMWindow *win;


    WMLabel *prevL;

    WMFrame *langF;
    WMPopUpButton *langP;
        
    /* single byte */
    WMTextField *fontT;
    WMButton *changeB;
    
    /* multibyte */
    WMLabel *fsetL;
    WMList *fsetLs;
    
    WMButton *addB;
    WMButton *editB;
    WMButton *remB;

    
    WMFont *windowTitleFont;
    WMFont *menuTitleFont;
    WMFont *menuItemFont;
    
    
    WMColor *white;
    WMColor *black;
    WMColor *light;
    WMColor *dark;
    
    WMColor *back;

    Pixmap preview;
    WMPixmap *previewPix;
} _Panel;



#define ICON_FILE	"fonts"



static proplist_t DefaultWindowTitleFont = NULL;
static proplist_t DefaultMenuTitleFont = NULL;
static proplist_t DefaultMenuTextFont = NULL;
static proplist_t DefaultIconTitleFont = NULL;
static proplist_t DefaultClipTitleFont = NULL;
static proplist_t DefaultDisplayFont = NULL;



static void
drawMenuItem(WMScreen *scr, Display *dpy, Drawable d, 
	     int x, int y, int w, int h, 
	     GC light, GC dark, GC black, GC white,
	     WMFont *font, int fh, char *text)
{
    XFillRectangle(dpy, d, light, x, y, w, h);
    
    XDrawLine(dpy, d, black, x, y, x, y+h);
    XDrawLine(dpy, d, black, x+w, y, x+w, y+h);
    
    XDrawLine(dpy, d, white, x+1, y, x+1, y+h-1);
    XDrawLine(dpy, d, white, x+1, y, x+w-1, y);
    
    XDrawLine(dpy, d, dark, x+w-1, y+1, x+w-1, y+h-3);
    XDrawLine(dpy, d, dark, x+1, y+h-2, x+w-1, y+h-2);
    
    XDrawLine(dpy, d, black, x, y+h-1, x+w, y+h-1);

    WMDrawString(scr, d, black, font, x + 5, y+(h-fh)/2, 
		 text, strlen(text));
}



static void
paintPreviewBox(Panel *panel)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    Display *dpy = WMScreenDisplay(scr);
    GC black = WMColorGC(panel->black);
    GC white = WMColorGC(panel->white);
    GC dark = WMColorGC(panel->dark);
    GC light = WMColorGC(panel->light);
    
    
    if (panel->preview == None) {
	WMPixmap *pix;
	
	panel->preview = XCreatePixmap(dpy, WMWidgetXID(panel->win),
				       240-4, 215-4, WMScreenDepth(scr));

	pix = WMCreatePixmapFromXPixmaps(scr, panel->preview, None,
					 240-4, 215-4, WMScreenDepth(scr));

	WMSetLabelImage(panel->prevL, pix);
	WMReleasePixmap(pix);
    }
    
    XFillRectangle(dpy, panel->preview, WMColorGC(panel->back),
		   0, 0, 240-4, 215-4);

    /* window title */
    {
	int h, fh;
	
	fh = WMFontHeight(panel->windowTitleFont);
	h = fh+6;
	
	XFillRectangle(dpy, panel->preview, black,
		       19, 19, 203, h+3);

	XDrawLine(dpy, panel->preview, light,
		  20, 20, 220, 20);
	XDrawLine(dpy, panel->preview, light,
		  20, 20, 20, 20+h);

	XDrawLine(dpy, panel->preview, dark,
		  20, 20+h, 220, 20+h);	
	XDrawLine(dpy, panel->preview, dark,
		  220, 20, 220, 20+h);
	
	WMDrawString(scr, panel->preview, white, panel->windowTitleFont,
		     20+(200-WMWidthOfString(panel->windowTitleFont, "Window Titlebar", 15))/2,
		     20+(h-fh)/2, "Window Titlebar", 15);
    }
    
    /* menu title */
    {
	int h, h2, fh, fh2;
	int i;
	const mx = 20;
	const my = 120;
	const mw = 100;

	
	fh = WMFontHeight(panel->menuTitleFont);
	h = fh+6;

	XFillRectangle(dpy, panel->preview, black,
		       mx-1, my-1, mw+3, h+3);

	XDrawLine(dpy, panel->preview, light,
		  mx, my, mx+mw, my);
	XDrawLine(dpy, panel->preview, light,
		  mx, my, mx, my+h);

	XDrawLine(dpy, panel->preview, dark,
		  mx, my+h, mx+mw, my+h);	
	XDrawLine(dpy, panel->preview, dark,
		  mx+mw, my, mx+mw, my+h);

	WMDrawString(scr, panel->preview, white, panel->menuTitleFont,
		     mx+5, my+(h-fh)/2, "Menu Title", 10);
	
	fh2 = WMFontHeight(panel->menuItemFont);
	h2 = fh2+6;
	
	/* menu items */
	for (i = 0; i < 4; i++) {
	    drawMenuItem(scr, dpy, panel->preview,
			 mx-1, my+2+h+i*h2, mw+2, h2,
			 light, dark, black, white, 
			 panel->menuItemFont, fh2,
			 "Menu Item");
	}
    }


    WMRedisplayWidget(panel->prevL);
}



static void
showData(_Panel *panel)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    char *str;
    
    str = GetStringForKey("WindowTitleFont");
        
    panel->windowTitleFont = WMCreateFont(scr, str);
    
    
    str = GetStringForKey("MenuTitleFont");
    
    panel->menuTitleFont = WMCreateFont(scr, str);
    
    
    str = GetStringForKey("MenuTextFont");
    
    panel->menuItemFont = WMCreateFont(scr, str);
    
    
    
    
    paintPreviewBox(panel);
}


static void
setLanguageType(Panel *p, Bool multiByte)
{
    if (multiByte) {
	WMMapWidget(p->fsetL);
	WMMapWidget(p->fsetLs);
	WMMapWidget(p->addB);
	WMMapWidget(p->editB);
	WMMapWidget(p->remB);
	
	WMUnmapWidget(p->fontT);
	WMUnmapWidget(p->changeB);
    } else {
	WMUnmapWidget(p->fsetL);
	WMUnmapWidget(p->fsetLs);
	WMUnmapWidget(p->addB);
	WMUnmapWidget(p->editB);
	WMUnmapWidget(p->remB);
	
	WMMapWidget(p->fontT);
	WMMapWidget(p->changeB);
    }
}






static void
readFontEncodings(Panel *panel)
{
    proplist_t pl = NULL;
    char *path;
    char *msg;

    path = WMPathForResourceOfType("font.data", NULL);
    if (!path) {
	msg = _("Could not locate font information file WPrefs.app/font.data");
	goto error;
    }
    
    pl = PLGetProplistWithPath(path);
    if (!pl) {
	msg = _("Could not read font information file WPrefs.app/font.data");
	goto error;
    } else {
	int i;
	proplist_t key = PLMakeString("Encodings");
	proplist_t array;
	WMMenuItem *mi;
	
	array = PLGetDictionaryEntry(pl, key);
	PLRelease(key);
	if (!array || !PLIsArray(array)) {
	    msg = _("Invalid data in font information file WPrefs.app/font.data.\n"
		    "Encodings data not found.");
	    goto error;
	}

	WMAddPopUpButtonItem(panel->langP, _("- Custom -"));
	
	for (i = 0; i < PLGetNumberOfElements(array); i++) {
	    proplist_t item, str;

	    item = PLGetArrayElement(array, i);
	    str = PLGetArrayElement(item, 0);
	    mi = WMAddPopUpButtonItem(panel->langP, PLGetString(str));
	    WMSetMenuItemRepresentedObject(mi, PLRetain(item));
	}
	
	key = PLMakeString("WindowTitleFont");
	DefaultWindowTitleFont = PLGetDictionaryEntry(pl, key);
	PLRelease(key);
	
	key = PLMakeString("MenuTitleFont");
	DefaultMenuTitleFont = PLGetDictionaryEntry(pl, key);
	PLRelease(key);

	key = PLMakeString("MenuTextFont");
	DefaultMenuTextFont = PLGetDictionaryEntry(pl, key);
	PLRelease(key);
    }

    PLRelease(pl);
    return;
error:
    if (pl)
	PLRelease(pl);

    WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win, 
		    _("Error"), msg, _("OK"), NULL, NULL);
}



static void
changeLanguageAction(WMWidget *w, void *data)
{
    Panel *panel = (Panel*)data;
    WMMenuItem *mi;
    proplist_t pl, font;
    char buffer[512];
    
    mi = WMGetPopUpButtonMenuItem(w, WMGetPopUpButtonSelectedItem(w));
    pl = WMGetMenuItemRepresentedObject(mi);

    if (!pl) {
	/* custom */
    } else {
	
    }
}


static void
createPanel(Panel *p)
{    
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    

    panel->prevL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->prevL, 240, FRAME_HEIGHT-20);
    WMMoveWidget(panel->prevL, 15, 10);
    WMSetLabelRelief(panel->prevL, WRSunken);
    WMSetLabelImagePosition(panel->prevL, WIPImageOnly);

    
    /* language selection */
    
    panel->langF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->langF, 245, 50);
    WMMoveWidget(panel->langF, 265, 10);
    WMSetFrameTitle(panel->langF, _("Default Font Sets"));

    panel->langP = WMCreatePopUpButton(panel->langF);
    WMResizeWidget(panel->langP, 215, 20);
    WMMoveWidget(panel->langP, 15, 20);

    WMMapSubwidgets(panel->langF);

    
    /* multibyte */
    panel->fsetL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->fsetL, 205, 20);
    WMMoveWidget(panel->fsetL, 215, 127);
    WMSetLabelText(panel->fsetL, _("Font Set"));
    WMSetLabelRelief(panel->fsetL, WRSunken);
    WMSetLabelTextAlignment(panel->fsetL, WACenter);
    {
	WMFont *font;
	WMColor *color;
	
	color = WMDarkGrayColor(scr);
	font = WMBoldSystemFontOfSize(scr, 12);
	
	WMSetWidgetBackgroundColor(panel->fsetL, color);
	WMSetLabelFont(panel->fsetL, font);
	
	WMReleaseFont(font);
	WMReleaseColor(color);
	
	color = WMWhiteColor(scr);
	WMSetLabelTextColor(panel->fsetL, color);
	WMReleaseColor(color);
    }

    panel->fsetLs = WMCreateList(panel->frame);
    WMResizeWidget(panel->fsetLs, 205, 71);
    WMMoveWidget(panel->fsetLs, 215, 149);
    

    panel->addB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->addB, 80, 24);
    WMMoveWidget(panel->addB, 430, 127);
    WMSetButtonText(panel->addB, _("Add..."));

    panel->editB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->editB, 80, 24);
    WMMoveWidget(panel->editB, 430, 161);
    WMSetButtonText(panel->editB, _("Change..."));

    panel->remB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->remB, 80, 24);
    WMMoveWidget(panel->remB, 430, 195);
    WMSetButtonText(panel->remB, _("Remove"));

    /* single byte */
    panel->fontT = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->fontT, 240, 20);
    WMMoveWidget(panel->fontT, 265, 130);
    
    panel->changeB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->changeB, 104, 24);
    WMMoveWidget(panel->changeB, 335, 160);
    WMSetButtonText(panel->changeB, _("Change..."));

    
    panel->black = WMBlackColor(scr);
    panel->white = WMWhiteColor(scr);
    panel->light = WMGrayColor(scr);
    panel->dark = WMDarkGrayColor(scr);
    panel->back = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True); 
    
#if 0    
    for (i = 0; Languages[i].language != NULL; i++) {
	WMAddPopUpButtonItem(panel->langP, Languages[i].language);
    }

    for (i = 0; Options[i].description != NULL; i++) {
	WMAddListItem(panel->settingLs, Options[i].description);
    }
#endif
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    setLanguageType(panel, False);

    showData(panel);
    
    readFontEncodings(panel);
}




Panel*
InitFont(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Font Preferences");
    panel->description = _("Font Configurations for Windows, Menus etc");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    
    AddSection(panel, ICON_FILE);
    
    return panel;
}

