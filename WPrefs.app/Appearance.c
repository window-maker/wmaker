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

    int textureIndex[8];

    WMFont *smallFont;
    WMFont *normalFont;
    WMFont *boldFont;

    TexturePanel *texturePanel;

    WMPixmap *onLed;
    WMPixmap *offLed;

    /* for preview shit */
    Pixmap preview;
    Pixmap ftitle;
    Pixmap utitle;
    Pixmap otitle;
    Pixmap icon;
    Pixmap back;
    Pixmap mtitle;
    Pixmap mitem;

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
} TextureListItem;


static void showData(_Panel *panel);


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



#define	FTITLE	(1<<0)
#define UTITLE	(1<<1)
#define OTITLE	(1<<2)
#define MTITLE	(1<<3)
#define MITEM	(1<<4)
#define ICON	(1<<5)
#define BACK	(1<<6)
#define EVERYTHING	0xff




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



static Pixmap
renderTexture(WMScreen *scr, proplist_t texture, int width, int height,
	      char *path, Bool bordered)
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
	 case 'D':
	    style = RDiagonalGradient;
	    break;
	}

	str = PLGetString(PLGetArrayElement(texture, 1));
	str2rcolor(rc, str, &rcolor);
	str = PLGetString(PLGetArrayElement(texture, 2));
	str2rcolor(rc, str, &rcolor2);
	
	image = RRenderGradient(width, height, &rcolor, &rcolor2, style);
    } else if (strcasecmp(&type[2], "gradient")==0) {
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
	int style;
	RImage *timage;
	int w, h;
	char *path;

	str = PLGetString(PLGetArrayElement(texture, 1));

	path = wfindfileinarray(GetObjectForKey("PixmapPath"), str);
	timage = RLoadImage(rc, path, 0);
	free(path);
	if (timage) {
	    w = timage->width;
	    h = timage->height;
	
	    if (w - TEXPREV_WIDTH > h - TEXPREV_HEIGHT) {
		h = (w * TEXPREV_HEIGHT)/TEXPREV_WIDTH;
	    } else {
		w = (h * TEXPREV_WIDTH)/TEXPREV_HEIGHT;
	    }

	    image = RScaleImage(timage, w, h);
	    RDestroyImage(timage);
	}
    }

    if (path) {
	dumpRImage(path, image);
    }

    RConvertImage(rc, image, &pixmap);
    RDestroyImage(image);

    return pixmap;
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

	panel->ftitle = renderTexture(scr, tmp, 180, 20, NULL, True);
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
cancelNewTexture(void *data)
{
    _Panel *panel = (_Panel*)data;
    
    HideTexturePanel(panel->texturePanel);
}


static char*
makeFileName(char *prefix, char *name)
{
    char *fname, *str;
    int i;

    str = wstrappend(prefix, name);
    fname = wstrdup(str);

    i = 1;
    while (access(fname, F_OK)==0) {
	char buf[16];

	free(fname);
	sprintf(buf, "%i", i++);
	fname = wstrappend(str, buf);
    }
    free(str);

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

    titem->path = makeFileName(panel->fprefix, name);
    titem->preview = renderTexture(scr, prop, TEXPREV_WIDTH, TEXPREV_HEIGHT,
				   titem->path, False);

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

    name = GetTexturePanelTextureName(panel->texturePanel);

    prop = GetTexturePanelTexture(panel->texturePanel);

    str = PLGetDescription(prop);

    free(titem->title);
    titem->title = name;

    PLRelease(titem->prop);
    titem->prop = prop;

    free(titem->texture);
    titem->texture = str;

    XFreePixmap(WMScreenDisplay(WMWidgetScreen(panel->texLs)), titem->preview);
    titem->preview = renderTexture(WMWidgetScreen(panel->texLs), titem->prop,
				   TEXPREV_WIDTH, TEXPREV_HEIGHT, 
				   titem->path, False);

    WMRedisplayWidget(panel->texLs);
}



static void
editTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *item;
    TextureListItem *titem;

    item = WMGetListItem(panel->texLs, WMGetListSelectedItemRow(panel->texLs));
    titem = (TextureListItem*)item->clientData;

    SetTexturePanelTexture(panel->texturePanel, titem->title, titem->prop);

    SetTexturePanelCancelAction(panel->texturePanel, cancelNewTexture, panel);
    SetTexturePanelOkAction(panel->texturePanel, okEditTexture, panel);

    SetTexturePanelPixmapPath(panel->texturePanel,
			      GetObjectForKey("PixmapPath"));
    ShowTexturePanel(panel->texturePanel);
}



static void
newTexture(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    SetTexturePanelTexture(panel->texturePanel, "New Texture", NULL);

    SetTexturePanelCancelAction(panel->texturePanel, cancelNewTexture, panel);

    SetTexturePanelOkAction(panel->texturePanel, okNewTexture, panel);

    SetTexturePanelPixmapPath(panel->texturePanel,
			      GetObjectForKey("PixmapPath"));
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
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->secP);

    WMSelectListItem(panel->texLs, panel->textureIndex[section]);

    WMSetListPosition(panel->texLs, panel->textureIndex[section] 
		      - WMGetListNumberOfRows(panel->texLs)/2);
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

	puts(titem->path);
	titem->preview = loadRImage(scr, titem->path);
	if (!titem->preview) {
	    titem->preview = renderTexture(scr, titem->prop, TEXPREV_WIDTH, 
					   TEXPREV_HEIGHT, NULL, False);
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
/*    WMAddPopUpButtonItem(panel->secP, _("Workspace Backgrounds"));
 */
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
    WMSetListUserDrawItemHeight(panel->texLs, 35);
    WMSetListUserDrawProc(panel->texLs, paintListItem);
    WMHangData(panel->texLs, panel);
    WMSetListAction(panel->texLs, textureClick, panel);
    WMSetListDoubleAction(panel->texLs, textureDoubleClick, panel);

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

    panel->ripB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->ripB, 56, 48);
    WMMoveWidget(panel->ripB, 341, 180);
    WMSetButtonFont(panel->ripB, font);
    WMSetButtonImagePosition(panel->ripB, WIPAbove);
    WMSetButtonText(panel->ripB, _("Extract..."));
    SetButtonAlphaImage(scr, panel->ripB, TEXTR_FILE);

    WMSetButtonEnabled(panel->ripB, False);

    panel->editB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->editB, 56, 48);
    WMMoveWidget(panel->editB, 397, 180);
    WMSetButtonFont(panel->editB, font);
    WMSetButtonImagePosition(panel->editB, WIPAbove);
    WMSetButtonText(panel->editB, _("Edit"));
    SetButtonAlphaImage(scr, panel->editB, TEDIT_FILE);
    WMSetButtonAction(panel->editB, editTexture, panel);

    panel->delB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->delB, 56, 48);
    WMMoveWidget(panel->delB, 453, 180);
    WMSetButtonFont(panel->delB, font);
    WMSetButtonImagePosition(panel->delB, WIPAbove);
    WMSetButtonText(panel->delB, _("Delete"));
    SetButtonAlphaImage(scr, panel->delB, TDEL_FILE);
    WMSetButtonEnabled(panel->delB, False);
    WMSetButtonAction(panel->delB, deleteTexture, panel);

    WMReleaseFont(font);

    /**/

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    WMSetPopUpButtonSelectedItem(panel->secP, 0);

    showData(panel);

    fillTextureList(panel->texLs);

    panel->texturePanel = CreateTexturePanel(panel->win);
}



static proplist_t
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

    titem->preview = renderTexture(WMWidgetScreen(list), titem->prop,
				   TEXPREV_WIDTH, TEXPREV_HEIGHT, NULL, False);

    item = WMAddListItem(list, "");
    item->clientData = titem;

    return titem->prop;
}



static void
showData(_Panel *panel)
{
    int i = 0;

    panel->ftitleTex = setupTextureFor(panel->texLs, "FTitleBack", 
				       "(solid, black)", "[Focused]", i);
    panel->textureIndex[i] = i++;

    panel->utitleTex = setupTextureFor(panel->texLs, "UTitleBack",
				       "(solid, gray)", "[Unfocused]", i);
    panel->textureIndex[i] = i++;

    panel->ptitleTex = setupTextureFor(panel->texLs, "PTitleBack",
				       "(solid, \"#616161\")",
				       "[Owner of Focused]", i);
    panel->textureIndex[i] = i++;

    panel->mtitleTex = setupTextureFor(panel->texLs, "MenuTitleBack", 
				       "(solid, black)", "[Menu Title]", i);
    panel->textureIndex[i] = i++;

    panel->menuTex = setupTextureFor(panel->texLs, "MenuTextBack", 
				     "(solid, gray)", "[Menu Item]", i);
    panel->textureIndex[i] = i++;

    panel->iconTex = setupTextureFor(panel->texLs, "IconBack",
				     "(solid, gray)", "[Icon]", i);
    panel->textureIndex[i] = i++;
/*
    panel->backTex = setupTextureFor(panel->texLs, "WorkspaceBack",
				     "(solid, black)", "[Workspace]", i);
    panel->textureIndex[i] = i++;
 */
}



static void
storeData(_Panel *panel)
{
    proplist_t textureList;
    proplist_t texture;
    int i;
    TextureListItem *titem;
    WMListItem *item;
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    textureList = PLMakeArrayFromElements(NULL, NULL);

    /* store list of textures */
    for (i = 6; i < WMGetListNumberOfRows(panel->texLs); i++) {
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
    SetObjectForKey(titem->prop, "MenuTitleBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[4]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "MenuTextBack");

    item = WMGetListItem(panel->texLs, panel->textureIndex[5]);
    titem = (TextureListItem*)item->clientData;
    SetObjectForKey(titem->prop, "IconBack");

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
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
