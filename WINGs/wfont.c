
#include <stdlib.h>

#include "wconfig.h"

#include "WINGsP.h"

#include <wraster.h>
#include <assert.h>
#include <X11/Xlocale.h>

#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>


#define DEFAULT_FONT "sans serif:pixelsize=12"

#define DEFAULT_SIZE WINGsConfiguration.defaultFontSize


static FcPattern*
xlfdToFcPattern(char *xlfd)
{
    FcPattern *pattern;
    char *fname, *ptr;

    /* Just skip old font names that contain %d in them.
     * We don't support that anymore. */
    if (strchr(xlfd, '%')!=NULL)
        return FcNameParse(DEFAULT_FONT);

    fname= wstrdup(xlfd);
    if ((ptr = strchr(fname, ','))) {
        *ptr = 0;
    }
    pattern = XftXlfdParse(fname, False, False);
    wfree(fname);

    if (!pattern) {
        wwarning(_("invalid font: %s. Trying '%s'"), xlfd, DEFAULT_FONT);
        pattern = FcNameParse(DEFAULT_FONT);
    }

    return pattern;
}


static char*
xlfdToFcName(char *xlfd)
{
    FcPattern *pattern;
    char *fname;

    pattern = xlfdToFcPattern(xlfd);
    fname = FcNameUnparse(pattern);
    FcPatternDestroy(pattern);

    return fname;
}


static Bool
hasProperty(FcPattern *pattern, const char *property)
{
    FcValue val;

    if (FcPatternGet(pattern, property, 0, &val)==FcResultMatch) {
        return True;
    }

    return False;
}


static Bool
hasPropertyWithStringValue(FcPattern *pattern, const char *object, char *value)
{
    FcChar8 *str;
    int id;

    if (!value || value[0]==0)
        return True;

    id = 0;
    while (FcPatternGetString(pattern, object, id, &str)==FcResultMatch) {
        if (strcasecmp(value, (char*)str) == 0) {
            return True;
        }
        id++;
    }

    return False;
}


static char*
makeFontOfSize(char *font, int size, char *fallback)
{
    FcPattern *pattern;
    char *result;

    if (font[0]=='-') {
        pattern = xlfdToFcPattern(font);
    } else {
        pattern = FcNameParse(font);
    }

    /*FcPatternPrint(pattern);*/

    if (size > 0) {
        FcPatternDel(pattern, FC_PIXEL_SIZE);
        FcPatternAddDouble(pattern, FC_PIXEL_SIZE, (double)size);
    } else if (size==0 && !hasProperty(pattern, "size") &&
               !hasProperty(pattern, FC_PIXEL_SIZE)) {
        FcPatternAddDouble(pattern, FC_PIXEL_SIZE, (double)DEFAULT_SIZE);
    }

    if (fallback && !hasPropertyWithStringValue(pattern, FC_FAMILY, fallback)) {
        FcPatternAddString(pattern, FC_FAMILY, fallback);
    }

    /*FcPatternPrint(pattern);*/

    result = FcNameUnparse(pattern);
    FcPatternDestroy(pattern);

    return result;
}


WMFont*
WMCreateFont(WMScreen *scrPtr, char *fontName)
{
    Display *display = scrPtr->display;
    WMFont *font;
    char *fname;

    if (fontName[0]=='-'){
        fname = xlfdToFcName(fontName);
    } else {
        fname = wstrdup(fontName);
    }

    if (!WINGsConfiguration.antialiasedText && !strstr(fname, ":antialias=")) {
        fname = wstrappend(fname, ":antialias=false");
    }

    font = WMHashGet(scrPtr->fontCache, fname);
    if (font) {
        WMRetainFont(font);
        wfree(fname);
        return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->screen = scrPtr;

    font->font = XftFontOpenName(display, scrPtr->screen, fname);
    if (!font->font) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    font->height = font->font->ascent+font->font->descent;
    font->y = font->font->ascent;

    font->refCount = 1;

    font->name = fname;

    assert(WMHashInsert(scrPtr->fontCache, font->name, font)==NULL);

    return font;
}


WMFont*
WMRetainFont(WMFont *font)
{
    wassertrv(font!=NULL, NULL);

    font->refCount++;

    return font;
}


void
WMReleaseFont(WMFont *font)
{
    wassertr(font!=NULL);

    font->refCount--;
    if (font->refCount < 1) {
        XftFontClose(font->screen->display, font->font);
        if (font->name) {
            WMHashRemove(font->screen->fontCache, font->name);
            wfree(font->name);
        }
        wfree(font);
    }
}


Bool
WMIsAntialiasingEnabled(WMScreen *scrPtr)
{
    return scrPtr->antialiasedText;
}


unsigned int
WMFontHeight(WMFont *font)
{
    wassertrv(font!=NULL, 0);

    return font->height;
}


char*
WMGetFontName(WMFont *font)
{
    wassertrv(font!=NULL, NULL);

    return font->name;
}


WMFont*
WMDefaultSystemFont(WMScreen *scrPtr)
{
    return WMRetainFont(scrPtr->normalFont);
}


WMFont*
WMDefaultBoldSystemFont(WMScreen *scrPtr)
{
    return WMRetainFont(scrPtr->boldFont);
}


WMFont*
WMSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;

    fontSpec = makeFontOfSize(WINGsConfiguration.systemFont, size, NULL);

    font = WMCreateFont(scrPtr, fontSpec);

    if (!font) {
        wwarning(_("could not load font: %s."), fontSpec);
    }

    wfree(fontSpec);

    return font;
}


WMFont*
WMBoldSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;

    fontSpec = makeFontOfSize(WINGsConfiguration.boldSystemFont, size, NULL);

    font = WMCreateFont(scrPtr, fontSpec);

    if (!font) {
        wwarning(_("could not load font: %s."), fontSpec);
    }

    wfree(fontSpec);

    return font;
}


int
WMWidthOfString(WMFont *font, char *text, int length)
{
    XGlyphInfo extents;

    wassertrv(font!=NULL && text!=NULL, 0);

    XftTextExtentsUtf8(font->screen->display, font->font,
                       (XftChar8 *)text, length, &extents);

    return extents.xOff; /* don't ask :P */
}



void
WMDrawString(WMScreen *scr, Drawable d, WMColor *color, WMFont *font,
             int x, int y, char *text, int length)
{
    XftColor xftcolor;

    wassertr(font!=NULL);

    xftcolor.color.red = color->color.red;
    xftcolor.color.green = color->color.green;
    xftcolor.color.blue = color->color.blue;
    xftcolor.color.alpha = color->alpha;;
    xftcolor.pixel = W_PIXEL(color);

    XftDrawChange(scr->xftdraw, d);

    XftDrawStringUtf8(scr->xftdraw, &xftcolor, font->font,
                      x, y + font->y, (XftChar8*)text, length);
}


void
WMDrawImageString(WMScreen *scr, Drawable d, WMColor *color, WMColor *background,
                  WMFont *font, int x, int y, char *text, int length)
{
    XftColor textColor;
    XftColor bgColor;

    wassertr(font!=NULL);

    textColor.color.red = color->color.red;
    textColor.color.green = color->color.green;
    textColor.color.blue = color->color.blue;
    textColor.color.alpha = color->alpha;;
    textColor.pixel = W_PIXEL(color);

    bgColor.color.red = background->color.red;
    bgColor.color.green = background->color.green;
    bgColor.color.blue = background->color.blue;
    bgColor.color.alpha = background->alpha;;
    bgColor.pixel = W_PIXEL(background);

    XftDrawChange(scr->xftdraw, d);

    XftDrawRect(scr->xftdraw, &bgColor, x, y,
                WMWidthOfString(font, text, length),
                font->height);

    XftDrawStringUtf8(scr->xftdraw, &textColor, font->font,
                      x, y + font->y, (XftChar8*)text, length);
}


WMFont*
WMCopyFontWithStyle(WMScreen *scrPtr, WMFont *font, WMFontStyle style)
{
    FcPattern *pattern;
    WMFont *copy;
    char *name;

    if (!font)
        return NULL;

    /* It's enough to add italic to slant, even if the font has no italic
     * variant, but only oblique. This is because fontconfig will actually
     * return the closest match font to what we requested which is the
     * oblique font. Same goes for using bold for weight.
     */
    pattern = FcNameParse(WMGetFontName(font));
    switch (style) {
    case WFSNormal:
        FcPatternDel(pattern, FC_WEIGHT);
        FcPatternDel(pattern, FC_SLANT);
        break;
    case WFSBold:
        FcPatternDel(pattern, FC_WEIGHT);
        FcPatternAddString(pattern, FC_WEIGHT, "bold");
        break;
    case WFSItalic:
        FcPatternDel(pattern, FC_SLANT);
        FcPatternAddString(pattern, FC_SLANT, "italic");
        break;
    case WFSBoldItalic:
        FcPatternDel(pattern, FC_WEIGHT);
        FcPatternDel(pattern, FC_SLANT);
        FcPatternAddString(pattern, FC_WEIGHT, "bold");
        FcPatternAddString(pattern, FC_SLANT, "italic");
        break;
    }

    name = FcNameUnparse(pattern);
    copy = WMCreateFont(scrPtr, name);
    FcPatternDestroy(pattern);
    wfree(name);

    return copy;
}

