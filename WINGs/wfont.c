
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


static char*
xlfdFromFontName(char *fontName, Bool antialiased)
{
    char *systemFont, *boldSystemFont;
    char *font;
    int size;

    if (antialiased) {
        systemFont = WINGsConfiguration.antialiasedSystemFont;
        boldSystemFont = WINGsConfiguration.antialiasedBoldSystemFont;
    } else {
        systemFont = WINGsConfiguration.systemFont;
        boldSystemFont = WINGsConfiguration.boldSystemFont;
    }

    size = WINGsConfiguration.defaultFontSize;

    if (strcmp(fontName, "SystemFont")==0) {
        font = systemFont;
        size = WINGsConfiguration.defaultFontSize;
    } else if (strncmp(fontName, "SystemFont-", 11)==0) {
        font = systemFont;
        if (sscanf(&fontName[11], "%i", &size)!=1) {
            size = WINGsConfiguration.defaultFontSize;
            wwarning(_("Invalid size specification '%s' in %s. "
                       "Using default %d\n"), &fontName[11], fontName, size);
        }
    } else if (strcmp(fontName, "BoldSystemFont")==0) {
        font = boldSystemFont;
        size = WINGsConfiguration.defaultFontSize;
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

    fname = xlfdFromFontName(fontName, False);

    font = WMHashGet(scrPtr->fontSetCache, fname);
    if (font) {
        WMRetainFont(font);
        wfree(fname);
	return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 0;
    font->antialiased = 0;

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

    fontName = xlfdFromFontName(fontName, False);

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
WMCreateAntialiasedFont(WMScreen *scrPtr, char *fontName)
{
#ifdef XFT
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    if (!scrPtr->hasXftSupport)
        return NULL;

    fontName = xlfdFromFontName(fontName, True);

    if ((ptr = strchr(fontName, ','))) {
	fname = wmalloc(ptr - fontName + 1);
	strncpy(fname, fontName, ptr - fontName);
	fname[ptr - fontName] = 0;
    } else {
	fname = wstrdup(fontName);
    }

    wfree(fontName);

    font = WMHashGet(scrPtr->xftFontCache, fname);
    if (font) {
	WMRetainFont(font);
	wfree(fname);
	return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 1;
    font->antialiased = 1;

    font->screen = scrPtr;

#if 0
    /* // Xft sux. Loading a font that doesn't exist will load the default
     * defined in XftConfig without any warning or error */
    font->font.normal = XLoadQueryFont(display, fname);
    if (!font->font.normal) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    XFreeFont(display, font->font.normal);
#endif

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

    assert(WMHashInsert(scrPtr->xftFontCache, font->name, font)==NULL);

    return font;
#else
    return NULL;
#endif
}


WMFont*
WMCreateAntialiasedFontSet(WMScreen *scrPtr, char *fontName)
{
#ifdef XFT
    WMFont *font;
    Display *display = scrPtr->display;
    char *fname, *ptr;

    if (!scrPtr->hasXftSupport)
        return NULL;

    fontName = xlfdFromFontName(fontName, True);

    // use the second in list if available, instead of first?
    if ((ptr = strchr(fontName, ','))) {
        fname = wmalloc(ptr - fontName + 1);
	strncpy(fname, fontName, ptr - fontName);
	fname[ptr - fontName] = 0;
    } else {
	fname = wstrdup(fontName);
    }

    wfree(fontName);

    font = WMHashGet(scrPtr->xftFontSetCache, fname);
    if (font) {
	WMRetainFont(font);
	wfree(fname);
	return font;
    }

    font = wmalloc(sizeof(WMFont));
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 0;
    font->antialiased = 1;

    font->screen = scrPtr;

#if 0
    /* // Xft sux. Loading a font that doesn't exist will load the default
     * defined in XftConfig without any warning or error */
    font->font.normal = XLoadQueryFont(display, fname);
    if (!font->font.normal) {
        wfree(font);
        wfree(fname);
        return NULL;
    }
    XFreeFont(display, font->font.normal);
#endif

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

    assert(WMHashInsert(scrPtr->xftFontSetCache, font->name, font)==NULL);

    return font;
#else
    return NULL;
#endif
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
    Bool antialiased = scrPtr->antialiasedText;
    WMFont *font;

    if (flags & WFFontSet) {
        multiByte = True;
    } else if (flags & WFNormalFont) {
        multiByte = False;
    }
    if (flags & WFAntialiased) {
        antialiased = True;
    } else if (flags & WFNotAntialiased) {
        antialiased = False;
    }

    if (antialiased && multiByte) {
        font = WMCreateAntialiasedFontSet(scrPtr, fontName);
        /* If we cannot create an antialiased font set and antialiasing is
         * not explicitly requested in flags, fallback to standard font sets */
        if (!font && (flags & WFAntialiased)==0) {
            font = WMCreateFontSet(scrPtr, fontName);
        }
    } else if (antialiased) {
        font = WMCreateAntialiasedFont(scrPtr, fontName);
        /* If we cannot create an antialiased font and antialiasing is
         * not explicitly requested in flags, fallback to normal font */
        if (!font && (flags & WFAntialiased)==0) {
            font = WMCreateNormalFont(scrPtr, fontName);
        }
    } else if (multiByte) {
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
        if (font->antialiased) {
#ifdef XFT
            XftFontClose(font->screen->display, font->font.xft);
#else
            assert(False);
#endif
        } else if (font->notFontSet) {
            XFreeFont(font->screen->display, font->font.normal);
        } else {
            XFreeFontSet(font->screen->display, font->font.set);
        }

        if (font->name) {
            if (font->antialiased && !font->notFontSet) {
                WMHashRemove(font->screen->xftFontSetCache, font->name);
            } else if (font->antialiased) {
                WMHashRemove(font->screen->xftFontCache, font->name);
            } else if (font->notFontSet) {
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
WMHasAntialiasingSupport(WMScreen *scrPtr)
{
    return scrPtr->hasXftSupport;
}


Bool
WMIsAntialiasingEnabled(WMScreen *scrPtr)
{
    return scrPtr->antialiasedText;
}


Bool
WMIsAntialiasedFont(WMFont *font)
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
    char *fontSpec, *xftFontSpec;

#define WConf WINGsConfiguration
    if (bold) {
        fontSpec = makeFontSetOfSize(WConf.boldSystemFont, size);
        xftFontSpec = makeFontSetOfSize(WConf.antialiasedBoldSystemFont, size);
    } else {
        fontSpec = makeFontSetOfSize(WConf.systemFont, size);
        xftFontSpec = makeFontSetOfSize(WConf.antialiasedSystemFont, size);
    }
#undef WConf

    if (scrPtr->antialiasedText && scrPtr->useMultiByte) {
        font = WMCreateAntialiasedFontSet(scrPtr, xftFontSpec);
    } else if (scrPtr->antialiasedText) {
        font = WMCreateAntialiasedFont(scrPtr, xftFontSpec);
    } else if (scrPtr->useMultiByte) {
        font = WMCreateFontSet(scrPtr, fontSpec);
    } else {
        font = WMCreateNormalFont(scrPtr, fontSpec);
    }

    if (!font) {
        if (scrPtr->antialiasedText && scrPtr->useMultiByte) {
            // is arial a good fallback for multibyte?
            wwarning(_("could not load font %s. Trying arial."), xftFontSpec);
            if (bold) {
                font = WMCreateAntialiasedFontSet(scrPtr, "-*-arial-bold-r-normal-*-12-*-*-*-*-*-*-*");
            } else {
                font = WMCreateAntialiasedFontSet(scrPtr, "-*-arial-medium-r-normal-*-12-*-*-*-*-*-*-*");
            }
            if (!font) {
                wwarning(_("could not load antialiased font set. Reverting to standard font sets."));
                font = WMCreateFontSet(scrPtr, fontSpec);
                if (!font) {
                    wwarning(_("could not load FontSet %s. Trying fixed."), fontSpec);
                    font = WMCreateFontSet(scrPtr, "fixed");
                }
            }
        } else if (scrPtr->antialiasedText) {
            wwarning(_("could not load font %s. Trying arial."), xftFontSpec);
            if (bold) {
                font = WMCreateAntialiasedFont(scrPtr, "-*-arial-bold-r-normal-*-12-*-*-*-*-*-*-*");
            } else {
                font = WMCreateAntialiasedFont(scrPtr, "-*-arial-medium-r-normal-*-12-*-*-*-*-*-*-*");
            }
            if (!font) {
                wwarning(_("could not load antialiased fonts. Reverting to normal fonts."));
                font = WMCreateNormalFont(scrPtr, fontSpec);
                if (!font) {
                    wwarning(_("could not load font %s. Trying fixed."), fontSpec);
                    font = WMCreateNormalFont(scrPtr, "fixed");
                }
            }
        } else if (scrPtr->useMultiByte) {
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
            wfree(xftFontSpec);
            return NULL;
        }
    }
    wfree(fontSpec);
    wfree(xftFontSpec);

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

    if (font->antialiased) {
#ifdef XFT
        XGlyphInfo extents;

        if (!font->notFontSet) {
            wchar_t *wtext;
            char *mtext;

            /* Use mtext instead of text, because mbstrtowcs() alters it */
            mtext = text;
            wtext = (wchar_t *)wmalloc(4*length+4);
            // pass a real ps instead of NULL below? for multithread safety as
            // from manual
            if (mbsrtowcs(wtext, &mtext, length, NULL)==length) {
                XftTextExtents32(font->screen->display, font->font.xft,
                                 (XftChar32 *)wtext, length, &extents);
            } else {
                // - should rather say that conversion to widechar failed?
                // - use mtext instead of text so that the position of the
                //   invalid sequence is shown?
                wwarning(_("Invalid multibyte sequence: '%s'\n"), text);
                XftTextExtents8(font->screen->display, font->font.xft,
                                (XftChar8 *)text, length, &extents);
            }
            wfree(wtext);
        } else {
            XftTextExtents8(font->screen->display, font->font.xft,
                            (XftChar8 *)text, length, &extents);
        }

        return extents.xOff; /* don't ask :P */
#else
        wassertrv(False, 0);
#endif
    } else if (font->notFontSet) {
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

    if (font->antialiased) {
#ifdef XFT
        XftColor xftcolor;

        xftcolor.color.red = color->color.red;
        xftcolor.color.green = color->color.green;
        xftcolor.color.blue = color->color.blue;
        xftcolor.color.alpha = color->alpha;;
        xftcolor.pixel = W_PIXEL(color);

        XftDrawChange(scr->xftdraw, d);

        if (!font->notFontSet) {
            wchar_t *wtext;
            char *mtext;

            /* Use mtext instead of text, because mbstrtowcs() alters it */
            mtext = text;
            wtext = (wchar_t *)wmalloc(4*length+4);
            if (mbsrtowcs(wtext, &mtext, length, NULL)==length) {
                XftDrawString32(scr->xftdraw, &xftcolor, font->font.xft,
                                x, y + font->y, (XftChar32*)wtext, length);
            } else {
                // - should rather say that conversion to widechar failed?
                // - use mtext instead of text so that the position of the
                //   invalid sequence is shown?
                wwarning(_("Invalid multibyte sequence: '%s'\n"), text);
                XftDrawString8(scr->xftdraw, &xftcolor, font->font.xft,
                               x, y + font->y, (XftChar8*)text, length);
            }
            wfree(wtext);
        } else {
            XftDrawString8(scr->xftdraw, &xftcolor, font->font.xft,
                           x, y + font->y, (XftChar8*)text, length);
        }
#else
        wassertr(False);
#endif
    } else if (font->notFontSet) {
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

        if (!font->notFontSet) {
            wchar_t *wtext;
            char *mtext;

            /* Use mtext instead of text, because mbstrtowcs() alters it */
            mtext = text;
            wtext = (wchar_t *)wmalloc(4*length+4);
            if (mbsrtowcs(wtext, &mtext, length, NULL)==length) {
                XftDrawString32(scr->xftdraw, &textColor, font->font.xft,
                                x, y + font->y, (XftChar32*)wtext, length);
            } else {
                // - should rather say that conversion to widechar failed?
                // - use mtext instead of text so that the position of the
                //   invalid sequence is shown?
                wwarning(_("Invalid multibyte sequence: '%s'\n"), text);
                XftDrawString8(scr->xftdraw, &textColor, font->font.xft,
                               x, y + font->y, (XftChar8*)text, length);
            }
            wfree(wtext);
        } else {
            XftDrawString8(scr->xftdraw, &textColor, font->font.xft,
                           x, y + font->y, (XftChar8*)text, length);
        }
#else
        wassertr(False);
#endif
    } else if (font->notFontSet) {
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
    char fname[256];
    WMFontFlags flag;

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "medium", 2);
    changeFontProp(fname, "r", 3);
    flag = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    return WMCreateFontWithFlags(scr, fname, flag);
}


WMFont*
WMStrengthenFont(WMScreen *scr, WMFont *font)
{
    char fname[256];
    WMFontFlags flag;

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "bold", 2);
    flag = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    return WMCreateFontWithFlags(scr, fname, flag);
}


WMFont*
WMUnstrengthenFont(WMScreen *scr, WMFont *font)
{
    char fname[256];
    WMFontFlags flag;

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "medium", 2);
    flag = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    return WMCreateFontWithFlags(scr, fname, flag);
}


WMFont*
WMEmphasizeFont(WMScreen *scr, WMFont *font)
{
    char fname[256];
    WMFontFlags flag;

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    if (font->antialiased)
        changeFontProp(fname, "i", 3);
    else
        changeFontProp(fname, "o", 3);

    flag = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    return WMCreateFontWithFlags(scr, fname, flag);
}


WMFont*
WMUnemphasizeFont(WMScreen *scr, WMFont *font)
{
    char fname[256];
    WMFontFlags flag;

    if (!scr || !font)
        return NULL;

    snprintf(fname, 255, "%s", font->name);
    changeFontProp(fname, "r", 3);
    flag = (font->antialiased ? WFAntialiased : WFNotAntialiased);
    return WMCreateFontWithFlags(scr, fname, flag);
}


