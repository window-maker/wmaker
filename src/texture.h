/*
 *  WindowMaker window manager
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


#ifndef WMTEXTURE_H_
#define WMTEXTURE_H_

#include "screen.h"
#include "wcore.h"

/* texture relief */
#define WREL_RAISED	0
#define WREL_SUNKEN	1
#define WREL_FLAT	2
#define WREL_ICON	4
#define WREL_MENUENTRY	6

/* texture types */
#define WREL_BORDER_MASK	1

#define WTEX_SOLID 	((1<<1)|WREL_BORDER_MASK)
#define WTEX_HGRADIENT	((1<<2)|WREL_BORDER_MASK)
#define WTEX_VGRADIENT	((1<<3)|WREL_BORDER_MASK)
#define WTEX_DGRADIENT	((1<<4)|WREL_BORDER_MASK)
#define WTEX_MHGRADIENT	((1<<5)|WREL_BORDER_MASK)
#define WTEX_MVGRADIENT	((1<<6)|WREL_BORDER_MASK)
#define WTEX_MDGRADIENT	((1<<7)|WREL_BORDER_MASK)
#define WTEX_PIXMAP	(1<<8)

/* pixmap subtypes */
#define WTP_TILE	2
#define WTP_SCALE	4
#define WTP_CENTER	6

/*
 * (solid <color>)
 * (hgradient <color> <color>)
 * (vgradient <color> <color>)
 * (dgradient <color> <color>)
 * (pixmap <file> <mode>)
 */


typedef struct {
    short type;			       /* type of texture */
    char subtype;		       /* subtype of the texture */
    XColor color;		       /* default background color */
    GC gc;			       /* gc for the background color */
} WTexAny;


typedef struct WTexSolid {
    short type;
    char subtype;
    XColor normal;
    GC normal_gc;
    
    GC light_gc;
    GC dim_gc;
    GC dark_gc;

    XColor light;
    XColor dim;
    XColor dark;
} WTexSolid;


typedef struct WTexGradient {
    short type;
    char subtype;
    XColor normal;
    GC normal_gc;

    XColor color1;
    XColor color2;
} WTexGradient;


typedef struct WTexMGradient {
    short type;
    char subtype;
    XColor normal;
    GC normal_gc;

    RColor **colors;
} WTexMGradient;


typedef struct WTexPixmap {
    short type;
    char subtype;
    XColor normal;
    GC normal_gc;

    struct RImage *pixmap;
} WTexPixmap;

typedef struct WTexCompose {
    short type;
    char subtype;
    XColor normal;
    GC normal_gc;

    union WTexture *back;
    union WTexture *fore;
    int opacity;
} WTexCompose;

typedef union WTexture {
    WTexAny any;    
    WTexSolid solid;
    WTexGradient gradient;
    WTexMGradient mgradient;
    WTexPixmap pixmap;
    WTexCompose compose;
} WTexture;


WTexSolid *wTextureMakeSolid(WScreen*, XColor*);
WTexGradient *wTextureMakeGradient(WScreen*, int, XColor*, XColor*);
WTexMGradient *wTextureMakeMGradient(WScreen*, int, RColor**);
WTexPixmap *wTextureMakePixmap(WScreen *scr, int style, char *pixmap_file, 
			       XColor *color);
void wTextureDestroy(WScreen*, WTexture*);
void wTexturePaint(WTexture *, Pixmap *, WCoreWindow*, int, int);
void wTextureRender(WScreen*, WTexture*, Pixmap*, int, int, int);
struct RImage *wTextureRenderImage(WTexture*, int, int, int);


void wTexturePaintTitlebar(struct WWindow *wwin, WTexture *texture, Pixmap *tdata,
			   int repaint);


#define FREE_PIXMAP(p) if ((p)!=None) XFreePixmap(dpy, (p)), (p)=None

void wDrawBevel(WCoreWindow *core, WTexSolid *texture, int relief);

#endif
