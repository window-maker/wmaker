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
    WMFrame *frame;
    char *sectionName;

    char *description;

    CallbackRec callbacks;

    WMWindow *win;

    WMLabel *prevL;

    WMPopUpButton *secP;

    /* texture list */
    WMLabel *texL;
    WMList *texLs;
    WMLabel *texsL;

    WMButton *newB;
    WMButton *editB;
    WMButton *ripB;
    WMButton *delB;

    int textureIndex[8];

    WMFont *smallFont;
    WMFont *normalFont;
    WMFont *boldFont;

    TexturePanel *texturePanel;

    WMPixmap *onLed;
    WMPixmap *offLed;
    WMPixmap *hand;

    int oldsection;
    
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


static void showData(_Panel *panel);

static void changePage(WMWidget *w, void *data);

static void OpenExtractPanelFor(_Panel *panel, char *path);

#define ICON_FILE	"appearance"

#define TNEW_FILE 	"tnew"
#define TDEL_FILE	"tdel"
#define TEDIT_FILE	"tedit"
#define TEXTR_FILE 	"textr"



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
"22 21 17 1",
" 	c None",
".	c #030305",
"+	c #101010",
"@	c #535355",
"#	c #7F7F7E",
"$	c #969697",
"%	c #B5B5B6",
"&	c #C5C5C6",
"*	c #D2D2D0",
"=	c #DCDCDC",
"-	c #E5E5E4",
";	c #ECECEC",
">	c #767674",
",	c #F2F2F4",
"'	c #676767",
")	c #FDFDFB",
"!	c #323234",
"                      ",
"       .....          ",
"     ..#%&&$.         ",
"    .))),%..........  ",
"   .)-)),&)))))))))$. ",
"  .-&))))))))),,,,;;. ",
" .=)))))))));-=***&$. ",
" .=)))))))),..+.....  ",
" +=)))))))))-&#.      ",
" +=)))))))))-%>.      ",
" +&)))))))))-%'.      ",
" +$,,))))));...       ",
" .#%%*;,)),*$>+       ",
"  .'>$%&&&&$#@.       ",
"   .!'>#$##>'.        ",
"     ..+++++.         ",
"                      ",
"     ##@@@##          ",
"   @@@@@@@@@@@        ",
"     >>@@@##          ",
"                      "};




#define	FTITLE	(1<<0)
#define UTITLE	(1<<1)
#define OTITLE	(1<<2)
#define RESIZEBAR (1<<3)
#define MTITLE	(1<<4)
#define MITEM	(1<<5)
#define ICON	(1<<6)
#define BACK	(1<<7)
#define EVERYTHING	0xff


#define RESIZEBAR_BEVEL	-1
#define MENU_BEVEL 	-2

#define TEXPREV_WIDTH	40
#define TEXPREV_HEIGHT	24





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

    f = fopen(path, "w");
    if (!f) {
	wsyserror(path);
	return;
    }
    fprintf(f, "%02x%02x%1x", image->width, image->height,
	    image->data[3]!=NULL ? 4 : 3);

    fwrite(image->data[0], 1, image->width * image->height, f);
    fwrite(image->data[1], 1, image->width * image->height, f);
    fwrite(image->data[2], 1, image->width * image->height, f);
    if (image->data[3])
	fwrite(image->data[3], 1, image->width * image->height, f);

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
    int iheight = img->height / 3;
    
    light.alpha = 0;
    light.red = light.green = light.blue = 80;
    
    dark.alpha = 255;
    dark.red = dark.green = dark.blue = 0;
    
    mid.alpha = 0;
    mid.red = mid.green = mid.blue = 40;
    
    for (i = 1; i < 3; i++) {	    
	ROperateLine(img, RSubtractOperation, 0, i*iheight-2,
		     img->width-1, i*iheight-2, &mid);
	
	RDrawLine(img, 0, i*iheight-1,
		  img->width-1, i*iheight-1, &dark);
	
	ROperateLine(img, RAddOperation, 0, i*iheight, 
		     img->width-1, i*iheight, &light);
    }
}


static Pixmap
renderTexture(WMScreen *scr, proplist_t texture, int width, int height,
	      char *path, int border)
{
    char *type;
    RImage *image;
    Pixmap pixmap;
    RContext *rc = WMScreenRContext(scr);
    char *str;
    RColor rcolor;


    type = PLGetString(PLGetArrayElement(texture, 0));

    image = RCreateImage(width, height, False);

    if (strcasecmp(type, "solid")==0) {

	str = PLGetString(PLGetArrayElement(texture, 1));

	str2rcolor(rc, str, &rcolor);

	RClearImage(image, &rcolor);
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

	path = wfindfileinarray(GetObjectForKey("PixmapPath"), str);
	timage = RLoadImage(rc, path, 0);

	if (!timage) {
	    wwarning("could not load file '%s': %s", path,
		     RMessageForError(RErrorCode));
	} else {	
	    grad = RRenderGradient(width, height, &rcolor, &rcolor2, style);

	    image = RMakeTiledImage(timage, width, height);
	    RDestroyImage(timage);

	    i = atoi(PLGetString(PLGetArrayElement(texture, 2)));
	
	    RCombineImagesWithOpaqueness(image, grad, i);
	    RDestroyImage(grad);
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
		free(colors[i]);
	    free(colors);
	}
    } else if (strcasecmp(&type[1], "pixmap")==0) {
	RImage *timage;
	char *path;
	RColor color;

	str = PLGetString(PLGetArrayElement(texture, 1));

	path = wfindfileinarray(GetObjectForKey("PixmapPath"), str);
	timage = RLoadImage(rc, path, 0);

	if (!timage) {
	    wwarning("could not load file '%s': %s", path,
		     RMessageForError(RErrorCode));
	} else {
	    str = PLGetString(PLGetArrayElement(texture, 2));
	    str2rcolor(rc, str, &color);

	    switch (toupper(type[0])) {
	     case 'T':
		image = RMakeTiledImage(timage, width, height);
		RDestroyImage(timage);
		timage = image;
		break;
	     case 'C':
		image = RMakeCenteredImage(timage, width, height, &color);
		RDestroyImage(timage);
		timage = image;
		break;
	     case 'S':
	     case 'M':
		image = RScaleImage(timage, width, height);
		RDestroyImage(timage);
		timage = image;
		break;
	    }

	}
	free(path);
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
	}
    } else if (border) {
	RBevelImage(image, border);
    }

    RConvertImage(rc, image, &pixmap);
    RDestroyImage(image);

    return pixmap;
}


static Pixmap
renderMenu(_Panel *panel, proplist_t texture, int width, int iheight)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    Display *dpy = WMScreenDisplay(scr);
    Pixmap pix, tmp;
    char *str;
    GC gc = XCreateGC(dpy, WMWidgetXID(panel->win), 0, NULL);

    
    str = GetStringForKey("MenuStyle");
    if (!str || strcasecmp(str, "normal")==0) {
	int i;

	tmp = renderTexture(scr, texture, width, iheight, NULL, RBEV_RAISED2);

	pix = XCreatePixmap(dpy, tmp, width, iheight*3,
			    WMScreenDepth(scr));
	for (i = 0; i < 3; i++) {
	    XCopyArea(dpy, tmp, pix, gc, 0, 0, width, iheight,
		      0, iheight*i);
	}
	XFreePixmap(dpy, tmp);
    } else if (strcasecmp(str, "flat")==0) {
	pix = renderTexture(scr, texture, width, iheight*3, NULL, RBEV_RAISED2);
    } else {
	pix = renderTexture(scr, texture, width, iheight*3, NULL, MENU_BEVEL);
    }

    XFreeGC(dpy, gc);

    return pix;
}


static void
updatePreviewBox(_Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    Display *dpy = WMScreenDisplay(scr);
 /*   RContext *rc = WMScreenRContext(scr);*/
    int refresh = 0;
    Pixmap pix;
    GC gc;
    WMListItem *item;
    TextureListItem *titem;

    gc = XCreateGC(dpy, WMWidgetXID(panel->win), 0, NULL);


    if (!panel->preview) {
	WMColor *color;

	panel->preview = XCreatePixmap(dpy, WMWidgetXID(panel->win),
				       260-4, 165-4, WMScreenDepth(scr));

	color = WMGrayColor(scr);
	XFillRectangle(dpy, panel->preview, WMColorGC(color),
		       0, 0, 260-4, 165-4);
	WMReleaseColor(color);

	refresh = -1;
    }

    if (elements & FTITLE) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[0]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 210, 20, NULL, RBEV_RAISED2);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 210, 20, 30, 5);

	XFreePixmap(dpy, pix);
    }
    if (elements & UTITLE) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[1]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 210, 20, NULL, RBEV_RAISED2);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 210, 20, 30, 30);

	XFreePixmap(dpy, pix);
    }
    if (elements & OTITLE) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[2]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 210, 20, NULL, RBEV_RAISED2);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 210, 20, 30, 55);

	XFreePixmap(dpy, pix);
    }
    if (elements & RESIZEBAR) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[3]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 210, 9, NULL, RESIZEBAR_BEVEL);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 210, 20, 30, 80);

	XFreePixmap(dpy, pix);
    }
    if (elements & MTITLE) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[4]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 100, 20, NULL, RBEV_RAISED2);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 100, 20, 30, 95);

	XFreePixmap(dpy, pix);
    }
    if (elements & MITEM) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[5]);
	titem = (TextureListItem*)item->clientData;

	pix = renderMenu(panel, titem->prop, 100, 18);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 100, 18*3, 30, 115);

	XFreePixmap(dpy, pix);
    }
    if (elements & (MITEM|MTITLE)) {
	XDrawLine(dpy, panel->preview, gc, 29, 95, 29, 115+36+20);
	XDrawLine(dpy, panel->preview, gc, 29, 94, 129, 94);
    }

    if (elements & ICON) {
	item = WMGetListItem(panel->texLs, panel->textureIndex[6]);
	titem = (TextureListItem*)item->clientData;

	pix = renderTexture(scr, titem->prop, 64, 64, NULL, 
			    titem->ispixmap ? 0 : RBEV_RAISED3);

	XCopyArea(dpy, pix, panel->preview, gc, 0, 0, 64, 64, 170, 95);

	XFreePixmap(dpy, pix);
    }


    if (refresh < 0) {
	WMPixmap *p;
	p = WMCreatePixmapFromXPixmaps(scr, panel->preview, None,
				       260-4, 165-4, WMScreenDepth(scr));

	WMSetLabelImage(panel->prevL, p);
	WMReleasePixmap(p);
    } else {
	WMRedisplayWidget(panel->prevL);
    }

    XFreeGC(dpy, gc);
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

	free(fname);
	sprintf(buf, "%08lx.cache", time(NULL));
	fname = wstrappend(prefix, buf);
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
    WMScreen *scr = WMWidgetScreen(panel->win);

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

	free(titem->title);
	titem->title = name;
    }

    prop = GetTexturePanelTexture(panel->texturePanel);

    str = PLGetDescription(prop);

    PLRelease(titem->prop);
    titem->prop = prop;

    titem->ispixmap = isPixmap(prop);

    free(titem->texture);
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

    free(titem->title);
    free(titem->texture);
    PLRelease(titem->prop);
    if (titem->path) {
	if (remove(titem->path) < 0 && errno != ENOENT) {
	    wsyserror("could not remove file %s", titem->path);
	}
	free(titem->path);
    }

    free(titem);

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

    if (WMRunModalFilePanelForDirectory(opanel, panel->win, wgethomedir(),
					_("Select File"), NULL)) {
	path = WMGetFilePanelFileName(opanel);

	OpenExtractPanelFor(panel, path);

	free(path);
    }
}


static void
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;
    WMListItem *item;
    TextureListItem *titem;
    char *str;
    WMScreen *scr = WMWidgetScreen(w);
    RContext *rc = WMScreenRContext(scr);
    static WMPoint positions[] = {
	{5, 5},
	{5, 30},
	{5, 55},
	{5, 80},
	{5, 95},
	{5, 130},
	{145, 110}
    };

    section = WMGetPopUpButtonSelectedItem(panel->secP);

    WMSelectListItem(panel->texLs, panel->textureIndex[section]);

    WMSetListPosition(panel->texLs, panel->textureIndex[section] - 2);

    item = WMGetListItem(panel->texLs, panel->textureIndex[section]);

    titem = (TextureListItem*)item->clientData;

    str = wmalloc(strlen(titem->title) + strlen(titem->texture) + 4);
    sprintf(str, "%s: %s", titem->title, titem->texture);
    WMSetLabelText(panel->texsL, str);
    free(str);

    {
	WMColor *color;
	
	color = WMGrayColor(scr);
	XFillRectangle(rc->dpy, panel->preview, WMColorGC(color),
		       positions[panel->oldsection].x, 
		       positions[panel->oldsection].y, 22, 22);
	WMReleaseColor(color);
    }
    panel->oldsection = section;
    WMDrawPixmap(panel->hand, panel->preview, positions[section].x, 
		 positions[section].y);

    WMRedisplayWidget(panel->prevL);
}



static void
previewClick(XEvent *event, void *clientData)
{
    _Panel *panel = (_Panel*)clientData;
    int i;
    static WMRect parts[] = {
	{{30, 5},{210, 20}},
	{{30,30},{210,20}},
	{{30,55},{210,20}},
	{{30,80},{210,9}},
	{{30,95},{100,20}},
	{{30,115},{100,60}},
	{{170,90},{64,64}}
    };

    for (i = 0; i < 7; i++) {
	if (event->xbutton.x >= parts[i].pos.x
	    && event->xbutton.y >= parts[i].pos.y
	    && event->xbutton.x < parts[i].pos.x + parts[i].size.width
	    && event->xbutton.y < parts[i].pos.y + parts[i].size.height) {
	    
	    WMSetPopUpButtonSelectedItem(panel->secP, i);
	    changePage(panel->secP, panel);
	    return;
	}
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
    char *str;

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

    str = wmalloc(strlen(titem->title) + strlen(titem->texture) + 4);
    sprintf(str, "%s: %s", titem->title, titem->texture);
    WMSetLabelText(panel->texsL, str);
    free(str);

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

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
        XFillRectangle(dpy, d, WMColorGC(white), x, y, width, height);
    else
        XClearArea(dpy, d, x, y, width, height, False);

    item = WMGetListItem(lPtr, index);
    titem = (TextureListItem*)item->clientData;

    if (titem->preview)
    XCopyArea(dpy, titem->preview, d, WMColorGC(black), 0, 0, TEXPREV_WIDTH,
	      TEXPREV_HEIGHT, x + 5, y + 5);

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
    int i;
    Pixmap pixmap;

    f = fopen(path, "r");
    if (!f)
	return None;

    fscanf(f, "%02x%02x%1x", &w, &h, &d);

    image = RCreateImage(w, h, d == 4);
    for (i = 0; i < d; i++) {
	fread(image->data[i], 1, w*h, f);
    }
    fclose(f);

    RConvertImage(WMScreenRContext(scr), image, &pixmap);
    RDestroyImage(image);

    return pixmap;
}



static void
fillTextureList(WMList *lPtr)
{
    proplist_t textureList;
    proplist_t texture;
    WMUserDefaults *udb = WMGetStandardUserDefaults();
    int i;
    TextureListItem *titem;
    WMScreen *scr  = WMWidgetScreen(lPtr);

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
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMColor *color;
    WMFont *font;
    WMScreen *scr = WMWidgetScreen(panel->win);

    char *tmp;
    Bool ok = True;
    
    panel->fprefix = wstrappend(wusergnusteppath(), "/.AppInfo");

    if (access(panel->fprefix, F_OK)!=0) {
	if (mkdir(panel->fprefix, 0755) < 0) {
	    wsyserror(panel->fprefix);
	    ok = False;
	}
    }
    if (ok) {
	tmp = wstrappend(panel->fprefix, "/WPrefs/");
	free(panel->fprefix);
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

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    /* preview box */
    panel->prevL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->prevL, 260, 165);
    WMMoveWidget(panel->prevL, 15, 10);
    WMSetLabelRelief(panel->prevL, WRSunken);
    WMSetLabelImagePosition(panel->prevL, WIPImageOnly);
    
    WMCreateEventHandler(WMWidgetView(panel->prevL), ButtonPressMask,
			 previewClick, panel);
    

    panel->secP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->secP, 260, 20);
    WMMoveWidget(panel->secP, 15, 180);
    WMSetPopUpButtonSelectedItem(panel->secP, 0);
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Unfocused Windows"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Focused Window's Owner"));
    WMAddPopUpButtonItem(panel->secP, _("Window Resizebar"));
    WMAddPopUpButtonItem(panel->secP, _("Titlebar of Menus"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Items"));
    WMAddPopUpButtonItem(panel->secP, _("Icon Background"));
/*    WMAddPopUpButtonItem(panel->secP, _("Workspace Backgrounds"));
 */
    WMSetPopUpButtonAction(panel->secP, changePage, panel);

    
    panel->texsL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->texsL, 260, 20);
    WMMoveWidget(panel->texsL, 15, 205);
    WMSetLabelWraps(panel->texsL, False);

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


    panel->newB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->newB, 56, 48);
    WMMoveWidget(panel->newB, 285, 180);
    WMSetButtonFont(panel->newB, font);
    WMSetButtonImagePosition(panel->newB, WIPAbove);
    WMSetButtonText(panel->newB, _("New"));
    WMSetButtonAction(panel->newB, newTexture, panel);
    SetButtonAlphaImage(scr, panel->newB, TNEW_FILE);

    WMSetBalloonTextForView(_("Create a new texture."),
			    WMWidgetView(panel->newB));

    panel->ripB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->ripB, 56, 48);
    WMMoveWidget(panel->ripB, 341, 180);
    WMSetButtonFont(panel->ripB, font);
    WMSetButtonImagePosition(panel->ripB, WIPAbove);
    WMSetButtonText(panel->ripB, _("Extract..."));
    WMSetButtonAction(panel->ripB, extractTexture, panel);
    SetButtonAlphaImage(scr, panel->ripB, TEXTR_FILE);

    WMSetBalloonTextForView(_("Extract texture(s) from a theme or a style file."),
			    WMWidgetView(panel->ripB));

    WMSetButtonEnabled(panel->ripB, False);

    panel->editB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->editB, 56, 48);
    WMMoveWidget(panel->editB, 397, 180);
    WMSetButtonFont(panel->editB, font);
    WMSetButtonImagePosition(panel->editB, WIPAbove);
    WMSetButtonText(panel->editB, _("Edit"));
    SetButtonAlphaImage(scr, panel->editB, TEDIT_FILE);
    WMSetButtonAction(panel->editB, editTexture, panel);
    WMSetBalloonTextForView(_("Edit the highlighted texture."),
			    WMWidgetView(panel->editB));

    panel->delB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->delB, 56, 48);
    WMMoveWidget(panel->delB, 453, 180);
    WMSetButtonFont(panel->delB, font);
    WMSetButtonImagePosition(panel->delB, WIPAbove);
    WMSetButtonText(panel->delB, _("Delete"));
    SetButtonAlphaImage(scr, panel->delB, TDEL_FILE);
    WMSetButtonEnabled(panel->delB, False);
    WMSetButtonAction(panel->delB, deleteTexture, panel);
    WMSetBalloonTextForView(_("Delete the highlighted texture."),
			    WMWidgetView(panel->delB));

    WMReleaseFont(font);

    /**/

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    WMSetPopUpButtonSelectedItem(panel->secP, 0);

    showData(panel);

    changePage(panel->secP, panel);

    fillTextureList(panel->texLs);

    panel->texturePanel = CreateTexturePanel(panel->win);
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
    if (!titem->prop) {
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
    int i = 0;

    setupTextureFor(panel->texLs, "FTitleBack", "(solid, black)", 
		    "[Focused]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "UTitleBack", "(solid, gray)", 
		    "[Unfocused]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "PTitleBack", "(solid, \"#616161\")",
		    "[Owner of Focused]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "ResizebarBack", "(solid, gray)",
		    "[Resizebar]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "MenuTitleBack", "(solid, black)", 
		    "[Menu Title]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "MenuTextBack", "(solid, gray)", 
		    "[Menu Item]", i);
    panel->textureIndex[i] = i++;

    setupTextureFor(panel->texLs, "IconBack", "(solid, gray)", "[Icon]", i);
    panel->textureIndex[i] = i++;
/*
    setupTextureFor(panel->texLs, "WorkspaceBack", "(solid, black)", 
		    "[Workspace]", i);
    panel->textureIndex[i] = i++;
 */

    updatePreviewBox(panel, EVERYTHING);
}



static void
storeData(_Panel *panel)
{
    TextureListItem *titem;
    WMListItem *item;

    item = WMGetListItem(panel->texLs, panel->textureIndex[0]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "FTitleBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[1]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "UTitleBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[2]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "PTitleBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[3]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "ResizebarBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[4]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "MenuTitleBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[5]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "MenuTextBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[6]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "IconBack");
}


static void
prepareForClose(_Panel *panel)
{
    proplist_t textureList;
    proplist_t texture;
    int i;
    TextureListItem *titem;
    WMListItem *item;
    WMUserDefaults *udb = WMGetStandardUserDefaults();

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

    panel->win = win;

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
    WMScreen *scr = WMWidgetScreen(panel->win);

    epanel = wmalloc(sizeof(ExtractPanel));
    epanel->win = WMCreatePanelWithStyleForWindow(panel->win, "extract",
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

