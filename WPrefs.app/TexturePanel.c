/* TexturePanel.c- texture editting panel
 * 
 *  WPrefs - WindowMaker Preferences Program
 * 
 *  Copyright (c) 1998, 1999 Alfredo K. Kojima
 *  Copyright (c) 1998 James Thompson
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <X11/Xlib.h>


#include <WINGs.h>
#include <WUtil.h>

#include "WPrefs.h"

#include "TexturePanel.h"


#define MAX_SECTION_PARTS 4

typedef struct _TexturePanel {
    WMWindow *win;

    /* texture name */
    WMFrame *nameF;
    WMTextField *nameT;
    
    /* texture type */
    WMPopUpButton *typeP;

    /* default color */
    WMFrame *defcF;
    WMColorWell *defcW;

    WMFont *listFont;

    /*-- Gradient --*/

    Pixmap gimage;

    /* colors */
    WMFrame *gcolF;
    WMList *gcolL;
    WMButton *gcolaB;
    WMButton *gcoldB;
    WMSlider *ghueS;
    WMSlider *gsatS;
    WMSlider *gvalS;

    WMSlider *gbriS;
    WMSlider *gconS;

    /* direction (common) */
    WMFrame *dirF;
    WMButton *dirhB;
    WMButton *dirvB;
    WMButton *dirdB;

    /*-- Simple Gradient --*/


    /*-- Textured Gradient --*/

    WMFrame *tcolF;
    WMColorWell *tcol1W;
    WMColorWell *tcol2W;

    WMFrame *topaF;
    WMSlider *topaS;

    /*-- Image --*/
    WMFrame *imageF;
    WMScrollView *imageV;
    WMTextField *imageT;
    WMLabel *imageL;
    WMButton *browB;
    WMButton *dispB;
    WMPopUpButton *arrP;

    RImage *image;
    char *imageFile;

    /*****/

    WMButton *okB;
    WMButton *cancelB;
    
    
    WMCallback *okAction;
    void *okData;
    
    WMCallback *cancelAction;
    void *cancelData;

    /****/
    WMWidget *sectionParts[5][MAX_SECTION_PARTS];

    int currentType;

    
    proplist_t pathList;

} _TexturePanel;



#define TYPE_SOLID	0
#define TYPE_GRADIENT	1
#define TYPE_SGRADIENT	2
#define TYPE_TGRADIENT	3
#define TYPE_PIXMAP	4


#define PTYPE_TILE	0
#define PTYPE_SCALE	1
#define PTYPE_CENTER	2
#define PTYPE_MAXIMIZE	3



/*
 *--------------------------------------------------------------------------
 * Private Functions
 *--------------------------------------------------------------------------
 */

/************/

static void
updateGradButtons(TexturePanel *panel)
{
    RImage *image;
    WMPixmap *pixmap;
    int colorn;
    RColor **colors;

    colorn = WMGetListNumberOfRows(panel->gcolL);
    if (colorn < 1) {
	pixmap = NULL;
    } else {
	int i;
	WMListItem *item;

	colors = wmalloc(sizeof(RColor*)*(colorn+1));

	for (i = 0; i < colorn; i++) {
	    item = WMGetListItem(panel->gcolL, i);
	    colors[i] = (RColor*)item->clientData;
	}
	colors[i] = NULL;

	image = RRenderMultiGradient(80, 30, colors, RHorizontalGradient);
	pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
					  image, 128);
	RDestroyImage(image);
	WMSetButtonImage(panel->dirhB, pixmap);
	WMReleasePixmap(pixmap);

	image = RRenderMultiGradient(80, 30, colors, RVerticalGradient);
	pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
					  image, 128);
	RDestroyImage(image);
	WMSetButtonImage(panel->dirvB, pixmap);
	WMReleasePixmap(pixmap);

	image = RRenderMultiGradient(80, 30, colors, RDiagonalGradient);
	pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
					  image, 128);
	RDestroyImage(image);
	WMSetButtonImage(panel->dirdB, pixmap);
	WMReleasePixmap(pixmap);

	free(colors);
    }
}



static void
updateTGradImage(TexturePanel *panel)
{
    RImage *image, *gradient;
    WMPixmap *pixmap;
    RColor from;
    RColor to;
    WMColor *color;

    if (!panel->image)
	return;
    
    color = WMGetColorWellColor(panel->tcol1W);
    from.red = WMRedComponentOfColor(color)>>8;
    from.green = WMGreenComponentOfColor(color)>>8;
    from.blue = WMBlueComponentOfColor(color)>>8;

    color = WMGetColorWellColor(panel->tcol2W);
    to.red = WMRedComponentOfColor(color)>>8;
    to.green = WMGreenComponentOfColor(color)>>8;
    to.blue = WMBlueComponentOfColor(color)>>8;

    if (panel->image->width < 141 || panel->image->height < 91) {
	image = RMakeTiledImage(panel->image, 141, 91);
    } else {
	image = RCloneImage(panel->image);
    }

    if (WMGetButtonSelected(panel->dirhB)) {
	gradient = RRenderGradient(image->width, image->height, &from, &to,
				   RHorizontalGradient);
    } else if (WMGetButtonSelected(panel->dirvB)) {
	gradient = RRenderGradient(image->width, image->height, &from, &to,
				   RVerticalGradient);
    } else {
	gradient = RRenderGradient(image->width, image->height, &from, &to,
				   RDiagonalGradient);
    }

    RCombineImagesWithOpaqueness(image, gradient, 
				 WMGetSliderValue(panel->topaS));
    RDestroyImage(gradient);
    pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->win),
				      image, 128);

    WMSetLabelImage(panel->imageL, pixmap);
    WMResizeWidget(panel->imageL, image->width, image->height);
    RDestroyImage(image);
}


static void
updateSGradButtons(TexturePanel *panel)
{
    RImage *image;
    WMPixmap *pixmap;
    RColor from;
    RColor to;
    WMColor *color;

    color = WMGetColorWellColor(panel->tcol1W);
    from.red = WMRedComponentOfColor(color)>>8;
    from.green = WMGreenComponentOfColor(color)>>8;
    from.blue = WMBlueComponentOfColor(color)>>8;

    color = WMGetColorWellColor(panel->tcol2W);
    to.red = WMRedComponentOfColor(color)>>8;
    to.green = WMGreenComponentOfColor(color)>>8;
    to.blue = WMBlueComponentOfColor(color)>>8;

    image = RRenderGradient(80, 30, &from, &to, RHorizontalGradient);
    pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
				      image, 128);
    RDestroyImage(image);
    WMSetButtonImage(panel->dirhB, pixmap);
    WMReleasePixmap(pixmap);
    
    image = RRenderGradient(80, 30, &from, &to, RVerticalGradient);
    pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
				      image, 128);
    RDestroyImage(image);
    WMSetButtonImage(panel->dirvB, pixmap);
    WMReleasePixmap(pixmap);
    
    image = RRenderGradient(80, 30, &from, &to, RDiagonalGradient);
    pixmap = WMCreatePixmapFromRImage(WMWidgetScreen(panel->gcolL),
				      image, 128);
    RDestroyImage(image);
    WMSetButtonImage(panel->dirdB, pixmap);
    WMReleasePixmap(pixmap);
}


/*********** Gradient ************/

static void
updateSVSlider(WMSlider *sPtr, Bool saturation, WMFont *font, RHSVColor *hsv)
{
    RImage *image;
    WMPixmap *pixmap;
    WMScreen *scr = WMWidgetScreen(sPtr);
    RColor from, to;
    RHSVColor tmp;

    tmp = *hsv;
    if (saturation) {
	tmp.saturation = 0;
	RHSVtoRGB(&tmp, &from);
	tmp.saturation = 255;
	RHSVtoRGB(&tmp, &to);
    } else {
	tmp.value = 0;
	RHSVtoRGB(&tmp, &from);
	tmp.value = 255;
	RHSVtoRGB(&tmp, &to);
    }
    image = RRenderGradient(130, 16, &from, &to, RHorizontalGradient);
    pixmap = WMCreatePixmapFromRImage(scr, image, 128);
    RDestroyImage(image);

    if (hsv->value < 128 || !saturation) {
	WMColor *col = WMWhiteColor(scr);

	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(col), font, 2,
		     (16 - WMFontHeight(font))/2 - 1,
		     saturation ? "Saturation" : "Brightness", 10);
	WMReleaseColor(col);
    } else {
	WMColor *col = WMBlackColor(scr);

	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(col), font, 2,
		     (16 - WMFontHeight(font))/2 - 1,
		     saturation ? "Saturation" : "Brightness", 10);
	WMReleaseColor(col);
    }
    WMSetSliderImage(sPtr, pixmap);
    WMReleasePixmap(pixmap);
}


static void
updateHueSlider(WMSlider *sPtr, WMFont *font, RHSVColor *hsv)
{
    RColor *colors[8];
    RImage *image;
    WMPixmap *pixmap;
    WMScreen *scr = WMWidgetScreen(sPtr);
    RHSVColor thsv;
    int i;

    thsv = *hsv;
    for (i = 0; i <= 6; i++) {
        thsv.hue = (360*i)/6;
        colors[i] = wmalloc(sizeof(RColor));
        RHSVtoRGB(&thsv, colors[i]);
    }
    colors[i] = NULL;

    image = RRenderMultiGradient(130, 16, colors, RGRD_HORIZONTAL);
    pixmap = WMCreatePixmapFromRImage(scr, image, 128);
    RDestroyImage(image);

    if (hsv->value < 128) {
	WMColor *col = WMWhiteColor(scr);

	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(col), font, 2,
		     (16 - WMFontHeight(font))/2 - 1, "Hue", 3);
	WMReleaseColor(col);
    } else {
	WMColor *col = WMBlackColor(scr);

	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(col), font, 2,
		     (16 - WMFontHeight(font))/2 - 1, "Hue", 3);
	WMReleaseColor(col);
    }
    WMSetSliderImage(sPtr, pixmap);
    WMReleasePixmap(pixmap);

    for (i = 0; i <= 6; i++)
	free(colors[i]);
}



static void
sliderChangeCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    RHSVColor hsv, *hsvp;
    int row, rows;
    WMListItem *item;
    RColor **colors;
    int i;
    RImage *image;
    WMScreen *scr = WMWidgetScreen(w);

    hsv.hue = WMGetSliderValue(panel->ghueS);
    hsv.saturation = WMGetSliderValue(panel->gsatS);
    hsv.value = WMGetSliderValue(panel->gvalS);

    row = WMGetListSelectedItemRow(panel->gcolL);
    if (row >= 0) {
	RColor *rgb;

	item = WMGetListItem(panel->gcolL, row);

	rgb = (RColor*)item->clientData;

	RHSVtoRGB(&hsv, rgb);
	
	sprintf(item->text, "%02x,%02x,%02x", rgb->red, rgb->green, rgb->blue);
    }

    if (w == panel->ghueS) {
	updateSVSlider(panel->gsatS, True, panel->listFont, &hsv);
	updateSVSlider(panel->gvalS, False, panel->listFont, &hsv);
    } else if (w == panel->gsatS) {
	updateHueSlider(panel->ghueS, panel->listFont, &hsv);
	updateSVSlider(panel->gvalS, False, panel->listFont, &hsv);
    } else {
	updateHueSlider(panel->ghueS, panel->listFont, &hsv);
	updateSVSlider(panel->gsatS, True, panel->listFont, &hsv);
    }

    rows = WMGetListNumberOfRows(panel->gcolL);
    if (rows == 0)
	return;

    colors = wmalloc(sizeof(RColor*)*(rows+1));

    for (i = 0; i < rows; i++) {
	item = WMGetListItem(panel->gcolL, i);

	colors[i] = (RColor*)item->clientData;
    }
    colors[i] = NULL;

    if (panel->gimage != None) {
	XFreePixmap(WMScreenDisplay(scr), panel->gimage);
    }

    image = RRenderMultiGradient(30, i*WMGetListItemHeight(panel->gcolL), 
				 colors, RVerticalGradient);
    RConvertImage(WMScreenRContext(scr), image, &panel->gimage);
    RDestroyImage(image);

    free(colors);

    WMRedisplayWidget(panel->gcolL);

    updateGradButtons(panel);
}


static void
paintGradListItem(WMList *lPtr, int index, Drawable d, char *text, int state, 
		  WMRect *rect)
{
    TexturePanel *panel = (TexturePanel*)WMGetHangedData(lPtr);
    WMScreen *scr = WMWidgetScreen(lPtr);
    int width, height, x, y;
    Display *dpy;
    WMColor *white = WMWhiteColor(scr);
    WMListItem *item;
    WMColor *black = WMBlackColor(scr);

    dpy = WMScreenDisplay(scr);

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
        XFillRectangle(dpy, d, WMColorGC(white), x, y, width, height);
    else
        XClearArea(dpy, d, x, y, width, height, False);

    item = WMGetListItem(lPtr, index);

    if (panel->gimage) {
	XCopyArea(WMScreenDisplay(scr), panel->gimage, d, WMColorGC(white),
		  0, height*index, 30, height, x + 5, y);
    }
    WMDrawString(scr, d, WMColorGC(black), panel->listFont, 
		 x + 40, y + 1, text, strlen(text));

    WMReleaseColor(white);
    WMReleaseColor(black);
}



static void
gradAddCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    WMListItem *item;
    int row;
    RColor *rgb;

    row = WMGetListSelectedItemRow(panel->gcolL) + 1;
    item = WMInsertListItem(panel->gcolL, row, "00,00,00");
    rgb = wmalloc(sizeof(RColor));
    memset(rgb, 0, sizeof(RColor));
    item->clientData = rgb;

    WMSelectListItem(panel->gcolL, row);

    updateGradButtons(panel);

    sliderChangeCallback(panel->ghueS, panel);
}



static void
gradClickCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    WMListItem *item;
    int row;
    RHSVColor hsv;

    row = WMGetListSelectedItemRow(w);
    if (row < 0)
	return;

    item = WMGetListItem(panel->gcolL, row);
    RRGBtoHSV((RColor*)item->clientData, &hsv);

    WMSetSliderValue(panel->ghueS, hsv.hue);
    WMSetSliderValue(panel->gsatS, hsv.saturation);
    WMSetSliderValue(panel->gvalS, hsv.value);

    sliderChangeCallback(panel->ghueS, panel);
    sliderChangeCallback(panel->gsatS, panel);
}


static void
gradDeleteCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    WMListItem *item;
    int row;

    row = WMGetListSelectedItemRow(panel->gcolL);
    if (row < 0)
	return;

    item = WMGetListItem(panel->gcolL, row);
    free(item->clientData);

    WMRemoveListItem(panel->gcolL, row);

    WMSelectListItem(panel->gcolL, row - 1);

    updateGradButtons(panel);

    gradClickCallback(panel->gcolL, panel);
}


/*************** Simple Gradient ***************/

static void
colorWellObserver(void *self, WMNotification *n)
{
    updateSGradButtons(self);
}




static void
opaqChangeCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;

    updateTGradImage(panel);
}

/****************** Image ******************/

static void
updateImage(TexturePanel *panel, char *path)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    RImage *image, *scaled;
    WMPixmap *pixmap;
    WMSize size;

    if (path) {
	image = RLoadImage(WMScreenRContext(scr), path, 0);
	if (!image) {
	    char *message;
	    
	    message = wstrappend(_("Could not load the selected file: "),
				 (char*)RMessageForError(RErrorCode));

	    WMRunAlertPanel(scr, panel->win, _("Error"), message,
			    _("OK"), NULL, NULL);
	    return;
	}

	if (panel->image)
	    RDestroyImage(panel->image);
	panel->image = image;
    } else {
	image = panel->image;
    }
    
    if (WMGetPopUpButtonSelectedItem(panel->typeP) == TYPE_PIXMAP) {
	if (image) {
	    pixmap = WMCreatePixmapFromRImage(scr, image, 128);

	    size = WMGetPixmapSize(pixmap);
	    WMSetLabelImage(panel->imageL, pixmap);
	    WMResizeWidget(panel->imageL, size.width, size.height);
	
	    WMReleasePixmap(pixmap);
	}
    } else {
	updateTGradImage(panel);
    }
}


static void
browseImageCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    WMOpenPanel *opanel;
    WMScreen *scr = WMWidgetScreen(w);
    static char *ipath = NULL;

    opanel = WMGetOpenPanel(scr);
    WMSetFilePanelCanChooseDirectories(opanel, False);
    WMSetFilePanelCanChooseFiles(opanel, True);

    if (!ipath)
	ipath = wstrdup(wgethomedir());
    
    if (WMRunModalFilePanelForDirectory(opanel, panel->win, ipath,
					"Open Image", NULL)) {
	char *path, *fullpath;
	char *tmp, *tmp2;

	tmp = WMGetFilePanelFileName(opanel);
	if (!tmp)
	    return;
	fullpath = tmp;

	free(ipath);
	ipath = fullpath;

	path = wstrdup(fullpath);

	tmp2 = strrchr(fullpath, '/');
	if (tmp2)
	    tmp2++;

	tmp = wfindfileinarray(panel->pathList, tmp2);

	if (tmp) {
	    if (strcmp(fullpath, tmp)==0) {
		free(path);
		path = tmp2;
	    }
	    free(tmp);
	}

	if (!RGetImageFileFormat(fullpath)) {
	    WMRunAlertPanel(scr, panel->win, _("Error"),
			    _("The selected file does not contain a supported image."),
			    _("OK"), NULL, NULL);
	    free(path);
	} else {
	    updateImage(panel, fullpath);
	    free(panel->imageFile);
	    panel->imageFile = path;

	    WMSetTextFieldText(panel->imageT, path);
	}
    }
}



static void
buttonCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    
    if (w == panel->okB) {
	(*panel->okAction)(panel->okData);
    } else {
	(*panel->cancelAction)(panel->cancelData);
    }
}



static void
changeTypeCallback(WMWidget *w, void *data)
{
    TexturePanel *panel = (TexturePanel*)data;
    int newType;
    int i;

    newType = WMGetPopUpButtonSelectedItem(w);
    if (newType == panel->currentType)
	return;

    if (panel->currentType >= 0) {
	for (i = 0; i < MAX_SECTION_PARTS; i++) {
	    if (panel->sectionParts[panel->currentType][i] == NULL)
		break;
	    WMUnmapWidget(panel->sectionParts[panel->currentType][i]);
	}
    }

    for (i = 0; i < MAX_SECTION_PARTS; i++) {
	if (panel->sectionParts[newType][i] == NULL)
	    break;
	WMMapWidget(panel->sectionParts[newType][i]);
    }
    panel->currentType = newType;

    switch (newType) {
     case TYPE_SGRADIENT:
	updateSGradButtons(panel);
	break;
     case TYPE_GRADIENT:
	updateGradButtons(panel);
	break;
     case TYPE_TGRADIENT:
     case TYPE_PIXMAP:
	updateImage(panel, NULL);
	break;
    }
}


/*
 *--------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------
 */
void
DestroyTexturePanel(TexturePanel *panel)
{

}


void
ShowTexturePanel(TexturePanel *panel)
{
    Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->win));
    Screen *scr = DefaultScreenOfDisplay(dpy);

    WMSetWindowUPosition(panel->win, 
			 (WidthOfScreen(scr)-WMWidgetWidth(panel->win))/2,
			 (HeightOfScreen(scr)-WMWidgetHeight(panel->win))/2);
    WMMapWidget(panel->win);
}


void
HideTexturePanel(TexturePanel *panel)
{
    WMUnmapWidget(panel->win);
}


void
SetTexturePanelOkAction(TexturePanel *panel, WMCallback *action, void *clientData)
{
    panel->okAction = action;
    panel->okData = clientData;
}


void
SetTexturePanelCancelAction(TexturePanel *panel, WMCallback *action, void *clientData)
{
    panel->cancelAction = action;
    panel->cancelData = clientData;
}


void
SetTexturePanelTexture(TexturePanel *panel, char *name, proplist_t texture)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    char *str, *type;
    proplist_t p;
    WMColor *color;
    int i;
    char buffer[64];
    int gradient = 0;

    WMSetTextFieldText(panel->nameT, name);

    if (!texture)
	return;

    p = PLGetArrayElement(texture, 0);
    if (!p) {
	goto bad_texture;
    }
    type = PLGetString(p);

    /*...............................................*/
    if (strcasecmp(type, "solid")==0) {

	WMSetPopUpButtonSelectedItem(panel->typeP, TYPE_SOLID);

	p = PLGetArrayElement(texture, 1);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->defcW, color);

	WMReleaseColor(color);
    /*...............................................*/
    } else if (strcasecmp(type, "hgradient")==0
	       || strcasecmp(type, "vgradient")==0
	       || strcasecmp(type, "dgradient")==0) {

	WMSetPopUpButtonSelectedItem(panel->typeP, TYPE_SGRADIENT);

	p = PLGetArrayElement(texture, 1);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->tcol1W, color);

	WMReleaseColor(color);

	p = PLGetArrayElement(texture, 2);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->tcol2W, color);

	WMReleaseColor(color);

	gradient = type[0];
    /*...............................................*/
    } else if (strcasecmp(type, "thgradient")==0
	       || strcasecmp(type, "tvgradient")==0
	       || strcasecmp(type, "tdgradient")==0) {
	int i;

	WMSetPopUpButtonSelectedItem(panel->typeP, TYPE_TGRADIENT);

	gradient = type[1];

	WMSetTextFieldText(panel->imageT,
			   PLGetString(PLGetArrayElement(texture, 1)));
	if (panel->imageFile)
	    free(panel->imageFile);
	panel->imageFile = wstrdup(PLGetString(PLGetArrayElement(texture, 1)));


	i = 180;
	sscanf(PLGetString(PLGetArrayElement(texture, 2)), "%i", &i);
	WMSetSliderValue(panel->topaS, i);

	p = PLGetArrayElement(texture, 3);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->tcol1W, color);

	WMReleaseColor(color);

	p = PLGetArrayElement(texture, 4);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->tcol2W, color);

	WMReleaseColor(color);
	
	WMSetTextFieldText(panel->imageT,
			   PLGetString(PLGetArrayElement(texture, 1)));
	
	if (panel->imageFile)
	    free(panel->imageFile);
	panel->imageFile = wfindfileinarray(panel->pathList,
			    PLGetString(PLGetArrayElement(texture, 1)));

	panel->image = RLoadImage(WMScreenRContext(scr), panel->imageFile, 0);
	updateTGradImage(panel);
	
	updateSGradButtons(panel);

    /*...............................................*/
    } else if (strcasecmp(type, "mhgradient")==0
	       || strcasecmp(type, "mvgradient")==0
	       || strcasecmp(type, "mdgradient")==0) {
	WMListItem *item;

	for (i = 0; i < WMGetListNumberOfRows(panel->gcolL); i++) {
	    item = WMGetListItem(panel->gcolL, i);
	    free(item->clientData);
	}
	WMClearList(panel->gcolL);

	WMSetPopUpButtonSelectedItem(panel->typeP, TYPE_GRADIENT);

	p = PLGetArrayElement(texture, 1);
	if (!p) {
	    str = "black";
	} else {
	    str = PLGetString(p);
	}
	color = WMCreateNamedColor(scr, str, False);

	WMSetColorWellColor(panel->defcW, color);

	WMReleaseColor(color);

	for (i = 2; i < PLGetNumberOfElements(texture); i++) {
	    RColor *rgb;
	    XColor xcolor;

	    p = PLGetArrayElement(texture, i);
	    if (!p) {
		str = "black";
	    } else {
		str = PLGetString(p);
	    }

	    XParseColor(WMScreenDisplay(scr), WMScreenRContext(scr)->cmap,
			str, &xcolor);

	    rgb = wmalloc(sizeof(RColor));
	    rgb->red = xcolor.red >> 8;
	    rgb->green = xcolor.green >> 8;
	    rgb->blue = xcolor.blue >> 8;
	    sprintf(buffer, "%02x,%02x,%02x", rgb->red, rgb->green, rgb->blue);

	    item = WMAddListItem(panel->gcolL, buffer);
	    item->clientData = rgb;
	}

	sliderChangeCallback(panel->ghueS, panel);

	gradient = type[1];
    /*...............................................*/
    } else if (strcasecmp(type, "cpixmap")==0
	       || strcasecmp(type, "spixmap")==0
	       || strcasecmp(type, "mpixmap")==0
	       || strcasecmp(type, "tpixmap")==0) {

	WMSetPopUpButtonSelectedItem(panel->typeP, TYPE_PIXMAP);

	switch (toupper(type[0])) {
	 case 'C':
	    WMSetPopUpButtonSelectedItem(panel->arrP, PTYPE_CENTER);
	    break;
	 case 'S':
	    WMSetPopUpButtonSelectedItem(panel->arrP, PTYPE_SCALE);
	    break;
	 case 'M':
	    WMSetPopUpButtonSelectedItem(panel->arrP, PTYPE_MAXIMIZE);
	    break;
	 default:
	 case 'T':
	    WMSetPopUpButtonSelectedItem(panel->arrP, PTYPE_TILE);
	    break;
	}

	WMSetTextFieldText(panel->imageT,
			   PLGetString(PLGetArrayElement(texture, 1)));
	
	if (panel->imageFile)
	    free(panel->imageFile);
	panel->imageFile = wfindfileinarray(panel->pathList,
			    PLGetString(PLGetArrayElement(texture, 1)));

	color = WMCreateNamedColor(scr,
			   PLGetString(PLGetArrayElement(texture, 2)), False);
	WMSetColorWellColor(panel->defcW, color);
	WMReleaseColor(color);

	updateImage(panel, panel->imageFile);
    }

    changeTypeCallback(panel->typeP, panel);

    if (gradient > 0) {
	updateGradButtons(panel);

	switch (toupper(gradient)) {
	 case 'H':
	    WMPerformButtonClick(panel->dirhB);
	    break;
	 case 'V':
	    WMPerformButtonClick(panel->dirvB);
	    break;
	 default:
	 case 'D':
	    WMPerformButtonClick(panel->dirdB);
	    break;
	}
    }

    return;

 bad_texture:
    str = PLGetDescription(texture);
    wwarning("error creating texture %s", str);
    free(str);

}



char*
GetTexturePanelTextureName(TexturePanel *panel)
{
    return WMGetTextFieldText(panel->nameT);

}



proplist_t
GetTexturePanelTexture(TexturePanel *panel)
{
    proplist_t prop = NULL;
    WMColor *color;
    char *str, *str2;
    char buff[32];
    int i;


    switch (WMGetPopUpButtonSelectedItem(panel->typeP)) {

     case TYPE_SOLID:
	color = WMGetColorWellColor(panel->defcW);
	str = WMGetColorRGBDescription(color);
	prop = PLMakeArrayFromElements(PLMakeString("solid"),
				       PLMakeString(str), NULL);
	free(str);

	break;

     case TYPE_PIXMAP:
	color = WMGetColorWellColor(panel->defcW);
	str = WMGetColorRGBDescription(color);

	switch (WMGetPopUpButtonSelectedItem(panel->arrP)) {
	 case PTYPE_SCALE:
	    prop = PLMakeArrayFromElements(PLMakeString("spixmap"), 
					   PLMakeString(panel->imageFile),
					   PLMakeString(str), NULL);
	    break;
	 case PTYPE_MAXIMIZE:
	    prop = PLMakeArrayFromElements(PLMakeString("mpixmap"), 
					   PLMakeString(panel->imageFile),
					   PLMakeString(str), NULL);
	    break;
	 case PTYPE_CENTER:
	    prop = PLMakeArrayFromElements(PLMakeString("cpixmap"), 
					   PLMakeString(panel->imageFile),
					   PLMakeString(str), NULL);
	    break;
	 case PTYPE_TILE:
	    prop = PLMakeArrayFromElements(PLMakeString("tpixmap"),
					   PLMakeString(panel->imageFile),
					   PLMakeString(str), NULL);
	    break;
	}
	free(str);
	break;

     case TYPE_TGRADIENT:
	color = WMGetColorWellColor(panel->tcol1W);
	str = WMGetColorRGBDescription(color);

	color = WMGetColorWellColor(panel->tcol2W);
	str2 = WMGetColorRGBDescription(color);

	sprintf(buff, "%i", WMGetSliderValue(panel->topaS));

	if (WMGetButtonSelected(panel->dirdB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("tdgradient"),
					   PLMakeString(panel->imageFile),
					   PLMakeString(buff),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	} else 	if (WMGetButtonSelected(panel->dirvB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("tvgradient"),
					   PLMakeString(panel->imageFile),
					   PLMakeString(buff),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	} else {
	    prop = PLMakeArrayFromElements(PLMakeString("thgradient"),
					   PLMakeString(panel->imageFile),
					   PLMakeString(buff),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	}
	free(str);
	free(str2);
	break;


     case TYPE_SGRADIENT:
	color = WMGetColorWellColor(panel->tcol1W);
	str = WMGetColorRGBDescription(color);

	color = WMGetColorWellColor(panel->tcol2W);
	str2 = WMGetColorRGBDescription(color);
	
	if (WMGetButtonSelected(panel->dirdB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("dgradient"),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	} else 	if (WMGetButtonSelected(panel->dirvB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("vgradient"),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	} else {
	    prop = PLMakeArrayFromElements(PLMakeString("hgradient"),
					   PLMakeString(str),
					   PLMakeString(str2), NULL);
	}
	free(str);
	free(str2);
	break;

     case TYPE_GRADIENT:
	color = WMGetColorWellColor(panel->defcW);
	str = WMGetColorRGBDescription(color);

	if (WMGetButtonSelected(panel->dirdB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("mdgradient"),
					   PLMakeString(str), NULL);
	} else 	if (WMGetButtonSelected(panel->dirvB)) {
	    prop = PLMakeArrayFromElements(PLMakeString("mvgradient"),
					   PLMakeString(str), NULL);
	} else {
	    prop = PLMakeArrayFromElements(PLMakeString("mhgradient"),
					   PLMakeString(str), NULL);
	}
	free(str);

	for (i = 0; i < WMGetListNumberOfRows(panel->gcolL); i++) {
	    RColor *rgb;
	    WMListItem *item;
	    
	    item = WMGetListItem(panel->gcolL, i);
	    
	    rgb = (RColor*)item->clientData;

	    sprintf(buff, "#%02x%02x%02x", rgb->red, rgb->green, rgb->blue);

	    PLAppendArrayElement(prop, PLMakeString(buff));
	}
	break;
    }


    return prop;
}



void
SetTexturePanelPixmapPath(TexturePanel *panel, proplist_t array)
{
    panel->pathList = array;
}



TexturePanel*
CreateTexturePanel(WMWindow *keyWindow)
/*CreateTexturePanel(WMScreen *scr)*/
{
    TexturePanel *panel;
    WMScreen *scr = WMWidgetScreen(keyWindow);

    panel = wmalloc(sizeof(TexturePanel));
    memset(panel, 0, sizeof(TexturePanel));

    panel->listFont = WMSystemFontOfSize(scr, 12);


    panel->win = WMCreatePanelWithStyleForWindow(keyWindow, "texturePanel",
						 WMTitledWindowMask
						 |WMClosableWindowMask);
 /*
    panel->win = WMCreateWindowWithStyle(scr, "texturePanel",
					 WMTitledWindowMask
					 |WMClosableWindowMask);
  */

    WMResizeWidget(panel->win, 325, 423);
    WMSetWindowTitle(panel->win, _("Texture Panel"));
    WMSetWindowCloseAction(panel->win, buttonCallback, panel);


    /* texture name */
    panel->nameF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->nameF, 185, 50);
    WMMoveWidget(panel->nameF, 15, 10);
    WMSetFrameTitle(panel->nameF, _("Texture Name"));

    panel->nameT = WMCreateTextField(panel->nameF);
    WMResizeWidget(panel->nameT, 160, 20);
    WMMoveWidget(panel->nameT, 12, 18);

    WMMapSubwidgets(panel->nameF);

    /* texture types */
    panel->typeP = WMCreatePopUpButton(panel->win);
    WMResizeWidget(panel->typeP, 185, 20);
    WMMoveWidget(panel->typeP, 15, 65);
    WMAddPopUpButtonItem(panel->typeP, _("Solid Color"));
    WMAddPopUpButtonItem(panel->typeP, _("Gradient Texture"));
    WMAddPopUpButtonItem(panel->typeP, _("Simple Gradient Texture"));
    WMAddPopUpButtonItem(panel->typeP, _("Textured Gradient"));
    WMAddPopUpButtonItem(panel->typeP, _("Image Texture"));
    WMSetPopUpButtonSelectedItem(panel->typeP, 0);
    WMSetPopUpButtonAction(panel->typeP, changeTypeCallback, panel);

    /* color */
    panel->defcF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->defcF, 100, 75);
    WMMoveWidget(panel->defcF, 210, 10);
    WMSetFrameTitle(panel->defcF, _("Default Color"));

    panel->defcW = WMCreateColorWell(panel->defcF);
    WMResizeWidget(panel->defcW, 60, 45);
    WMMoveWidget(panel->defcW, 20, 20);

    WMMapSubwidgets(panel->defcF);

    /****** Gradient ******/
    panel->gcolF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->gcolF, 295, 205);
    WMMoveWidget(panel->gcolF, 15, 95);
    WMSetFrameTitle(panel->gcolF, _("Gradient Colors"));

    panel->gcolL = WMCreateList(panel->gcolF);
    WMResizeWidget(panel->gcolL, 130, 140);
    WMMoveWidget(panel->gcolL, 10, 25);
    WMHangData(panel->gcolL, panel);
    WMSetListUserDrawProc(panel->gcolL, paintGradListItem);
    WMSetListAction(panel->gcolL, gradClickCallback, panel);

    panel->gcolaB = WMCreateCommandButton(panel->gcolF);
    WMResizeWidget(panel->gcolaB, 64, 24);
    WMMoveWidget(panel->gcolaB, 10, 170);
    WMSetButtonText(panel->gcolaB, _("Add"));
    WMSetButtonAction(panel->gcolaB, gradAddCallback, panel);
    
    panel->gcoldB = WMCreateCommandButton(panel->gcolF);
    WMResizeWidget(panel->gcoldB, 64, 24);
    WMMoveWidget(panel->gcoldB, 75, 170);
    WMSetButtonText(panel->gcoldB, _("Delete"));
    WMSetButtonAction(panel->gcoldB, gradDeleteCallback, panel);

#if 0
    panel->gbriS = WMCreateSlider(panel->gcolF);
    WMResizeWidget(panel->gbriS, 130, 16);
    WMMoveWidget(panel->gbriS, 150, 25);
    WMSetSliderKnobThickness(panel->gbriS, 8);
    WMSetSliderMaxValue(panel->gbriS, 100);
    WMSetSliderAction(panel->gbriS, sliderChangeCallback, panel);
    {
	WMPixmap *pixmap;
	WMColor *color;

	pixmap = WMCreatePixmap(scr, 130, 16, WMScreenDepth(scr), False);
	color = WMDarkGrayColor(scr);
	XFillRectangle(WMScreenDisplay(scr), WMGetPixmapXID(pixmap),
		       WMColorGC(color), 0, 0, 130, 16);
	WMReleaseColor(color);
	color = WMWhiteColor(color);
	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(color),
		     panel->listFont, 2, 
		     (16 - WMFontHeight(panel->listFont))/2 - 1,
		     "Brightness", 10);
	WMSetSliderImage(panel->gbriS, pixmap);
	WMReleasePixmap(pixmap);
    }

    panel->gconS = WMCreateSlider(panel->gcolF);
    WMResizeWidget(panel->gconS, 130, 16);
    WMMoveWidget(panel->gconS, 150, 50);
    WMSetSliderKnobThickness(panel->gconS, 8);
    WMSetSliderMaxValue(panel->gconS, 100);
    WMSetSliderAction(panel->gconS, sliderChangeCallback, panel);
    {
	WMPixmap *pixmap;
	WMColor *color;

	pixmap = WMCreatePixmap(scr, 130, 16, WMScreenDepth(scr), False);
	color = WMDarkGrayColor(scr);
	XFillRectangle(WMScreenDisplay(scr), WMGetPixmapXID(pixmap),
		       WMColorGC(color), 0, 0, 130, 16);
	WMReleaseColor(color);
	color = WMWhiteColor(scr);
	WMDrawString(scr, WMGetPixmapXID(pixmap), WMColorGC(color), 
		     panel->listFont, 2,
		     (16 - WMFontHeight(panel->listFont))/2 - 1,
		     "Contrast", 8);
	WMSetSliderImage(panel->gconS, pixmap);
	WMReleasePixmap(pixmap);
    }
#endif
    panel->ghueS = WMCreateSlider(panel->gcolF);
    WMResizeWidget(panel->ghueS, 130, 16);
    WMMoveWidget(panel->ghueS, 150, 100);
    WMSetSliderKnobThickness(panel->ghueS, 8);
    WMSetSliderMaxValue(panel->ghueS, 359);
    WMSetSliderAction(panel->ghueS, sliderChangeCallback, panel);

    panel->gsatS = WMCreateSlider(panel->gcolF);
    WMResizeWidget(panel->gsatS, 130, 16);
    WMMoveWidget(panel->gsatS, 150, 125);
    WMSetSliderKnobThickness(panel->gsatS, 8);
    WMSetSliderMaxValue(panel->gsatS, 255);
    WMSetSliderAction(panel->gsatS, sliderChangeCallback, panel);

    panel->gvalS = WMCreateSlider(panel->gcolF);
    WMResizeWidget(panel->gvalS, 130, 16);
    WMMoveWidget(panel->gvalS, 150, 150);
    WMSetSliderKnobThickness(panel->gvalS, 8);
    WMSetSliderMaxValue(panel->gvalS, 255);
    WMSetSliderAction(panel->gvalS, sliderChangeCallback, panel);


    WMMapSubwidgets(panel->gcolF);

    /** Direction **/
    panel->dirF = WMCreateFrame(panel->win);
    WMSetFrameTitle(panel->dirF, _("Direction"));
    WMResizeWidget(panel->dirF, 295, 75);
    WMMoveWidget(panel->dirF, 15, 305);

    panel->dirvB = WMCreateButton(panel->dirF, WBTOnOff);
    WMSetButtonImagePosition(panel->dirvB, WIPImageOnly);
    WMResizeWidget(panel->dirvB, 90, 40);
    WMMoveWidget(panel->dirvB, 10, 20);

    panel->dirhB = WMCreateButton(panel->dirF, WBTOnOff);
    WMSetButtonImagePosition(panel->dirhB, WIPImageOnly);
    WMResizeWidget(panel->dirhB, 90, 40);
    WMMoveWidget(panel->dirhB, 102, 20);

    panel->dirdB = WMCreateButton(panel->dirF, WBTOnOff);
    WMSetButtonImagePosition(panel->dirdB, WIPImageOnly);
    WMResizeWidget(panel->dirdB, 90, 40);
    WMMoveWidget(panel->dirdB, 194, 20);

    WMGroupButtons(panel->dirvB, panel->dirhB);
    WMGroupButtons(panel->dirvB, panel->dirdB);

    WMMapSubwidgets(panel->dirF);

    /****************** Textured Gradient ******************/
    panel->tcolF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->tcolF, 100, 135);
    WMMoveWidget(panel->tcolF, 210, 10);
    WMSetFrameTitle(panel->tcolF, _("Gradient"));

    panel->tcol1W = WMCreateColorWell(panel->tcolF);
    WMResizeWidget(panel->tcol1W, 60, 45);
    WMMoveWidget(panel->tcol1W, 20, 25);
    WMAddNotificationObserver(colorWellObserver, panel,
			      WMColorWellDidChangeNotification, panel->tcol1W);

    panel->tcol2W = WMCreateColorWell(panel->tcolF);
    WMResizeWidget(panel->tcol2W, 60, 45);
    WMMoveWidget(panel->tcol2W, 20, 75);
    WMAddNotificationObserver(colorWellObserver, panel,
			      WMColorWellDidChangeNotification, panel->tcol2W);

    /** Opacity */
    panel->topaF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->topaF, 185, 50);
    WMMoveWidget(panel->topaF, 15, 95);
    WMSetFrameTitle(panel->topaF, _("Gradient Opacity"));

    panel->topaS = WMCreateSlider(panel->topaF);
    WMResizeWidget(panel->topaS, 155, 18);
    WMMoveWidget(panel->topaS, 15, 20);
    WMSetSliderMaxValue(panel->topaS, 255);
    WMSetSliderValue(panel->topaS, 200);
    WMSetSliderContinuous(panel->topaS, False);
    WMSetSliderAction(panel->topaS, opaqChangeCallback, panel);
    
    WMMapSubwidgets(panel->topaF);

    {
	WMPixmap *pixmap;
	Pixmap p;
	WMColor *color;

	pixmap = WMCreatePixmap(scr, 155, 18, WMScreenDepth(scr), False);
	p = WMGetPixmapXID(pixmap);

	color = WMDarkGrayColor(scr);
	XFillRectangle(WMScreenDisplay(scr), p, WMColorGC(color),
		       0, 0, 155, 18);
	WMReleaseColor(color);

	color = WMWhiteColor(scr);
	WMDrawString(scr, p, WMColorGC(color), panel->listFont,
		     2, 1, "0%", 2);
	WMDrawString(scr, p, WMColorGC(color), panel->listFont,
		     153 - WMWidthOfString(panel->listFont, "100%", 4), 1,
		     "100%", 4);
	WMReleaseColor(color);

	WMSetSliderImage(panel->topaS, pixmap);
	WMReleasePixmap(pixmap);
    }

    WMMapSubwidgets(panel->tcolF);

    /****************** Image ******************/
    panel->imageF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->imageF, 295, 150);
    WMMoveWidget(panel->imageF, 15, 150);
    WMSetFrameTitle(panel->imageF, _("Image"));

    panel->imageL = WMCreateLabel(panel->imageF);
    WMSetLabelImagePosition(panel->imageL, WIPImageOnly);

    panel->imageT = WMCreateTextField(panel->imageF);
    WMResizeWidget(panel->imageT, 90, 20);
    WMMoveWidget(panel->imageT, 190, 25);

    panel->imageV = WMCreateScrollView(panel->imageF);
    WMResizeWidget(panel->imageV, 165, 115);
    WMMoveWidget(panel->imageV, 15, 20);
    WMSetScrollViewRelief(panel->imageV, WRSunken);
    WMSetScrollViewHasHorizontalScroller(panel->imageV, True);
    WMSetScrollViewHasVerticalScroller(panel->imageV, True);
    WMSetScrollViewContentView(panel->imageV, WMWidgetView(panel->imageL));

    panel->browB = WMCreateCommandButton(panel->imageF);
    WMResizeWidget(panel->browB, 90, 24);
    WMMoveWidget(panel->browB, 190, 50);
    WMSetButtonText(panel->browB, _("Browse..."));
    WMSetButtonAction(panel->browB, browseImageCallback, panel);

/*    panel->dispB = WMCreateCommandButton(panel->imageF);
    WMResizeWidget(panel->dispB, 90, 24);
    WMMoveWidget(panel->dispB, 190, 80);
    WMSetButtonText(panel->dispB, _("Show"));
 */

    panel->arrP = WMCreatePopUpButton(panel->imageF);
    WMResizeWidget(panel->arrP, 90, 20);
    WMMoveWidget(panel->arrP, 190, 120);
    WMAddPopUpButtonItem(panel->arrP, _("Tile"));
    WMAddPopUpButtonItem(panel->arrP, _("Scale"));
    WMAddPopUpButtonItem(panel->arrP, _("Center"));
    WMAddPopUpButtonItem(panel->arrP, _("Maximize"));
    WMSetPopUpButtonSelectedItem(panel->arrP, 0);

    WMMapSubwidgets(panel->imageF);

    /****/

    panel->okB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->okB, 84, 24);
    WMMoveWidget(panel->okB, 225, 390);
    WMSetButtonText(panel->okB, _("OK"));
    WMSetButtonAction(panel->okB, buttonCallback, panel);
    
    panel->cancelB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancelB, 84, 24);
    WMMoveWidget(panel->cancelB, 130, 390);
    WMSetButtonText(panel->cancelB, _("Cancel"));
    WMSetButtonAction(panel->cancelB, buttonCallback, panel);

    WMMapWidget(panel->nameF);
    WMMapWidget(panel->typeP);
    WMMapWidget(panel->okB);
    WMMapWidget(panel->cancelB);

    WMUnmapWidget(panel->arrP);

    WMRealizeWidget(panel->win);

    panel->currentType = -1;

    panel->sectionParts[TYPE_SOLID][0] = panel->defcF;

    panel->sectionParts[TYPE_GRADIENT][0] = panel->defcF;
    panel->sectionParts[TYPE_GRADIENT][1] = panel->gcolF;
    panel->sectionParts[TYPE_GRADIENT][2] = panel->dirF;

    panel->sectionParts[TYPE_SGRADIENT][0] = panel->tcolF;
    panel->sectionParts[TYPE_SGRADIENT][1] = panel->dirF;

    panel->sectionParts[TYPE_TGRADIENT][0] = panel->tcolF;
    panel->sectionParts[TYPE_TGRADIENT][1] = panel->dirF;
    panel->sectionParts[TYPE_TGRADIENT][2] = panel->imageF;
    panel->sectionParts[TYPE_TGRADIENT][3] = panel->topaF;
    panel->sectionParts[TYPE_TGRADIENT][4] = panel->arrP;

    panel->sectionParts[TYPE_PIXMAP][0] = panel->defcF;
    panel->sectionParts[TYPE_PIXMAP][1] = panel->imageF;
    panel->sectionParts[TYPE_PIXMAP][2] = panel->arrP;


    /* setup for first time */

    changeTypeCallback(panel->typeP, panel);

    sliderChangeCallback(panel->ghueS, panel);
    sliderChangeCallback(panel->gsatS, panel);

    return panel;
}



/*
 *--------------------------------------------------------------------------
 * Test stuff
 *--------------------------------------------------------------------------
 */

#if 0

char *ProgName = "test";

void
testOKButton(WMWidget *self, void *data)
{
    char *test;
    Display *dpy;
    Window win;
    Pixmap pix;
    RImage *image;
    
    TexturePanel *panel = (TexturePanel*)data;
    /* test = GetTexturePanelTextureString(panel); */
    
    wwarning(test);
    
    dpy = WMScreenDisplay(WMWidgetScreen(panel->okB));
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, 250, 250,
			      0, 0, 0);
    XMapRaised(dpy, win);
    XFlush(dpy);
    
    /* image = RenderTexturePanelTexture(panel, 250, 250); */
    
    RConvertImage(WMScreenRContext(WMWidgetScreen(panel->okB)), image, &pix);
    
    XCopyArea(dpy, pix, win, (WMScreenRContext(WMWidgetScreen(panel->okB)))->copy_gc, 0, 0, image->width, image->height, 
	      0, 0);
    
    free (test);
    
}

void testCancelButton(WMWidget *self, void *data){
    wwarning("Exiting test....");
    exit(0);
}

void wAbort()
{
    exit(1);
}

int main(int argc, char **argv)
{
    TexturePanel *panel;
    
    Display *dpy = XOpenDisplay("");
    WMScreen *scr;
    
    /* char *test; */
    
    WMInitializeApplication("Test", &argc, argv);
    
    if (!dpy) {
        wfatal("could not open display");
        exit(1);
    }
    
    scr = WMCreateSimpleApplicationScreen(dpy);

    panel = CreateTexturePanel(scr);

    SetTexturePanelOkAction(panel,(WMAction*)testOKButton,panel);
    SetTexturePanelCancelAction(panel,(WMAction*)testCancelButton,panel);

    SetTexturePanelTexture(panel, "pinky",
			   PLGetProplistWithDescription("(mdgradient, pink, red, blue, yellow)"));

    ShowTexturePanel(panel);

    WMScreenMainLoop(scr);
    return 0;
}
#endif
