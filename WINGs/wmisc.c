
#include "WINGsP.h"

#include <ctype.h>

static void curve_rectangle(cairo_t *cr,
		double x0, double y0, double rect_width, double rect_height,
		double radius)
{
	double x1,y1;

	x1=x0+rect_width;
	y1=y0+rect_height;
	if (!rect_width || !rect_height)
		return;
	if (rect_width/2<radius) {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	} else {
		if (rect_height/2<radius) {
			cairo_move_to  (cr, x0, (y0 + y1)/2);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
		} else {
			cairo_move_to  (cr, x0, y0 + radius);
			cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
			cairo_line_to (cr, x1 - radius, y0);
			cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
			cairo_line_to (cr, x1 , y1 - radius);
			cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
			cairo_line_to (cr, x0 + radius, y1);
			cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
		}
	}
	cairo_close_path (cr);
}

void W_DrawButtonRelief(W_Screen *scr, cairo_t *cairo, int x, int y, unsigned int width, unsigned int height,
		WMReliefType relief)
{
	unsigned char border[4]= {0x00, 0x00, 0x00, 0x70};
	//unsigned char color1[4]= {0x8c, 0xb1, 0xbc, 0xff};
	//unsigned char color2[4]= {0xcb, 0xf3, 0xff, 0xff};
	unsigned char color1[4]= {0x0, 0x0, 0x0, 0xff};
	unsigned char color2[4]= {0xcf, 0xcf, 0xcf, 0xff};
	unsigned char scolor1[4]= {0xff, 0xff, 0xff, 0xe5};
	unsigned char scolor2[4]= {0xff, 0xff, 0xff, 0x70};
	cairo_pattern_t *shine;
	cairo_pattern_t *base;

	x+=1;
	y+=1;
	width-=2;
	height-=2;

	cairo_save(cairo);

	shine= cairo_pattern_create_linear(0, 0, 0, height*2/5);
	cairo_pattern_add_color_stop_rgba(shine, 0,
			scolor1[0]/255.0, scolor1[1]/255.0, scolor1[2]/255.0,
			scolor1[3]/255.0);
	cairo_pattern_add_color_stop_rgba(shine, 1,
			scolor2[0]/255.0, scolor2[1]/255.0, scolor2[2]/255.0,
			scolor2[3]/255.0);

	base= cairo_pattern_create_linear(0, 0, 0, height-1);
	cairo_pattern_add_color_stop_rgba(base, 0,
			color1[0]/255.0, color1[1]/255.0, color1[2]/255.0,
			color1[3]/255.0);
	cairo_pattern_add_color_stop_rgba(base, 1,
			color2[0]/255.0, color2[1]/255.0, color2[2]/255.0,
			color2[3]/255.0);


	curve_rectangle(cairo, x, y, width-1, height-1, height*2/3);
	cairo_set_source(cairo, base);
	cairo_fill_preserve(cairo);
	cairo_clip(cairo);

	curve_rectangle(cairo, x, y, width-1, height*2/5, width);
	cairo_set_source(cairo, shine);
	cairo_fill(cairo);

	curve_rectangle(cairo, x, y, width-1, height-1, height*2/3);
	cairo_set_source_rgba(cairo, border[0]/255.0, border[1]/255.0, border[2]/255.0, border[3]/255.0);
	cairo_set_line_width(cairo, 2.0);
	cairo_stroke(cairo);

	cairo_pattern_destroy(shine);
	cairo_pattern_destroy(base);

	cairo_restore(cairo);
}

void W_DrawRelief(W_Screen *scr, cairo_t *cairo, int x, int y, unsigned int width, unsigned int height,
		WMReliefType relief)
{
	WMColorSpec b;
	WMColorSpec w;
	WMColorSpec d;
	WMColorSpec l;

	switch (relief) {
		case WRSimple:
			WMColorSpecSet(cairo, &b);
			cairo_rectangle(cairo, x, y, width-1, height-1);
			cairo_stroke(cairo);
			return;

		case WRRaised:
			b= WMBlackColorSpec();
			w= WMWhiteColorSpec();
			d= WMDarkGrayColorSpec();
			l= WMGrayColorSpec();
			break;

		case WRSunken:
			l= WMBlackColorSpec();
			b= WMWhiteColorSpec();
			w= WMDarkGrayColorSpec();
			d= WMGrayColorSpec();
			break;

		case WRPushed:
			l= w= WMBlackColorSpec();
			d= b= WMWhiteColorSpec();
			break;

		case WRRidge:
			l= b= WMDarkGrayColorSpec();
			d= w= WMWhiteColorSpec();
			break;

		case WRGroove:
			w= d= WMDarkGrayColorSpec();
			l= b= WMWhiteColorSpec();
			break;

		default:
			return;
	}
	/* top left */
	WMColorSpecSet(cairo, &w);
	cairo_move_to(cairo, x, y);
	cairo_line_to(cairo, x+width-1, y);
	cairo_stroke(cairo);
	if (width > 2 && relief != WRRaised && relief!=WRPushed) {
		WMColorSpecSet(cairo, &l);
		cairo_move_to(cairo, x+1, y+1);
		cairo_line_to(cairo, x+width-3, y+1);
		cairo_stroke(cairo);
	}

	WMColorSpecSet(cairo, &w);
	cairo_move_to(cairo, x, y);
	cairo_line_to(cairo, x, y+height-1);
	cairo_stroke(cairo);
	if (height > 2 && relief != WRRaised && relief!=WRPushed) {
		WMColorSpecSet(cairo, &l);
		cairo_move_to(cairo, x+1, y+1);
		cairo_line_to(cairo, x+1, y+height-3);
		cairo_stroke(cairo);
	}

	/* bottom right */
	WMColorSpecSet(cairo, &b);
	cairo_move_to(cairo, x, y+height-1);
	cairo_line_to(cairo, x+width-1, y+height-1);
	cairo_stroke(cairo);
	if (width > 2 && relief!=WRPushed) {
		WMColorSpecSet(cairo, &d);
		cairo_move_to(cairo, x+1, y+height-2);
		cairo_line_to(cairo, x+width-2, y+height-2);
		cairo_stroke(cairo);
	}

	WMColorSpecSet(cairo, &b);
	cairo_move_to(cairo, x+width-1, y);
	cairo_line_to(cairo, x+width-1, y+height-1);
	cairo_stroke(cairo);
	if (height > 2 && relief!=WRPushed) {
		WMColorSpecSet(cairo, &d);
		cairo_move_to(cairo, x+width-2, y+1);
		cairo_line_to(cairo, x+width-2, y+height-2);
		cairo_stroke(cairo);
	}
}

static int findNextWord(const char *text, int limit)
{
	int pos, len;

	//XXX this is broken
	len = strcspn(text, " \t\n\r");
	pos = len + strspn(text + len, " \t\n\r");
	if (pos > limit)
		pos = limit;

	return pos;
}

static int fitText(const char *text, WMFont * font, int width, int wrap)
{
	int i, w, beforecrlf, word1, word2;

	/* text length before first cr/lf */
	beforecrlf = strcspn(text, "\n");

	if (!wrap || beforecrlf == 0)
		return beforecrlf;

	//XXX w = WMWidthOfString(font, text, beforecrlf);
	if (w <= width) {
		/* text up to first crlf fits */
		return beforecrlf;
	}

	word1 = 0;
	while (1) {
		word2 = word1 + findNextWord(text + word1, beforecrlf - word1);
		if (word2 >= beforecrlf)
			break;
		//XXXw = WMWidthOfString(font, text, word2);
		if (w > width)
			break;
		word1 = word2;
	}

	for (i = word1; i < word2; i++) {
		//XXXw = WMWidthOfString(font, text, i);
		if (w > width) {
			break;
		}
	}

	/* keep words complete if possible */
	if (!isspace(text[i]) && word1 > 0) {
		i = word1;
	} else if (isspace(text[i]) && i < beforecrlf) {
		/* keep space on current row, so new row has next word in column 1 */
		i++;
	}

	return i;
}

#ifdef OLD_CODE
static int fitText(char *text, WMFont * font, int width, int wrap)
{
	int i, j;
	int w;

	if (text[0] == 0)
		return 0;

	i = 0;
	if (wrap) {
		if (text[0] == '\n')
			return 0;

		do {
			i++;
			w = WMWidthOfString(font, text, i);
		} while (w < width && text[i] != '\n' && text[i] != 0);

		if (text[i] == '\n')
			return i;

		/* keep words complete */
		if (!isspace(text[i])) {
			j = i;
			while (j > 1 && !isspace(text[j]) && text[j] != 0)
				j--;
			if (j > 1)
				i = j;
		}
	} else {
		i = strcspn(text, "\n");
	}
	return i;
}
#endif

int W_GetTextHeight(cairo_t *cairo, WMFont *font, const char *text, int width, int wrap)
{
	const char *ptr = text;
	int count;
	int length = strlen(text);
	int h;
	int fheight = WMFontHeight(font);

	h = 0;
	while (length > 0) {
		count = fitText(ptr, font, width, wrap);

		h += fheight;

		if (isspace(ptr[count]))
			count++;

		ptr += count;
		length -= count;
	}
	return h;
}

void W_PaintText(cairo_t *cairo, WMFont *font,  int x, int y,
		int width, WMAlignment alignment, WMColorSpec *color,
		int wrap, const char *text)
{
	const char *ptr = text;
	int length= strlen(ptr);
	int line_width;
	int line_x= 0;
	int count;
	int fheight = WMFontHeight(font);

	line_x= x + (width - WMWidthOfString(font, ptr))/2;
	WMDrawString(cairo, color, font, line_x, y, ptr);
	return;

	while (length > 0) {
		count = fitText(ptr, font, width, wrap);

		//XXX        line_width = WMWidthOfString(font, ptr, count);
		if (alignment == WALeft)
			line_x = x;
		else if (alignment == WARight)
			line_x = x + width - line_width;
		else
			line_x = x + (width - line_width) / 2;

		WMDrawString(cairo, color, font, line_x, y, ptr);

		if (wrap && ptr[count] != '\n')
			y += fheight;

		while (ptr[count] && ptr[count] == '\n') {
			y += fheight;
			count++;
		}

		ptr += count;
		length -= count;
	}
}

void W_PaintTextAndImage(W_Screen *screen, cairo_t *cairo, W_View *view, int wrap, WMColorSpec *textColor, W_Font *font,
		WMReliefType relief, const char *text,
		WMAlignment alignment,  WMImage *image,
		WMImagePosition position, WMColorSpec *backColor, int ofs)
{
	int ix, iy;
	int x, y, w, h;

	cairo_save(cairo);

	if (backColor)
	{
		cairo_rectangle(cairo, 0, 0, view->size.width, view->size.height);
		WMColorSpecSet(cairo, backColor);
		cairo_fill(cairo);
	}
	if (relief == WRFlat) {
		x = 0;
		y = 0;
		w = view->size.width;
		h = view->size.height;
	} else {
		x = 1;
		y = 1;
		w = view->size.width - 3;
		h = view->size.height - 3;
	}

	/* calc. image alignment */
	if (position != WIPNoImage && image != NULL) {
		switch (position) {
		case WIPOverlaps:
		case WIPImageOnly:
			ix = (view->size.width - WMGetImageWidth(image)) / 2;
			iy = (view->size.height - WMGetImageHeight(image)) / 2;
			/*
			   x = 2;
			   y = 0;
			 */
			break;

		case WIPLeft:
			ix = x;
			iy = y + (h - WMGetImageHeight(image)) / 2;
			x = x + WMGetImageWidth(image) + 5;
			y = 0;
			w -= WMGetImageWidth(image) + 5;
			break;

		case WIPRight:
			ix = view->size.width - WMGetImageWidth(image) - x;
			iy = y + (h - WMGetImageHeight(image)) / 2;
			w -= WMGetImageWidth(image) + 5;
			break;

		case WIPBelow:
			ix = (view->size.width - WMGetImageWidth(image)) / 2;
			iy = h - WMGetImageHeight(image);
			y = 0;
			h -= WMGetImageHeight(image);
			break;

		default:
		case WIPAbove:
			ix = (view->size.width - WMGetImageWidth(image)) / 2;
			iy = y;
			y = WMGetImageHeight(image);
			h -= WMGetImageHeight(image);
			break;
		}

		ix += ofs;
		iy += ofs;

		//XXX        XSetClipOrigin(screen->display, screen->clipGC, ix, iy);
		//XXX        XSetClipMask(screen->display, screen->clipGC, image->mask);
		/*XXX
		  if (image->depth==1)
		  XCopyPlane(screen->display, image->pixmap, d, screen->clipGC,
		  0, 0, WMGetImageWidth(image), WMGetImageHeight(image), ix, iy, 1);
		  else
		  XCopyArea(screen->display, image->pixmap, d, screen->clipGC,
		  0, 0, WMGetImageWidth(image), WMGetImageHeight(image), ix, iy);
		  */
	}

	/* draw text */
	if (position != WIPImageOnly && text!=NULL) {
		int textHeight;

		textHeight = W_GetTextHeight(cairo, font, text, w-8, wrap);
		W_PaintText(cairo, font, x+ofs+4, y+ofs + (h-textHeight)/2, w-8,
				alignment, textColor, wrap, text);
	}

	/* draw relief */
	W_DrawRelief(screen, cairo, 0, 0, view->size.width, view->size.height, relief);
}

WMPoint wmkpoint(int x, int y)
{
	WMPoint point;

	point.x = x;
	point.y = y;

	return point;
}

WMSize wmksize(unsigned int width, unsigned int height)
{
	WMSize size;

	size.width = width;
	size.height = height;

	return size;
}

WMRect wmkrect(int x, int y, unsigned int width, unsigned int height)
{
	WMRect rect;

	rect.pos.x = x;
	rect.pos.y = y;
	rect.size.width = width;
	rect.size.height = height;

	return rect;
}
