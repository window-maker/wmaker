
#include "wconfig.h"

#ifdef XFT
# include <X11/Xft/Xft.h>
#endif

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


WMFont*
WMCreateFontSet(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char **missing;
    int nmissing = 0;
    char *defaultString;
    XFontSetExtents *extents;

    font = WMHashGet(scrPtr->fontSetCache, fontName);
    if (font) {
	WMRetainFont(font);
	return font;
    }

    font = malloc(sizeof(WMFont));
    if (!font)
        return NULL;
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 0;
    font->antialiased = 0;

    font->screen = scrPtr;

    font->font.set = W_CreateFontSetWithGuess(display, fontName, &missing,
                                              &nmissing, &defaultString);
    if (nmissing > 0 && font->font.set) {
	int i;
	
	wwarning(_("the following character sets are missing in %s:"),
		 fontName);
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
	return NULL;
    }
    
    extents = XExtentsOfFontSet(font->font.set);
    
    font->height = extents->max_logical_extent.height;
    font->y = font->height - (font->height + extents->max_logical_extent.y);

    font->refCount = 1;

    font->name = wstrdup(fontName);

    assert(WMHashInsert(scrPtr->fontSetCache, font->name, font)==NULL);

    return font;
}



WMFont*
WMCreateNormalFont(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    if ((ptr = strchr(fontName, ','))) {
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

    font = malloc(sizeof(WMFont));
    if (!font) {
	wfree(fname);
	return NULL;
    }
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 1;
    font->antialiased = 0;

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
WMCreateAAFont(WMScreen *scrPtr, char *fontName)
{
#ifdef XFT
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    if ((ptr = strchr(fontName, ','))) {
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

    font = malloc(sizeof(WMFont));
    if (!font) {
	wfree(fname);
	return NULL;
    }
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 1;
    font->antialiased = 1;

    font->screen = scrPtr;

    /* // Xft sux */
    font->font.normal = XLoadQueryFont(display, fname);
    if (!font->font.normal) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    XFreeFont(display, font->font.normal);

    font->font.xft = XftFontOpenXlfd(display, scrPtr->screen, fname);
    if (!font->font.xft) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    font->height = font->font.xft->ascent+font->font.xft->descent;
    font->y = font->font.xft->ascent;

    font->refCount = 1;

    font->name = fname;

    assert(WMHashInsert(scrPtr->fontCache, font->name, font)==NULL);

    return font;
#else
    return NULL;
#endif
}


WMFont*
WMCreateFont(WMScreen *scrPtr, char *fontName)
{
    if (scrPtr->useMultiByte) {
	return WMCreateFontSet(scrPtr, fontName);
    } else if (scrPtr->antialiasedText) {
        WMFont *font = WMCreateAAFont(scrPtr, fontName);
        return font ? font : WMCreateNormalFont(scrPtr, fontName);
    } else {
        return WMCreateNormalFont(scrPtr, fontName);
    }
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
            if (font->antialiased) {
#ifdef XFT
                XftFontClose(font->screen->display, font->font.xft);
#else
                assert(False);
#endif
            } else {
                XFreeFont(font->screen->display, font->font.normal);
            }
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
WMIsAAFont(WMFont *font)
{
    return font->antialiased;
}


unsigned int
WMFontHeight(WMFont *font)
{
    wassertrv(font!=NULL, 0);

    return font->height;
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
    char *fontSpec, *aaFontSpec;

#define WConf WINGsConfiguration
    if (bold) {
        fontSpec = makeFontSetOfSize(WConf.boldSystemFont, size);
        aaFontSpec = makeFontSetOfSize(WConf.antialiasedBoldSystemFont, size);
    } else {
        fontSpec = makeFontSetOfSize(WConf.systemFont, size);
        aaFontSpec = makeFontSetOfSize(WConf.antialiasedSystemFont, size);
    }
#undef WConf

    if (scrPtr->useMultiByte) {
        font = WMCreateFontSet(scrPtr, fontSpec);
    } else if (scrPtr->antialiasedText) {
        font = WMCreateAAFont(scrPtr, aaFontSpec);
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
        } else if (scrPtr->antialiasedText) {
            wwarning(_("could not load font %s. Trying arial."), aaFontSpec);
            if (bold) {
                font = WMCreateAAFont(scrPtr, "-*-arial-bold-r-normal-*-12-*-*-*-*-*-*-*");
            } else {
                font = WMCreateAAFont(scrPtr, "-*-arial-medium-r-normal-*-12-*-*-*-*-*-*-*");
            }
            if (!font) {
                wwarning(_("could not load antialiased fonts. Reverting to normal fonts."));
                font = WMCreateNormalFont(scrPtr, fontSpec);
                if (!font) {
                    wwarning(_("could not load font %s. Trying fixed."), fontSpec);
                    font = WMCreateNormalFont(scrPtr, "fixed");
                }
            }
        } else {
            wwarning(_("could not load font %s. Trying fixed."), fontSpec);
            font = WMCreateNormalFont(scrPtr, "fixed");
        }
        if (!font) {
            wwarning(_("could not load fixed font!"));
            wfree(fontSpec);
            wfree(aaFontSpec);
            return NULL;
        }
    }
    wfree(fontSpec);
    wfree(aaFontSpec);

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

    if (font->notFontSet)
	return NULL;
    else
	return font->font.set;
}


int
WMWidthOfString(WMFont *font, char *text, int length)
{
    wassertrv(font!=NULL, 0);
    wassertrv(text!=NULL, 0);

    if (font->notFontSet) {
        if (font->antialiased) {
#ifdef XFT
            XGlyphInfo extents;

            XftTextExtents8(font->screen->display, font->font.xft,
                            (XftChar8 *)text, length, &extents);
            return extents.xOff; /* don't ask :P */
#else
            assert(False);
#endif
        } else {
            return XTextWidth(font->font.normal, text, length);
        }
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
        if (font->antialiased) {
#ifdef XFT
            XftColor xftcolor;

            xftcolor.color.red = color->color.red;
            xftcolor.color.green = color->color.green;
            xftcolor.color.blue = color->color.blue;
            xftcolor.color.alpha = color->alpha;;
            xftcolor.pixel = W_PIXEL(color);

            XftDrawChange(scr->xftdraw, d);

            XftDrawString8(scr->xftdraw, &xftcolor, font->font.xft,
                           x, y + font->y, text, length);
#else
            assert(False);
#endif
        } else {
            XSetFont(scr->display, scr->drawStringGC, font->font.normal->fid);
            XSetForeground(scr->display, scr->drawStringGC, W_PIXEL(color));
            XDrawString(scr->display, d, scr->drawStringGC, x, y + font->y,
                        text, length);
        }
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
    wassertr(font != NULL);

    if (font->notFontSet) {
        if (font->antialiased) {
#ifdef XFT
            XftColor textColor;
            XftColor bgColor;

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
                        WMWidthOfString(font, text, length), font->height);

            XftDrawString8(scr->xftdraw, &textColor, font->font.xft,
                           x, y + font->y, text, length);
#else
            assert(False);
#endif
        } else {
            XSetForeground(scr->display, scr->drawImStringGC, W_PIXEL(color));
            XSetBackground(scr->display, scr->drawImStringGC, W_PIXEL(background));
            XSetFont(scr->display, scr->drawImStringGC, font->font.normal->fid);
            XDrawImageString(scr->display, d, scr->drawImStringGC,
                             x, y + font->y, text, length);
        }
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


static void
changeFontProp(char *fname, char *newprop, int which)
{
    char before[128], prop[128], after[128];
    char *ptr, *bptr;
    int part=0;

    if (!fname || !prop)
        return;

    ptr = fname;
    bptr = before;
    while (*ptr) {
        if(*ptr == '-') {
            *bptr = 0;
            if (part==which)
                bptr = prop;
            else if (part==which+1)
                bptr = after;
            *bptr++ = *ptr;
            part++;
        } else {
            *bptr++ = *ptr;
        }
        ptr++;
    }
    *bptr = 0;
    snprintf(fname, 255, "%s-%s%s", before, newprop, after);
}


WMFont*
WMNormalizeFont(WMScreen *scr, WMFont *font)
{
    WMFont *newfont=NULL;
    char fname[256];

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "medium", 2);
    changeFontProp(fname, "r", 3);
    newfont = WMCreateNormalFont(scr, fname);

    if (!newfont)
        return NULL;

    return newfont;
}


WMFont*
WMStrengthenFont(WMScreen *scr, WMFont *font)
{
    WMFont *newfont=NULL;
    char fname[256];

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "bold", 2);
    newfont = WMCreateNormalFont(scr, fname);

    if (!newfont)
        return NULL;

    return newfont;
}


WMFont*
WMUnstrengthenFont(WMScreen *scr, WMFont *font)
{
    WMFont *newfont=NULL;
    char fname[256];

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "medium", 2);
    newfont = WMCreateNormalFont(scr, fname);

    if (!newfont)
        return NULL;

    return newfont;
}


WMFont*
WMEmphasizeFont(WMScreen *scr, WMFont *font)
{
    WMFont *newfont=NULL;
    char fname[256];

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "o", 3);
    newfont = WMCreateNormalFont(scr, fname);

    if (!newfont)
        return NULL;

    return newfont;
}


WMFont*
WMUnemphasizeFont(WMScreen *scr, WMFont *font)
{
    WMFont *newfont=NULL;
    char fname[256];

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "r", 3);
    newfont = WMCreateNormalFont(scr, fname);

    if (!newfont)
        return NULL;

    return newfont;
}


WMFont*
WMGetFontOfSize(WMScreen *scr, WMFont *font, int size)
{
    if(!scr || !font || size<1)
        return NULL;

    return font;
}


