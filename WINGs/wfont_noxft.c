

#include "wconfig.h"

#ifndef XFT

#include "WINGsP.h"

#include <wraster.h>
#include <assert.h>
#include <X11/Xlocale.h>


static char *makeFontSetOfSize(char *fontset, int size);


/* XLFD pattern matching */
static char*
getElementFromXLFD(const char *xlfd, int index)
{
    const char *p = xlfd;
    while (*p != 0) {
        if (*p == '-' && --index == 0) {
            const char *end = strchr(p + 1, '-');
            char *buf;
            size_t len;
            if (end == 0) end = p + strlen(p);
            len = end - (p + 1);
            buf = wmalloc(len);
            memcpy(buf, p + 1, len);
            buf[len] = 0;
            return buf;
        }
        p++;
    }
    return strdup("*");
}


/* XLFD pattern matching */
static char*
generalizeXLFD(const char *xlfd)
{
    char *buf;
    int len;
    char *weight = getElementFromXLFD(xlfd, 3);
    char *slant  = getElementFromXLFD(xlfd, 4);
    char *pxlsz  = getElementFromXLFD(xlfd, 7);

#define Xstrlen(A) ((A)?strlen(A):0)
    len = Xstrlen(xlfd)+Xstrlen(weight)+Xstrlen(slant)+Xstrlen(pxlsz)*2+60;
#undef Xstrlen

    buf = wmalloc(len + 1);
    snprintf(buf, len + 1, "%s,-*-*-%s-%s-*-*-%s-*-*-*-*-*-*-*,"
             "-*-*-*-*-*-*-%s-*-*-*-*-*-*-*,*",
             xlfd, weight, slant, pxlsz, pxlsz);

    wfree(pxlsz);
    wfree(slant);
    wfree(weight);

    return buf;
}

/* XLFD pattern matching */
static XFontSet
W_CreateFontSetWithGuess(Display *dpy, char *xlfd, char ***missing,
                         int *nmissing, char **def_string)
{
    XFontSet fs = XCreateFontSet(dpy, xlfd, missing, nmissing, def_string);

    if (fs != NULL && *nmissing == 0) return fs;

    /* for non-iso8859-1 language and iso8859-1 specification
     (this fontset is only for pattern analysis) */
    if (fs == NULL) {
        if (*nmissing != 0) XFreeStringList(*missing);
        setlocale(LC_CTYPE, "C");
        fs = XCreateFontSet(dpy, xlfd, missing, nmissing, def_string);
        setlocale(LC_CTYPE, "");
    }

    /* make XLFD font name for pattern analysis */
    if (fs != NULL) {
        XFontStruct **fontstructs;
        char **fontnames;
        if (XFontsOfFontSet(fs, &fontstructs, &fontnames) > 0)
            xlfd = fontnames[0];
    }

    xlfd = generalizeXLFD(xlfd);

    if (*nmissing != 0) XFreeStringList(*missing);
    if (fs != NULL) XFreeFontSet(dpy, fs);

    fs = XCreateFontSet(dpy, xlfd, missing, nmissing, def_string);

    wfree(xlfd);
    return fs;
}


static char*
xlfdFromFontName(char *fontName)
{
    char *systemFont, *boldSystemFont;
    char *font;
    int size;

    systemFont = WINGsConfiguration.systemFont;
    boldSystemFont = WINGsConfiguration.boldSystemFont;

    size = WINGsConfiguration.defaultFontSize;

    if (strcmp(fontName, "SystemFont")==0) {
        font = systemFont;
    } else if (strncmp(fontName, "SystemFont-", 11)==0) {
        font = systemFont;
        if (sscanf(&fontName[11], "%i", &size)!=1) {
            size = WINGsConfiguration.defaultFontSize;
            wwarning(_("Invalid size specification '%s' in %s. "
                       "Using default %d\n"), &fontName[11], fontName, size);
        }
    } else if (strcmp(fontName, "BoldSystemFont")==0) {
        font = boldSystemFont;
    } else if (strncmp(fontName, "BoldSystemFont-", 15)==0) {
        font = boldSystemFont;
        if (sscanf(&fontName[15], "%i", &size)!=1) {
            size = WINGsConfiguration.defaultFontSize;
            wwarning(_("Invalid size specification '%s' in %s. "
                       "Using default %d\n"), &fontName[15], fontName, size);
        }
    } else {
        font = NULL;
    }

    return (font!=NULL ? makeFontSetOfSize(font, size) : wstrdup(fontName));
}


WMFont*
WMCreateFontSet(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char **missing;
    int nmissing = 0;
    char *defaultString;
    char *fname;
    XFontSetExtents *extents;

    fname = xlfdFromFontName(fontName);

    font = WMHashGet(scrPtr->fontSetCache, fname);
    if (font) {
        WMRetainFont(font);
        wfree(fname);
        return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 0;

    font->screen = scrPtr;

    font->font.set = W_CreateFontSetWithGuess(display, fname, &missing,
                                              &nmissing, &defaultString);
    if (nmissing > 0 && font->font.set) {
        int i;

        wwarning(_("the following character sets are missing in %s:"), fname);
        for (i = 0; i < nmissing; i++) {
            wwarning(missing[i]);
        }
        XFreeStringList(missing);
        if (defaultString)
            wwarning(_("the string \"%s\" will be used in place of any characters from those sets."),
                     defaultString);
    }
    if (!font->font.set) {
        wfree(font);
        wfree(fname);
        return NULL;
    }

    extents = XExtentsOfFontSet(font->font.set);

    font->height = extents->max_logical_extent.height;
    font->y = font->height - (font->height + extents->max_logical_extent.y);

    font->refCount = 1;

    font->name = fname;

    assert(WMHashInsert(scrPtr->fontSetCache, font->name, font)==NULL);

    return font;
}



WMFont*
WMCreateNormalFont(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    fontName = xlfdFromFontName(fontName);

    if ((ptr = strchr(fontName, ','))) {
        fname = wmalloc(ptr - fontName + 1);
        strncpy(fname, fontName, ptr - fontName);
        fname[ptr - fontName] = 0;
    } else {
        fname = wstrdup(fontName);
    }

    wfree(fontName);

    font = WMHashGet(scrPtr->fontCache, fname);
    if (font) {
        WMRetainFont(font);
        wfree(fname);
        return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 1;

    font->screen = scrPtr;

    font->font.normal = XLoadQueryFont(display, fname);
    if (!font->font.normal) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    font->height = font->font.normal->ascent+font->font.normal->descent;
    font->y = font->font.normal->ascent;

    font->refCount = 1;

    font->name = fname;

    assert(WMHashInsert(scrPtr->fontCache, font->name, font)==NULL);

    return font;
}


WMFont*
WMCreateFont(WMScreen *scrPtr, char *fontName)
{
    return WMCreateFontWithFlags(scrPtr, fontName, WFDefaultFont);
}


WMFont*
WMCreateFontWithFlags(WMScreen *scrPtr, char *fontName, WMFontFlags flags)
{
    Bool multiByte = scrPtr->useMultiByte;
    WMFont *font;

    if (flags & WFFontSet) {
        multiByte = True;
    } else if (flags & WFNormalFont) {
        multiByte = False;
    }

    if (multiByte) {
        font = WMCreateFontSet(scrPtr, fontName);
    } else {
        font = WMCreateNormalFont(scrPtr, fontName);
    }

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
        if (font->notFontSet) {
            XFreeFont(font->screen->display, font->font.normal);
        } else {
            XFreeFontSet(font->screen->display, font->font.set);
        }

        if (font->name) {
            if (font->notFontSet) {
                WMHashRemove(font->screen->fontCache, font->name);
            } else {
                WMHashRemove(font->screen->fontSetCache, font->name);
            }
            wfree(font->name);
        }
        wfree(font);
    }
}


Bool
WMIsAntialiasingEnabled(WMScreen *scrPtr)
{
    return False;
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


static WMFont*
makeSystemFontOfSize(WMScreen *scrPtr, int size, Bool bold)
{
    WMFont *font;
    char *fontSpec;

#define WConf WINGsConfiguration
    if (bold) {
        fontSpec = makeFontSetOfSize(WConf.boldSystemFont, size);
    } else {
        fontSpec = makeFontSetOfSize(WConf.systemFont, size);
    }
#undef WConf

    if (scrPtr->useMultiByte) {
        font = WMCreateFontSet(scrPtr, fontSpec);
    } else {
        font = WMCreateNormalFont(scrPtr, fontSpec);
    }

    if (!font) {
        if (scrPtr->useMultiByte) {
            wwarning(_("could not load font set %s. Trying fixed."), fontSpec);
            font = WMCreateFontSet(scrPtr, "fixed");
            if (!font) {
                font = WMCreateFontSet(scrPtr, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*");
            }
        } else {
            wwarning(_("could not load font %s. Trying fixed."), fontSpec);
            font = WMCreateNormalFont(scrPtr, "fixed");
        }
        if (!font) {
            wwarning(_("could not load fixed font!"));
            wfree(fontSpec);
            return NULL;
        }
    }
    wfree(fontSpec);

    return font;
}


WMFont*
WMSystemFontOfSize(WMScreen *scrPtr, int size)
{
    return makeSystemFontOfSize(scrPtr, size, False);
}


WMFont*
WMBoldSystemFontOfSize(WMScreen *scrPtr, int size)
{
    return makeSystemFontOfSize(scrPtr, size, True);
}


XFontSet
WMGetFontFontSet(WMFont *font)
{
    wassertrv(font!=NULL, NULL);

    if (!font->notFontSet && !font->antialiased)
        return font->font.set;

    return NULL;
}


int
WMWidthOfString(WMFont *font, char *text, int length)
{
    wassertrv(font!=NULL, 0);
    wassertrv(text!=NULL, 0);

    if (font->notFontSet) {
        return XTextWidth(font->font.normal, text, length);
    } else {
        XRectangle rect;
        XRectangle AIXsucks;

        XmbTextExtents(font->font.set, text, length, &AIXsucks, &rect);

        return rect.width;
    }
}



void
WMDrawString(WMScreen *scr, Drawable d, WMColor *color, WMFont *font,
             int x, int y, char *text, int length)
{
    wassertr(font!=NULL);

    if (font->notFontSet) {
        XSetFont(scr->display, scr->drawStringGC, font->font.normal->fid);
        XSetForeground(scr->display, scr->drawStringGC, W_PIXEL(color));
        XDrawString(scr->display, d, scr->drawStringGC, x, y + font->y,
                    text, length);
    } else {
        XSetForeground(scr->display, scr->drawStringGC, W_PIXEL(color));
        XmbDrawString(scr->display, d, font->font.set, scr->drawStringGC,
                      x, y + font->y, text, length);
    }
}


void
WMDrawImageString(WMScreen *scr, Drawable d, WMColor *color, WMColor *background,
                  WMFont *font, int x, int y, char *text, int length)
{
    wassertr(font!=NULL);

    if (font->notFontSet) {
        XSetForeground(scr->display, scr->drawImStringGC, W_PIXEL(color));
        XSetBackground(scr->display, scr->drawImStringGC, W_PIXEL(background));
        XSetFont(scr->display, scr->drawImStringGC, font->font.normal->fid);
        XDrawImageString(scr->display, d, scr->drawImStringGC,
                         x, y + font->y, text, length);
    } else {
        XSetForeground(scr->display, scr->drawImStringGC, W_PIXEL(color));
        XSetBackground(scr->display, scr->drawImStringGC, W_PIXEL(background));
        XmbDrawImageString(scr->display, d, font->font.set, scr->drawImStringGC,
                           x, y + font->y, text, length);
    }
}




static char*
makeFontSetOfSize(char *fontset, int size)
{
    char font[300], *f;
    char *newfs = NULL;
    char *ptr;

    do {
        char *tmp;
        int end;


        f = fontset;
        ptr = strchr(fontset, ',');
        if (ptr) {
            int count = ptr-fontset;

            if (count > 255) {
                wwarning(_("font description %s is too large."), fontset);
            } else {
                memcpy(font, fontset, count);
                font[count] = 0;
                f = (char*)font;
            }
        }

        if (newfs)
            end = strlen(newfs);
        else
            end = 0;

        tmp = wmalloc(end + strlen(f) + 8);
        if (end != 0) {
            sprintf(tmp, "%s,", newfs);
            sprintf(tmp + end + 1, f, size);
        } else {
            sprintf(tmp + end, f, size);
        }

        if (newfs)
            wfree(newfs);
        newfs = tmp;

        fontset = ptr+1;
    } while (ptr!=NULL);

    return newfs;
}


#define FONT_PROPS 14

typedef struct {
    char *props[FONT_PROPS];
} W_FontAttributes;


static void
changeFontProp(char *buf, char *newprop, int position)
{
    char buf2[512];
    char *ptr, *pptr, *rptr;
    int count;

    if (buf[0]!='-') {
        /* // remove warning later. or maybe not */
        wwarning(_("Invalid font specification: '%s'\n"), buf);
        return;
    }

    ptr = pptr = rptr = buf;
    count = 0;
    while (*ptr && *ptr!=',') {
        if (*ptr == '-') {
            count++;
            if (count-1==position+1) {
                rptr = ptr;
                break;
            }
            if (count-1==position) {
                pptr = ptr+1;
            }
        }
        ptr++;
    }
    if (position==FONT_PROPS-1) {
        rptr = ptr;
    }

    *pptr = 0;
    snprintf(buf2, 512, "%s%s%s", buf, newprop, rptr);
    strcpy(buf, buf2);
}


static WMArray*
getOptions(char *options)
{
    char *ptr, *ptr2, *str;
    WMArray *result;
    int count;

    result = WMCreateArrayWithDestructor(2, (WMFreeDataProc*)wfree);

    ptr = options;
    while (1) {
        ptr2 = strchr(ptr, ',');
        if (!ptr2) {
            WMAddToArray(result, wstrdup(ptr));
            break;
        } else {
            count = ptr2 - ptr;
            str = wmalloc(count+1);
            memcpy(str, ptr, count);
            str[count] = 0;
            WMAddToArray(result, str);
            ptr = ptr2 + 1;
        }
    }

    return result;
}


#define WFAUnchanged (NULL)
/* Struct for font change operations */
typedef struct WMFontAttributes {
    char *foundry;
    char *family;
    char *weight;
    char *slant;
    char *setWidth;
    char *addStyle;
    char *pixelSize;
    char *pointSize;
    char *resolutionX;
    char *resolutionY;
    char *spacing;
    char *averageWidth;
    char *registry;
    char *encoding;
} WMFontAttributes;

WMFont*
WMCopyFontWithChanges(WMScreen *scrPtr, WMFont *font,
                      const WMFontAttributes *changes)
{
    int index[FONT_PROPS], count[FONT_PROPS];
    int totalProps, i, j, carry;
    char fname[512];
    WMFontFlags fFlags;
    WMBag *props;
    WMArray *options;
    WMFont *result;
    char *prop;

    snprintf(fname, 512, "%s", font->name);

    fFlags = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    fFlags |= (font->notFontSet ? WFNormalFont : WFFontSet);

    props = WMCreateBagWithDestructor(1, (WMFreeDataProc*)WMFreeArray);

    totalProps = 0;
    for (i=0; i<FONT_PROPS; i++) {
        prop = ((W_FontAttributes*)changes)->props[i];
        count[i] = index[i] = 0;
        if (!prop) {
            /* No change for this property */
            continue;
        } else if (strchr(prop, ',')==NULL) {
            /* Simple option */
            changeFontProp(fname, prop, i);
        } else {
            /* Option with fallback alternatives */
            if ((changes==WFAEmphasized || changes==WFABoldEmphasized) &&
                font->antialiased && strcmp(prop, "o,i")==0) {
                options = getOptions("i,o");
            } else {
                options = getOptions(prop);
            }
            WMInsertInBag(props, i, options);
            count[i] = WMGetArrayItemCount(options);
            if (totalProps==0)
                totalProps = 1;
            totalProps = totalProps * count[i];
        }
    }

    if (totalProps == 0) {
        /* No options with fallback alternatives at all */
        WMFreeBag(props);
        return WMCreateFontWithFlags(scrPtr, fname, fFlags);
    }

    for (i=0; i<totalProps; i++) {
        for (j=0; j<FONT_PROPS; j++) {
            if (count[j]!=0) {
                options = WMGetFromBag(props, j);
                prop = WMGetFromArray(options, index[j]);
                if (prop) {
                    changeFontProp(fname, prop, j);
                }
            }
        }
        result = WMCreateFontWithFlags(scrPtr, fname, fFlags);
        if (result) {
            WMFreeBag(props);
            return result;
        }
        for (j=FONT_PROPS-1, carry=1; j>=0; j--) {
            if (count[j]!=0) {
                index[j] += carry;
                carry = (index[j]==count[j]);
                index[j] %= count[j];
            }
        }
    }

    WMFreeBag(props);

    return NULL;
}

// should WFANormal also set "normal" or leave it alone?
static const WMFontAttributes W_FANormal = {
    WFAUnchanged, WFAUnchanged, "medium,normal,regular", "r", "normal",
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


static const WMFontAttributes W_FABold = {
    WFAUnchanged, WFAUnchanged, "bold", WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


static const WMFontAttributes W_FANotBold = {
    WFAUnchanged, WFAUnchanged, "medium,normal,regular", WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


static const WMFontAttributes W_FAEmphasized = {
    WFAUnchanged, WFAUnchanged, WFAUnchanged, "o,i",
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


static const WMFontAttributes W_FANotEmphasized = {
    WFAUnchanged, WFAUnchanged, WFAUnchanged, "r",
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


static const WMFontAttributes W_FABoldEmphasized = {
    WFAUnchanged, WFAUnchanged, "bold", "o,i",
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged,
    WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged, WFAUnchanged
};


const WMFontAttributes *WFANormal         = &W_FANormal;
const WMFontAttributes *WFABold           = &W_FABold;
const WMFontAttributes *WFANotBold        = &W_FANotBold;
const WMFontAttributes *WFAEmphasized     = &W_FAEmphasized;
const WMFontAttributes *WFANotEmphasized  = &W_FANotEmphasized;
const WMFontAttributes *WFABoldEmphasized = &W_FABoldEmphasized;


#endif

