
#include "WINGsP.h"


#include <wraster.h>
#include <assert.h>

static char *makeFontSetOfSize(char *fontset, int size);


WMFont*
WMCreateFont(WMScreen *scrPtr, char *fontName)
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
WMCreateFontInDefaultEncoding(WMScreen *scrPtr, char *fontName)
{
    WMFont *font;
    Display *display = scrPtr->display;

    font = malloc(sizeof(WMFont));
    if (!font)
	return NULL;
    memset(font, 0, sizeof(WMFont));
    
    font->notFontSet = 1;
    
    font->screen = scrPtr;
    
    font->font.normal = XLoadQueryFont(display, fontName);
    if (!font->font.normal) {
	free(font);
	return NULL;
    }

    font->height = font->font.normal->ascent+font->font.normal->descent;
    font->y = font->font.normal->ascent;
    
    font->refCount = 1;

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

    font = WMCreateFont(scrPtr, fontSpec);
    
    if (!font) {
	wwarning("could not load font set %s. Trying fixed.", fontSpec);
	font = WMCreateFont(scrPtr, "fixed");
	if (!font) {
	    font = WMCreateFont(scrPtr, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*");
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

    font = WMCreateFont(scrPtr, fontSpec);
    
    if (!font) {
	wwarning("could not load font set %s. Trying fixed.", fontSpec);
	font = WMCreateFont(scrPtr, "fixed");
	if (!font) {
	    font = WMCreateFont(scrPtr, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*");
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
    char font[300];
    char *newfs = NULL;
    char *ptr;
    char *tmp;

    do {
	int hold = ' ';
	
	ptr = strchr(fontset, ',');
	if (ptr) {
	    hold = *(ptr+1);
	    *(ptr+1) = 0;
	}
	if (strlen(fontset)>255) {
	    wwarning("font description %s is too large.", fontset);
	} else {
	    sprintf(font, fontset, size);
	    tmp = wstrappend(newfs, font);
	    if (newfs)
		free(newfs);
	    newfs = tmp;
	}
	if (ptr) {
	    *(ptr+1) = hold;
	}
	fontset = ptr+1;
    } while (ptr!=NULL);

    return newfs;
}

