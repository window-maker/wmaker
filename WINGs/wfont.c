
#include "WINGsP.h"


#include <wraster.h>
#include <assert.h>

static char *makeFontSetOfSize(char *fontset, int size);


WMFont*
WMCreateFontSet(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;
    char **missing;
    int nmissing = 0;
    char *defaultString;
    XFontSetExtents *extents;

    font = WMHashGet(scrPtr->fontCache, fontName);
    if (font) {
	WMRetainFont(font);
	return font;
    }

    font = malloc(sizeof(WMFont));
    if (!font)
      return NULL;
    memset(font, 0, sizeof(WMFont));

    font->notFontSet = 0;

    font->screen = scrPtr;

    font->font.set = XCreateFontSet(display, fontName, &missing, &nmissing,
				    &defaultString);
    if (nmissing > 0 && font->font.set) {
	int i;
	
	wwarning("the following character sets are missing in %s:",
		 fontName);
	for (i = 0; i < nmissing; i++) {
	    wwarning(missing[i]);
	}
	XFreeStringList(missing);
	if (defaultString)
	    wwarning("the string \"%s\" will be used in place of any characters from those sets.",
		     defaultString);
    }
    if (!font->font.set) {
	free(font);
	return NULL;
    }
    
    extents = XExtentsOfFontSet(font->font.set);
    
    font->height = extents->max_logical_extent.height;
    font->y = font->height - (font->height + extents->max_logical_extent.y);

    font->refCount = 1;

    font->name = wstrdup(fontName);

    assert(WMHashInsert(scrPtr->fontCache, font->name, font)==NULL);

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
	free(fname);
	return font;
    }

    font = malloc(sizeof(WMFont));
    if (!font) {
	free(fname);
	return NULL;
    }
    memset(font, 0, sizeof(WMFont));
    
    font->notFontSet = 1;
    
    font->screen = scrPtr;
    
    font->font.normal = XLoadQueryFont(display, fname);
    if (!font->font.normal) {
	free(font);
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
    if (scrPtr->useMultiByte)
	return WMCreateFontSet(scrPtr, fontName);
    else
	return WMCreateNormalFont(scrPtr, fontName);
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
	if (font->notFontSet)
	    XFreeFont(font->screen->display, font->font.normal);
	else {
	    XFreeFontSet(font->screen->display, font->font.set);
	}
	if (font->name) {
	    WMHashRemove(font->screen->fontCache, font->name);
	    free(font->name);
	}
	free(font);
    }
}



unsigned int
WMFontHeight(WMFont *font)
{
    wassertrv(font!=NULL, 0);

    return font->height;
}



WMFont*
WMSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;

    fontSpec = makeFontSetOfSize(WINGsConfiguration.systemFont, size);

    if (scrPtr->useMultiByte)
	font = WMCreateFontSet(scrPtr, fontSpec);
    else
	font = WMCreateNormalFont(scrPtr, fontSpec);

    if (!font) {
	if (scrPtr->useMultiByte) {
	    wwarning("could not load font set %s. Trying fixed.", fontSpec);
	    font = WMCreateFontSet(scrPtr, "fixed");
	    if (!font) {
		font = WMCreateFontSet(scrPtr, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*");
	    }
	} else {
	    wwarning("could not load font %s. Trying fixed.", fontSpec);
	    font = WMCreateNormalFont(scrPtr, "fixed");
	}
	if (!font) {
	    wwarning("could not load fixed font!");
	    free(fontSpec);
	    return NULL;
	}
    }
    free(fontSpec);

    return font;
}


WMFont*
WMBoldSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;

    fontSpec = makeFontSetOfSize(WINGsConfiguration.boldSystemFont, size);

    if (scrPtr->useMultiByte)
	font = WMCreateFontSet(scrPtr, fontSpec);
    else
	font = WMCreateNormalFont(scrPtr, fontSpec);

    if (!font) {
	if (scrPtr->useMultiByte) {
	    wwarning("could not load font set %s. Trying fixed.", fontSpec);
	    font = WMCreateFontSet(scrPtr, "fixed");
	    if (!font) {
		font = WMCreateFontSet(scrPtr, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*");
	    }
	} else {
	    wwarning("could not load font %s. Trying fixed.", fontSpec);
	    font = WMCreateNormalFont(scrPtr, "fixed");
	}
	if (!font) {
	    wwarning("could not load fixed font!");
	    free(fontSpec);
	    return NULL;
	}
    }
    free(fontSpec);
    
    return font;
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
    
    if (font->notFontSet)
	return XTextWidth(font->font.normal, text, length);
    else {
	XRectangle rect;
	XRectangle AIXsucks;
	
	XmbTextExtents(font->font.set, text, length, &AIXsucks, &rect);
	
	return rect.width;
    }
}



void
WMDrawString(WMScreen *scr, Drawable d, GC gc, WMFont *font, int x, int y,
	     char *text, int length)
{
    wassertr(font!=NULL);

    if (font->notFontSet) {
	XSetFont(scr->display, gc, font->font.normal->fid);
	XDrawString(scr->display, d, gc, x, y + font->y, text, length);
    } else {
	XmbDrawString(scr->display, d, font->font.set, gc, x, y + font->y,
		      text, length);
    }
}


void
WMDrawImageString(WMScreen *scr, Drawable d, GC gc, WMFont *font, int x, int y,
		  char *text, int length)
{
    wassertr(font != NULL);

    if (font->notFontSet) {
	XSetFont(scr->display, gc, font->font.normal->fid);
	XDrawImageString(scr->display, d, gc, x, y + font->y, text, length);
    } else {
	XmbDrawImageString(scr->display, d, font->font.set, gc, x, y + font->y,
			   text, length);
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
		wwarning("font description %s is too large.", fontset);
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
	    free(newfs);
	newfs = tmp;

	fontset = ptr+1;
    } while (ptr!=NULL);

    return newfs;
}

