
#include <stdlib.h>

#include "wconfig.h"

#include "WINGsP.h"

#include <assert.h>
#include <X11/Xlocale.h>

#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>

#define DEFAULT_FONT "sans serif:pixelsize=12"

#define DEFAULT_SIZE WINGsConfiguration.defaultFontSize

static WMHashTable *_fontCache= NULL;

static FcPattern *xlfdToFcPattern(char *xlfd)
{

	FcPattern *pattern;
	char *fname, *ptr;

	/* Just skip old font names that contain %d in them.
	 * We don't support that anymore. */
	if (strchr(xlfd, '%') != NULL)
		return FcNameParse((FcChar8 *) DEFAULT_FONT);

	fname = wstrdup(xlfd);
	if ((ptr = strchr(fname, ','))) {
		*ptr = 0;
	}
	//XXX pattern = XftXlfdParse(fname, False, False);
	wfree(fname);

	if (!pattern) {
		wwarning(_("invalid font: %s. Trying '%s'"), xlfd, DEFAULT_FONT);
		pattern = FcNameParse((FcChar8 *) DEFAULT_FONT);
	}

	return pattern;
}

static char *xlfdToFcName(char *xlfd)
{
	FcPattern *pattern;
	char *fname;

	pattern = xlfdToFcPattern(xlfd);
	fname = (char *)FcNameUnparse(pattern);
	FcPatternDestroy(pattern);

	return fname;
}

static Bool hasProperty(FcPattern * pattern, const char *property)
{
	FcValue val;

	if (FcPatternGet(pattern, property, 0, &val) == FcResultMatch) {
		return True;
	}

	return False;
}

static Bool hasPropertyWithStringValue(FcPattern * pattern, const char *object, char *value)
{
	FcChar8 *str;
	int id;

	if (!value || value[0] == 0)
		return True;

	id = 0;
	while (FcPatternGetString(pattern, object, id, &str) == FcResultMatch) {
		if (strcasecmp(value, (char *)str) == 0) {
			return True;
		}
		id++;
	}

	return False;
}

static char *makeFontOfSize(char *font, int size, char *fallback)
{
	FcPattern *pattern;
	char *result;

	if (font[0] == '-') {
		pattern = xlfdToFcPattern(font);
	} else {
		pattern = FcNameParse((FcChar8 *) font);
	}

	/*FcPatternPrint(pattern); */

	if (size > 0) {
		FcPatternDel(pattern, FC_PIXEL_SIZE);
		FcPatternAddDouble(pattern, FC_PIXEL_SIZE, (double)size);
	} else if (size == 0 && !hasProperty(pattern, "size") && !hasProperty(pattern, FC_PIXEL_SIZE)) {
		FcPatternAddDouble(pattern, FC_PIXEL_SIZE, (double)DEFAULT_SIZE);
	}

	if (fallback && !hasPropertyWithStringValue(pattern, FC_FAMILY, fallback)) {
		FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *) fallback);
	}

	/*FcPatternPrint(pattern); */

	result = (char *)FcNameUnparse(pattern);
	FcPatternDestroy(pattern);

	return result;
}

WMFont *WMCreateFont(WMScreen * scrPtr, char *fontName)
{
	Display *display = scrPtr->display;
	WMFont *font;
	char *fname;
	FcPattern *pattern, *theFont;
	cairo_font_extents_t extents;
	cairo_matrix_t matrix;
	cairo_font_options_t *options;
	double size;

	if (!_fontCache)
	{
		_fontCache = WMCreateHashTable(WMStringPointerHashCallbacks);
	}

	if (fontName[0] == '-') {
		fname = xlfdToFcName(fontName);
	} else {
		fname = wstrdup(fontName);
	}

	if (!WINGsConfiguration.antialiasedText && !strstr(fname, ":antialias=")) {
		fname = wstrappend(fname, ":antialias=false");
	}

	font = WMHashGet(_fontCache, fname);

	if (font) {
		WMRetainFont(font);
		wfree(fname);
		return font;
	}

	pattern= FcNameParse((FcChar8*)fname);
	if (!pattern)
		return NULL;

	FcConfigSubstitute(NULL, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	theFont= FcFontMatch(NULL, pattern, NULL);

	FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &size);

	FcPatternDestroy(pattern);
	if (!theFont)
		return NULL;

	font = wmalloc(sizeof(WMFont));
	memset(font, 0, sizeof(WMFont));

	font->cfont = cairo_ft_font_face_create_for_pattern(theFont);
	FcPatternDestroy(theFont);

	if (cairo_font_face_status(font->cfont) != CAIRO_STATUS_SUCCESS)
	{
		cairo_font_face_destroy(font->cfont);
		wfree(font);
		wfree(fname);
		exit(1);
	}

	cairo_matrix_init_scale(&matrix, size, size);

	options= cairo_font_options_create();
	font->metrics = cairo_scaled_font_create(font->cfont, &matrix, &matrix, options);
	cairo_font_options_destroy(options);

	cairo_scaled_font_extents(font->metrics, &extents);

	font->ascent = extents.ascent;
	font->refCount = 1;
	font->name= fname;

	assert(WMHashInsert(_fontCache, fname, font)==NULL);

	return font;
}

WMFont *WMRetainFont(WMFont * font)
{
	wassertrv(font != NULL, NULL);

	font->refCount++;

	return font;
}

void WMReleaseFont(WMFont * font)
{
	wassertr(font != NULL);

	font->refCount--;
	if (font->refCount < 1) {
		if (font->name) {
			WMHashRemove(_fontCache, font->name);
			wfree(font->name);
		}
		cairo_font_face_destroy(font->cfont);
		cairo_scaled_font_destroy(font->metrics);
		wfree(font);
	}
}

Bool WMIsAntialiasingEnabled(WMScreen * scrPtr)
{
	return True;
}

unsigned int WMFontHeight(WMFont * font)
{
	cairo_font_extents_t extents;

	wassertrv(font!=NULL, 0);

	cairo_scaled_font_extents(font->metrics, &extents);

	return extents.height;

}
char *WMGetFontName(WMFont * font)
{
	wassertrv(font != NULL, NULL);

	return font->name;
}

WMFont *WMDefaultSystemFont(WMScreen * scrPtr)
{
	return WMRetainFont(scrPtr->normalFont);
}

WMFont *WMDefaultBoldSystemFont(WMScreen * scrPtr)
{
	return WMRetainFont(scrPtr->boldFont);
}

WMFont *WMSystemFontOfSize(WMScreen * scrPtr, int size)
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

WMFont *WMBoldSystemFontOfSize(WMScreen * scrPtr, int size)
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

int WMWidthOfString(WMFont *font, const char *text)
{
	cairo_text_extents_t extents;

	wassertrv(font!=NULL && text!=NULL, 0);

	cairo_scaled_font_text_extents(font->metrics, text, &extents);

	return extents.x_advance;
}


void WMFontSet(cairo_t *cairo, WMFont *font)
{
	cairo_set_scaled_font(cairo, font->metrics);
}


void WMDrawString(cairo_t *cairo, WMColorSpec *color, WMFont *font,
		int x, int y, char *text)
{
	cairo_text_extents_t extents;

	wassertr(font!=NULL);

	cairo_save(cairo);

	WMColorSpecSet(cairo, color);
	WMFontSet(cairo, font);

	cairo_scaled_font_text_extents(font->metrics, text, &extents);

	cairo_move_to(cairo, x - extents.x_bearing, y + font->ascent);

	cairo_show_text(cairo, text);

	cairo_restore(cairo);
}


WMFont *WMCopyFontWithStyle(WMScreen * scrPtr, WMFont * font, WMFontStyle style)
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
	pattern = FcNameParse((FcChar8 *) WMGetFontName(font));
	switch (style) {
	case WFSNormal:
		FcPatternDel(pattern, FC_WEIGHT);
		FcPatternDel(pattern, FC_SLANT);
		break;
	case WFSBold:
		FcPatternDel(pattern, FC_WEIGHT);
		FcPatternAddString(pattern, FC_WEIGHT, (FcChar8 *) "bold");
		break;
	case WFSItalic:
		FcPatternDel(pattern, FC_SLANT);
		FcPatternAddString(pattern, FC_SLANT, (FcChar8 *) "italic");
		break;
	case WFSBoldItalic:
		FcPatternDel(pattern, FC_WEIGHT);
		FcPatternDel(pattern, FC_SLANT);
		FcPatternAddString(pattern, FC_WEIGHT, (FcChar8 *) "bold");
		FcPatternAddString(pattern, FC_SLANT, (FcChar8 *) "italic");
		break;
	}

	name = (char *)FcNameUnparse(pattern);
	copy = WMCreateFont(scrPtr, name);
	FcPatternDestroy(pattern);
	wfree(name);

	return copy;
}
