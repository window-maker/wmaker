/* resources.c - manage X resources (fonts, colors etc)
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <wraster.h>

#include "WindowMaker.h"
#include "texture.h"
#include "screen.h"
#include "pixmap.h"


/*
 *---------------------------------------------------------------------- 
 * wLoadFont--
 * 	Loads a single font into a WFont structure, initializing
 * data in it. If the font could not be loaded, fixed will be used.
 * If fixed can't be loaded either, the function returns NULL.
 * 
 * Returns:
 * 	The font structure or NULL on error.
 * 
 *---------------------------------------------------------------------- 
 */
WFont*
wLoadFont(char *font_name)
{
    WFont *font;
#ifdef I18N_MB
    char **missing;
    int num_missing = 0;
    char *default_string;
    int skyline;   
#endif
    
    font = malloc(sizeof(WFont));
    if (!font)
      return NULL;

#ifdef I18N_MB
    font->font = XCreateFontSet(dpy, font_name, &missing, &num_missing, 
				&default_string);
    if (num_missing > 0 && font->font) {
        wwarning(_("The following character sets are missing in %s:"),
		 font_name);
        for (skyline = 0; skyline < num_missing; skyline++)
	  wwarning(missing[skyline]);
        XFreeStringList(missing);
        wwarning(_("The string \"%s\" will be used in place"),
		 default_string);
        wwarning(_("of any characters from those sets."));
    }
    if (!font->font) {
	wwarning(_("could not create font set %s. Trying fixed"), font_name);
	font->font = XCreateFontSet(dpy, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",
				    &missing, &num_missing, &default_string);
	if (num_missing > 0) {
	    XFreeStringList(missing);
	}
	if (!font->font) {
	    free(font);
	    return NULL;
	}
    }
    font->height = XExtentsOfFontSet(font->font)->max_logical_extent.height;
    font->y = font->height+XExtentsOfFontSet(font->font)->max_logical_extent.y;
    font->y = font->height-font->y;
#else
    font->font = XLoadQueryFont(dpy, font_name);
    if (!font->font) {
	wwarning(_("could not load font %s. Trying fixed"), font_name);
	font->font = XLoadQueryFont(dpy, "fixed");
	if (!font->font) {
	    free(font);
	    return NULL;
	}
    }
    font->height = font->font->ascent+font->font->descent;
    font->y = font->font->ascent;
#endif /* !I18N_MB */
    
    return font;
}



void
wFreeFont(WFont *font)
{
#ifdef I18N_MB
    XFreeFontSet(dpy, font->font);
#else
    XFreeFont(dpy, font->font);
#endif
    free(font);
}


/*
 * 	    wfatal(_("could not load any usable font set"));
	    wfatal(_("could not load any usable font"));
 */


int
wGetColor(WScreen *scr, char *color_name, XColor *color)
{
    if (!XParseColor(dpy, scr->w_colormap, color_name, color)) {
	wwarning(_("could not parse color \"%s\""), color_name);
	return False;
    }
    if (!XAllocColor(dpy, scr->w_colormap, color)) {
	wwarning(_("could not allocate color \"%s\""), color_name);
	return False;
    }
    return True;
}


void
wFreeColor(WScreen *scr, unsigned long pixel)
{
    if (pixel!=scr->white_pixel && pixel!=scr->black_pixel) {
	unsigned long colors[1];
	
	colors[0] = pixel;
	XFreeColors(dpy, scr->w_colormap, colors, 1, 0);
    }
}
