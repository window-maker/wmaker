
#include "wconfig.h"

#ifdef XFT

#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>

#if defined(HAVE_MBSNRTOWCS)
# define __USE_GNU
#endif

#ifdef HAVE_WCHAR_H
# include <wchar.h>
#endif

#include <stdlib.h>

#include "WINGsP.h"

#include <wraster.h>
#include <assert.h>
#include <X11/Xlocale.h>



#if defined(HAVE_MBSNRTOWCS)

static size_t
wmbsnrtowcs(wchar_t *dest, const char **src, size_t nbytes, size_t len)
{
    mbstate_t ps;
    size_t n;

    memset(&ps, 0, sizeof(mbstate_t));
    n = mbsnrtowcs(dest, src, nbytes, len, &ps);
    if (n!=(size_t)-1 && *src) {
        *src -= ps.__count;
    }

    return n;
}

#elif defined(HAVE_MBRTOWC)

// This is 8 times slower than the version above.
static size_t
wmbsnrtowcs(wchar_t *dest, const char **src, size_t nbytes, size_t len)
{
    mbstate_t ps;
    const char *ptr;
    size_t n;
    int nb;

    if (nbytes==0)
        return 0;

    memset(&ps, 0, sizeof(mbstate_t));

    if (dest == NULL) {
        for (ptr=*src, n=0; nbytes>0; n++) {
            nb = mbrtowc(NULL, ptr, nbytes, &ps);
            if (nb == -1) {
                return ((size_t)-1);
            } else if (nb==0 || nb==-2) {
                return n;
            }
            ptr += nb;
            nbytes -= nb;
        }
    }

    for (ptr=*src, n=0; n<len && nbytes>0; n++, dest++) {
        nb = mbrtowc(dest, ptr, nbytes, &ps);
        if (nb == -2) {
            *src = ptr;
            return n;
        } else if (nb == -1) {
            *src = ptr;
            return ((size_t)-1);
        } else if (nb == 0) {
            *src = NULL;
            return n;
        }
        ptr += nb;
        nbytes -= nb;
    }

    *src = ptr;
    return n;
}

#else

// Not only 8 times slower than the version based on mbsnrtowcs
// but also this version is not thread safe nor reentrant

static size_t
wmbsnrtowcs(wchar_t *dest, const char **src, size_t nbytes, size_t len)
{
    const char *ptr;
    size_t n;
    int nb;

    if (nbytes==0)
        return 0;

    mbtowc(NULL, NULL, 0); /* reset shift state */

    if (dest == NULL) {
        for (ptr=*src, n=0; nbytes>0; n++) {
            nb = mbtowc(NULL, ptr, nbytes);
            if (nb == -1) {
                mbtowc(NULL, NULL, 0);
                nb = mbtowc(NULL, ptr, strlen(ptr));
                return (nb == -1 ? (size_t)-1 : n);
            } else if (nb==0) {
                return n;
            }
            ptr += nb;
            nbytes -= nb;
        }
    }

    for (ptr=*src, n=0; n<len && nbytes>0; n++, dest++) {
        nb = mbtowc(dest, ptr, nbytes);
        if (nb == -1) {
            mbtowc(NULL, NULL, 0);
            nb = mbtowc(NULL, ptr, strlen(ptr));
            *src = ptr;
            return (nb == -1 ? (size_t)-1 : n);
        } else if (nb == 0) {
            *src = NULL;
            return n;
        }
        ptr += nb;
        nbytes -= nb;
    }

    *src = ptr;
    return n;
}

#endif


#define DEFAULT_SIZE 12

static char*
fixXLFD(char *xlfd, int size)
{
    char *fname, *ptr;

    fname = wmalloc(strlen(xlfd) + 20);
    if (strstr(xlfd, "%d")!=NULL)
        sprintf(fname, xlfd, size ? size : DEFAULT_SIZE);
    else
        strcpy(fname, xlfd);

    if ((ptr = strchr(fname, ','))) {
        *ptr = 0;
    }

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


// also handle an xlfd with %d for size?
static char*
makeFontOfSize(char *font, int size, char *fallback)
{
    FcPattern *pattern;
    char *result;

    if (font[0]=='-') {
        char *fname;

        fname = fixXLFD(font, size);
        pattern = XftXlfdParse(fname, False, False);
        wfree(fname);
    } else {
        pattern = FcNameParse(font);
    }

    //FcPatternPrint(pattern);
    if (size > 0) {
        FcPatternDel(pattern, "pixelsize");
        FcPatternAddDouble(pattern, "pixelsize", (double)size);
    } else if (size==0 && !hasProperty(pattern, "size") &&
               !hasProperty(pattern, "pixelsize")) {
        FcPatternAddDouble(pattern, "pixelsize", (double)DEFAULT_SIZE);
    }

    if (fallback && !hasPropertyWithStringValue(pattern, "family", fallback)) {
        FcPatternAddString(pattern, "family", fallback);
    }

    result = FcNameUnparse(pattern);
    FcPatternDestroy(pattern);

    return result;
}


WMFont*
WMCreateFont(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    /* This is for back-compat (to allow reading of old xlfd descriptions) */
    if (fontName[0]=='-' && (ptr = strchr(fontName, ','))) {
        // warn for deprecation
        fname = wmalloc(ptr - fontName + 1);
        strncpy(fname, fontName, ptr - fontName);
        fname[ptr - fontName] = 0;
    } else {
        fname = wstrdup(fontName);
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

    // remove
    printf("WMCreateFont: %s\n", fname);

    if (fname[0] == '-') {
        // Backward compat thing. Remove in a later version
        font->font = XftFontOpenXlfd(display, scrPtr->screen, fname);
    } else {
        font->font = XftFontOpenName(display, scrPtr->screen, fname);
    }
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

    fontSpec = makeFontOfSize(WINGsConfiguration.systemFont, size, "sans");

    font = WMCreateFont(scrPtr, fontSpec);

    if (!font) {
        wwarning(_("could not load font %s."), fontSpec);
    }

    wfree(fontSpec);

    return font;
}


WMFont*
WMBoldSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;

    fontSpec = makeFontOfSize(WINGsConfiguration.boldSystemFont, size, "sans");

    font = WMCreateFont(scrPtr, fontSpec);

    if (!font) {
        wwarning(_("could not load font %s."), fontSpec);
    }

    wfree(fontSpec);

    return font;
}


int
WMWidthOfString(WMFont *font, char *text, int length)
{
    XGlyphInfo extents;

    wassertrv(font!=NULL, 0);
    wassertrv(text!=NULL, 0);

    if (font->screen->useWideChar) {
        wchar_t *wtext;
        const char *mtext;
        int len;

        wtext = (wchar_t *)wmalloc(sizeof(wchar_t)*(length+1));
        mtext = text;
        len = wmbsnrtowcs(wtext, &mtext, length, length);
        if (len>0) {
            wtext[len] = L'\0'; /* not really necessary here */
            XftTextExtents32(font->screen->display, font->font,
                             (XftChar32 *)wtext, len, &extents);
        } else {
            if (len==-1) {
                wwarning(_("Conversion to widechar failed (possible "
                           "invalid multibyte sequence): '%s':(pos %d)\n"),
                         text, mtext-text+1);
            }
            extents.xOff = 0;
        }
        wfree(wtext);
    } else if (font->screen->useMultiByte) {
        XftTextExtentsUtf8(font->screen->display, font->font,
                           (XftChar8 *)text, length, &extents);
    } else {
        XftTextExtents8(font->screen->display, font->font,
                        (XftChar8 *)text, length, &extents);
    }

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

    if (font->screen->useWideChar) {
        wchar_t *wtext;
        const char *mtext;
        int len;

        wtext = (wchar_t *)wmalloc(sizeof(wchar_t)*(length+1));
        mtext = text;
        len = wmbsnrtowcs(wtext, &mtext, length, length);
        if (len>0) {
            XftDrawString32(scr->xftdraw, &xftcolor, font->font,
                            x, y + font->y, (XftChar32*)wtext, len);
        } else if (len==-1) {
            wwarning(_("Conversion to widechar failed (possible invalid "
                       "multibyte sequence): '%s':(pos %d)\n"),
                     text, mtext-text+1);
            /* we can draw normal text, or we can draw as much widechar
             * text as was already converted until the error. go figure */
            /*XftDrawString8(scr->xftdraw, &xftcolor, font->font,
             x, y + font->y, (XftChar8*)text, length);*/
        }
        wfree(wtext);
    } else if (font->screen->useMultiByte) {
        XftDrawStringUtf8(scr->xftdraw, &xftcolor, font->font,
                          x, y + font->y, (XftChar8*)text, length);
    } else {
        XftDrawString8(scr->xftdraw, &xftcolor, font->font,
                       x, y + font->y, (XftChar8*)text, length);
    }
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

    if (font->screen->useWideChar) {
        wchar_t *wtext;
        const char *mtext;
        int len;

        mtext = text;
        wtext = (wchar_t *)wmalloc(sizeof(wchar_t)*(length+1));
        len = wmbsnrtowcs(wtext, &mtext, length, length);
        if (len>0) {
            XftDrawString32(scr->xftdraw, &textColor, font->font,
                            x, y + font->y, (XftChar32*)wtext, len);
        } else if (len==-1) {
            wwarning(_("Conversion to widechar failed (possible invalid "
                       "multibyte sequence): '%s':(pos %d)\n"),
                     text, mtext-text+1);
            /* we can draw normal text, or we can draw as much widechar
             * text as was already converted until the error. go figure */
            /*XftDrawString8(scr->xftdraw, &textColor, font->font,
             x, y + font->y, (XftChar8*)text, length);*/
        }
        wfree(wtext);
    } else if (font->screen->useMultiByte) {
        XftDrawStringUtf8(scr->xftdraw, &textColor, font->font,
                          x, y + font->y, (XftChar8*)text, length);
    } else {
        XftDrawString8(scr->xftdraw, &textColor, font->font,
                       x, y + font->y, (XftChar8*)text, length);
    }
}


WMFont*
WMCopyFontWithStyle(WMScreen *scrPtr, WMFont *font, WMFontStyle style)
{
    FcPattern *pattern;
    WMFont *copy;
    char *name;

    if (!font)
        return NULL;

    pattern = FcNameParse(WMGetFontName(font));
    switch (style) {
    case WFSNormal:
        FcPatternDel(pattern, "weight");
        FcPatternDel(pattern, "slant");
        FcPatternAddString(pattern, "weight", "regular");
        FcPatternAddString(pattern, "weight", "medium");
        FcPatternAddString(pattern, "slant", "roman");
        break;
    case WFSBold:
        FcPatternDel(pattern, "weight");
        FcPatternAddString(pattern, "weight", "bold");
        break;
    case WFSEmphasized:
        FcPatternDel(pattern, "slant");
        FcPatternAddString(pattern, "slant", "italic");
        FcPatternAddString(pattern, "slant", "oblique");
        break;
    case WFSBoldEmphasized:
        FcPatternDel(pattern, "weight");
        FcPatternDel(pattern, "slant");
        FcPatternAddString(pattern, "weight", "bold");
        FcPatternAddString(pattern, "slant", "italic");
        FcPatternAddString(pattern, "slant", "oblique");
        break;
    }

    name = FcNameUnparse(pattern);
    copy = WMCreateFont(scrPtr, name);
    FcPatternDestroy(pattern);
    wfree(name);

    return copy;
}


#endif /* XFT */


