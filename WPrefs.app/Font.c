/* Font.c- text/font settings
 *
 *  WPrefs - Window Maker Preferences Program
 *
 *  Copyright (c) 1999-2003 Alfredo K. Kojima
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
#include <X11/Xlocale.h>
#include <ctype.h>

typedef struct _Panel {
    WMBox *box;
    char *sectionName;

    char *description;

    CallbackRec callbacks;

    WMWidget *parent;


    WMLabel *prevL;

    WMFontPanel *fontPanel;

    WMPopUpButton *fontSel;
    WMFrame *multiF;
    WMButton *togMulti;
    WMFrame *langF;
    WMPopUpButton *langP;
    WMFrame *aaF;
    WMButton *togAA;

    /* single byte */
    WMTextField *fontT;
    WMButton *changeB;

    /* multibyte */
    WMLabel *fsetL;
    WMList *fsetLs;

    WMButton *addB;
    WMButton *editB;
    WMButton *remB;

    WMButton *upB;
    WMButton *downB;

    WMColor *white;
    WMColor *black;
    WMColor *light;
    WMColor *dark;

    WMColor *back;
    WMColor *colors[9];

    Pixmap preview;
    WMPixmap *previewPix;
    WMPixmap *hand;
    WMPixmap *up_arrow;
    WMPixmap *down_arrow;
    WMPixmap *alt_up_arrow;
    WMPixmap *alt_down_arrow;

    int oldsection;
    char menuStyle;
    char titleAlignment;
    Bool MultiByteText;

    Bool AntialiasedText;
} _Panel;



#define ICON_FILE	"fonts"

static WMRect previewPositions[] = {
#define WINTITLE	0
    {{30, 10},{190, 20}},
#define DISTITLE	1
    {{30, 35},{90, 64}},
#define PMTITLE		2
    {{30, 105},{90, 20}},
#define PMITEM		3
    {{30, 125},{90, 20*4}},
#define PCLIP		4
    {{156, 35},{64, 64}},
#define PICON		5
    {{156, 105},{64, 64}}
};
#define EVERYTHING 0xff

static char *colorOptions[] = {
#define FTITLE	(1<<0)
    "FTitleColor", "white",
#define DISCOL	(1<<1)			/* Display uses white always */
#define MTITLE	(1<<2)
    "MenuTitleColor", "white",
#define MITEM	(1<<3)
    "MenuTextColor", "black",
#define CLIP	(1<<4)
    "ClipTitleColor", "black",
#define CCLIP	(1<<4)
    "ClipTitleColor", "#454045",
#define ICONT	(1<<5)
    "IconTitleColor", "white",
#define ICONB	(1<<5)
    "IconTitleBack", "black"
};

#define MSTYLE_NORMAL	0
#define MSTYLE_SINGLE	1
#define MSTYLE_FLAT	2


#define RESIZEBAR_BEVEL	-1
#define MENU_BEVEL	-2
#define CLIP_BUTTON_SIZE	23
#define ICON_SIZE	64


static char *textureOptions[] = {
    "FTitleBack",
    NULL,
    "MenuTitleBack",
    "MenuTextBack",
    "IconBack",
    "IconBack"
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

static char *up_arrow_xpm[] = {
    "9 9 3 1",
    ".		c #acaaac",
    "%		c #525552",
    "#		c #000000",
    "....%....",
    "....#....",
    "...%#%...",
    "...###...",
    "..%###%..",
    "..#####..",
    ".%#####%.",
    ".#######.",
    "%#######%"
};

static char *down_arrow_xpm[] = {
    "9 9 3 1",
    ".		c #acaaac",
    "%		c #525552",
    "#		c #000000",
    "%#######%",
    ".#######.",
    ".%#####%.",
    "..#####..",
    "..%###%..",
    "...###...",
    "...%#%...",
    "....#....",
    "....%...."
};

static char *alt_up_arrow_xpm[] = {
    "9 9 2 1",
    ".		c #ffffff",
    "%		c #525552",
    "....%....",
    "....%....",
    "...%%%...",
    "...%%%...",
    "..%%%%%..",
    "..%%%%%..",
    ".%%%%%%%.",
    ".%%%%%%%.",
    "%%%%%%%%%"
};

static char *alt_down_arrow_xpm[] = {
    "9 9 2 1",
    ".		c #ffffff",
    "%		c #525552",
    "%%%%%%%%%",
    ".%%%%%%%.",
    ".%%%%%%%.",
    "..%%%%%..",
    "..%%%%%..",
    "...%%%...",
    "...%%%...",
    "....%....",
    "....%...."
};
/* XPM */

static WMPropList *CurrentFontArray = NULL;
//static WMPropList *CurrentMenuTitleFont = NULL;
//static WMPropList *CurrentMenuTextFont = NULL;
//static WMPropList *CurrentIconTitleFont = NULL;
//static WMPropList *CurrentClipTitleFont = NULL;
//static WMPropList *CurrentLargeDisplayFont = NULL;

static WMPropList *DefaultWindowTitleFont = NULL;
static WMPropList *DefaultMenuTitleFont = NULL;
static WMPropList *DefaultMenuTextFont = NULL;
static WMPropList *DefaultIconTitleFont = NULL;
static WMPropList *DefaultClipTitleFont = NULL;
static WMPropList *DefaultLargeDisplayFont = NULL;

static void changePage(WMWidget *w, void *data);
static void setLanguageType(void *data, Bool multiByte);
static void refillFontSetList(void *data);
static void readFontEncodings(void *data);
static void changeLanguageAction(WMWidget *w, void *data);
static void checkListForArrows(void *data);

static char* getFontEncoding(void *data);
static char* getFontSampleString(void *data);

/* note single element */
static WMFont* getFontForPreview(void *data, int element);
static WMFont* getDefaultSystemFont(void *data, int element);

static WMPropList* getDefaultFontProp(void *data, char *encoding, int section);
static WMPropList* getCurrentFontProp(void *data, int section);

static Bool isEncodingMultiByte(void *data);

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

static void
paintTitle(WMScreen *scr, Drawable d, WMColor *color, WMFont *font,
           int part, WMAlignment align, char *text)
{
    int l = strlen(text);
    int x = previewPositions[part].pos.x;
    int y = previewPositions[part].pos.y;
    int w = previewPositions[part].size.width;
    int h = previewPositions[part].size.height;

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
    WMDrawString(scr, d, color, font, x,
                 y + (h - WMFontHeight(font))/2, text, l);
}

static void
paintItems(WMScreen *scr, Drawable d, WMColor *color, WMFont *font,
           int part, char *text)
{
    int l = strlen(text);
    int x = previewPositions[part].pos.x;
    int y = previewPositions[part].pos.y;
    //int w = previewPositions[part].size.width;
    int h = previewPositions[part].size.height/4;
    int i;
    for( i = 0; i < 4 ; i++) {
        WMDrawString(scr, d, color, font, x+5,
                     y+(20*i)+(h - WMFontHeight(font))/2, text, l);
    }
}

static void
paintIcon(WMScreen *scr,Drawable d, WMColor *color, WMColor *Iback,
          WMFont *font, int part, char *text)
{
    Display *dpy = WMScreenDisplay(scr);
    int l = strlen(text);
    int x = previewPositions[part].pos.x+1;
    int y = previewPositions[part].pos.y+1;
    int w = previewPositions[part].size.width-2;
    int h = WMFontHeight(font)+2;

    XFillRectangle(dpy, d, WMColorGC(Iback), x, y, w, h);
    x += (w - WMWidthOfString(font, text, l))/2;
    WMDrawString(scr, d, color, font, x,
                 y + (h - WMFontHeight(font))/2, text, l);

}

static void
drawFonts(_Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->box);
    WMPixmap *pixmap;
    Pixmap d;

    pixmap = WMGetLabelImage(panel->prevL);
    d = WMGetPixmapXID(pixmap);

    if(elements & FTITLE) {
        paintTitle(scr, d, panel->colors[0], getFontForPreview(panel, WINTITLE),
                   WINTITLE, panel->titleAlignment, _("Window Title Font"));
    }
    if(elements & DISCOL) {
        paintTitle(scr, d, panel->white, getFontForPreview(panel, DISTITLE),
                   DISTITLE, WACenter, _("Display"));
    }
    if(elements & MTITLE) {
        paintTitle(scr, d, panel->colors[1], getFontForPreview(panel, PMTITLE),
                   PMTITLE, WALeft, _("Menu Title"));
    }
    if(elements & MITEM) {
        paintItems(scr, d, panel->colors[2], getFontForPreview(panel, PMITEM),
                   PMITEM, _("Menu Item"));
    }
    if(elements & CLIP) {
        WMDrawString(scr, d, panel->colors[4],
                     getFontForPreview(panel, PCLIP), 169,37, "1",1);
        WMDrawString(scr, d, panel->colors[3],
                     getFontForPreview(panel, PCLIP),179, 84, _("Clip title"), 10);
    }
    if(elements & ICONT) {
        paintIcon(scr, d, panel->colors[5], panel->colors[6],
                  getFontForPreview(panel, PICON), PICON, _("Icon Title"));
    }
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

/*static int
 isPixmap(WMPropList *prop)
 {
 WMPropList *p;
 char *s;

 p = WMGetFromPLArray(prop, 0);
 s = WMGetFromPLString(p);
 if (strcasecmp(&s[1], "pixmap")==0)
 return 1;
 else
 return 0;
 }*/

static Pixmap
renderTexture(WMScreen *scr, WMPropList *texture, int width, int height,
              char *path, int border)
{
    char *type;
    RImage *image = NULL;
    Pixmap pixmap;
    RContext *rc = WMScreenRContext(scr);
    char *str;
    RColor rcolor;


    type = WMGetFromPLString(WMGetFromPLArray(texture, 0));

    if (strcasecmp(type, "solid")==0) {

        str = WMGetFromPLString(WMGetFromPLArray(texture, 1));

        str2rcolor(rc, str, &rcolor);

        image = RCreateImage(width, height, False);
        RClearImage(image, &rcolor);
    } else if (strcasecmp(type, "igradient")==0) {
        int t1, t2;
        RColor c1[2], c2[2];

        str = WMGetFromPLString(WMGetFromPLArray(texture, 1));
        str2rcolor(rc, str, &c1[0]);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 2));
        str2rcolor(rc, str, &c1[1]);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 3));
        t1 = atoi(str);

        str = WMGetFromPLString(WMGetFromPLArray(texture, 4));
        str2rcolor(rc, str, &c2[0]);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 5));
        str2rcolor(rc, str, &c2[1]);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 6));
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

        str = WMGetFromPLString(WMGetFromPLArray(texture, 1));
        str2rcolor(rc, str, &rcolor);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 2));
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

        str = WMGetFromPLString(WMGetFromPLArray(texture, 3));
        str2rcolor(rc, str, &rcolor);
        str = WMGetFromPLString(WMGetFromPLArray(texture, 4));
        str2rcolor(rc, str, &rcolor2);

        str = WMGetFromPLString(WMGetFromPLArray(texture, 1));

        if ((path=wfindfileinarray(GetObjectForKey("PixmapPath"), str))!=NULL)
            timage = RLoadImage(rc, path, 0);

        if (!path || !timage) {
            wwarning("could not load file '%s': %s", path,
                     RMessageForError(RErrorCode));
        } else {
            grad = RRenderGradient(width, height, &rcolor, &rcolor2, style);

            image = RMakeTiledImage(timage, width, height);
            RReleaseImage(timage);

            i = atoi(WMGetFromPLString(WMGetFromPLArray(texture, 2)));

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

        j = WMGetPropListItemCount(texture);

        if (j > 0) {
            colors = wmalloc(j * sizeof(RColor*));

            for (i = 2; i < j; i++) {
                str = WMGetFromPLString(WMGetFromPLArray(texture, i));
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

        str = WMGetFromPLString(WMGetFromPLArray(texture, 1));

        if ((path=wfindfileinarray(GetObjectForKey("PixmapPath"), str))!=NULL)
            timage = RLoadImage(rc, path, 0);

        if (!path || !timage) {
            wwarning("could not load file '%s': %s", path ? path : str,
                     RMessageForError(RErrorCode));
        } else {
            str = WMGetFromPLString(WMGetFromPLArray(texture, 2));
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
        if (border == MENU_BEVEL) {
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
renderMenu(_Panel *panel, WMPropList *texture, int width, int iheight)
{
    WMScreen *scr = WMWidgetScreen(panel->parent);
    Display *dpy = WMScreenDisplay(scr);
    Pixmap pix, tmp;
    //RContext *rc = WMScreenRContext(scr);
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
renderClip(_Panel *panel, GC gc, int part, int relief)
{
    WMScreen *scr = WMWidgetScreen(panel->box);
    Display *dpy = WMScreenDisplay(scr);
    RContext *rc = WMScreenRContext(scr);
    WMPropList *prop;
    Pixmap pix;
    XImage *original;
    XPoint p[4];
    RImage *tile;
    RColor black;
    RColor dark;
    RColor light;
    int pt, tp;
    int as;

    prop = GetObjectForKey(textureOptions[part]);

    pix = renderTexture(scr, prop,
                        previewPositions[part].size.width,
                        previewPositions[part].size.height,
                        NULL, relief);


    original = XGetImage(dpy, pix, 0, 0, 64, 64,
                         AllPlanes, ZPixmap);
    if (!original){
        wwarning(_("error capturing \"original\" tile image"),
                 RMessageForError(RErrorCode));
    }
    tile = RCreateImageFromXImage(rc, original, NULL);

    XDestroyImage(original);
    XFreePixmap(WMScreenDisplay(scr), pix);

    pt = CLIP_BUTTON_SIZE*ICON_SIZE/64;
    tp = ICON_SIZE-1 - pt;
    as = pt - 15;

    black.alpha = 255;
    black.red = black.green = black.blue = 0;

    dark.alpha = 0;
    dark.red = dark.green = dark.blue = 60;

    light.alpha = 0;
    light.red = light.green = light.blue = 80;


    /* top right */
    ROperateLine(tile, RSubtractOperation, tp, 0, ICON_SIZE-2,
                 pt-1, &dark);
    RDrawLine(tile, tp-1, 0, ICON_SIZE-1, pt+1, &black);
    ROperateLine(tile, RAddOperation, tp, 2, ICON_SIZE-3,
                 pt, &light);

    /* arrow bevel */
    ROperateLine(tile, RSubtractOperation, ICON_SIZE - 7 - as, 4,
                 ICON_SIZE - 5, 4, &dark);
    ROperateLine(tile, RSubtractOperation, ICON_SIZE - 6 - as, 5,
                 ICON_SIZE - 5, 6 + as, &dark);
    ROperateLine(tile, RAddOperation, ICON_SIZE - 5, 4, ICON_SIZE - 5, 6 + as,
                 &light);

    /* bottom left */
    ROperateLine(tile, RAddOperation, 2, tp+2, pt-2,
                 ICON_SIZE-3, &dark);
    RDrawLine(tile, 0, tp-1, pt+1, ICON_SIZE-1, &black);
    ROperateLine(tile, RSubtractOperation, 0, tp-2, pt+1,
                 ICON_SIZE-2, &light);

    /* arrow bevel */
    ROperateLine(tile, RSubtractOperation, 4, ICON_SIZE - 7 - as, 4,
                 ICON_SIZE - 5, &dark);
    ROperateLine(tile, RSubtractOperation, 5, ICON_SIZE - 6 - as,
                 6 + as, ICON_SIZE - 5, &dark);
    ROperateLine(tile, RAddOperation, 4, ICON_SIZE - 5, 6 + as, ICON_SIZE - 5,
                 &light);

    RConvertImage(rc, tile, &pix);

    /* top right arrow */
    p[0].x = p[3].x = ICON_SIZE-5-as;
    p[0].y = p[3].y = 5;
    p[1].x = ICON_SIZE-6;
    p[1].y = 5;
    p[2].x = ICON_SIZE-6;
    p[2].y = 4+as;
    XFillPolygon(dpy, pix, WMColorGC(panel->colors[4]), p, 3, Convex, CoordModeOrigin);
    XDrawLines(dpy, pix, WMColorGC(panel->colors[4]), p, 4, CoordModeOrigin);

    /* bottom left arrow */
    p[0].x = p[3].x = 5;
    p[0].y = p[3].y = ICON_SIZE-5-as;
    p[1].x = 5;
    p[1].y = ICON_SIZE-6;
    p[2].x = 4+as;
    p[2].y = ICON_SIZE-6;
    XFillPolygon(dpy, pix, WMColorGC(panel->colors[4]), p, 3, Convex, CoordModeOrigin);
    XDrawLines(dpy, pix, WMColorGC(panel->colors[4]), p, 4, CoordModeOrigin);

    XCopyArea(dpy, pix, panel->preview, gc, 0, 0,
              previewPositions[part].size.width,
              previewPositions[part].size.height,
              previewPositions[part].pos.x,
              previewPositions[part].pos.y);

    RReleaseImage(tile);
    XFreePixmap(WMScreenDisplay(scr), pix);
}

static void
renderPreview(_Panel *panel, GC gc, int part, int relief)
{
    WMPropList *prop;
    Pixmap pix;
    WMScreen *scr = WMWidgetScreen(panel->box);

    prop = GetObjectForKey(textureOptions[part]);

    pix = renderTexture(scr, prop,
                        previewPositions[part].size.width,
                        previewPositions[part].size.height,
                        NULL, relief);
    XCopyArea(WMScreenDisplay(scr), pix,
              panel->preview, gc, 0, 0,
              previewPositions[part].size.width,
              previewPositions[part].size.height,
              previewPositions[part].pos.x,
              previewPositions[part].pos.y);

    XFreePixmap(WMScreenDisplay(scr), pix);
}

static void
paintPreviewBox(Panel *panel, int elements)
{
    WMScreen *scr = WMWidgetScreen(panel->parent);
    Display *dpy = WMScreenDisplay(scr);
    //int refresh = 0;
    GC gc;
    WMColor *black = WMBlackColor(scr);
    Pixmap mitem;

    gc = XCreateGC(dpy, WMWidgetXID(panel->parent), 0, NULL);

    if (panel->preview == None) {
        WMPixmap *pix;

        panel->preview = XCreatePixmap(dpy, WMWidgetXID(panel->parent),
                                       240-4, 190-4, WMScreenDepth(scr));

        pix = WMCreatePixmapFromXPixmaps(scr, panel->preview, None,
                                         240-4, 190-4, WMScreenDepth(scr));

        WMSetLabelImage(panel->prevL, pix);
        WMReleasePixmap(pix);
    }
    XFillRectangle(dpy, panel->preview, WMColorGC(panel->back),
                   0, 0, 240-4, 190-4);

    if (elements & (1<<WINTITLE)) {
        renderPreview(panel, gc, WINTITLE, RBEV_RAISED2);
        XDrawRectangle(dpy, panel->preview, WMColorGC(black),
                       previewPositions[WINTITLE].pos.x-1,
                       previewPositions[WINTITLE].pos.y-1,
                       previewPositions[WINTITLE].size.width,
                       previewPositions[WINTITLE].size.height);
    }
    if (elements & (1<<DISTITLE)) {
        XDrawRectangle(dpy, panel->preview, WMColorGC(panel->back),
                       previewPositions[DISTITLE].pos.x-1,
                       previewPositions[DISTITLE].pos.y-1,
                       previewPositions[DISTITLE].size.width,
                       previewPositions[DISTITLE].size.height);
    }
    if (elements & (1<<PMTITLE)) {
        renderPreview(panel, gc, PMTITLE, RBEV_RAISED2);
        XDrawRectangle(dpy, panel->preview, WMColorGC(black),
                       previewPositions[PMTITLE].pos.x-1,
                       previewPositions[PMTITLE].pos.y-1,
                       previewPositions[PMTITLE].size.width,
                       previewPositions[PMTITLE].size.height);
    }
    if (elements & (1<<PMITEM)) {
        WMPropList *prop;

        prop = GetObjectForKey(textureOptions[PMITEM]);
        mitem = renderMenu(panel, prop,
                           previewPositions[PMITEM].size.width,
                           previewPositions[PMITEM].size.height/4);

        XCopyArea(dpy, mitem, panel->preview, gc, 0, 0,
                  previewPositions[PMITEM].size.width,
                  previewPositions[PMITEM].size.height,
                  previewPositions[PMITEM].pos.x,
                  previewPositions[PMITEM].pos.y);

        XFreePixmap(dpy, mitem);
    }
    if (elements & (1<<PMITEM|1<<PMTITLE)) {
        XDrawLine(dpy, panel->preview, gc, 29, 125, 29, 125+20*4+25);
        XDrawLine(dpy, panel->preview, gc, 119, 125, 119, 125+20*4+25);
    }
    if (elements & (1<<PCLIP)) {
        renderClip(panel, gc, PCLIP, RBEV_RAISED3);
        XDrawRectangle(dpy, panel->preview, WMColorGC(black),
                       previewPositions[PCLIP].pos.x-1,
                       previewPositions[PCLIP].pos.y-1,
                       previewPositions[PCLIP].size.width,
                       previewPositions[PCLIP].size.height);
    }
    if (elements & (1<<PICON)) {
        renderPreview(panel, gc, PICON, RBEV_RAISED3);
        XDrawRectangle(dpy, panel->preview, WMColorGC(black),
                       previewPositions[PICON].pos.x-1,
                       previewPositions[PICON].pos.y-1,
                       previewPositions[PICON].size.width,
                       previewPositions[PICON].size.height);
    }
    drawFonts(panel, elements);
    WMRedisplayWidget(panel->prevL);
    XFreeGC(dpy, gc);
    WMReleaseColor(black);
}

static void
paintTextField(void *data, int section)
{
    _Panel *panel = (_Panel*)data;
    //char *sample = NULL;
    int encoding;
    encoding = WMGetPopUpButtonSelectedItem(panel->langP);
    WMSetTextFieldFont(panel->fontT, getFontForPreview(panel, section));
    switch(encoding) {
    case 0:  /* Current Font in theme */
        WMSetTextFieldText(panel->fontT,
                           "ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
    case 1:  /* default */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //	"ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
    case 2:  /* latin1 iso8859-1 */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //  "ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
    case 3:  /* latin2 iso8859-2 */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
    case 4:  /* Greek iso8859-7 */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
        /* luckily all these happen to have the MultiByte chars in the same places */
    case 5:  /* Japanese jisx0208.1983 */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"Window Maker ÀßÄê¥æ¡¼¥Æ¥£¥ê¥Æ¥£");
        break;
    case 6:  /* Korean ksc5601.1987 */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"À©µµ¿ì ¸ÞÀÌÄ¿ ¼³Á¤");
        break;
    case 7:  /* korean2 daewoo */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"À©µµ¿ì ¸ÞÀÌÄ¿ ¼³Á¤");
        break;
    case 8:  /* Russian koi8-r */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"ó×ÏÊÓÔ×Á Window Maker");
        break;
    case 9:  /* Ukranian koi8-u */
        WMSetTextFieldText(panel->fontT, getFontSampleString(panel));
        //		"ABCDEFGHIKLMNOPQRSTUVXYWZabcdefghiklmnopqrstuvxywz0123456789\x00e0\x00e6\x00e7\x00eb\x00ee\x00f0\x00f1\x00f3\x00f9\x00fd\x00c0\x00c6\x00c7\x00cb\x00ce\x00d0\x00d1\x00d3\x00d9\x00dd");
        break;
    }
}

static void
previewClick(XEvent *event, void *clientData)
{
    _Panel *panel = (_Panel*)clientData;
    int i;

    for (i = 0; i < sizeof(previewPositions)/sizeof(WMRect); i++) {
        if (event->xbutton.x >= previewPositions[i].pos.x
            && event->xbutton.y >= previewPositions[i].pos.y
            && event->xbutton.x < previewPositions[i].pos.x
            + previewPositions[i].size.width
            && event->xbutton.y < previewPositions[i].pos.y
            + previewPositions[i].size.height) {

            WMSetPopUpButtonSelectedItem(panel->fontSel, i);
            changePage(panel->fontSel, panel);
            return;
        }
    }
}

static void
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int section;
    WMScreen *scr = WMWidgetScreen(panel->box);
    RContext *rc = WMScreenRContext(scr);
    static WMPoint positions[] = {
        {5, 15},
        {5, 62},
        {5, 110},
        {5, 140},
        {130, 62},
        {130, 132}
    };

    if (w) {
        section = WMGetPopUpButtonSelectedItem(panel->fontSel);
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
    paintTextField(panel, section);
    refillFontSetList(panel);
}

static void
setLanguageType(void *data, Bool multiByte)
{
    _Panel *p = (_Panel*)data;

    if (multiByte) {
        WMMapWidget(p->fsetL);
        WMMapWidget(p->fsetLs);
        WMMapWidget(p->addB);
        WMMapWidget(p->editB);
        WMMapWidget(p->remB);
        WMMapWidget(p->upB);
        WMMapWidget(p->downB);

        WMUnmapWidget(p->fontT);
        WMUnmapWidget(p->changeB);
    } else {
        WMUnmapWidget(p->fsetL);
        WMUnmapWidget(p->fsetLs);
        WMUnmapWidget(p->addB);
        WMUnmapWidget(p->editB);
        WMUnmapWidget(p->remB);
        WMUnmapWidget(p->upB);
        WMUnmapWidget(p->downB);

        WMMapWidget(p->fontT);
        WMMapWidget(p->changeB);
    }
}

static void
refillFontSetList(void *data)
{
    _Panel *panel = (_Panel*)data;
    WMPropList *array;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);
    int i;
    //int pos;
    WMClearList(panel->fsetLs);
    if(!encoding) {
        array = getCurrentFontProp(panel, section);
    } else {
        array = getDefaultFontProp(panel, encoding, section);
    }
    if(!array){
        wwarning("error not Font prop given");
    } else {
        for (i = 0; i < WMGetPropListItemCount(array); i++) {
            WMGetFromPLArray(array, i);
            WMAddListItem( panel->fsetLs,
                          WMGetFromPLString(
                                            WMGetFromPLArray(array, i)));
        }
        WMReleasePropList(array);
        WMSelectListItem(panel->fsetLs, 0);
    }

    checkListForArrows(panel);
}

static void
insertCurrentFont(char *data, char *type)
{
    WMPropList *key;
    WMPropList *array;
    char *tmp, *str;

    key = WMCreatePLString(type);
    array = WMCreatePLArray(NULL);

    str = wstrdup(data);
    tmp = strtok(str, ",");
    while(tmp) {
        WMAddToPLArray(array, WMCreatePLString(tmp));
        tmp = strtok(NULL, ",");
    }
    wfree(str);


    WMPutInPLDictionary(CurrentFontArray, key, array);
}

static void
readFontEncodings(void *data)
{
    _Panel *panel = (_Panel*)data;
    WMPropList *pl = NULL;
    char *path;
    char *msg;

    path = WMPathForResourceOfType("font.data", NULL);
    if (!path) {
        msg = _("Could not locate font information file WPrefs.app/font.data");
        goto error;
    }

    pl = WMReadPropListFromFile(path);
    if (!pl) {
        msg = _("Could not read font information file WPrefs.app/font.data");
        goto error;
    } else {
        int i;
        WMPropList *key = WMCreatePLString("Encodings");
        WMPropList *array;
        WMMenuItem *mi;

        array = WMGetFromPLDictionary(pl, key);
        WMReleasePropList(key);
        if (!array || !WMIsPLArray(array)) {
            msg = _("Invalid data in font information file WPrefs.app/font.data.\n"
                    "Encodings data not found.");
            goto error;
        }

        WMAddPopUpButtonItem(panel->langP, _("Current"));

        for (i = 0; i < WMGetPropListItemCount(array); i++) {
            WMPropList *item, *str;

            item = WMGetFromPLArray(array, i);
            str = WMGetFromPLArray(item, 0);
            mi = WMAddPopUpButtonItem(panel->langP, WMGetFromPLString(str));
            WMSetMenuItemRepresentedObject(mi, WMRetainPropList(item));
        }
        WMSetPopUpButtonSelectedItem(panel->langP, 0);


        key = WMCreatePLString("WindowTitleFont");
        DefaultWindowTitleFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);

        key = WMCreatePLString("MenuTitleFont");
        DefaultMenuTitleFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);

        key = WMCreatePLString("MenuTextFont");
        DefaultMenuTextFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);

        key = WMCreatePLString("IconTitleFont");
        DefaultIconTitleFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);

        key = WMCreatePLString("ClipTitleFont");
        DefaultClipTitleFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);

        key = WMCreatePLString("LargeDisplayFont");
        DefaultLargeDisplayFont = WMRetainPropList(WMGetFromPLDictionary(pl, key));
        WMReleasePropList(key);
    }

    WMReleasePropList(pl);
    return;
error:
    if (pl)
        WMReleasePropList(pl);

    WMRunAlertPanel(WMWidgetScreen(panel->parent), panel->parent,
                    _("Error"), msg, _("OK"), NULL, NULL);
}

static void
checkListForArrows(void *data)
{
    _Panel *panel = (_Panel*)data;
    int list;
    list = WMGetListNumberOfRows(panel->fsetLs);

    if(list > 1)
    {
        if(WMGetListSelectedItemRow(panel->fsetLs) == 0) {
            WMSetButtonEnabled(panel->upB, False);
            WMSetButtonEnabled(panel->downB, True);
        } else if(WMGetListSelectedItemRow(panel->fsetLs) == list-1) {
            WMSetButtonEnabled(panel->downB, False);
            WMSetButtonEnabled(panel->upB, True);
        } else {
            WMSetButtonEnabled(panel->upB, True);
            WMSetButtonEnabled(panel->downB, True);
        }

    } else {
        WMSetButtonEnabled(panel->upB, False);
        WMSetButtonEnabled(panel->downB, False);
    }
    /* added to control the Remove button */
    if(list > 1)
        WMSetButtonEnabled(panel->remB, True);
    else
        WMSetButtonEnabled(panel->remB, False);
}

static char*
fontOfLang(void *data, char *encoding, int section)
{
    _Panel *panel = (_Panel*)data;
    WMPropList *array;
    char *buf = NULL;
    int i;

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    if(!array) {
        wwarning("error no font prop given");
        return NULL;
    } else {
        for(i=0; i<WMGetPropListItemCount(array); i++)
        {
            if(buf) buf = wstrconcat(buf, ",");
            buf = wstrconcat(buf, WMGetFromPLString(WMGetFromPLArray(array, i)));
        }
        WMReleasePropList(array);
        return wstrdup(buf);
    }
}

static void
changeLanguageAction(WMWidget *w, void *data)
{
    Panel *panel = (Panel*)data;
    //WMScreen *scr = WMWidgetScreen(panel->box);
    int section;

    section = WMGetPopUpButtonSelectedItem(w);

    if(isEncodingMultiByte(panel)) {
        setLanguageType(panel, True);
    } else {
        if(panel->MultiByteText) setLanguageType(panel, True);
        else setLanguageType(panel, False);
    }

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);
}

static WMFont*
getFontForPreview(void *data, int element)
{
    _Panel *panel = (_Panel*)data;
    WMFont *font;
    char *fname;
    WMScreen *scr = WMWidgetScreen(panel->box);
    char *encoding = getFontEncoding(panel);
    fname = fontOfLang(panel, encoding, element);
    //if (WMHasAntialiasingSupport(scr)) {
    if(panel->AntialiasedText) {
        // fix this -Dan font = WMCreateFontWithFlags(scr, fname, WFAntialiased);
        font = WMCreateFont(scr, fname);
    } else {
        font = WMCreateFont(scr, fname);
    }
    //} else {
    //	font = WMCreateFont(scr, fname);
    //}
    if(!font) {
        char *msg;
        int length;
        length = strlen("\"")+
            strlen(fname)+strlen("\" was not loaded correctly.  Make sure the font is available for that encoding.\nLoadind default system font.");
        msg = wmalloc(length +1);
        snprintf(msg, length + 1,
                 "\"%s\" was not loaded correctly.  Make sure the font is available for that encoding.\nLoading default system font.",
                 fname);
        WMRunAlertPanel(WMWidgetScreen(panel->parent),panel->parent,
                        _("Warning"), msg, _("OK"), NULL, NULL);
        font = getDefaultSystemFont(panel, element);
    }
    return font;
}

static char*
getFontSampleString(void *data)
{
    _Panel *panel = (_Panel*)data;
    //WMScreen *scr = WMWidgetScreen(panel->box);
    WMMenuItem *mi;
    WMPropList *pl;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->langP);
    mi = WMGetPopUpButtonMenuItem(panel->langP, section);
    pl = WMGetMenuItemRepresentedObject(mi);

    if (!pl) {
        return NULL;
    } else {
        char *sample;
        sample = WMGetFromPLString(WMGetFromPLArray(pl,3));
        return sample;
    }
}

static char*
getFontEncoding(void *data)
{
    _Panel *panel = (_Panel*)data;
    //WMScreen *scr = WMWidgetScreen(panel->box);
    WMMenuItem *mi;
    WMPropList *pl;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->langP);
    mi = WMGetPopUpButtonMenuItem(panel->langP, section);
    pl = WMGetMenuItemRepresentedObject(mi);

    if (!pl) {
        return NULL;
    } else {
        char *encoding;
        encoding = WMGetFromPLString(WMGetFromPLArray(pl,2));
        return encoding;
    }
}

static Bool
isEncodingMultiByte(void *data)
{
    _Panel *panel = (_Panel*)data;
    //WMScreen *scr = WMWidgetScreen(panel->box);
    WMMenuItem *mi;
    WMPropList *pl;
    int section;

    section = WMGetPopUpButtonSelectedItem(panel->langP);
    mi = WMGetPopUpButtonMenuItem(panel->langP, section);
    pl = WMGetMenuItemRepresentedObject(mi);

    if (!pl) {
        return False;
    } else {
        char *multiByte;
        int res;
        multiByte = WMGetFromPLString(WMGetFromPLArray(pl,1));
        res = atoi(multiByte);
        if(res)
            return True;
        else
            return False;
    }
}

static WMPropList*
getCurrentFontProp(void *data, int section)
{
    WMPropList *array;
    switch (section) {
    case 0:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("WindowTitleFont")));
        break;
    case 1:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("LargeDisplayFont")));
        break;
    case 2:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("MenuTitleFont")));
        break;
    case 3:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("MenuTextFont")));
        break;
    case 4:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("ClipTitleFont")));
        break;
    case 5:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(CurrentFontArray,
                                                       WMCreatePLString("IconTitleFont")));
        break;
    }
    if(!WMIsPLArray(array)) {
        return NULL;
    } else {
        return array;
    }
}

static WMPropList*
getDefaultFontProp(void *data, char *encoding, int section)
{
    WMPropList *array;
    WMPropList *key = WMCreatePLString(encoding);
    switch (section) {
    case 0:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultWindowTitleFont, key));
        WMReleasePropList(key);
        break;
    case 1:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultLargeDisplayFont, key));
        WMReleasePropList(key);
        break;
    case 2:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultMenuTitleFont, key));
        WMReleasePropList(key);
        break;
    case 3:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultMenuTextFont, key));
        WMReleasePropList(key);
        break;
    case 4:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultClipTitleFont, key));
        WMReleasePropList(key);
        break;
    case 5:
        array = WMRetainPropList(
                                 WMGetFromPLDictionary(DefaultIconTitleFont, key));
        WMReleasePropList(key);
        break;
    }
    if(!WMIsPLArray(array)) {
        return NULL;
    } else {
        return array;
    }
}

static WMFont*
getDefaultSystemFont(void *data, int element)
{
    _Panel *panel = (_Panel*)data;
    WMScreen *scr = WMWidgetScreen(panel->box);

    switch(element) {
    case 0:
    case 2:
        return WMBoldSystemFontOfSize(scr, 12);
    case 1:
        return WMBoldSystemFontOfSize(scr, 24);
    case 4:
    case 5:
        return WMSystemFontOfSize(scr, 8);
    case 3:
    default:
        return WMSystemFontOfSize(scr, 12);
    }
}

static void
multiClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    if(!panel->MultiByteText) {
        WMSetButtonText(panel->togMulti, _("Yes"));
        setLanguageType(panel, True);
        panel->MultiByteText = True;
    } else {
        WMSetButtonText(panel->togMulti, _("Auto"));
        if(isEncodingMultiByte(panel)) setLanguageType(panel, True);
        else setLanguageType(panel, False);
        panel->MultiByteText = False;
    }
}

static void
toggleAA(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    //int section;
    if(panel->AntialiasedText)
        panel->AntialiasedText = False;
    else
        panel->AntialiasedText = True;
    /* hmm now i gotta redraw all the fonts in the preview section
     * and the text field
     */
    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);
    if(isEncodingMultiByte(panel)) setLanguageType(panel, True);
}

static void
listClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    checkListForArrows(panel);
}

static void
moveUpListItem(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *tmp;
    int pos;
    char *listtext;
    WMPropList *array;
    WMPropList *string;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    pos = WMGetListSelectedItemRow(panel->fsetLs);

    tmp = WMGetListItem(panel->fsetLs, pos);
    listtext = wstrdup(tmp->text);
    string = WMCreatePLString(listtext);

    WMRemoveListItem(panel->fsetLs, pos);
    WMDeleteFromPLArray(array, pos);
    WMInsertListItem(panel->fsetLs, pos-1, listtext);
    WMInsertInPLArray(array, pos-1, string);

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);

    WMSelectListItem(panel->fsetLs, pos-1);
    checkListForArrows(panel);
}

static void
moveDownListItem(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMListItem *tmp;
    int pos;
    char *listtext;
    WMPropList *array;
    WMPropList *string;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    pos = WMGetListSelectedItemRow(panel->fsetLs);

    tmp = WMGetListItem(panel->fsetLs, pos);
    listtext = wstrdup(tmp->text);
    string = WMCreatePLString(listtext);
    WMRemoveListItem(panel->fsetLs, pos);
    WMDeleteFromPLArray(array, pos);
    WMInsertListItem(panel->fsetLs, pos+1, listtext);
    WMInsertInPLArray(array, pos+1, string);

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);

    WMSelectListItem(panel->fsetLs, pos+1);
    checkListForArrows(panel);
}

static void
addButtonAction(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char *chosenFont;
    int pos;
    WMPropList *array;
    WMPropList *string;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    WMHideFontPanel(panel->fontPanel);
    chosenFont = WMGetFontName(WMGetFontPanelFont(panel->fontPanel));
    string = WMCreatePLString(chosenFont);
    pos = WMGetListSelectedItemRow(panel->fsetLs);
    WMInsertListItem(panel->fsetLs, pos+1, chosenFont);
    WMInsertInPLArray(array, pos+1, string);
    WMSelectListItem(panel->fsetLs, pos+1);

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);
}

static void
changeButtonAction(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char *chosenFont;
    int pos;
    WMPropList *array;
    WMPropList *string;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    WMHideFontPanel(panel->fontPanel);

    chosenFont = WMGetFontName(WMGetFontPanelFont(panel->fontPanel));
    string = WMCreatePLString(chosenFont);

    pos = WMGetListSelectedItemRow(panel->fsetLs);
    WMRemoveListItem(panel->fsetLs, pos);
    WMDeleteFromPLArray(array, pos);
    WMInsertListItem(panel->fsetLs, pos, chosenFont);
    WMInsertInPLArray(array, pos, string);
    WMSelectListItem(panel->fsetLs, pos);

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);
}

static void
changeButtonClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    WMSetFontPanelAction(panel->fontPanel, changeButtonAction, panel);
    WMShowFontPanel(panel->fontPanel);
}

static void
addButtonClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    WMSetFontPanelAction(panel->fontPanel, addButtonAction, panel);
    WMShowFontPanel(panel->fontPanel);
}

static void
removeButtonClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int pos;
    int list;
    WMPropList *array;
    char *encoding = getFontEncoding(panel);
    int section = WMGetPopUpButtonSelectedItem(panel->fontSel);

    if(!encoding)
        array = getCurrentFontProp(panel, section);
    else
        array = getDefaultFontProp(panel, encoding, section);

    pos = WMGetListSelectedItemRow(panel->fsetLs);
    WMRemoveListItem(panel->fsetLs, pos);
    WMDeleteFromPLArray(array, pos);

    list = WMGetListNumberOfRows(panel->fsetLs);
    if(list != 0) {
        if(list > pos)
            WMSelectListItem(panel->fsetLs, pos);
        else if(list == pos)
            WMSelectListItem(panel->fsetLs, list-1);
        else
            WMSelectListItem(panel->fsetLs, 0);
    }
    checkListForArrows(panel);

    paintPreviewBox(panel, EVERYTHING);
    changePage(panel->fontSel, panel);
}

static void
showData(_Panel *panel)
{
    //WMScreen *scr = WMWidgetScreen(panel->parent);
    char *str;
    int i;

    CurrentFontArray = WMCreatePLDictionary(NULL, NULL);

    str = GetStringForKey("WindowTitleFont");
    insertCurrentFont(wstrdup(str), "WindowTitleFont");

    str = GetStringForKey("LargeDisplayFont");
    insertCurrentFont(wstrdup(str), "LargeDisplayFont");

    str = GetStringForKey("MenuTitleFont");
    insertCurrentFont(wstrdup(str), "MenuTitleFont");

    str = GetStringForKey("MenuTextFont");
    insertCurrentFont(wstrdup(str), "MenuTextFont");

    str = GetStringForKey("ClipTitleFont");
    insertCurrentFont(wstrdup(str), "ClipTitleFont");

    str = GetStringForKey("IconTitleFont");
    insertCurrentFont(wstrdup(str), "IconTitleFont");

    /* i put this here cause it needs to be known before we paint */
    readFontEncodings(panel);

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

    str = GetStringForKey("MultiByteText");
    if (str)
    {
        if (strcasecmp(str, "YES")==0) {
            setLanguageType(panel, True);
            WMSetButtonText(panel->togMulti, "Yes");
            printf("yes multi\n");
            panel->MultiByteText = True;
        } else if (strcasecmp(str, "AUTO") == 0) {
            char *locale;
            locale = setlocale(LC_CTYPE, NULL);
            if(locale != NULL
               && (strncmp(locale, "ja", 2) == 0
                   || strncmp(locale, "zh", 2) == 0
                   || strncmp(locale, "ko", 2) == 0)) {
                setLanguageType(panel, True);
                WMSetButtonText(panel->togMulti, "Auto");
                printf("auto multi\n");
                panel->MultiByteText = True;
            } else {
                setLanguageType(panel, False);
                WMSetButtonText(panel->togMulti, "Auto");
                panel->MultiByteText = False;
            }
        }
    } else {
        char *locale;
        locale = setlocale(LC_CTYPE, NULL);
        if(locale != NULL
           && (strncmp(locale, "ja", 2) == 0
               || strncmp(locale, "zh", 2) == 0
               || strncmp(locale, "ko", 2) == 0)) {
            setLanguageType(panel, True);
            WMSetButtonText(panel->togMulti, "Auto");
            printf("auto multi\n");
            panel->MultiByteText = True;
        } else {
            setLanguageType(panel, False);
            WMSetButtonText(panel->togMulti, "Auto");
            panel->MultiByteText = False;
        }
    }
    /* gotta check for Antialiasing AFTER MultiByte incase the use has both
     * to maintain behavior in Current Fonts set or i could add another if
     * statement to setLanguageType =) */
    //if (WMHasAntialiasingSupport(scr)) {
    WMMapWidget(panel->togAA);
    if(GetBoolForKey("AntialiasedText")){
        WMSetButtonSelected(panel->togAA, True);
        panel->AntialiasedText = True;
    } else {
        WMSetButtonSelected(panel->togAA, False);
        panel->AntialiasedText = False;
    }
    //} else {
    //	WMUnmapWidget(panel->togAA);
    //}


    paintPreviewBox(panel, EVERYTHING);
}

static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->parent);


    panel->box = WMCreateBox(panel->parent);
    WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    panel->hand = WMCreatePixmapFromXPMData(scr, hand_xpm);
    panel->up_arrow = WMCreatePixmapFromXPMData(scr, up_arrow_xpm);
    panel->down_arrow = WMCreatePixmapFromXPMData(scr, down_arrow_xpm);
    panel->alt_up_arrow = WMCreatePixmapFromXPMData(scr, alt_up_arrow_xpm);
    panel->alt_down_arrow = WMCreatePixmapFromXPMData(scr, alt_down_arrow_xpm);

    panel->prevL = WMCreateLabel(panel->box);
    WMResizeWidget(panel->prevL, 240, FRAME_HEIGHT - 45);
    WMMoveWidget(panel->prevL, 15, 35);
    WMSetLabelRelief(panel->prevL, WRSunken);
    WMSetLabelImagePosition(panel->prevL, WIPImageOnly);

    WMCreateEventHandler(WMWidgetView(panel->prevL), ButtonPressMask,
                         previewClick, panel);

    /* Widget Selection */
    panel->fontSel = WMCreatePopUpButton(panel->box);
    WMResizeWidget(panel->fontSel, 135, 20);
    WMMoveWidget(panel->fontSel, 15, 10);
    WMAddPopUpButtonItem(panel->fontSel, _("Window Title Font"));
    WMAddPopUpButtonItem(panel->fontSel, _("Large Display Font"));
    WMAddPopUpButtonItem(panel->fontSel, _("Menu Title Font"));
    WMAddPopUpButtonItem(panel->fontSel, _("Menu Item Font" ));
    WMAddPopUpButtonItem(panel->fontSel, _("Clip Title Font"));
    WMAddPopUpButtonItem(panel->fontSel, _("Icon Title Font"));

    WMSetPopUpButtonSelectedItem(panel->fontSel, 0);

    WMSetPopUpButtonAction(panel->fontSel, changePage, panel);

    /* MultiByteText toggle */
    panel->multiF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->multiF, 70, 50);
    WMMoveWidget(panel->multiF, 440, 10);
    WMSetFrameTitle(panel->multiF, _("MultiByte"));


    panel->togMulti = WMCreateCommandButton(panel->multiF);
    WMResizeWidget(panel->togMulti, 40, 20);
    WMMoveWidget(panel->togMulti, 15, 20);
    WMSetButtonAction(panel->togMulti, multiClick, panel);

    WMMapSubwidgets(panel->multiF);

    /* language selection */
    panel->langF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->langF, 165, 50);
    WMMoveWidget(panel->langF, 265, 10);
    WMSetFrameTitle(panel->langF, _("Default Font Encodings"));

    panel->langP = WMCreatePopUpButton(panel->langF);
    WMResizeWidget(panel->langP, 135, 20);
    WMMoveWidget(panel->langP, 15, 20);

    WMSetPopUpButtonAction(panel->langP, changeLanguageAction, panel);

    WMMapSubwidgets(panel->langF);

    /* Antialiasing */
    //if (WMHasAntialiasingSupport(scr)) {
    panel->togAA = WMCreateSwitchButton(panel->box);
    WMResizeWidget(panel->togAA, 110, 20);
    WMMoveWidget(panel->togAA, 155, 10);
    WMSetButtonText(panel->togAA, _("Smooth Fonts"));
    WMSetBalloonTextForView(_("Smooth Font edges for the eye candy\n"
                              "requires a restart after saving"),
                            WMWidgetView(panel->togAA));
    WMSetButtonAction(panel->togAA, toggleAA, panel);
    //}
    /* multibyte */
    panel->fsetL = WMCreateLabel(panel->box);
    WMResizeWidget(panel->fsetL, 245, 20);
    WMMoveWidget(panel->fsetL, 265, 70);
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

    panel->fsetLs = WMCreateList(panel->box);
    WMResizeWidget(panel->fsetLs, 245, 86);
    WMMoveWidget(panel->fsetLs, 265, 92);
    WMSetListAction(panel->fsetLs, listClick, panel);
    WMSetListDoubleAction(panel->fsetLs, changeButtonClick, panel);

    panel->addB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->addB, 78, 24);
    WMMoveWidget(panel->addB, 265, 201);
    WMSetButtonText(panel->addB, _("Add..."));
    WMSetButtonAction(panel->addB, addButtonClick, panel);

    panel->editB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->editB, 78, 24);
    WMMoveWidget(panel->editB, 348, 201);
    WMSetButtonText(panel->editB, _("Change..."));
    WMSetButtonAction(panel->editB, changeButtonClick, panel);

    panel->remB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->remB, 78, 24);
    WMMoveWidget(panel->remB, 431, 201);
    WMSetButtonText(panel->remB, _("Remove"));
    WMSetButtonAction(panel->remB, removeButtonClick, panel);

    /* happy Up/Down buttons */
    panel->upB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->upB, 16, 16);
    WMMoveWidget(panel->upB, 265, 182);
    WMSetButtonImage(panel->upB, panel->up_arrow);
    WMSetButtonAltImage(panel->upB, panel->alt_up_arrow);
    WMSetButtonImagePosition(panel->upB, WIPImageOnly);
    WMSetButtonImageDimsWhenDisabled(panel->upB, True);
    WMSetButtonAction(panel->upB, moveUpListItem, panel);

    panel->downB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->downB, 16, 16);
    WMMoveWidget(panel->downB, 286, 182);
    WMSetButtonImage(panel->downB, panel->down_arrow);
    WMSetButtonAltImage(panel->downB, panel->alt_down_arrow);
    WMSetButtonImagePosition(panel->downB, WIPImageOnly);
    WMSetButtonImageDimsWhenDisabled(panel->downB, True);
    WMSetButtonAction(panel->downB, moveDownListItem, panel);

    /* single byte */
    panel->fontT = WMCreateTextField(panel->box);
    WMResizeWidget(panel->fontT, 245, 30);
    WMMoveWidget(panel->fontT, 265, 120);

    panel->changeB = WMCreateCommandButton(panel->box);
    WMResizeWidget(panel->changeB, 104, 24);
    WMMoveWidget(panel->changeB, 335, 160);
    WMSetButtonText(panel->changeB, _("Change..."));
    WMSetButtonAction(panel->changeB, changeButtonClick, panel);


    panel->black = WMBlackColor(scr);
    panel->white = WMWhiteColor(scr);
    panel->light = WMGrayColor(scr);
    panel->dark = WMDarkGrayColor(scr);
    panel->back = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);

    /* Font Panel !!!!! */
    panel->fontPanel = WMGetFontPanel(scr);

#if 0
    for (i = 0; Languages[i].language != NULL; i++) {
        WMAddPopUpButtonItem(panel->langP, Languages[i].language);
    }

    for (i = 0; Options[i].description != NULL; i++) {
        WMAddListItem(panel->settingLs, Options[i].description);
    }
#endif
    WMRealizeWidget(panel->box);
    WMMapSubwidgets(panel->box);

    showData(panel);
    changePage(panel->fontSel, panel);
}

static void
storeData(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    int i;

    char *encoding = getFontEncoding(panel);

    for(i=0;i < 6; i++)
    {
        switch(i) {
        case 0:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "WindowTitleFont");
            break;
        case 1:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "LargeDisplayFont");
            break;
        case 2:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "MenuTitleFont");
            break;
        case 3:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "MenuTextFont");
            break;
        case 4:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "ClipTitleFont");
            break;
        case 5:
            SetStringForKey(fontOfLang(panel, encoding, i),
                            "IconTitleFont");
            break;
        }
    }

    //if (WMHasAntialiasingSupport(WMWidgetScreen(panel->box)))
    SetBoolForKey(WMGetButtonSelected(panel->togAA), "AntialiasedText");

    if(panel->MultiByteText)
        SetStringForKey("YES", "MultiByteText");
    else {
        if(isEncodingMultiByte(panel)) SetStringForKey("YES", "MultiByteText");
        else SetStringForKey("AUTO", "MultiByteText");
    }
}

static void
prepClosure(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMFreeFontPanel(panel->fontPanel);
    WMReleasePropList(CurrentFontArray);
    /* and what ever else i've forgotten or overlooked
     * maybe someone will add them */
}

Panel*
InitFont(WMScreen *scr, WMWidget *parent)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Font Preferences");
    panel->description = _("Font Configurations for Windows, Menus etc");

    panel->parent = parent;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.prepareForClose = prepClosure;

    AddSection(panel, ICON_FILE);

    return panel;
}

