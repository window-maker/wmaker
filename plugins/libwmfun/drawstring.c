#include <proplist.h>
#include <WINGs.h>
#include <WINGsP.h>
#include "generic.h"

#ifdef USE_FREETYPE
#include <freetype/freetype.h>
#endif

#define MAX_GLYPHS 256

#define _debug(f...) {fprintf(stderr, "debug: ");fprintf(stderr, ##f);fflush(stderr);}

/* #define _debug(s) printf(s);*/

/* drawPlainString */
static Display *ds_dpy = 0;
static Colormap ds_cmap;
static RContext *rc = 0;

RColor black_color = {0, 0, 0, 0};

#ifdef USE_FREETYPE
FT_Library ft_library;
static int inst_ft_library = 0;

typedef struct __FreeTypeRImage{
    RImage *image;
    int advance_x;
    int advance_y;
    int left;
    int top;
} WMFreeTypeRImage;

typedef struct __FreeTypeData{
    FT_Face face;
    RColor color;
    WMFreeTypeRImage **glyphs_array;
    /* will use this when we have frame window plugin */
    /* char *last_titlestr; */
} WMFreeTypeData;
#endif /* USE_FREETYPE */

int getColor (const char *string, Colormap cmap, XColor *xcolor) {
	if (!XParseColor (ds_dpy, cmap, string, xcolor)) {
        return 0;
	}
    if (!XAllocColor (ds_dpy, cmap, xcolor)) {
        return 0;
    }

	return 1;
}

void
initDrawPlainString(Display *dpy, Colormap cmap) {
    ds_dpy = dpy;
    ds_cmap = cmap;
}

void
drawPlainString (proplist_t pl, Drawable d,
        int x, int y, unsigned width, unsigned height,
        char *text, void **func_data)
{
    XColor color1, color2, color3, color4;
    char *plcolor;
    int i, length;
    static int g;
    Pixmap drawbuffer;
    GC gc;
    WMFont *font;

    length = strlen(text);
    gc = func_data[0];
    font = func_data[1];


    /*
    printf("%d members\n",PLGetNumberOfElements(pl));
    for (i =0;i<7;i++) {
	    printf("%d %s\n",i,PLGetString(PLGetArrayElement(pl,i)));
    }
    return;
    */
    drawbuffer = XCreatePixmap(ds_dpy, d,
            width, height*4+6, DefaultDepth(ds_dpy,DefaultScreen(ds_dpy)));
    XCopyArea(ds_dpy, d, drawbuffer,gc,0,y-1,width,height,0,0);

    if (PLGetNumberOfElements(pl) > 5) {
        plcolor = PLGetArrayElement(pl, 5);
        if (getColor(PLGetString(plcolor),ds_cmap, &color3)) {
            XSetForeground(ds_dpy, gc, color3.pixel);

            if (font->notFontSet) {
                XSetFont(ds_dpy, gc, font->font.normal->fid);
                XDrawString(ds_dpy, drawbuffer, gc, x+3, font->y+3, text, length);
            } else {
                XmbDrawString(ds_dpy, drawbuffer, font->font.set, gc, x+4, y+4 + font->y,
                        text, length);
            }
        }
    }

    if (PLGetNumberOfElements(pl) > 4) {
        plcolor = PLGetArrayElement(pl, 4);
        if (getColor(PLGetString(plcolor),ds_cmap, &color1)) {
            XSetForeground(ds_dpy, gc, color1.pixel);

            if (font->notFontSet) {
                XSetFont(ds_dpy, gc, font->font.normal->fid);
                XDrawString(ds_dpy, drawbuffer, gc, x+1, font->y+1, text, length);
            } else {
                XmbDrawString(ds_dpy, drawbuffer, font->font.set, gc, x, y + font->y,
                        text, length);
            }
        }
    }

    if (PLGetNumberOfElements(pl) > 3) {
        plcolor = PLGetArrayElement(pl, 3);
        if (getColor(PLGetString(plcolor),ds_cmap, &color2)) {
            XSetForeground(ds_dpy, gc, color2.pixel);

            if (font->notFontSet) {
                XSetFont(ds_dpy, gc, font->font.normal->fid);
                XDrawString(ds_dpy, drawbuffer, gc, x,font->y, text, length);
            } else {
                XmbDrawString(ds_dpy, drawbuffer, font->font.set, gc, x-1, y-1 + font->y,
                        text, length);
            }
        }
    }

    /*
    plcolor = PLGetArrayElement(pl, 6);
    parse_xcolor(PLGetString(plcolor), &color4);
    */

    XCopyArea(ds_dpy, drawbuffer, d,gc,0,0,width,height,0,y-1);

    XFreePixmap(ds_dpy, drawbuffer);
}

#ifdef USE_FREETYPE

WMFreeTypeRImage *renderChar(FT_Face face, FT_ULong char_index, RColor *color) {
    FT_GlyphSlot slot;
    FT_Bitmap* bitmap;
    WMFreeTypeRImage *tmp_data;
    int index, x, y, i, error; /* error? no no no */

    tmp_data = malloc(sizeof(WMFreeTypeRImage));

    index = FT_Get_Char_Index(face, char_index);

    FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, ft_render_mode_normal);

    slot = face->glyph;
    bitmap = &slot->bitmap;
    tmp_data->advance_x = slot->advance.x;
    tmp_data->advance_y = slot->advance.y;
    tmp_data->top = slot->bitmap_top;
    tmp_data->left = slot->bitmap_left;
    if (bitmap->width > 0 && bitmap->rows > 0) {
        tmp_data->image = RCreateImage(bitmap->width, bitmap->rows, True);
    }
    else tmp_data->image = NULL;

    for (y=0; y < bitmap->rows; y++) {
        for (x=0; x < bitmap->width; x++) {
            color->alpha = bitmap->buffer[y * bitmap->width + x];
            ROperatePixel(tmp_data->image,
                    RCopyOperation, x, y, color);
        } 
    }
    return tmp_data;
}

/* drawFreetypeString */
void initDrawFreeTypeString(proplist_t pl, void **init_data) {
    WMFreeTypeData *data;
    XColor xcolor;

    _debug("invoke initDrawFreeTypeString with init_data[2] %s\n", init_data[2]);

    _debug("%x is ds_dpy\n", ds_dpy);
    initDrawPlainString((Display *)init_data[0], (Colormap)init_data[1]);
    _debug("then %x is ds_dpy\n", ds_dpy);

    /* set init_data[2] to array of RImage */

    /*
    * this would better to have sharable font system but
    * I want to see this more in WINGs though -- ]d
    */
    init_data[2] = malloc(sizeof(WMFreeTypeData));
    data = init_data[2];
    getColor(PLGetString(PLGetArrayElement(pl, 3)), ds_cmap, &xcolor);
    data->color.red = xcolor.red >> 8;
    data->color.green = xcolor.green >> 8;
    data->color.blue = xcolor.blue >> 8;
    
    data->glyphs_array = malloc(sizeof(WMFreeTypeRImage*) * MAX_GLYPHS);
    memset(data->glyphs_array, 0, sizeof(WMFreeTypeRImage*) * MAX_GLYPHS);

    if (!rc) {
        RContextAttributes rcattr;

        rcattr.flags = RC_RenderMode | RC_ColorsPerChannel;
        rcattr.render_mode = RDitheredRendering;
        rcattr.colors_per_channel = 4;

        rc = RCreateContext(ds_dpy, DefaultScreen(ds_dpy), &rcattr);
    }

    /* case 1 -- no no case 2 yet :P */
    if (!inst_ft_library) {
        FT_Init_FreeType(ft_library);
        inst_ft_library++;
    }
    _debug("initialize freetype library\n");

    FT_New_Face(ft_library, PLGetString(PLGetArrayElement(pl, 4)), 0, &data->face);
    FT_Set_Pixel_Sizes(data->face, 0, atoi(PLGetString(PLGetArrayElement(pl, 5))));
}

void
destroyDrawFreeTypeString(proplist_t pl, void **func_data) {
    int i; /* error? no no no */
    WMFreeTypeData *data;

    data = (WMFreeTypeData *) ((void **)*func_data)[2];
    for (i = 0; i < MAX_GLYPHS; i++) {
        if (data->glyphs_array[i]) {
            if (data->glyphs_array[i]->image)
                RDestroyImage(data->glyphs_array[i]->image);
            free(data->glyphs_array[i]);
        }
    }
    free(data->glyphs_array);
    free(data);
    FT_Done_Face(data->face);
    inst_ft_library--;
    if (!inst_ft_library) FT_Done_FreeType(ft_library);
}

void
drawFreeTypeString (proplist_t pl, Drawable d,
        int x, int y, unsigned width, unsigned height,
        unsigned char *text, void **func_data) {
    WMFreeTypeData *data;
    RImage *rimg;
    int i, j;
    int length = strlen(text);
    Pixmap pixmap;
    GC gc;
    int xwidth, xheight, dummy;
    Window wdummy;

    /*pixmap = XCreatePixmap(ds_dpy, d, width, height, DefaultDepth(ds_dpy, DefaultScreen(ds_dpy)));*/
    gc = func_data[0];
    data = ((void **)func_data[2])[2];
    XClearWindow(ds_dpy, d);
    /* create temp for drawing */
    XGetGeometry(ds_dpy, d, &wdummy, &dummy, &dummy, &xwidth, &xheight, &dummy, &dummy);
    pixmap = XCreatePixmap(ds_dpy, d, xwidth, xheight, DefaultDepth(ds_dpy, DefaultScreen(ds_dpy)));
    XCopyArea(ds_dpy, d, pixmap, gc, 0, 0, xwidth, xheight, 0, 0);
    rimg = RCreateImageFromDrawable(rc, pixmap, None);
    XFreePixmap(ds_dpy, pixmap);

    if (rimg) {
        for (i = 0, j = 3; i < strlen(text); i++) {
            if (!data->glyphs_array[text[i]]) {
                data->glyphs_array[text[i]] = renderChar(data->face, (FT_ULong)text[i], &data->color);
                _debug("alloc %c\n", text[i]);
            }
            if (data->glyphs_array[text[i]]->image) {
                int _sx, _dx, _sy, _dy, _sw, _sh;

                _sx = 0;
                _dx = j + data->glyphs_array[text[i]]->left;
                _sw = data->glyphs_array[text[i]]->image->width;
                _sy = 0;
                _dy = rimg->height - data->glyphs_array[text[i]]->top ;
                _sh = data->glyphs_array[text[i]]->image->height;

                if (_dx >= rimg->width) {
                    j += data->glyphs_array[text[i]]->advance_x >> 6;
                    continue;
                } else if (_dx + data->glyphs_array[text[i]]->image->width > rimg->width) {
                    _sw = rimg->width - _dx;
                }
                if (_dx + data->glyphs_array[text[i]]->image->width < 0) {
                    j += data->glyphs_array[text[i]]->advance_x >> 6;
                    continue;
                } else if (_dx < 0) {
                    _sx = -_dx;
                    _sw = data->glyphs_array[text[i]]->image->width + _dx;
                    _dx = 0;
                }


                if (_dy >= rimg->height) {
                    j += data->glyphs_array[text[i]]->advance_x >> 6;
                    continue;
                } else if (_dy + data->glyphs_array[text[i]]->image->height > rimg->height) {
                    _sh = rimg->height - _dy;
                }
                if (_dy + data->glyphs_array[text[i]]->image->height < 0) {
                    j += data->glyphs_array[text[i]]->advance_x >> 6;
                    continue;
                } else if (_dy < 0) {
                    _sy = -_dy;
                    _sh = data->glyphs_array[text[i]]->image->height + _dy;
                    _dy = 0;
                }


                if (_sh > 0 && _sw > 0)
                    RCombineArea(rimg, data->glyphs_array[text[i]]->image, _sx, _sy,
                            _sw, _sh, _dx, _dy);

                j += data->glyphs_array[text[i]]->advance_x >> 6;
            }
        }

        RConvertImage(rc, rimg, &pixmap);
        XCopyArea(ds_dpy, pixmap, d, gc, 0, 0, rimg->width, rimg->height, 0, 0);
        XFreePixmap(ds_dpy, pixmap);

        RDestroyImage(rimg);
    }
}
 
#endif /* USE_FREETYPE */

/* core */

void
destroyDrawString (proplist_t pl, void **init_data) {
    if (strcmp(PLGetString(PLGetArrayElement(pl, 2)), "drawPlainString") == 0) 
        destroyDrawPlainString((Display *)init_data[0], (Colormap)init_data[1]);
    else if (strcmp(PLGetString(PLGetArrayElement(pl, 2)), "drawFreeTypeString") == 0) 
        destroyDrawFreeTypeString(pl, init_data);
}

void
initDrawString (proplist_t pl, void **init_data) {
    _debug("invoke initDrawString: %s\n", PLGetString(PLGetArrayElement(pl, 2)));
    if (strcmp(PLGetString(PLGetArrayElement(pl, 2)), "drawPlainString") == 0) 
        initDrawPlainString((Display *)init_data[0], (Colormap)init_data[1]);
#ifdef USE_FREETYPE
    else if (strcmp(PLGetString(PLGetArrayElement(pl, 2)), "drawFreeTypeString") == 0) 
        initDrawFreeTypeString(pl, init_data);
#endif
    _debug("finish initDrawString\n");
}

