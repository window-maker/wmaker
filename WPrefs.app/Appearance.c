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

#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>




#include "TexturePanel.h"

typedef struct _Panel {
    WMBox *box;
    char *sectionName;

    char *description;

    CallbackRec callbacks;

    WMWidget *parent;

    WMLabel *prevL;


    WMTabView *tabv;

    /* texture list */
    WMFrame *texF;
    WMList *texLs;

    WMPopUpButton *secP;

    WMButton *newB;
    WMButton *editB;
    WMButton *ripB;
    WMButton *delB;

    /* text color */
    WMFrame *colF;

    WMPopUpButton *colP;
    WMColor *colors[14];

    WMColorWell *colW;

    WMColorWell *sampW[24];

    /* options */
    WMFrame *optF;

    WMFrame *mstyF;
    WMButton *mstyB[3];

    WMFrame *taliF;
    WMButton *taliB[3];

    /* root bg */
    WMFrame *bgF;
    
    WMLabel *bgprevL;
    WMButton *selbgB;

    WMPopUpButton *modeB[3];
    
    /* */

    int textureIndex[8];

    WMFont *smallFont;
    WMFont *normalFont;
    WMFont *boldFont;

    TexturePanel *texturePanel;

    WMPixmap *onLed;
    WMPixmap *offLed;
    WMPixmap *hand;

    int oldsection;
    int oldcsection;

    char oldTabItem;

    char menuStyle;

    char titleAlignment;

    Pixmap preview;

    char *fprefix;
} _Panel;


typedef struct {
    char *title;
    char *texture;
    proplist_t prop;
    Pixmap preview;

    char *path;

    char selectedFor;
    unsigned current:1;
    unsigned ispixmap:1;
} TextureListItem;

static void updateColorPreviewBox(_Panel *panel, int elements);

static void showData(_Panel *panel);

static void changePage(WMWidget *w, void *data);

static void changeColorPage(WMWidget *w, void *data);

static void OpenExtractPanelFor(_Panel *panel, char *path);



static void changedTabItem(struct WMTabViewDelegate *self, WMTabView *tabView,
			   WMTabViewItem *item);



static WMTabViewDelegate tabviewDelegate = {
    NULL,
	NULL, /* didChangeNumberOfItems */
	changedTabItem, /* didSelectItem */
	NULL, /* shouldSelectItem */
	NULL /* willSelectItem */
};


#define ICON_FILE	"appearance"

#define TNEW_FILE 	"tnew"
#define TDEL_FILE	"tdel"
#define TEDIT_FILE	"tedit"
#define TEXTR_FILE 	"textr"

#define MSTYLE1_FILE	"msty1"
#define MSTYLE2_FILE	"msty2"
#define MSTYLE3_FILE	"msty3"


/* XPM */
static char * blueled_xpm[] = {
"8 8 17 1",
" 	c None",
".	c #020204",
"+	c #16B6FC",
"@	c #176AFC",
"#	c #163AFC",
"$	c #72D2FC",
"%	c #224CF4",
"&	c #76D6FC",
"*	c #16AAFC",
"=	c #CEE9FC",
"-	c #229EFC",
";	c #164CFC",
">	c #FAFEFC",
",	c #2482FC",
"'	c #1652FC",
")	c #1E76FC",
"!	c #1A60FC",
"  ....  ",
" .=>-@. ",
".=>$@@'.",
".=$@!;;.",
".!@*+!#.",
".#'*+*,.",
" .@)@,. ",
"  ....  "};


/* XPM */
static char *blueled2_xpm[] = {
/* width height num_colors chars_per_pixel */
"     8     8       17            1",
/* colors */
". c None",
"# c #090909",
"a c #4b63a4",
"b c #011578",
"c c #264194",
"d c #04338c",
"e c #989dac",
"f c #011a7c",
"g c #465c9c",
"h c #03278a",
"i c #6175ac",
"j c #011e74",
"k c #043a90",
"l c #042f94",
"m c #0933a4",
"n c #022184",
"o c #012998",
/* pixels */
"..####..",
".#aimn#.",
"#aechnf#",
"#gchnjf#",
"#jndknb#",
"#bjdddl#",
".#nono#.",
"..####.."
};


/* XPM */
static char * hand_xpm[] = {
"22 21 19 1",
"       c None",
".      c #030305",
"+      c #7F7F7E",
"@      c #B5B5B6",
"#      c #C5C5C6",
"$      c #969697",
"%      c #FDFDFB",
"&      c #F2F2F4",
"*      c #E5E5E4",
"=      c #ECECEC",
"-      c #DCDCDC",
";      c #D2D2D0",
">      c #101010",
",      c #767674",
"'      c #676767",
")      c #535355",
"!      c #323234",
"~      c #3E3C56",
"{      c #333147",
"                      ",
"       .....          ",
"     ..+@##$.         ",
"    .%%%&@..........  ",
"   .%*%%&#%%%%%%%%%$. ",
"  .*#%%%%%%%%%&&&&==. ",
" .-%%%%%%%%%=*-;;;#$. ",
" .-%%%%%%%%&..>.....  ",
" >-%%%%%%%%%*#+.      ",
" >-%%%%%%%%%*@,.      ",
" >#%%%%%%%%%*@'.      ",
" >$&&%%%%%%=...       ",
" .+@@;=&%%&;$,>       ",
"  .',$@####$+).       ",
"   .!',+$++,'.        ",
"     ..>>>>>.         ",
"                      ",
"     ~~{{{~~          ",
"   {{{{{{{{{{{        ",
"     ~~{{{~~          ",
"                      "};



static char *sampleColors[] = {
    "black",
	"#292929",
	"#525252",
	"#848484",
	"#adadad",
	"#d6d6d6",
	"white",
	"#d6d68c",
	"#d6a57b",
	"#8cd68c",
	"#8cd6ce",
	"#d68c8c",
	"#8c9cd6",
	"#bd86d6",
	"#d68cbd",
	"#d64a4a",
	"#4a5ad6",
	"#4ad6ce",
	"#4ad65a",
	"#ced64a",
	"#d6844a",
	"#8ad631",
	"#ce29c6",
	"#ce2973",
	"black"
};


static char *textureOptions[] = {
    "FTitleBack", "(solid, black)", "[Focused]", 
	"UTitleBack", "(solid, gray)", "[Unfocused]",
	"PTitleBack", "(solid, \"#616161\")", "[Owner of Focused]",
	"ResizebarBack", "(solid, gray)", "[Resizebar]",
	"MenuTitleBack", "(solid, black)", "[Menu Title]",
	"MenuTextBack", "(solid, gray)", "[Menu Item]",
	"IconBack", "(solid, gray)", "[Icon]"
};


#define RESIZEBAR_BEVEL	-1
#define MENU_BEVEL 	-2

#define TEXPREV_WIDTH	40
#define TEXPREV_HEIGHT	24


#define MSTYLE_NORMAL	0
#define MSTYLE_SINGLE	1
#define MSTYLE_FLAT	2


#define FTITLE_COL	(1<<0)
#define UTITLE_COL	(1<<1)
#define OTITLE_COL	(1<<2)
#define MTITLE_COL	(1<<3)
#define MITEM_COL	(1<<4)
#define MDISAB_COL	(1<<5)
#define MHIGH_COL	(1<<6)
#define MHIGHT_COL	(1<<7)
#define ICONT_COL	(1<<8)
#define ICONB_COL	(1<<9)
#define CLIP_COL	(1<<10)
#define CCLIP_COL	(1<<11)


static char *colorOptions[] = {
    "FTitleColor", "white",
	"UTitleColor", "black",
	"PTitleColor", "white",
	"MenuTitleColor", "white",
	"MenuTextColor", "black",
	"MenuDisabledColor", "#616161",
	"HighlightColor", "white",
	"HighlightTextColor", "black",
	"IconTitleColor", "white",
	"IconTitleBack", "black",
	"ClipTitleColor", "black",
	"CClipTitleColor", "#454045"
};




static WMRect previewPositions[] = {
#define PFOCUSED	0
    {{30,10},{190, 20}},
#define PUNFOCUSED	1
    {{30,40},{190,20}},
#define POWNER		2
    {{30,70},{190,20}},
#define PRESIZEBAR	3
    {{30,100},{190,9}},
#define PMTITLE		4
    {{30,120},{90,20}},
#define PMITEM		5
    {{30,140},{90,20*4}},
#define PICON		6
    {{155,130},{64,64}}
};
#define EVERYTHING	0xff


static WMRect previewColorPositions[] = {
    {{30,10},{190, 20}},
    {{30,40},{190,20}},
    {{30,70},{190,20}},
    {{30,120},{90,20}},
    {{30,140},{90,20}},
    {{30,160},{90,20}},
    {{30,180},{90,20}},
    {{30,200},{90,20}},
    {{155,130},{64,64}},
    {{155,130},{64,64}},
    {{155,130},{64,64}},
    {{155,130},{64,64}}
};



static void
str2rcolor(RContext *rc, char *name, RColor *color)
{
    XColor xcolor;
    
    XParseColor(rc->dpy, rc->cmap, name, &xcolor);

    color->alpha = 255;
    color->red = xcolor.red >> 8;
    color->green = xcolor.green >> 8;
    color->blue = xcolor.blue >> 8;
}


static void
dumpRImage(char *path, RImage *image)
{
    FILE *f;
    int channels = (image->format == RRGBAFormat ? 4 : 3);

    f = fopen(path, "w");
    if (!f) {
	wsyserror(path);
	return;
    }
    fprintf(f, "%02x%02x%1x", image->width, image->height, channels);

    fwrite(image->data, 1, image->width * image->height * channels, f);

    if (fclose(f) < 0) {
	wsyserror(path);
    }
}



static int
isPixmap(proplist_t prop)
{
    proplist_t p;
    char *s;

    p = PLGetArrayElement(prop, 0);
    s = PLGetString(p);
    if (strcasecmp(&s[1], "pixmap")==0)
	return 1;
    else
	return 0;
}


/**********************************************************************/

static void
drawResizebarBevel(RImage *img)
{
    RColor light;
    RColor dark;
    RColor black;
    int width = img->width;
    int height = img->height;
    int cwidth = 28;

    black.alpha = 255;
    black.red = black.green = black.blue = 0;
    
    light.alpha = 0;
    light.red = light.green = light.blue = 80;

    dark.alpha = 0;
    dark.red = dark.green = dark.blue = 40;

    ROperateLine(img, RSubtractOperation, 0, 0, width-1, 0, &dark);
    ROperateLine(img, RAddOperation, 0, 1, width-1, 1, &light);

    ROperateLine(img, RSubtractOperation, cwidth, 2, cwidth, height-1, &dark);
    ROperateLine(img, RAddOperation, cwidth+1, 2, cwidth+1, height-1, &light);

    ROperateLine(img, RSubtractOperation, width-cwidth-2, 2, width-cwidth-2,
		 height-1, &dark);
    ROperateLine(img, RAddOperation, width-cwidth-1, 2, width-cwidth-1, 
		 height-1, &light);
    
    RDrawLine(img, 0, height-1, width-1, height-1, &black);
    RDrawLine(img, 0, 0, 0, height-1, &black);
    RDrawLine(img, width-1, 0, width-1, height-1, &black);
}


static void
drawMenuBevel(RImage *img)
{
    RColor light, dark, mid;
    int i;
    int iheight = img->height / 4;
    
    light.alpha = 0;
    light.red = light.green = light.blue = 80;
    
    dark.alpha = 255;
    dark.red = dark.green = dark.blue = 0;
    
    mid.alpha = 0;
    mid.red = mid.green = mid.blue = 40;
    
    for (i = 1; i < 4; i++) {
	ROperateLine(img, RSubtractOperation, 0, i*iheight-2,
		     img->width-1, i*iheight-2, &mid);
	
	RDrawLine(img, 0, i*iheight-1, img->width-1, i*iheight-1, &dark);
	
	ROperateLine(img, RAddOperation, 1, i*iheight,
		     img->width-2, i*iheight, &light);
    }
}


static Pixmap
renderTexture(WMScreen *scr, proplist_t texture, int width, int height,
	      char *path, int border)
{
    char *type;
    RImage *image = NULL;
    Pixmap pixmap;
    RContext *rc = WMScreenRContext(scr);
    char *str;
    RColor rcolor;


    type = PLGetString(PLGetArrayElement(texture, 0));

    if (strcasecmp(type, "solid")==0) {

	str = PLGetString(PLGetArrayElement(texture, 1));

	str2rcolor(rc, str, &rcolor);

	image = RCreateImage(width, height, False);
	RClearImage(image, &rcolor);
    } else if (strcasecmp(type, "igradient")==0) {
	int t1, t2;
	RColor c1[2], c2[2];

	str = PLGetString(PLGetArrayElement(texture, 1));
	str2rcolor(rc, str, &c1[0]);
	str = PLGetString(PLGetArrayElement(texture, 2));
	str2rcolor(rc, str, &c1[1]);
	str = PLGetString(PLGetArrayElement(texture, 3));
	t1 = atoi(str);

	str = PLGetString(PLGetArrayElement(texture, 4));
	str2rcolor(rc, str, &c2[0]);
	str = PLGetString(PLGetArrayElement(texture, 5));
	str2rcolor(rc, str, &c2[1]);
	str = PLGetString(PLGetArrayElement(texture, 6));
	t2 = atoi(str);

	image = RRenderInterwovenGradient(width, height, c1, t1, c2, t2);
    } else if (strcasecmp(&type[1], "gradient")==0) {
	int style;
	RColor rcolor2;

	switch (toupper(type[0])) {
	 case 'V':
	    style = RVerticalGradient;
	    break;
	 case 'H':
	    style = RHorizontalGradient;
	    break;
	 default:
	 case 'D':
	    style = RDiagonalGradient;
	    break;
	}

	str = PLGetString(PLGetArrayElement(texture, 1));
	str2rcolor(rc, str, &rcolor);
	str = PLGetString(PLGetArrayElement(texture, 2));
	str2rcolor(rc, str, &rcolor2);

	image = RRenderGradient(width, height, &rcolor, &rcolor2, style);
    } else if (strcasecmp(&type[2], "gradient")==0 && toupper(type[0])=='T') {
	int style;
	RColor rcolor2;
	int i;
	RImage *grad, *timage;
	char *path;

	switch (toupper(type[1])) {
	 case 'V':
	    style = RVerticalGradient;
	    break;
	 case 'H':
	    style = RHorizontalGradient;
	    break;
	 default:
	 case 'D':
	    style = RDiagonalGradient;
	    break;
	}

	str = PLGetString(PLGetArrayElement(texture, 3));
	str2rcolor(rc, str, &rcolor);
	str = PLGetString(PLGetArrayElement(texture, 4));
	str2rcolor(rc, str, &rcolor2);

	str = PLGetString(PLGetArrayElement(texture, 1));

	if ((path=wfindfileinarray(GetObjectForKey("PixmapPath"), str))!=NULL)
            timage = RLoadImage(rc, path, 0);

	if (!path || !timage) {
	    wwarning("could not load file '%s': %s", path,
		     RMessageForError(RErrorCode));
	} else {	
	    grad = RRenderGradient(width, height, &rcolor, &rcolor2, style);

	    image = RMakeTiledImage(timage, width, height);
	    RReleaseImage(timage);

	    i = atoi(PLGetString(PLGetArrayElement(texture, 2)));
	
	    RCombineImagesWithOpaqueness(image, grad, i);
	    RReleaseImage(grad);
	}
    } else if (strcasecmp(&type[2], "gradient")==0 && toupper(type[0])=='M') {
	int style;
	RColor **colors;
	int i, j;

	switch (toupper(type[1])) {
	 case 'V':
	    style = RVerticalGradient;
	    break;
	 case 'H':
	    style = RHorizontalGradient;
	    break;
	 default:
	 case 'D':
	    style = RDiagonalGradient;
	    break;
	}

	j = PLGetNumberOfElements(texture);
	
	if (j > 0) {
	    colors = wmalloc(j * sizeof(RColor*));

	    for (i = 2; i < j; i++) {
		str = PLGetString(PLGetArrayElement(texture, i));
		colors[i-2] = wmalloc(sizeof(RColor));
		str2rcolor(rc, str, colors[i-2]);
	    }
	    colors[i-2] = NULL;

	    image = RRenderMultiGradient(width, height, colors, style);
	    
	    for (i = 0; colors[i]!=NULL; i++)
		wfree(colors[i]);
	    wfree(colors);
	}
    } else if (strcasecmp(&type[1], "pixmap")==0) {
	RImage *timage = NULL;
	char *path;
	RColor color;

	str = PLGetString(PLGetArrayElement(texture, 1));

	if ((path=wfindfileinarray(GetObjectForKey("PixmapPath"), str))!=NULL)
            timage = RLoadImage(rc, path, 0);

	if (!path || !timage) {
	    wwarning("could not load file '%s': %s", path ? path : str,
		     RMessageForError(RErrorCode));
	} else {
	    str = PLGetString(PLGetArrayElement(texture, 2));
	    str2rcolor(rc, str, &color);

	    switch (toupper(type[0])) {
	     case 'T':
		image = RMakeTiledImage(timage, width, height);
		RReleaseImage(timage);
		timage = image;
		break;
	     case 'C':
		image = RMakeCenteredImage(timage, width, height, &color);
		RReleaseImage(timage);
		timage = image;
		break;
	     case 'S':
	     case 'M':
		image = RScaleImage(timage, width, height);
		RReleaseImage(timage);
		timage = image;
		break;
	    }

	}
	wfree(path);
    }

    if (!image)
	return None;

    if (path) {
	dumpRImage(path, image);
    }
    
    if (border < 0) {
	if (border == RESIZEBAR_BEVEL) {
	    drawResizebarBevel(image);
	} else if (border == MENU_BEVEL) {
	    drawMenuBevel(image);
	    RBevelImage(image, RBEV_RAISED2);
	}
    } else if (border) {
	RBevelImage(image, border);
    }

    RConvertImage(rc, image, &pixmap);
    RReleaseImage(image);

    return pixmap;
}


static Pixmap
renderMenu(_Panel *panel, proplist_t texture, int width, int iheight)
{
    WMScreen *scr = WMWidgetScreen(panel->parent);
    Display *dpy = WMScreenDisplay(scr);
    Pixmap pix, tmp;
    GC gc = XCreateGC(dpy, WMWidgetXID(panel->parent), 0, NULL);
    int i;

    switch (panel->menuStyle) {
     case MSTYLE_NORMAL:
	tmp = renderTexture(scr, texture, width, iheight, NULL, RBEV_RAISED2);

	pix = XCreatePixmap(dpy, tmp, width, iheight*4, WMScreenDepth(scr));
	for (i = 0; i < 4; i++) {
	    XCopyArea(dpy, tmp, pix, gc, 0, 0, width, iheight, 0, iheight*i);
	}
	XFreePixmap(dpy, tmp);
	break;
     case MSTYLE_SINGLE:
	pix = renderTexture(scr, texture, width, iheight*4, NULL, MENU_BEVEL);
	break;
     case MSTYLE_FLAT:
	pix = renderTexture(scr, texture, width, iheight*4, NULL, RBEV_RAISED2);
	break;
    }
    XFreeGC(dpy, gc);

    return pix;
}


static void
renderPreview(_Panel *panel, GC gc, int part, int relief)
{
    WMListItem *item;
    TextureListItem *titem;
    Pixmap pix;
    WMScreen *scr = WMWidgetScreen(panel->box);

    item = WMGetListItem(panel->texLs, panel->textureIndex[part]);
    titem = (TextureListItem*)item->clientData;

    pix = renderTexture(scr, titem->prop, 
			previewPositions[part].size.width,
			previewPositions[part].size.height,
			NULL, relief);

    XCopyArea(WMScreenDisplay(scr), pix, panel->preview, gc, 0, 0, 
	      previewPositions[part].size.width, 
	      previewPositions[part].size.height,
	      previewPositions[part].pos.x,
	      previewPositions[part].pos.y);

    XFreePixmap(WMScreenDisplay(scr), pix);
}


static void
updatePreviewBox(_Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->parent);
    Display *dpy = WMScreenDisplay(scr);
 /*   RContext *rc = WMScreenRContext(scr);*/
    int refresh = 0;
    Pixmap pix;
    GC gc;
    int colorUpdate = 0;
    WMColor *black = WMBlackColor(scr);

    gc = XCreateGC(dpy, WMWidgetXID(panel->parent), 0, NULL);


    if (panel->preview == None) {
	WMColor *color;

	panel->preview = XCreatePixmap(dpy, WMWidgetXID(panel->parent),
				       240-4, 215-4, WMScreenDepth(scr));

	color = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);
	XFillRectangle(dpy, panel->preview, WMColorGC(color),
		       0, 0, 240-4, 215-4);
	WMReleaseColor(color);

	refresh = -1;
    }


    if (elements & (1<<PFOCUSED)) {
	renderPreview(panel, gc, PFOCUSED, RBEV_RAISED2);
	XDrawRectangle(dpy, panel->preview, WMColorGC(black),
		       previewPositions[PFOCUSED].pos.x-1,
		       previewPositions[PFOCUSED].pos.y-1,
		       previewPositions[PFOCUSED].size.width,
		       previewPositions[PFOCUSED].size.height);
	colorUpdate |= FTITLE_COL;
    }
    if (elements & (1<<PUNFOCUSED)) {
	renderPreview(panel, gc, PUNFOCUSED, RBEV_RAISED2);
	XDrawRectangle(dpy, panel->preview, WMColorGC(black),
		       previewPositions[PUNFOCUSED].pos.x-1,
		       previewPositions[PUNFOCUSED].pos.y-1,
		       previewPositions[PUNFOCUSED].size.width,
		       previewPositions[PUNFOCUSED].size.height);
	colorUpdate |= UTITLE_COL;
    }
    if (elements & (1<<POWNER)) {
	renderPreview(panel, gc, POWNER, RBEV_RAISED2);
	XDrawRectangle(dpy, panel->preview, WMColorGC(black),
		       previewPositions[POWNER].pos.x-1,
		       previewPositions[POWNER].pos.y-1,
		       previewPositions[POWNER].size.width,
		       previewPositions[POWNER].size.height);
	colorUpdate |= OTITLE_COL;
    }
    if (elements & (1<<PRESIZEBAR)) {
	renderPreview(panel, gc, PRESIZEBAR, RESIZEBAR_BEVEL);
    }
    if (elements & (1<<PMTITLE)) {
	renderPreview(panel, gc, PMTITLE, RBEV_RAISED2);
	colorUpdate |= MTITLE_COL;
    }
    if (elements & (1<<PMITEM)) {
	WMListItem *item;
	TextureListItem *titem;

	item = WMGetListItem(panel->texLs, panel->textureIndex[5]);
	titem = (TextureListItem*)item->clientData;

	pix = renderMenu(panel, titem->prop,
			 previewPositions[PMITEM].size.width, 
			 previewPositions[PMITEM].size.height/4);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0,
		  previewPositions[PMITEM].size.width, 
		  previewPositions[PMITEM].size.height,
		  previewPositions[PMITEM].pos.x,
		  previewPositions[PMITEM].pos.y);

	XFreePixmap(dpy, pix);

	colorUpdate |= MITEM_COL|MDISAB_COL|MHIGH_COL|MHIGHT_COL;
    }
    if (elements & (1<<PMITEM|1<<PMTITLE)) {
	XDrawLine(dpy, panel->preview, gc, 29, 120, 29, 120+20*4+20);
	XDrawLine(dpy, panel->preview, gc, 29, 119, 119, 119);
    }
    if (elements & (1<<PICON)) {
	WMListItem *item;
	TextureListItem *titem;

	item = WMGetListItem(panel->texLs, panel->textureIndex[6]);
	titem = (TextureListItem*)item->clientData;

	renderPreview(panel, gc, PICON,
		      titem->ispixmap ? 0 : RBEV_RAISED3);

	colorUpdate |= ICONT_COL|ICONB_COL|CLIP_COL|CCLIP_COL;
    }
    if (refresh < 0) {
	WMPixmap *p;

	p = WMCreatePixmapFromXPixmaps(scr, panel->preview, None,
				       240-4, 215-4, WMScreenDepth(scr));

	WMSetLabelImage(panel->prevL, p);
	WMReleasePixmap(p);

	if (colorUpdate)
	    updateColorPreviewBox(panel, colorUpdate);
    } else {
	if (colorUpdate)
	    updateColorPreviewBox(panel, colorUpdate);
	else
	    WMRedisplayWidget(panel->prevL);
    }

    XFreeGC(dpy, gc);
    WMReleaseColor(black);
}




static void
cancelNewTexture(void *data)
{
    _Panel *panel = (_Panel*)data;
    
    HideTexturePanel(panel->texturePanel);
}




static char*
makeFileName(char *prefix)
{
    char *fname;

    fname = wstrdup(prefix);

    while (access(fname, F_OK)==0) {
	char buf[30];

	wfree(fname);
	sprintf(buf, "%08lx.cache", time(NULL));
	fname = wstrconcat(prefix, buf);
    }

    return fname;
}




static void
okNewTexture(void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *item;
    char *name;
    char *str;
    proplist_t prop;
    TextureListItem *titem;
    WMScreen *scr = WMWidgetScreen(panel->parent);

    titem = wmalloc(sizeof(TextureListItem));
    memset(titem, 0, sizeof(TextureListItem));

    HideTexturePanel(panel->texturePanel);

    name = GetTexturePanelTextureName(panel->texturePanel);

    prop = GetTexturePanelTexture(panel->texturePanel);

    str = PLGetDescription(prop);

    titem->title = name;
    titem->prop = prop;
    titem->texture = str;
    titem->selectedFor = 0;

    titem->ispixmap = isPixmap(prop);

    titem->path = makeFileName(panel->fprefix);
    titem->preview = renderTexture(scr, prop, TEXPREV_WIDTH, TEXPREV_HEIGHT,
				   titem->path, 0);

    item = WMAddListItem(panel->texLs, "");
    item->clientData = titem;

    WMSetListPosition(panel->texLs, WMGetListNumberOfRows(panel->texLs));
}


static void
okEditTexture(void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *item;
    char *name;
    char *str;
    proplist_t prop;
    TextureListItem *titem;

    item = WMGetListItem(panel->texLs, WMGetListSelectedItemRow(panel->texLs));
    titem = (TextureListItem*)item->clientData;

    HideTexturePanel(panel->texturePanel);

    if (titem->current) {
	name = GetTexturePanelTextureName(panel->texturePanel);

	wfree(titem->title);
	titem->title = name;
    }

    prop = GetTexturePanelTexture(panel->texturePanel);

    str = PLGetDescription(prop);

    PLRelease(titem->prop);
    titem->prop = prop;

    titem->ispixmap = isPixmap(prop);

    wfree(titem->texture);
    titem->texture = str;

    XFreePixmap(WMScreenDisplay(WMWidgetScreen(panel->texLs)), titem->preview);
    titem->preview = renderTexture(WMWidgetScreen(panel->texLs), titem->prop,
				   TEXPREV_WIDTH, TEXPREV_HEIGHT,
				   titem->path, 0);

    WMRedisplayWidget(panel->texLs);

    if (titem->selectedFor)
	updatePreviewBox(panel, titem->selectedFor);

    changePage(panel->secP, panel);
}



static void
editTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *item;
    TextureListItem *titem;

    item = WMGetListItem(panel->texLs, WMGetListSelectedItemRow(panel->texLs));
    titem = (TextureListItem*)item->clientData;

    SetTexturePanelPixmapPath(panel->texturePanel,
			      GetObjectForKey("PixmapPath"));

    SetTexturePanelTexture(panel->texturePanel, titem->title, titem->prop);

    SetTexturePanelCancelAction(panel->texturePanel, cancelNewTexture, panel);
    SetTexturePanelOkAction(panel->texturePanel, okEditTexture, panel);

    ShowTexturePanel(panel->texturePanel);
}



static void
newTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    SetTexturePanelPixmapPath(panel->texturePanel,
			      GetObjectForKey("PixmapPath"));

    SetTexturePanelTexture(panel->texturePanel, "New Texture", NULL);

    SetTexturePanelCancelAction(panel->texturePanel, cancelNewTexture, panel);

    SetTexturePanelOkAction(panel->texturePanel, okNewTexture, panel);

    ShowTexturePanel(panel->texturePanel);
}



static void
deleteTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *item;
    TextureListItem *titem;
    int row;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->secP);
    row = WMGetListSelectedItemRow(panel->texLs);
    item = WMGetListItem(panel->texLs, row);
    titem = (TextureListItem*)item->clientData;

    if (titem->selectedFor & (1 << section)) {
	TextureListItem *titem2;

	panel->textureIndex[section] = section;
	item = WMGetListItem(panel->texLs, section);
	titem2 = (TextureListItem*)item->clientData;
	titem2->selectedFor |= 1 << section;
    }

    wfree(titem->title);
    wfree(titem->texture);
    PLRelease(titem->prop);
    if (titem->path) {
	if (remove(titem->path) < 0 && errno != ENOENT) {
	    wsyserror("could not remove file %s", titem->path);
	}
	wfree(titem->path);
    }

    wfree(titem);

    WMRemoveListItem(panel->texLs, row);
    WMSetButtonEnabled(panel->delB, False);
}




static void
extractTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char *path;
    WMOpenPanel *opanel;
    WMScreen *scr = WMWidgetScreen(w);

    opanel = WMGetOpenPanel(scr);
    WMSetFilePanelCanChooseDirectories(opanel, False);
    WMSetFilePanelCanChooseFiles(opanel, True);

    if (WMRunModalFilePanelForDirectory(opanel, panel->parent, wgethomedir(),
					_("Select File"), NULL)) {
	path = WMGetFilePanelFileName(opanel);

	OpenExtractPanelFor(panel, path);

	wfree(path);
    }
}


static void
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;
    WMListItem *item;
    TextureListItem *titem;
    WMScreen *scr = WMWidgetScreen(panel->box);
    RContext *rc = WMScreenRContext(scr);
    static WMPoint positions[] = {
	{5, 10},
	{5, 40},
	{5, 70},
	{5, 100},
	{5, 120},
	{5, 160},
	{130, 150}
    };

    if (w) {
	section = WMGetPopUpButtonSelectedItem(panel->secP);

	WMSelectListItem(panel->texLs, panel->textureIndex[section]);

	WMSetListPosition(panel->texLs, panel->textureIndex[section] - 2);

	item = WMGetListItem(panel->texLs, panel->textureIndex[section]);

	titem = (TextureListItem*)item->clientData;
    }
    {
	WMColor *color;

	color = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);
	XFillRectangle(rc->dpy, panel->preview, WMColorGC(color),
		       positions[panel->oldsection].x, 
		       positions[panel->oldsection].y, 22, 22);
	WMReleaseColor(color);
    }
    if (w) {
	panel->oldsection = section;
	WMDrawPixmap(panel->hand, panel->preview, positions[section].x, 
		     positions[section].y);
    }
    WMRedisplayWidget(panel->prevL);
}



static void
previewClick(XEvent *event, void *clientData)
{
    _Panel *panel = (_Panel*)clientData;
    int i;

    switch (panel->oldTabItem) {
     case 0:
	for (i = 0; i < sizeof(previewPositions)/sizeof(WMRect); i++) {
	    if (event->xbutton.x >= previewPositions[i].pos.x
		&& event->xbutton.y >= previewPositions[i].pos.y
		&& event->xbutton.x < previewPositions[i].pos.x 
		+ previewPositions[i].size.width
		&& event->xbutton.y < previewPositions[i].pos.y 
		+ previewPositions[i].size.height) {
		
		WMSetPopUpButtonSelectedItem(panel->secP, i);
		changePage(panel->secP, panel);
		return;
	    }
	}
	break;
     case 1:
	for (i = 0; i < WMGetPopUpButtonNumberOfItems(panel->colP); i++) {
	    if (event->xbutton.x >= previewColorPositions[i].pos.x
		&& event->xbutton.y >= previewColorPositions[i].pos.y
		&& event->xbutton.x < previewColorPositions[i].pos.x 
		+ previewColorPositions[i].size.width
		&& event->xbutton.y < previewColorPositions[i].pos.y 
		+ previewColorPositions[i].size.height) {
		
		/* yuck kluge */
		if (i == 7)
		    i = 4;
		
		WMSetPopUpButtonSelectedItem(panel->colP, i);
		changeColorPage(panel->colP, panel);
		return;
	    }
	}
	break;
    }
}


static void
textureClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i;
    WMListItem *item;
    TextureListItem *titem;

    i = WMGetListSelectedItemRow(panel->texLs);

    item = WMGetListItem(panel->texLs, i);

    titem = (TextureListItem*)item->clientData;

    if (titem->current) {
	WMSetButtonEnabled(panel->delB, False);
    } else {
	WMSetButtonEnabled(panel->delB, True);
    }
}



static void
textureDoubleClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i, section;
    WMListItem *item;
    TextureListItem *titem;

    /* unselect old texture */
    section = WMGetPopUpButtonSelectedItem(panel->secP);

    item = WMGetListItem(panel->texLs, panel->textureIndex[section]);
    titem = (TextureListItem*)item->clientData;
    titem->selectedFor &= ~(1 << section);

    /* select new texture */
    i = WMGetListSelectedItemRow(panel->texLs);

    item = WMGetListItem(panel->texLs, i);

    titem = (TextureListItem*)item->clientData;

    titem->selectedFor |= 1<<section;

    panel->textureIndex[section] = i;

    WMRedisplayWidget(panel->texLs);

    updatePreviewBox(panel, 1<<section);
}



static void
paintListItem(WMList *lPtr, int index, Drawable d, char *text, int state, 
	      WMRect *rect)
{
    _Panel *panel = (_Panel*)WMGetHangedData(lPtr);
    WMScreen *scr = WMWidgetScreen(lPtr);
    int width, height, x, y;
    Display *dpy = WMScreenDisplay(scr);
    WMColor *white = WMWhiteColor(scr);
    WMListItem *item;
    WMColor *black = WMBlackColor(scr);
    TextureListItem *titem;

    item = WMGetListItem(lPtr, index);
    titem = (TextureListItem*)item->clientData;
    if (!titem) {
	WMReleaseColor(white);
	WMReleaseColor(black);
	return;
    }

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
        XFillRectangle(dpy, d, WMColorGC(white), x, y, width, height);
    else
        XClearArea(dpy, d, x, y, width, height, False);


    if (titem->preview)
	XCopyArea(dpy, titem->preview, d, WMColorGC(black), 0, 0, 
		  TEXPREV_WIDTH, TEXPREV_HEIGHT, x + 5, y + 5);
    
    if ((1 << WMGetPopUpButtonSelectedItem(panel->secP)) & titem->selectedFor)
	WMDrawPixmap(panel->onLed, d, x + TEXPREV_WIDTH + 10, y + 6);
    else if (titem->selectedFor)
	WMDrawPixmap(panel->offLed, d, x + TEXPREV_WIDTH + 10, y + 6);

    WMDrawString(scr, d, WMColorGC(black), panel->boldFont,
		 x + TEXPREV_WIDTH + 22, y + 2, titem->title, 
		 strlen(titem->title));

    WMDrawString(scr, d, WMColorGC(black), panel->smallFont,
		 x + TEXPREV_WIDTH + 14, y + 18, titem->texture, 
		 strlen(titem->texture));


    WMReleaseColor(white);
    WMReleaseColor(black);
}



static Pixmap
loadRImage(WMScreen *scr, char *path)
{
    FILE *f;
    RImage *image;
    int w, h, d;
    Pixmap pixmap;

    f = fopen(path, "r");
    if (!f)
	return None;

    fscanf(f, "%02x%02x%1x", &w, &h, &d);

    image = RCreateImage(w, h, d == 4);
    fread(image->data, 1, w*h*d, f);
    fclose(f);

    RConvertImage(WMScreenRContext(scr), image, &pixmap);
    RReleaseImage(image);

    return pixmap;
}



static void
fillTextureList(WMList *lPtr)
{
    proplist_t textureList;
    proplist_t texture;
    WMUserDefaults *udb = WMGetStandardUserDefaults();
    TextureListItem *titem;
    WMScreen *scr  = WMWidgetScreen(lPtr);
    int i;

    textureList = WMGetUDObjectForKey(udb, "TextureList");
    if (!textureList)
	return;

    for (i = 0; i < PLGetNumberOfElements(textureList); i++) {
	WMListItem *item;

	texture = PLGetArrayElement(textureList, i);

	titem = wmalloc(sizeof(TextureListItem));
	memset(titem, 0, sizeof(TextureListItem));

	titem->title = wstrdup(PLGetString(PLGetArrayElement(texture, 0)));
	titem->prop = PLRetain(PLGetArrayElement(texture, 1));
	titem->texture = PLGetDescription(titem->prop);
	titem->selectedFor = 0;
	titem->path = wstrdup(PLGetString(PLGetArrayElement(texture, 2)));

	titem->preview = loadRImage(scr, titem->path);
	if (!titem->preview) {
	    titem->preview = renderTexture(scr, titem->prop, TEXPREV_WIDTH, 
					   TEXPREV_HEIGHT, NULL, 0);
	}
	item = WMAddListItem(lPtr, "");
	item->clientData = titem;
    }
}


static void
fillColorList(_Panel *panel)
{
    WMColor *color;
    proplist_t list;
    WMUserDefaults *udb = WMGetStandardUserDefaults();
    WMScreen *scr = WMWidgetScreen(panel->box);
    int i;
    
    list = WMGetUDObjectForKey(udb, "ColorList");
    if (!list) {
	for (i = 0; i < 24; i++) {
	    color = WMCreateNamedColor(scr, sampleColors[i], False);
	    if (!color)
		continue;
	    WMSetColorWellColor(panel->sampW[i], color);
	    WMReleaseColor(color);
	}
    } else {
	proplist_t c;

	for (i = 0; i < WMIN(24, PLGetNumberOfElements(list)); i++) {
	    c = PLGetArrayElement(list, i);
	    if (!c || !PLIsString(c))
		continue;
	    color = WMCreateNamedColor(scr, PLGetString(c), False);
	    if (!color)
		continue;
	    WMSetColorWellColor(panel->sampW[i], color);
	    WMReleaseColor(color);
	}
    }
}


/*************************************************************************/


static void
changeColorPage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;
    WMScreen *scr = WMWidgetScreen(panel->box);
    RContext *rc = WMScreenRContext(scr);
    static WMPoint positions[] = {
	{5, 10},
	{5, 40},
	{5, 70},
	{5, 120},
	{5, 140},
	{5, 160},
	{5, 180},
	{5, 180},
	{130, 140},
	{130, 140},
	{130, 140},
	{130, 140}
    };

    if (panel->preview) {
	WMColor *color;

	color = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);
	XFillRectangle(rc->dpy, panel->preview, WMColorGC(color),
		       positions[panel->oldcsection].x, 
		       positions[panel->oldcsection].y, 22, 22);
	WMReleaseColor(color);
    }
    if (w) {
	section = WMGetPopUpButtonSelectedItem(panel->colP);

	panel->oldcsection = section;
	if (panel->preview)
	    WMDrawPixmap(panel->hand, panel->preview, positions[section].x, 
			 positions[section].y);

	section = WMGetPopUpButtonSelectedItem(panel->colP);

	WMSetColorWellColor(panel->colW, panel->colors[section]);
    }
    WMRedisplayWidget(panel->prevL);
}


static void
paintText(WMScreen *scr, Drawable d, WMColor *color, WMFont *font,
	  int x, int y, int w, int h, WMAlignment align, char *text)
{
    int l = strlen(text);

    switch (align) {
     case WALeft:
	x += 5;
	break;
     case WARight:
	x += w - 5 - WMWidthOfString(font, text, l);
	break;
     default:
     case WACenter:
	x += (w - WMWidthOfString(font, text, l))/2;
	break;
    }
    WMDrawString(scr, d, WMColorGC(color), font, x,
		 y + (h - WMFontHeight(font))/2, text, l);
}



static void
updateColorPreviewBox(_Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->box);
    WMPixmap *pixmap;
    Pixmap d;

    pixmap = WMGetLabelImage(panel->prevL);
    d = WMGetPixmapXID(pixmap);

    if (elements & FTITLE_COL) {
	paintText(scr, d, panel->colors[0], panel->boldFont, 30, 10, 190, 20,
		  panel->titleAlignment, _("Focused Window"));
    }
    if (elements & UTITLE_COL) {
	paintText(scr, d, panel->colors[1], panel->boldFont, 30, 40, 190, 20,
		  panel->titleAlignment, _("Unfocused Window"));
    }
    if (elements & OTITLE_COL) {
	paintText(scr, d, panel->colors[2], panel->boldFont, 30, 70, 190, 20,
		  panel->titleAlignment, _("Owner of Focused Window"));
    }
    if (elements & MTITLE_COL) {
	paintText(scr, d, panel->colors[3], panel->boldFont, 30, 120, 90, 20,
		  WALeft, _("Menu Title"));
    }
    if (elements & MITEM_COL) {
	paintText(scr, d, panel->colors[4], panel->normalFont, 30, 140, 90, 20,
		  WALeft, _("Normal Item"));
	paintText(scr, d, panel->colors[4], panel->normalFont, 30, 200, 90, 20,
		  WALeft, _("Normal Item"));
    }
    if (elements & MDISAB_COL) {
	paintText(scr, d, panel->colors[5], panel->normalFont, 30, 160, 90, 20,
		  WALeft, _("Disabled Item"));
    }
    if (elements & MHIGH_COL) {
	XFillRectangle(WMScreenDisplay(scr), d, WMColorGC(panel->colors[6]),
		       31, 181, 87, 17);
	elements |= MHIGHT_COL;
    }
    if (elements & MHIGHT_COL) {
	paintText(scr, d, panel->colors[7], panel->normalFont, 30, 180, 90, 20,
		  WALeft, _("Highlighted"));
    }
/*
    if (elements & ICONT_COL) {
	WRITE(_("Focused Window"), panel->colors[8], panel->boldFont, 
	      155, 130, 64);
    }
    if (elements & ICONB_COL) {
	WRITE(_("Focused Window"), panel->colors[9], panel->boldFont, 
	      0, 0, 30);
    }
    if (elements & CLIP_COL) {
	WRITE(_("Focused Window"), panel->colors[10], panel->boldFont, 
	      0, 0, 30);
    }
    if (elements & CCLIP_COL) {
	WRITE(_("Focused Window"), panel->colors[11], panel->boldFont, 
	      0, 0, 30);
    }
 */
    WMRedisplayWidget(panel->prevL);
}


static void
colorWellObserver(void *self, WMNotification *n)
{
    _Panel *panel = (_Panel*)self;
    int p;

    p = WMGetPopUpButtonSelectedItem(panel->colP);

    WMReleaseColor(panel->colors[p]);

    panel->colors[p] = WMRetainColor(WMGetColorWellColor(panel->colW));

    updateColorPreviewBox(panel, 1<<p);
}


static void
changedTabItem(struct WMTabViewDelegate *self, WMTabView *tabView,
	       WMTabViewItem *item)
{
    _Panel *panel = self->data;
    int i;

    i = WMGetTabViewItemIdentifier(item); 
    switch (i) {
     case 0:
	switch (panel->oldTabItem) {
	 case 1:
	    changeColorPage(NULL, panel);
	    break;
	}
	changePage(panel->secP, panel);
	break;
     case 1:
	switch (panel->oldTabItem) {
	 case 0:
	    changePage(NULL, panel);
	    break;	    
	}
	changeColorPage(panel->colP, panel);
	break;
     case 3:
	switch (panel->oldTabItem) {
	 case 0:
	    changePage(NULL, panel);
	    break;
	 case 1:
	    changeColorPage(NULL, panel);
	    break;
	}
	break;
    }

    panel->oldTabItem = i;
}


/*************************************************************************/

static void
menuStyleCallback(WMWidget *self, void *data)
{
    _Panel *panel = (_Panel*)data;

    if (self == panel->mstyB[0]) {
	panel->menuStyle = MSTYLE_NORMAL;
	updatePreviewBox(panel, 1<<PMITEM);

    } else if (self == panel->mstyB[1]) {
	panel->menuStyle = MSTYLE_SINGLE;
	updatePreviewBox(panel, 1<<PMITEM);

    } else if (self == panel->mstyB[2]) {
	panel->menuStyle = MSTYLE_FLAT;
	updatePreviewBox(panel, 1<<PMITEM);
    }
}


static void
titleAlignCallback(WMWidget *self, void *data)
{
    _Panel *panel = (_Panel*)data;

    if (self == panel->taliB[0]) {
	panel->titleAlignment = WALeft;
	updatePreviewBox(panel, 1<<PFOCUSED|1<<PUNFOCUSED|1<<POWNER);

    } else if (self == panel->taliB[1]) {
	panel->titleAlignment = WACenter;
	updatePreviewBox(panel, 1<<PFOCUSED|1<<PUNFOCUSED|1<<POWNER);

    } else if (self == panel->taliB[2]) {
	panel->titleAlignment = WARight;
	updatePreviewBox(panel, 1<<PFOCUSED|1<<PUNFOCUSED|1<<POWNER);
    }
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMFont *font;
    WMScreen *scr = WMWidgetScreen(panel->parent);
    WMTabViewItem *item;
    int i;
    char *tmp;
    Bool ok = True;
    
    panel->fprefix = wstrconcat(wusergnusteppath(), "/.AppInfo");

    if (access(panel->fprefix, F_OK)!=0) {
	if (mkdir(panel->fprefix, 0755) < 0) {
	    wsyserror(panel->fprefix);
	    ok = False;
	}
    }
    if (ok) {
	tmp = wstrconcat(panel->fprefix, "/WPrefs/");
	wfree(panel->fprefix);
	panel->fprefix = tmp;
	if (access(panel->fprefix, F_OK)!=0) {
	    if (mkdir(panel->fprefix, 0755) < 0) {
		wsyserror(panel->fprefix);
	    }
	}
    }

    panel->smallFont = WMSystemFontOfSize(scr, 10);
    panel->normalFont = WMSystemFontOfSize(scr, 12);
    panel->boldFont = WMBoldSystemFontOfSize(scr, 12);

    panel->onLed = WMCreatePixmapFromXPMData(scr, blueled_xpm);
    panel->offLed = WMCreatePixmapFromXPMData(scr, blueled2_xpm);
    panel->hand = WMCreatePixmapFromXPMData(scr, hand_xpm);

    panel->box = WMCreateBox(panel->parent);
    WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 0, 0);

    /* preview box */
    panel->prevL = WMCreateLabel(panel->box);
    WMResizeWidget(panel->prevL, 240, FRAME_HEIGHT - 20);
    WMMoveWidget(panel->prevL, 15, 10);
    WMSetLabelRelief(panel->prevL, WRSunken);
    WMSetLabelImagePosition(panel->prevL, WIPImageOnly);
    
    WMCreateEventHandler(WMWidgetView(panel->prevL), ButtonPressMask,
			 previewClick, panel);
    

    /* tabview */

    tabviewDelegate.data = panel;

    panel->tabv = WMCreateTabView(panel->box);
    WMResizeWidget(panel->tabv, 245, FRAME_HEIGHT - 20);
    WMMoveWidget(panel->tabv, 265, 10);
    WMSetTabViewDelegate(panel->tabv, &tabviewDelegate);

    /*** texture list ***/

    panel->texF = WMCreateFrame(panel->box);
    WMSetFrameRelief(panel->texF, WRFlat);

    item = WMCreateTabViewItemWithIdentifier(0);
    WMSetTabViewItemView(item, WMWidgetView(panel->texF));
    WMSetTabViewItemLabel(item, _("Texture"));

    WMAddItemInTabView(panel->tabv, item);


    panel->secP = WMCreatePopUpButton(panel->texF);
    WMResizeWidget(panel->secP, 228, 20);
    WMMoveWidget(panel->secP, 7, 7);
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Unfocused Windows"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window's Owner"));
    WMAddPopUpButtonItem(panel->secP, _("Window Resizebar"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Menus"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Items"));
    WMAddPopUpButtonItem(panel->secP, _("Icon Background"));
/*    WMAddPopUpButtonItem(panel->secP, _("Workspace Backgrounds"));
 */
    WMSetPopUpButtonSelectedItem(panel->secP, 0);
    WMSetPopUpButtonAction(panel->secP, changePage, panel);

    panel->texLs = WMCreateList(panel->texF);
    WMResizeWidget(panel->texLs, 165, 155);
    WMMoveWidget(panel->texLs, 70, 33);
    WMSetListUserDrawItemHeight(panel->texLs, 35);
    WMSetListUserDrawProc(panel->texLs, paintListItem);
    WMHangData(panel->texLs, panel);
    WMSetListAction(panel->texLs, textureClick, panel);
    WMSetListDoubleAction(panel->texLs, textureDoubleClick, panel);

    WMSetBalloonTextForView(_("Double click in the texture you want to use\n"
			      "for the selected item."),
			    WMWidgetView(panel->texLs));

    /* command buttons */

    font = WMSystemFontOfSize(scr, 10);


    panel->newB = WMCreateCommandButton(panel->texF);
    WMResizeWidget(panel->newB, 57, 39);
    WMMoveWidget(panel->newB, 7, 33);
    WMSetButtonFont(panel->newB, font);
    WMSetButtonImagePosition(panel->newB, WIPAbove);
    WMSetButtonText(panel->newB, _("New"));
    WMSetButtonAction(panel->newB, newTexture, panel);
    SetButtonAlphaImage(scr, panel->newB, TNEW_FILE, NULL, NULL);

    WMSetBalloonTextForView(_("Create a new texture."),
			    WMWidgetView(panel->newB));

    panel->ripB = WMCreateCommandButton(panel->texF);
    WMResizeWidget(panel->ripB, 57, 39);
    WMMoveWidget(panel->ripB, 7, 72);
    WMSetButtonFont(panel->ripB, font);
    WMSetButtonImagePosition(panel->ripB, WIPAbove);
    WMSetButtonText(panel->ripB, _("Extract..."));
    WMSetButtonAction(panel->ripB, extractTexture, panel);
    SetButtonAlphaImage(scr, panel->ripB, TEXTR_FILE, NULL, NULL);

    WMSetBalloonTextForView(_("Extract texture(s) from a theme or a style file."),
			    WMWidgetView(panel->ripB));

    WMSetButtonEnabled(panel->ripB, False);

    panel->editB = WMCreateCommandButton(panel->texF);
    WMResizeWidget(panel->editB, 57, 39);
    WMMoveWidget(panel->editB, 7, 111);
    WMSetButtonFont(panel->editB, font);
    WMSetButtonImagePosition(panel->editB, WIPAbove);
    WMSetButtonText(panel->editB, _("Edit"));
    SetButtonAlphaImage(scr, panel->editB, TEDIT_FILE, NULL, NULL);
    WMSetButtonAction(panel->editB, editTexture, panel);
    WMSetBalloonTextForView(_("Edit the highlighted texture."),
			    WMWidgetView(panel->editB));

    panel->delB = WMCreateCommandButton(panel->texF);
    WMResizeWidget(panel->delB, 57, 38);
    WMMoveWidget(panel->delB, 7, 150);
    WMSetButtonFont(panel->delB, font);
    WMSetButtonImagePosition(panel->delB, WIPAbove);
    WMSetButtonText(panel->delB, _("Delete"));
    SetButtonAlphaImage(scr, panel->delB, TDEL_FILE, NULL, NULL);
    WMSetButtonEnabled(panel->delB, False);
    WMSetButtonAction(panel->delB, deleteTexture, panel);
    WMSetBalloonTextForView(_("Delete the highlighted texture."),
			    WMWidgetView(panel->delB));

    WMReleaseFont(font);

    WMMapSubwidgets(panel->texF);

    /*** colors ***/
    panel->colF = WMCreateFrame(panel->box);
    WMSetFrameRelief(panel->colF, WRFlat);

    item = WMCreateTabViewItemWithIdentifier(1);
    WMSetTabViewItemView(item, WMWidgetView(panel->colF));
    WMSetTabViewItemLabel(item, _("Color"));

    WMAddItemInTabView(panel->tabv, item);

    panel->colP = WMCreatePopUpButton(panel->colF);
    WMResizeWidget(panel->colP, 228, 20);
    WMMoveWidget(panel->colP, 7, 7);
    WMAddPopUpButtonItem(panel->colP, _("Focused Window Title"));
    WMAddPopUpButtonItem(panel->colP, _("Unfocused Window Title"));
    WMAddPopUpButtonItem(panel->colP, _("Owner of Focused Window Title"));
    WMAddPopUpButtonItem(panel->colP, _("Menu Title"));
    WMAddPopUpButtonItem(panel->colP, _("Menu Item Text"));
    WMAddPopUpButtonItem(panel->colP, _("Disabled Menu Item Text"));
    WMAddPopUpButtonItem(panel->colP, _("Menu Highlight Color"));
    WMAddPopUpButtonItem(panel->colP, _("Highlighted Menu Text Color"));
    /*
    WMAddPopUpButtonItem(panel->colP, _("Miniwindow Title"));
    WMAddPopUpButtonItem(panel->colP, _("Miniwindow Title Back"));
    WMAddPopUpButtonItem(panel->colP, _("Clip Title"));
    WMAddPopUpButtonItem(panel->colP, _("Collapsed Clip Title"));
     */

    WMSetPopUpButtonSelectedItem(panel->colP, 0);

    WMSetPopUpButtonAction(panel->colP, changeColorPage, panel);
 

    panel->colW = WMCreateColorWell(panel->colF);
    WMResizeWidget(panel->colW, 65, 50);
    WMMoveWidget(panel->colW, 30, 75);
    WMAddNotificationObserver(colorWellObserver, panel,
			      WMColorWellDidChangeNotification, panel->colW);

    for (i = 0; i < 4; i++) {
	int j;
	for (j = 0; j < 6; j++) {
	    panel->sampW[i+j*4] = WMCreateColorWell(panel->colF);
	    WMResizeWidget(panel->sampW[i+j*4], 22, 22);
	    WMMoveWidget(panel->sampW[i+j*4], 130 + i*22, 40 + j*22);
	    WSetColorWellBordered(panel->sampW[i+j*4], False);
	}
    }

    WMMapSubwidgets(panel->colF);
    
#ifdef unfinished
    /*** root bg ***/
    
    panel->bgF = WMCreateFrame(panel->box);
    WMSetFrameRelief(panel->bgF, WRFlat);
    
    item = WMCreateTabViewItemWithIdentifier(2);
    WMSetTabViewItemView(item, WMWidgetView(panel->bgF));
    WMSetTabViewItemLabel(item, _("Background"));

    WMAddItemInTabView(panel->tabv, item);

    panel->bgprevL = WMCreateLabel(panel->bgF);
    WMResizeWidget(panel->bgprevL, 230, 155);
    WMMoveWidget(panel->bgprevL, 5, 5);
    WMSetLabelRelief(panel->bgprevL, WRSunken);

    panel->selbgB = WMCreateCommandButton(panel->bgF);
    WMMoveWidget(panel->selbgB, 5, 165);
    WMResizeWidget(panel->selbgB, 100, 24);
    WMSetButtonText(panel->selbgB, _("Browse..."));
    
    
    
    
    WMMapSubwidgets(panel->bgF);
#endif /* unfinished */
    /*** options ***/
    panel->optF = WMCreateFrame(panel->box);
    WMSetFrameRelief(panel->optF, WRFlat);

    item = WMCreateTabViewItemWithIdentifier(3);
    WMSetTabViewItemView(item, WMWidgetView(panel->optF));
    WMSetTabViewItemLabel(item, _("Options"));

    WMAddItemInTabView(panel->tabv, item);

    panel->mstyF = WMCreateFrame(panel->optF);
    WMResizeWidget(panel->mstyF, 215, 85);
    WMMoveWidget(panel->mstyF, 15, 10);
    WMSetFrameTitle(panel->mstyF, _("Menu Style"));

    for (i = 0; i < 3; i++) {
	WMPixmap *icon;
	char *path;

	panel->mstyB[i] = WMCreateButton(panel->mstyF, WBTOnOff);
	WMResizeWidget(panel->mstyB[i], 54, 54);
	WMMoveWidget(panel->mstyB[i], 15 + i*65, 20);
	WMSetButtonImagePosition(panel->mstyB[i], WIPImageOnly);
	WMSetButtonAction(panel->mstyB[i], menuStyleCallback, panel);
	switch (i) {
	 case 0:
	    path = LocateImage(MSTYLE1_FILE);
	    break;
	 case 1:
	    path = LocateImage(MSTYLE2_FILE);
	    break;
	 case 2:
	    path = LocateImage(MSTYLE3_FILE);
	    break;
	}
	if (path) {
	    icon = WMCreatePixmapFromFile(scr, path);
	    if (icon) {
		WMSetButtonImage(panel->mstyB[i], icon);
		WMReleasePixmap(icon);
	    } else {
		wwarning(_("could not load icon file %s"), path);
	    }
	    wfree(path);
	}
    }
    WMGroupButtons(panel->mstyB[0], panel->mstyB[1]);
    WMGroupButtons(panel->mstyB[0], panel->mstyB[2]);

    WMMapSubwidgets(panel->mstyF);


    panel->taliF = WMCreateFrame(panel->optF);
    WMResizeWidget(panel->taliF, 110, 80);
    WMMoveWidget(panel->taliF, 15, 100);
    WMSetFrameTitle(panel->taliF, _("Title Alignment"));

    for (i = 0; i < 3; i++) {
	panel->taliB[i] = WMCreateRadioButton(panel->taliF);
	WMSetButtonAction(panel->taliB[i], titleAlignCallback, panel);
	switch (i) {
	 case 0:
	    WMSetButtonText(panel->taliB[i], _("Left"));
	    break;
	 case 1:
	    WMSetButtonText(panel->taliB[i], _("Center"));
	    break;
	 case 2:
	    WMSetButtonText(panel->taliB[i], _("Right"));
	    break;
	}
	WMResizeWidget(panel->taliB[i], 90, 18);
	WMMoveWidget(panel->taliB[i], 10, 15 + 20*i);
    }
    WMGroupButtons(panel->taliB[0], panel->taliB[1]);
    WMGroupButtons(panel->taliB[0], panel->taliB[2]);

    WMMapSubwidgets(panel->taliF);
    
    WMMapSubwidgets(panel->optF);

    /**/

    WMRealizeWidget(panel->box);
    WMMapSubwidgets(panel->box);

    WMSetPopUpButtonSelectedItem(panel->secP, 0);

    showData(panel);

    changePage(panel->secP, panel);

    fillTextureList(panel->texLs);

    fillColorList(panel);

    panel->texturePanel = CreateTexturePanel(panel->parent);
}



static void
setupTextureFor(WMList *list, char *key, char *defValue, char *title, 
		int index)
{
    WMListItem *item;
    TextureListItem *titem;

    titem = wmalloc(sizeof(TextureListItem));
    memset(titem, 0, sizeof(TextureListItem));

    titem->title = wstrdup(title);
    titem->prop = GetObjectForKey(key);
    if (!titem->prop || !PLIsArray(titem->prop)) {
        /* Maybe also give a error message to stderr that the entry is bad? */
	titem->prop = PLGetProplistWithDescription(defValue);
    } else {
	PLRetain(titem->prop);
    }
    titem->texture = PLGetDescription((proplist_t)titem->prop);
    titem->current = 1;
    titem->selectedFor = 1<<index;
    
    titem->ispixmap = isPixmap(titem->prop);

    titem->preview = renderTexture(WMWidgetScreen(list), titem->prop,
				   TEXPREV_WIDTH, TEXPREV_HEIGHT, NULL, 0);

    item = WMAddListItem(list, "");
    item->clientData = titem;
}



static void
showData(_Panel *panel)
{
    int i;
    char *str;

    str = GetStringForKey("MenuStyle");
    if (str && strcasecmp(str, "flat")==0) {
	panel->menuStyle = MSTYLE_FLAT;
    } else if (str && strcasecmp(str, "singletexture")==0) {
	panel->menuStyle = MSTYLE_SINGLE;
    } else {
	panel->menuStyle = MSTYLE_NORMAL;
    }

    str = GetStringForKey("TitleJustify");
    if (str && strcasecmp(str, "left")==0) {
	panel->titleAlignment = WALeft;
    } else if (str && strcasecmp(str, "right")==0) {
	panel->titleAlignment = WARight;
    } else {
	panel->titleAlignment = WACenter;
    }

    for (i = 0; i < sizeof(colorOptions)/(2*sizeof(char*)); i++) {
	WMColor *color;

	str = GetStringForKey(colorOptions[i*2]);
	if (!str)
	    str = colorOptions[i*2+1];

	if (!(color = WMCreateNamedColor(WMWidgetScreen(panel->box), str, False))) {
	    color = WMCreateNamedColor(WMWidgetScreen(panel->box), "#000000", False);
	}

	panel->colors[i] = color;
    }
    changeColorPage(panel->colP, panel);

    for (i = 0; i < sizeof(textureOptions)/(3*sizeof(char*)); i++) {
	setupTextureFor(panel->texLs, textureOptions[i*3],
			textureOptions[i*3+1], textureOptions[i*3+2], i);
	panel->textureIndex[i] = i;
    }
    updatePreviewBox(panel, EVERYTHING);

    WMSetButtonSelected(panel->mstyB[panel->menuStyle], True);
    WMSetButtonSelected(panel->taliB[panel->titleAlignment], True);
}


static void
storeData(_Panel *panel)
{
    TextureListItem *titem;
    WMListItem *item;
    int i;

    for (i = 0; i < sizeof(textureOptions)/(sizeof(char*)*3); i++) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[i]);
	titem = (TextureListItem*)item->clientData;
	SetObjectForKey(titem->prop, textureOptions[i*3]);
    }

    for (i = 0; i < 8; i++) {
	char *str;

	str = WMGetColorRGBDescription(panel->colors[i]);

	if (str) {
	    SetStringForKey(str, colorOptions[i*2]);
	    wfree(str);
	}
    }

    switch (panel->menuStyle) {
     case MSTYLE_SINGLE:
	SetStringForKey("singletexture", "MenuStyle");
	break;
     case MSTYLE_FLAT:
	SetStringForKey("flat", "MenuStyle");
	break;
     default:
     case MSTYLE_NORMAL:
	SetStringForKey("normal", "MenuStyle");
	break;
    }
    switch (panel->titleAlignment) {
     case WALeft:
	SetStringForKey("left", "TitleJustify");
	break;
     case WARight:
	SetStringForKey("right", "TitleJustify");
	break;
     default:
     case WACenter:
	SetStringForKey("center", "TitleJustify");
	break;
    }
}


static void
prepareForClose(_Panel *panel)
{
    proplist_t textureList;
    proplist_t texture;
    TextureListItem *titem;
    WMListItem *item;
    WMUserDefaults *udb = WMGetStandardUserDefaults();
    int i;

    textureList = PLMakeArrayFromElements(NULL, NULL);

    /* store list of textures */
    for (i = 7; i < WMGetListNumberOfRows(panel->texLs); i++) {
	item = WMGetListItem(panel->texLs, i);
	titem = (TextureListItem*)item->clientData;

	texture = PLMakeArrayFromElements(PLMakeString(titem->title),
					  PLRetain(titem->prop),
					  PLMakeString(titem->path),
					  NULL);

	PLAppendArrayElement(textureList, texture);
    }

    WMSetUDObjectForKey(udb, textureList, "TextureList");
    PLRelease(textureList);
    
    /* store list of colors */
    textureList = PLMakeArrayFromElements(NULL, NULL);
    for (i = 0; i < 24; i++) {
	WMColor *color;
	char *str;

	color = WMGetColorWellColor(panel->sampW[i]);

	str = WMGetColorRGBDescription(color);
	PLAppendArrayElement(textureList, PLMakeString(str));
	wfree(str);
    }
    WMSetUDObjectForKey(udb, textureList, "ColorList");
    PLRelease(textureList);

    WMSynchronizeUserDefaults(udb);
}



Panel*
InitAppearance(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));
    
    panel->sectionName = _("Appearance Preferences");

    panel->description = _("Background texture configuration for windows,\n"
			   "menus and icons.");

    panel->parent = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.prepareForClose = prepareForClose;

    AddSection(panel, ICON_FILE);

    return panel;
}



/****************************************************************************/



typedef struct ExtractPanel {
    WMWindow *win;

    WMLabel *label;
    WMList *list;

    WMButton *closeB;
    WMButton *extrB;
} ExtractPanel;



static void
OpenExtractPanelFor(_Panel *panel, char *path)
{
    ExtractPanel *epanel;
    WMColor *color;
    WMFont *font;
    WMScreen *scr = WMWidgetScreen(panel->parent);

    epanel = wmalloc(sizeof(ExtractPanel));
    epanel->win = WMCreatePanelWithStyleForWindow(panel->parent, "extract",
						  WMTitledWindowMask
						  |WMClosableWindowMask);
    WMResizeWidget(epanel->win, 245, 250);
    WMSetWindowTitle(epanel->win, _("Extract Texture"));

    epanel->label = WMCreateLabel(epanel->win);
    WMResizeWidget(epanel->label, 225, 18);
    WMMoveWidget(epanel->label, 10, 10);
    WMSetLabelTextAlignment(epanel->label, WACenter);
    WMSetLabelRelief(epanel->label, WRSunken);

    color = WMDarkGrayColor(scr);
    WMSetWidgetBackgroundColor(epanel->label, color);
    WMReleaseColor(color);

    color = WMWhiteColor(scr);
    WMSetLabelTextColor(epanel->label, color);
    WMReleaseColor(color);

    font = WMBoldSystemFontOfSize(scr, 12);
    WMSetLabelFont(epanel->label, font);
    WMReleaseFont(font);

    WMSetLabelText(epanel->label, _("Textures"));

    epanel->list = WMCreateList(epanel->win);
    WMResizeWidget(epanel->list, 225, 165);
    WMMoveWidget(epanel->list, 10, 30);

    

    epanel->closeB = WMCreateCommandButton(epanel->win);
    WMResizeWidget(epanel->closeB, 74, 24);
    WMMoveWidget(epanel->closeB, 165, 215);
    WMSetButtonText(epanel->closeB, _("Close"));

    epanel->extrB = WMCreateCommandButton(epanel->win);
    WMResizeWidget(epanel->extrB, 74, 24);
    WMMoveWidget(epanel->extrB, 80, 215);
    WMSetButtonText(epanel->extrB, _("Extract"));

    WMMapSubwidgets(epanel->win);

    
    /* take textures from file */
    
    

    WMRealizeWidget(epanel->win);
    
    WMMapWidget(epanel->win);
}

