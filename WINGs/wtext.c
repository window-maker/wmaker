/* WMText: multi-line/font/color text widget for WINGs */
/* Copyleft (>) 1999, 2000 Nwanua Elumeze <nwanua@colorado.edu> */


/*           .( * .
          .*  .  ) .

         . . POOF .* .
          '* . (  .) '
     jgs   ` ( . *          */


/* if monoFont, ignore pixmaps, colors, fonts, script, underline */


//#include <WINGs.h>
#include <WMaker.h>
#include <WINGsP.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <ctype.h>

#if 0
#include "wruler.h"
#include "wtext.h"
#endif

void wgdbFree(void *ptr) 
{ if(!ptr) printf("err... cannot ");
printf("gdbFree [%p]\n", ptr);
wfree(ptr);
}


typedef enum {ctText=0, ctImage=1} ChunkType;
typedef enum { dtDelete=0, dtBackSpace } DeleteType;
typedef enum {wrWord=0, wrChar=1, wrNone=2} Wrapping;

/* Why singly-linked and not say doubly-linked?  	
   99% of the time (draw, append), the "prior" 
   member would have been a useless memory and CPU overhead, 
   and deletes _are_ relatively infrequent.  
   When the "prior" member needs to be used, the overhead of 
   doing things the hard way will be incurred... but seldomly. */


/* a Chunk is a singly-linked list of chunks containing: 
    o text with a given format
    o or an image 
    o but NOT both */
typedef struct _Chunk {
	char *text;			/* the text in the chunk */
	WMPixmap *pixmap;	/* OR the pixmap it holds */
	short chars; 		/* the number of characters in this chunk */
	short mallocedSize;	/* the number of characters that can be held */

	WMFont *font;		/* the chunk's font */
	WMColor *color;		/* the chunk's color */
	short ul:1;			/* underlined or not */
	ChunkType type:1;	/* a "Text" or "Image" chunk */
	short script:4;		/* script in points: negative for subscript */
//hrmm selec...
	ushort selected;
	ushort sStart;
	ushort sEnd;
	ushort RESERVED:10; 
	struct _Chunk *next;/*the next member in this list */

}Chunk;
	

	
/* a Paragraph is a singly-linked list of paragraphs containing:
    o a list of chunks in that paragraph
    o the formats for that paragraph 
    o its (draw) position relative to the entire document */
typedef struct _Paragraph {
	Chunk *chunks;		/* the list of text and/or image chunks */
	short fmargin;		/* the start position of the first line */
	short bmargin;		/* the start positions of the rest of the lines */
	short rmargin;		/* the end position of the entire paragraph */
	short numTabs;		/* the number of tabstops */
	short *tabstops;	/* an array of tabstops */
	
	Pixmap drawbuffer;	/* the pixmap onto which the (entire) 
						       paragraph will be drawn */
	WMPixmap *bulletPix;/* the pixmap to use for bulleting */
	int top;			/* the top of the paragraph relative to document */
	int bottom;		/* the bottom of the paragraph relative to document */
	int width;		/* the width of the paragraph */
	int height;		/* the height of the paragraph */
	WMAlignment align:2;/* justification of this paragraph */
	ushort RESERVED:14; 

	struct _Paragraph *next; /* the next member in this list */
} Paragraph;


static char *default_bullet[] = {
"6 6 4 1",
"   c None s None", ".  c black",
"X  c white", "o  c #808080",
" ...  ",
".XX.. ",
".XX..o",
".....o",
" ...oo",
"  ooo "};

/* this is really a shrunk down version of the original
"broken" icon... I did not draw it, I simply shrunk it */
static char * unk_xpm[] = {
"24 24 17 1",
"   c None", ".  c #0B080C", "+  c #13A015", "@  c #5151B8",
"#  c #992719", "$  c #5B1C20", "%  c #1DF51D", "&  c #D1500D", "*  c #2F304A",
"=  c #0C6A0C", "-  c #F2F1DE", ";  c #D59131", ">  c #B2B083", ",  c #DD731A",
"'  c #CC3113", ")  c #828238", "!  c #6A6A94",
"......!@@@@@@@....$$....",
"...@!@@@@@@@**...$#'....",
"..!!@@@@@@@@.......#....",
"..!@@@@@@@@@*.......$...",
".!@@@#,,#*@@*..*>.*.#...",
"*@@@@#'',,@@@...---!....",
"!@@@@@*.#;*@@..!--->....",
"@@@@@@@@#,.@@..!----@...",
"!@@@@@@*#;'$...!----@...",
"*@@@@@@..'&;;#.)----)...",
".@@@@@@..$..&'.>----)...",
".@@@@@@**---,'>-----!...",
".@@@@@@**---,'>-----@...",
"..@@@@@@@---;;;,;---....",
"..*@@@@*@--->#',;,-*.)..",
"........)---->)@;#!..>..",
".....)----------;$..>)..",
"=%%%*.*!-------);..)-*..",
"=%%%%+...*)>!@*$,.>--...",
"*+++++++.......*$@-->...",
"............**@)!)>->...",
"........................",
"........................",
"........................"};

typedef struct W_Text {   
	W_Class widgetClass;	/* the class number of this widget */
	W_View  *view;			/* the view referring to this instance */
	WMColor *bg;			/* the background color to use when drawing */
	
	WMRuler *ruler;			/* the ruler subwiget to maipulate paragraphs */

	WMScroller *hscroller;	/* the horizontal scroller */
	short hpos;				/* the current horizontal position */
	short prevHpos;			/* the previous horizontal position */

	WMScroller *vscroller;	/* the vertical scroller */
	int vpos;				/* the current vertical position */
	int prevVpos;			/* the previous vertical position */

	int visibleW;			/* the actual horizontal space available */
	int visibleH;			/* the actual vertical space available */

	Paragraph *paragraphs;	/* the linked list of the paragraphs in the doc. */
	int docWidth;			/* the width of the entire document */
	int docHeight;		/* the height of the entire document */

    WMFont *dFont;			/* the default font */ 
    WMColor *dColor;		/* the default color */
	WMPixmap *dBulletPix; 	/* the default pixmap for bullets */
	WMPixmap *dUnknownImg; 	/* the pixmap for (missing/broken) images */

	WMRect sRect; 			/* the selected area */
	Paragraph *currentPara;	/* the current paragraph, in which actions occur */
	Chunk *currentChunk;	/* the current chunk, about which actions occur */
	short tpos;				/* the cursor position (text position) */
	WMParseAction *parser;	/* what action to use to parse input text */
	WMParseAction *writer;	/* what action to use to write text */
	WMParserActions funcs;	/* the "things" that parsers/writers might do */ 
	XPoint clicked;			/* the position of the last mouse click */
	XPoint cursor;			/* where the cursor is "placed" */
	short clheight;			/* the height of the "line" clicked on */
	short clwidth;			/* the width of the "line" clicked on */

	WMReliefType relief:2;	/* the relief to display with */
	Wrapping wrapping:2;	/* the type of wrapping to use in drawing */
	WMAlignment dAlignment:2;/* default justification */
	ushort monoFont:1;		/* whether to ignore "rich" commands */
	ushort fixedPitch:1;	/* assume each char in dFont is the same size */
	ushort editable:1;		/* whether to accept user changes or not*/
	ushort rulerShown:1;	/* whether the ruler is shown or not */
	ushort cursorShown:1;	/* whether the cursor is currently being shown */
	ushort frozen:1;		/* whether screen updates are to be made */
	ushort focused:1;		/* whether this instance has input focus */
	ushort pointerGrabbed:1;/* whether this instance has the pointer */
	ushort buttonHeld:1;	/* the user is still holding down the button */
	ushort ignoreNewLine:1;	/* whether to ignore the newline character */
	ushort waitingForSelection:1;	/* whether there is a pending paste event */
	ushort ownsSelection:1; /* whether it ownz the current selection */
	ushort findingClickPoint:1;/* whether the search for a clickpoint is on */
	ushort foundClickPoint:1;/* whether the clickpoint has been found */
	ushort hasVscroller:1;	/* whether to enable the vertical scroller */
	ushort hasHscroller:1;	/* whether to enable the horizontal scroller */
	ushort RESERVED:10; 
} Text;
	
	

/* --------- static functions that are "private". don't touch :-) --------- */


/* max "characters per chunk that will be drawn at a time" */
#define MAX_WORD_LENGTH 100
/* max on a line */
#define MAX_CHUNX 64
#define MIN_DOC_WIDTH 200
typedef struct _LocalMargins {
    short left, right, first, body;
 } LocalMargins;

typedef struct _MyTextItems {
    char text[MAX_WORD_LENGTH+1];
    WMPixmap *pix;
    short chars;
    short x; 
    Chunk *chunk;/* used for "click" events */
    short start;	/* ditto... where in the chunk we start (ie. wrapped chunk) */
    ushort type:1;
	ushort RESERVED:15; 
} MyTextItems;


static WMRect 
chunkSelectionRect(Text *tPtr, Paragraph *para, MyTextItems item, 
	short y, short j, short lh)
{
	WMRect rect;
	short type=0;  /* 0:none 1:partial: 2:all */
	short rh=(tPtr->rulerShown)?45:5;
	short w, lm;
	WMFont *font = (tPtr->monoFont || item.chunk->type != ctText)?
		tPtr->dFont:item.chunk->font;

	rect.pos.x = -23;
	if(y+para->top+rh > tPtr->sRect.pos.y+tPtr->sRect.size.height
		|| y+para->top+rh+lh < tPtr->sRect.pos.y) 
		return rect;
	
	if(item.chunk->type == ctText)
		w = WMWidthOfString(font, item.text, item.chars);
	else w = item.chunk->pixmap->width;

	if(y+para->top+rh >= tPtr->sRect.pos.y && (y+para->top+rh+lh 
				<=  tPtr->sRect.pos.y+tPtr->sRect.size.height))
				//&& item.x+j >= tPtr->sRect.pos.x+tPtr->sRect.size.width))  
		type = 2;
	else type = 1;


#if 0
	if(item.x+j >= tPtr->sRect.pos.x && 
		item.x+j+w < tPtr->sRect.pos.x+tPtr->sRect.size.width) 
		type = 2;

	if(type == 1 &&  y+para->top+rh+lh <= 
		 tPtr->sRect.pos.y+tPtr->sRect.size.height)
		type = 2;
#endif


	if(type == 1 && item.chunk->type == ctText) {  /* partial coverage */
 		lm = 2+WMGetRulerMargin(tPtr->ruler, WRulerDocLeft);
			/* even I am still confused, so don't ask please */
		if(		(item.x+j+lm >= tPtr->sRect.pos.x &&
					item.x+j+lm <= tPtr->sRect.pos.x+tPtr->sRect.size.width)
			||	(item.x+j+lm >= tPtr->sRect.pos.x+tPtr->sRect.size.width
					&&	y+para->top+rh+lh <= 
						tPtr->sRect.pos.y+tPtr->sRect.size.height) 
			||	(tPtr->sRect.pos.y < y+para->top+rh 
					&&	tPtr->sRect.pos.x+tPtr->sRect.size.width >
						item.x+j+lm)   ){
			rect.size.width = w;
			rect.pos.x = item.x+j;
			item.chunk->selected = True;
			if(item.chunk->chars > 6) {
			item.chunk->sStart = 3;
			item.chunk->sEnd = item.chunk->chars;
			} else {
			item.chunk->sStart = 0;
			item.chunk->sEnd = item.chunk->chars;
			}
		}
	} else if(type == 2) {
		rect.pos.x = item.x+j;
		item.chunk->selected = True;
		if(item.chunk->type == ctText) {
			item.chunk->sStart = 0;
			item.chunk->sStart = item.chunk->chars;
			rect.size.width = WMWidthOfString(font,
				item.text, item.chars);
		} else {
			rect.size.width = item.chunk->pixmap->width;
		}
	}

	rect.pos.y = y;
	rect.size.height = lh;
	return rect;
}
	
static int
myDrawText(Text *tPtr, Paragraph *para, MyTextItems *items,
    short nitems, short pwidth, int y, short draw, short spacepos)
{   
	short i, ul_thick, u, j=0;  /* j = justification */
	short line_width = 0, line_height=0, mx_descent=0;
	WMScreen *screen = tPtr->view->screen;
	GC gc;
	WMFont *font;

	if(tPtr->findingClickPoint && tPtr->foundClickPoint) return 0;
	for(i=0; i<nitems; i++) {
		if(items[i].type == ctText) {
			font = (tPtr->monoFont)?tPtr->dFont:items[i].chunk->font;
			mx_descent = WMIN(mx_descent, -font->y); 
			line_height = WMAX(line_height, font->height);
			//printf("chunk.x %d  xpoint.x %d\n",
			//	items[i].x, tPtr->clicked.x);
				
			line_width += WMWidthOfString(font,	
				 items[i].text, items[i].chars);
		 } else {
			mx_descent = WMIN(mx_descent, -(items[i].pix->height-3));
/* replace -3 wif descent... */
			line_height = WMAX(line_height, items[i].pix->height);
			if(para->align == WARight || para->align == WACenter) {
				line_width += items[i].pix->width;
			} } }   

	if(para->align == WARight) {
		j = pwidth - line_width;
	} else if (para->align == WACenter) {
		j = (short) ((float)(pwidth - line_width))/2.0;
	}   

	if(tPtr->findingClickPoint && (y+line_height >= tPtr->clicked.y)) { 
		tPtr->foundClickPoint = True;
		tPtr->currentChunk = items[0].chunk;  /* just first on this "line" */
		tPtr->tpos = items[0].start; /* where to "start" counting from */
		tPtr->clicked.x = j+items[0].x;
		tPtr->clicked.y = y+line_height+mx_descent;
		tPtr->clheight = line_height; /* to draw the cursor */
		tPtr->clwidth = line_width; /* where to stop searching */
		return 0;
	} if(!draw) return line_height;
        
	for(i=0; i<nitems; i++) {

	//account for vpos
	if(tPtr->ownsSelection) {
		WMRect rect = chunkSelectionRect(tPtr, para,
			items[i], y, j, line_height);
		if(rect.pos.x != -23) { /* has been selected */
			XFillRectangle(tPtr->view->screen->display, para->drawbuffer,
				WMColorGC(WMGrayColor(tPtr->view->screen)),
				rect.pos.x, rect.pos.y, rect.size.width, rect.size.height);
		}
	}

	if(items[i].type == ctText) {
	gc = WMColorGC(items[i].chunk->color);
	font = (tPtr->monoFont)?tPtr->dFont:items[i].chunk->font;
	WMDrawString(screen, para->drawbuffer, gc, font, 
		items[i].x+j, y - font->y - mx_descent,
		items[i].text, items[i].chars);
	if(items[i].chunk->ul && !tPtr->monoFont) {
		ul_thick = (short) ((float)font->height)/12.0;
         if (ul_thick < 1) ul_thick = 1;
		for(u=0; u<ul_thick; u++) {
        XDrawLine(screen->display, para->drawbuffer, gc, items[i].x+j,            
			y + 1 + u - mx_descent, 
			items[i].x + j + WMWidthOfString(font, 
			items[i].text, items[i].chars), y + 1 + u - mx_descent);
                       
		}
	} } else {        
		WMDrawPixmap(items[i].pix, para->drawbuffer, items[i].x+j,
		y + 3 - mx_descent - items[i].pix->height);
	} } 
    return line_height;
}   

static void
drawPChunkPart(Text *tPtr, Chunk *chunk, LocalMargins m, Paragraph *para, 
	MyTextItems *items, short *nitems, short *Lmargin, XPoint *where, short draw)
{   
	short p_width;
    
    if(!chunk) return;
	if(!chunk->pixmap) 
		chunk->pixmap = WMRetainPixmap(tPtr->dUnknownImg);
    
	p_width = m.right - WMIN(m.first, m.body) - WMGetRulerOffset(tPtr->ruler);
	if(p_width < MIN_DOC_WIDTH) // need WMRuler to take care of this...
		return;
	if(where->x + chunk->pixmap->width <= p_width - *Lmargin) {
	/* it can fit on rest of line */
		items[*nitems].pix = chunk->pixmap;
		items[*nitems].type = ctImage;
		items[*nitems].chars = 0;
		items[*nitems].x = *Lmargin+where->x;
		items[*nitems].chunk = chunk;
		items[*nitems].start = 0;
 
		if(*nitems >= MAX_CHUNX) {
			items[*nitems].chars = 0;
			items[*nitems].x = *Lmargin+where->x;
			items[*nitems].chunk = chunk;
			items[*nitems].start = 0;
			where->y += myDrawText(tPtr, para, items, *nitems+1,
				p_width-*Lmargin, where->y, draw, 0);
			if(tPtr->findingClickPoint && tPtr->foundClickPoint) return;
			*nitems = 0;    
			where->x = 0;   
		} else {
			(*nitems)++;
			where->x += chunk->pixmap->width;
		}   
	 } else if(chunk->pixmap->width <= p_width - *Lmargin) {
	/* it can fit on an entire line, flush the myDrawText then wrap it */
		where->y += myDrawText(tPtr, para, items, *nitems+1,
			p_width-*Lmargin, where->y, draw, 0);
		*nitems = 0;
		*Lmargin = WMAX(0, m.body - m.first);
		where->x = 0;
		drawPChunkPart(tPtr, chunk, m, para, items, nitems, 
			Lmargin, where, draw);
	} else {
#if 1
		*nitems = 0;
		where->x = 0;
		*Lmargin = WMAX(0, m.body - m.first);
		items[*nitems].pix = chunk->pixmap;
		items[*nitems].type = ctImage;
		items[*nitems].chars = 0;
		items[*nitems].x = *Lmargin+where->x;
		items[*nitems].chunk = chunk;
		items[*nitems].start = 0;
		where->y += myDrawText(tPtr, para, items, *nitems+1,
			p_width-*Lmargin, where->y, draw, 0);
#endif

		/* scale image to fit, call self again */
		/* deprecated - the management */
	}       
    
}   

static void
drawTChunkPart(Text *tPtr, Chunk *chunk, char *bufr, LocalMargins m, 
	Paragraph *para, MyTextItems *items, short *nitems, short len, short start, 
	short *Lmargin, XPoint *where, short draw, short spacepos)
{   
    short t_chunk_width, p_width, chars;
	WMFont *font = (tPtr->monoFont)?tPtr->dFont:chunk->font;
/* if(doc->clickstart.yes && doc->clickstart.done) return; */
        
	if(len==0) return;
	p_width = m.right - WMIN(m.first, m.body);
	if(p_width < MIN_DOC_WIDTH) // need WMRuler to take care of this...
		return;
    

	t_chunk_width = WMWidthOfString(font, bufr, len);
	if((where->x + t_chunk_width <= p_width - *Lmargin)
		|| (tPtr->wrapping == wrNone)) {
		/* if it can fit on rest of line, append to line */
		chars = WMIN(len, MAX_WORD_LENGTH);
		snprintf(items[*nitems].text, chars+1, "%s", bufr);
		items[*nitems].chars = chars;
		items[*nitems].x = *Lmargin+where->x;
		items[*nitems].type = ctText;
		items[*nitems].chunk = chunk;
		items[*nitems].start = start;
        
		if(*nitems >= MAX_CHUNX) {
			chars  = WMIN(len, MAX_WORD_LENGTH);
			snprintf(items[*nitems].text, chars+1, "%s", bufr);
			items[*nitems].chars = chars;
			items[*nitems].x = *Lmargin+where->x;
			items[*nitems].type = ctText;
			items[*nitems].chunk = chunk;
			items[*nitems].start = start;
			where->y += myDrawText(tPtr, para, items, *nitems+1, 
				p_width-*Lmargin, where->y, draw, spacepos);
			if(tPtr->findingClickPoint && tPtr->foundClickPoint) return;
			*nitems = 0;
			where->x = 0;
		} else { 
			(*nitems)++;
			where->x += t_chunk_width;
		}   
	} else if(t_chunk_width <= p_width - *Lmargin) {
	/* it can fit on an entire line, flush and wrap it to a new line */
		where->y += myDrawText(tPtr, para, items, *nitems, 
			p_width-*Lmargin, where->y, draw, spacepos);
		if(tPtr->findingClickPoint && tPtr->foundClickPoint) return;
		*nitems = 0;
		*Lmargin = WMAX(0, m.body - m.first);
		where->x = 0;
		drawTChunkPart(tPtr, chunk, bufr, m, para, items, nitems, 
			len, start, Lmargin, where, draw, spacepos);
	} else { 
		/* otherwise, chop line, call ourself recursively until it's all gone */
		short J=0; /* bufr */
		short j=0; /* local tmp buffer */
		char tmp[len];
		short diff = p_width - *Lmargin - where->x;
		short _start=0;

		if(diff < 20) {
			where->y += myDrawText(tPtr, para,  items, *nitems,
				p_width-*Lmargin, where->y, draw, spacepos);
			if(tPtr->findingClickPoint && tPtr->foundClickPoint) return;
			*nitems = 0;
			*Lmargin = WMAX(0, m.body - m.first);
			where->x = 0;
			diff = p_width - *Lmargin - where->x;
		}   

		for(J=0; J<len; J++) {
			tmp[j] = bufr[J];
			if(WMWidthOfString(font, tmp, j+1) > diff) {
				drawTChunkPart(tPtr, chunk, tmp, m, para, items, nitems, 
					j, start+_start, Lmargin, where, draw, spacepos);
				_start = J;
				J--; j=0;
			} else j++; 
		}   
		/* and there's always that last chunk, get it too */
		drawTChunkPart(tPtr, chunk, tmp, m, para, items, nitems, 
			j, start+_start, Lmargin, where, draw, spacepos);
	}
}   

/*  this function does what it's called :-) 
 o  It is also used for calculating extents of para, 
      (returns height) so watch out for (Bool) draw 
 o  Also used to determine where mouse was clicked */
static int
putParagraphOnPixmap(Text *tPtr, Paragraph *para, Bool draw)
{ 
	char bufr[MAX_WORD_LENGTH+1];  /* a single word + '\0' */
	MyTextItems items[MAX_CHUNX+1];
    short lmargin, spacepos, i, s, nitems, start;
    LocalMargins m;
    XPoint where;
	Chunk *chunk;
    
	if(!tPtr->view->flags.realized || !para) return 0;
        
	where.x = 0, where.y =0, nitems = 0;
	m.left =  WMGetRulerMargin(tPtr->ruler, WRulerDocLeft);
	m.right = WMGetRulerMargin(tPtr->ruler, WRulerRight) - m.left;
	m.first = para->fmargin, m.body = para->bmargin;
    
	if(draw) {
		W_Screen *screen = tPtr->view->screen;
		if(para->drawbuffer) 
			XFreePixmap(screen->display, para->drawbuffer);
		if(para->width<2*tPtr->dFont->height) para->width = 2*tPtr->dFont->height;
		if(para->height<tPtr->dFont->height) para->height = tPtr->dFont->height;
		para->drawbuffer = XCreatePixmap(screen->display, 
			tPtr->view->window, para->width, para->height, screen->depth);
		XFillRectangle(screen->display, para->drawbuffer, 
			WMColorGC(tPtr->bg), 0, 0, para->width, para->height);

	}

	//if(para->align != tPtr->dAlignment)
	//	para->align = tPtr->dAlignment;

	/* draw the bullet if appropriate */
	if(m.body>m.first && !tPtr->monoFont) {
		lmargin = m.body - m.first;
		if(draw) { 
			if(para->bulletPix) 
				WMDrawPixmap(para->bulletPix, para->drawbuffer, lmargin-10, 5);
			else
				WMDrawPixmap(tPtr->dBulletPix, para->drawbuffer, lmargin-10, 5);
		}
		/* NeXT sez next tab, I say the m.body - m.first margin */
	} else {
		lmargin = WMAX(0, m.first - m.body);
	}	
	
	if(tPtr->findingClickPoint && !para->chunks) {
		tPtr->currentChunk = NULL;
		tPtr->foundClickPoint = True;
		tPtr->tpos = 0;
		tPtr->clicked.x = lmargin;
		tPtr->clicked.y = 5;
		tPtr->clheight = para->height;
		tPtr->clwidth = 0;
		return 0;
	}
    
	chunk = para->chunks;
	while(chunk) {

		if(tPtr->findingClickPoint && tPtr->foundClickPoint) return 0;

		if(chunk->type == ctImage && !tPtr->monoFont ) {
			drawPChunkPart(tPtr, chunk, m, para, items, &nitems,
				&lmargin, &where, draw);
		} else if(chunk->text && chunk->type == ctText) {
			if(tPtr->wrapping == wrNone) {
			drawTChunkPart(tPtr, chunk, chunk->text, m, para, items, &nitems, 
				chunk->chars, 0, &lmargin, &where, draw, spacepos);
			} else if(tPtr->wrapping == wrWord) {
				spacepos=0, i=0, start=0;
				while(spacepos < chunk->chars) {
					bufr[i] = chunk->text[spacepos];
					if( bufr[i] == ' ' || i >= MAX_WORD_LENGTH ) {
						if(bufr[i] == ' ') s=1; else s=0;
							drawTChunkPart(tPtr, chunk, bufr, m, para, 
								items, &nitems, i+s, start, &lmargin, &where,
								draw, spacepos);
						start = spacepos+s;
						if(i > MAX_WORD_LENGTH-1) 
							spacepos--;
						i=0;
					} else i++;
					spacepos++;
			}	
			/* catch that last onery one. */
			drawTChunkPart(tPtr, chunk, bufr, m, para, 
				items, &nitems, i, start, &lmargin, &where, draw, spacepos);
		} } chunk = chunk->next;
    }   
	/* we might have a few leftover items that need drawing */
	if(nitems >0) { 
		where.y += myDrawText(tPtr, para, items, 
			nitems, m.right-m.left-lmargin, where.y, draw, spacepos);
		if(tPtr->findingClickPoint && tPtr->foundClickPoint) return 0;
	}        
	return where.y;
}

static int 
calcParaExtents(Text *tPtr, Paragraph *para)
{   
	if(!para) return 0;
    
	if(tPtr->monoFont) {
		para->width = tPtr->visibleW;
		para->fmargin = 0;
		para->bmargin = 0;
		para->rmargin = tPtr->visibleW;
	} else {
		para->width = WMGetRulerMargin(tPtr->ruler, WRulerRight) -
			WMIN(para->fmargin, para->bmargin)
			- WMGetRulerOffset(tPtr->ruler);
	}
              
	if(!para->chunks) 
		para->height = tPtr->dFont->height;
	else 
		para->height = putParagraphOnPixmap(tPtr, para, False);
	
	if(para->height<tPtr->dFont->height) 
		para->height = tPtr->dFont->height;
	para->bottom = para->top + para->height;
	return para->height;
}


/* rather than bother with redrawing _all_ the pixmaps, simply 
   rearrange (i.e., push down or pull up) paragraphs after this one */
static void
affectNextParas(Text *tPtr, Paragraph *para, int move_y)
{
	Paragraph *next;
	int old_y = 0;
    
	if(!para || move_y==0) return;
	if(move_y == -23) {
		old_y = para->bottom;
		calcParaExtents(tPtr, para);
		old_y -= para->bottom;
		if(old_y == 0) return;
		move_y = -old_y;
	}if(move_y == 0) return;

	next = para->next;
	while(next) {
		next->top += move_y;
		next->bottom = next->top + next->height;
		next = next->next; // I know, I know 
	}tPtr->docHeight += move_y;

#if 0
	tPtr->vpos += move_y;
	if(tPtr->vpos < 0) tPtr->vpos = 0;
	if(tPtr->vpos > tPtr->docHeight - tPtr->visibleH) 
		tPtr->vpos = tPtr->docHeight - tPtr->visibleH;
#endif
	
}   


static void
calcDocExtents(Text *tPtr)
{   
	Paragraph *para;
	
	if(tPtr->monoFont) {
		tPtr->docWidth = tPtr->visibleW;
	} else {
		tPtr->docWidth = WMGetRulerMargin(tPtr->ruler, WRulerRight) -
		WMGetRulerMargin(tPtr->ruler, WRulerDocLeft);
	}
	tPtr->docHeight = 0;
	para = tPtr->paragraphs;
	if(para) {
		while(para) { 
			para->top = tPtr->docHeight; 
			tPtr->docHeight += calcParaExtents(tPtr, para);
			para->bottom = tPtr->docHeight;
			para = para->next;
		}   
	} else { /* default to this if no paragraphs */
		tPtr->docHeight = tPtr->dFont->height; 
	}   
#if 0
	if(tPtr->editable) /* add space at bottom to enter new stuff */
		tPtr->docHeight += tPtr->dFont->height; 
#endif
}       


/* If any part of a paragraph is viewable, the entire
paragraph is drawn on an otherwise empty (XFreePixmap) pixmap.
The actual viewable parts of the paragraph(s) are then pieced 
together via paintText:

    -------------------------------------------
    ||    this is a paragraph in this document||
    ||========================================||
    ||  | only part of it is visible though.  ||
    ||  |-------------------------------------||
    ||[.|     This is another paragraph       ||
    ||  | which I'll make relatively long     ||
    ||  | just for the sake of writing a long ||
    ||  | paragraph with a picture:   ^_^     ||
    ||  |-------------------------------------|| 
    ||==|     Of the three paragraphs, only   ||
    ||/\| the preceding was totally copied to ||
    ||\/| totally copied to the window, even  ||
    ==========================================||
          though they are all on pixmaps.
    -------------------------------------------
         This paragraph exists only in 
         memory and so has a NULL pixmap.
    -------------------------------------------


simple, right? Performance: the best of both worlds... 
   o  fast scrolling: no need to rewrite what's already 
         on the screen, simply XCopy it.
   o  fast typing: only change current para, then simply
         affect other (i.e., subsequent) paragraphs.  
   o  If no part of para is on screen, gdbFree pixmap; else draw on 
         individual pixmap per para then piece several paras together 
   o  Keep track of who to XCopy to window  (see paintText) */
static void
drawDocumentPartsOnPixmap(Text *tPtr, Bool all)
{
	Paragraph *para;
    
	para = tPtr->paragraphs;
	while(para) {
		/* the 32 reduces jitter on the human eye by preparing paragraphs
		in anticipation of when the _moving_ scrollbar reaches them */
		if(para->bottom + 32 < tPtr->vpos ||  
			para->top >  tPtr->visibleH + tPtr->vpos + 32 ) {
			if(para->drawbuffer) {
				XFreePixmap(tPtr->view->screen->display, para->drawbuffer);
				para->drawbuffer = (Pixmap) NULL;
			}   
		} else {
			if(!para->drawbuffer || all) 
				putParagraphOnPixmap(tPtr, para, True);
		}	   
		para = para->next;
	}   
}   



/* this function blindly copies the "visible" parts of a pragraph 
   unto the view, (top-down approach).  It starts drawing from 
   the top of the view (which paragraph to draw is determined by 
   drawDocumentPartsOnPixmap); it stops at the bottom of the view. */
static void 
paintText(Text *tPtr)
{   
	short lmargin, para_lmargin;
	int from=5, to=5, height;
	Paragraph *para;
	short vS=0, hS=0, rh=0;
	
	if(!tPtr->view->flags.realized) return;

	if(tPtr->rulerShown) rh = 40;
	to += rh;

	if(tPtr->hasVscroller) vS = 21;
	if(tPtr->hasHscroller) hS = 21;

//XClearWindow(tPtr->view->screen->display, tPtr->view->window);

	lmargin = WMGetRulerMargin(tPtr->ruler, WRulerDocLeft);
	if(tPtr->paragraphs) {
		para = tPtr->paragraphs;
		while(para) {
			if(para->drawbuffer) {
				from = (para->top<=tPtr->vpos)?tPtr->vpos-para->top:0;
				height = para->height - from; 
				if(from>=0 && height>0 ) {
					para_lmargin = WMIN(para->fmargin, para->bmargin);
				if(lmargin-vS<WMIN(para->fmargin, para->bmargin)) {
#if 0
					XClearArea(tPtr->view->screen->display, tPtr->view->window,
						lmargin, to, 2+para_lmargin, height, False);
#else
					XFillRectangle(tPtr->view->screen->display, tPtr->view->window,
						WMColorGC(tPtr->dColor), lmargin, to, 2+para_lmargin, height);
#endif
				}
					XCopyArea(tPtr->view->screen->display, para->drawbuffer,
						tPtr->view->window, WMColorGC(tPtr->bg), 0, from, 
						para->width-4, height, lmargin+para_lmargin+2, to);
					if( (to+=height) > tPtr->visibleH+rh)
						break;
			} 	}   
			para = para->next;
		}	}
        
	
#if 0
	/* clear any left over space (esp. during para deletes/ ruler changes) */
	if(tPtr->docHeight < tPtr->visibleH && tPtr->visibleH+rh+5-to>0) {
		XClearArea(tPtr->view->screen->display, tPtr->view->window, vS, to,
			tPtr->view->size.width-vS, tPtr->visibleH+rh+hS+5-to, False);
	}

	if(lmargin>vS) 
		XClearArea(tPtr->view->screen->display, tPtr->view->window, 
			vS+1, rh+5, lmargin-vS, tPtr->visibleH+rh+5-vS, False);
		

// from the "selection" days...
	W_DrawRelief(tPtr->view->screen, WMWidgetXID(tPtr),
		tPtr->sRect.pos.x, tPtr->sRect.pos.y,
		tPtr->sRect.size.width, tPtr->sRect.size.height, tPtr->relief);
#endif

	W_DrawRelief(tPtr->view->screen, WMWidgetXID(tPtr), 0, rh,
		tPtr->visibleW+vS, tPtr->visibleH+hS, tPtr->relief);        

	if(tPtr->editable && tPtr->clheight > 0) {
		int top = tPtr->cursor.y-tPtr->vpos;
		int bot = top+tPtr->clheight;
		if(bot>5) {
			if(top<5) top=5;
			if(bot>tPtr->visibleH+hS-2) bot = tPtr->visibleH+hS-2;
			if(bot-top>1) {
				//do something about italic text...
				XDrawLine(tPtr->view->screen->display, tPtr->view->window,
					WMColorGC(tPtr->dColor), lmargin+tPtr->cursor.x, top, 
					lmargin+tPtr->cursor.x, bot);
		} } }


}


/* called anytime either the ruler, vscroller or hscroller is hidden/shown,
 or when the widget is resized by some user action */ 
static void
resizeText(W_ViewDelegate *self, WMView *view)
{   
	Text *tPtr = (Text *)view->self;
	short rh=0;

    if(!tPtr->monoFont && tPtr->rulerShown) 
		rh = 40;

	W_ResizeView(view, view->size.width, view->size.height);
	WMResizeWidget(tPtr->ruler, view->size.width, 40);


	if(tPtr->hasVscroller) {
		WMMoveWidget(tPtr->vscroller, 1, 1+rh);
		WMResizeWidget(tPtr->vscroller, 20, view->size.height-rh-2);
		tPtr->visibleW = view->size.width-21;
	
		if(tPtr->hasHscroller) {
			WMMoveWidget(tPtr->hscroller, 20, view->size.height-21);
			WMResizeWidget(tPtr->hscroller, view->size.width-21, 20);
			tPtr->visibleH = view->size.height-21-rh;
		} else tPtr->visibleH = view->size.height-rh;
	} else { 
		tPtr->visibleW = view->size.width;
		if(tPtr->hasHscroller) {
			WMMoveWidget(tPtr->hscroller, 1, view->size.height-21);
			WMResizeWidget(tPtr->hscroller, view->size.width-2, 20);
			tPtr->visibleH = view->size.height-21-rh;
		} else tPtr->visibleH = view->size.width-2-rh;
	}
	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}   

W_ViewDelegate _TextViewDelegate =
{
	NULL,
	NULL,
	resizeText,
	NULL,
};  



/* a plain text parser */
/* this gives useful hints on how to make a more 
   interesting parser for say HTML, RTF */
static void
defaultParser(Text *tPtr, void *data, short type)
{
	char *start, *mark, *text = (char *) data;
	Chunk *chunk = NULL;
	Paragraph *para = NULL;

	start = text;
	while(start) {
		mark = strchr(start, '\n');
		if(mark) {
			/* there is a newline, indicating the need for a new paragraph */
			/* attach the chunk to the current paragraph */
			if((short)(mark-start) > 1) {  
			/* ignore chunks with just a single newline but still make a 
				blank paragraph */ 
				chunk = (tPtr->funcs.createTChunk) (start, (short)(mark-start),
					tPtr->dFont, tPtr->dColor, 0, False);
				(tPtr->funcs.insertChunk) (tPtr, chunk, type);
			} 
			/* _then_ create a new paragraph for the _next_ chunk */
			para = (tPtr->funcs.createParagraph) (0, 0, tPtr->visibleW,
				NULL, 0, WALeft);
			(tPtr->funcs.insertParagraph) (tPtr, para, type);
			start = mark+1;
		} else {
			/* just attach the chunk to the current paragraph */
			if(strlen(start) > 0) {
				chunk = (tPtr->funcs.createTChunk) (start, strlen(start), 
					tPtr->dFont, tPtr->dColor, 0, False);
				(tPtr->funcs.insertChunk) (tPtr, chunk, type);
			} start = mark;
	} } 
	
}

static void
updateScrollers(Text *tPtr)
{   
	if(tPtr->hasVscroller) {
		if(tPtr->docHeight < tPtr->visibleH) {
			WMSetScrollerParameters(tPtr->vscroller, 0, 1);
			tPtr->vpos = 0;
		} else {   
			float vmax = (float)(tPtr->docHeight);
			WMSetScrollerParameters(tPtr->vscroller,
				((float)tPtr->vpos)/(vmax - (float)tPtr->visibleH), 
				(float)tPtr->visibleH/vmax);
		}       
	}
    
	if(tPtr->hasHscroller)
		;
}

static void
scrollersCallBack(WMWidget *w, void *self)
{   
	Text *tPtr = (Text *)self;
    Bool scroll = False;
    Bool dimple = False;

	if(!tPtr->view->flags.realized) return;

	if(w == tPtr->vscroller) {
		float vmax; 
		int height; 
		vmax = (float)(tPtr->docHeight);
		height = tPtr->visibleH;
		if(height>7)
			height -= 7; /* the top border (5) + bottom (2) */

		switch(WMGetScrollerHitPart(tPtr->vscroller)) {
			case WSDecrementLine:
				if(tPtr->vpos > 0) {
					if(tPtr->vpos>16) tPtr->vpos-=16;
					else tPtr->vpos=0;
					scroll=True;
			}break;
			case WSIncrementLine: {
				int limit = tPtr->docHeight - height;
				if(tPtr->vpos < limit) {
					if(tPtr->vpos<limit-16) tPtr->vpos+=16;
					else tPtr->vpos=limit;
					scroll = True;
			}}break;
			case WSDecrementPage:
				tPtr->vpos -= height;

				if(tPtr->vpos < 0)
					tPtr->vpos = 0;
				dimple = True;
				scroll = True;
				printf("dimple needs to jump to mouse location ;-/\n");
				break;
			case WSIncrementPage:
				tPtr->vpos += height;
				if(tPtr->vpos > (tPtr->docHeight - height))
					tPtr->vpos = tPtr->docHeight - height;
				dimple = True;
				scroll = True;
				printf("dimple needs to jump to mouse location ;-/\n");
			break;
            
            
			case WSKnob:
				tPtr->vpos = WMGetScrollerValue(tPtr->vscroller)
					* (float)(tPtr->docHeight - height);
					scroll = True;
			break; 
            
#if 0
			case WSKnobSlot:
			case WSNoPart:
float vmax = (float)(tPtr->docHeight);
((float)tPtr->vpos)/(vmax - (float)tPtr->visibleH), 
(float)tPtr->visibleH/vmax);
dimple =where mouse is.
#endif
			break;
		} 
		scroll = (tPtr->vpos != tPtr->prevVpos);
		tPtr->prevVpos = tPtr->vpos;
	}
		
	if(w == tPtr->hscroller)
		;
    
//need scrollv  || scrollh
	if(scroll) {
/*
		if(0&&dimple) { 
			if(tPtr->rulerShown) 
				XClearArea(tPtr->view->screen->display, tPtr->view->window, 22, 47,
					tPtr->view->size.width-24, tPtr->view->size.height-49, True);
			else    
				XClearArea(tPtr->view->screen->display, tPtr->view->window, 22, 2,
					tPtr->view->size.width-24, tPtr->view->size.height-4, True);
		 	}
*/
		updateScrollers(tPtr);
		drawDocumentPartsOnPixmap(tPtr, False);
		paintText(tPtr);
	}   
}


void 
W_InsertText(WMText *tPtr, void *data, int position)
{
	if(!tPtr) return;
	if(!data) {
		Paragraph *para = tPtr->paragraphs, *ptmp;
		Chunk *chunk, *ctmp;
		WMFreezeText(tPtr);
		while(para) {
			chunk = para->chunks;
			while(chunk) {
				if(chunk->type == ctText && chunk->text) 
					wgdbFree(chunk->text);
				else if(chunk->pixmap)
					WMReleasePixmap(chunk->pixmap);
				ctmp = chunk;
				chunk = chunk->next;
				wgdbFree(ctmp);
			} 
			ptmp = para;
			para = para->next;
			if(ptmp->drawbuffer) 
				XFreePixmap(tPtr->view->screen->display, ptmp->drawbuffer);
			wgdbFree(ptmp);
		}
		tPtr->paragraphs = NULL;
		tPtr->currentPara = NULL;
		tPtr->currentChunk = NULL;
		WMThawText(tPtr);
		WMRefreshText(tPtr, 0, 0);
		return;
	}

	if(tPtr->parser)
		(tPtr->parser)(tPtr, data, position >= 0 ? 1 : 0);
	else
		defaultParser(tPtr, data, position >= 0 ? 1 : 0);
}
	
static  void
cursorToTextPosition(Text *tPtr, int x, int y)
{
	Paragraph *para = NULL;
	Chunk *chunk = NULL;
	WMFont *font;
	short line_width=0;
	short orig_x, orig_y;

	if(x<(tPtr->hasVscroller?21:1)) {
		y -= tPtr->clheight;
		x = tPtr->view->size.width; //tPtr->visibleW;
	} else if(x>tPtr->clwidth && x<tPtr->clicked.x) {
		//x = (tPtr->hasVscroller)?21:1;
		//y += tPtr->clheight;
	}
	
	if(x<0) x=0;
	orig_x = x;

	if(y<0 || y>tPtr->view->size.height-3) return;
	orig_y = y;
	tPtr->clicked.x = orig_x;	
	tPtr->clicked.y = y;	
	tPtr->clicked.y += tPtr->vpos;
	tPtr->clicked.y -= tPtr->rulerShown?40:0;
	para = tPtr->paragraphs;
	if(!para) return;
	while(para->next) {
		if( tPtr->clicked.y>= para->top-4 && 
			tPtr->clicked.y < para->bottom+4) break;
		para = para->next;
	} if(!(tPtr->currentPara = para)) return;

	tPtr->clicked.y -= para->top;
    if(tPtr->clicked.y<0) tPtr->clicked.y=0;
	if(tPtr->hasVscroller) x -= 21;
	if(x<0) x=0;

	tPtr->findingClickPoint = True;
	tPtr->foundClickPoint = False;
	/* also affects tPtr->currentChunk, tPtr->clicked.x and y, 
		tPtr->clheight and ->width */
	putParagraphOnPixmap(tPtr, para, False);
	tPtr->findingClickPoint = False;
	tPtr->clicked.y += para->top;

	if(tPtr->currentChunk) {
		short _width=0, start=tPtr->tpos, done=False, w=0;
		chunk = tPtr->currentChunk;
		while(!done && chunk && line_width<tPtr->clwidth) {
			if(chunk->type == ctText) {
				font = (tPtr->monoFont)?tPtr->dFont:chunk->font;
                for (w=start; w<chunk->chars; w++) {
					_width = WMWidthOfString(font, &chunk->text[w], 1); 
					line_width += _width;
					if(line_width+tPtr->clicked.x >= x) {
						line_width -= _width;
						done = True;
printf("break\n");
						break; 
				} }

				if(0&&chunk->next) {
					if(chunk->next->type == ctImage) {
					if(x+10 < line_width+chunk->next->pixmap->width) {
printf("true\n");
						done = True; 
					} } }
			} else  {
				_width  = chunk->pixmap->width;
				line_width += _width;
				if(line_width+tPtr->clicked.x >= x) {
					line_width -= _width;
					tPtr->tpos = 0;
					done = True;
			} }

			if(!done) {
				chunk = chunk->next;
				start = w = 0;
			} else { 
				tPtr->tpos = w;
				tPtr->currentChunk = chunk;
				break;   
	} }	} else {
		short vS = (tPtr->hasVscroller)?32:12;
		if(para->align == WARight) {
			tPtr->clicked.x = tPtr->view->size.width-vS;
		} else if (para->align == WACenter) {
			tPtr->clicked.x = -(vS/2)+(tPtr->view->size.width-vS)/2;
		} else {	
			tPtr->clicked.x = 2;
	} }

	tPtr->cursor.x = tPtr->clicked.x+2+line_width;
	tPtr->cursor.y = tPtr->clicked.y;
	tPtr->clicked.y = orig_y;	
	tPtr->clicked.x = orig_x;	
	putParagraphOnPixmap(tPtr, para, True); 
	paintText(tPtr);
}

static void
deleteTextInteractively(Text *tPtr, DeleteType type)
{
	Paragraph *para;
	Chunk *chunk;
	short pos,w=0,h=0, doprev=False, doprevpara=False;
	WMFont *font;
	int current = WMGetTextCurrentChunk(tPtr);
    
	if(!(para = tPtr->currentPara)) return;
	if(!(chunk = tPtr->currentChunk)) return;
	font = (tPtr->monoFont)?tPtr->dFont:chunk->font;
	doprev = (tPtr->tpos < 2);

	switch(type) {
		case dtDelete: /* delete _after_ cursor ... implement later */
		case dtBackSpace: /* delete _before_ cursor */
			if(chunk->chars > 1) {
				pos = tPtr->tpos-1;
printf("here %d\n", pos);
				if(pos>0) { 
					w = WMWidthOfString(font, &chunk->text[pos], 1); 
					memmove(&(chunk->text[pos]),
						&(chunk->text[pos+1]), chunk->chars-pos+1);
					tPtr->tpos--;
					chunk->chars--;
			} } else {
				WMRemoveTextChunk(tPtr, current);
				doprev = True;
			}
				
			if(doprev) {
				if(current > 0) {
					WMSetTextCurrentChunk(tPtr, current-1);
					if(!tPtr->currentChunk) {
						printf("PREV PARA\n");
					} else {
						tPtr->tpos = tPtr->currentChunk->chars;
					} 
				} else if(0){
					int currentp = WMGetTextCurrentParagraph(tPtr);
					doprevpara = True;
					if(currentp > 1) {
						para->chunks = NULL;
						WMRemoveTextParagraph(tPtr, currentp);
						WMSetTextCurrentParagraph(tPtr, currentp-1);
						WMSetTextCurrentChunk(tPtr, -1);
						para = tPtr->currentPara;
						if(para) {
							if(!tPtr->currentChunk || !para->chunks)  {
								para->chunks = chunk;
								tPtr->currentChunk =  chunk;
							} else
								tPtr->currentChunk->next = chunk;
						} } } } }
    
	if(1)   { //if(1||(para && !doprevpara)) {
		affectNextParas(tPtr, para, -23);
		putParagraphOnPixmap(tPtr, para, True);
		drawDocumentPartsOnPixmap(tPtr, False);
		updateScrollers(tPtr);
paintText(tPtr);
		//cursorToTextPosition(tPtr, tPtr->clicked.x-w, tPtr->clicked.y);
    } else WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}   


/* give us nice chunk sizes (multiples of 16) */
static short
reqBlockSize(short requested)
{
	return requested+16-(requested%16);
}

static void
insertTextInteractively(Text *tPtr, char *text)
{
	Paragraph *para=NULL;
	Chunk *chunk=NULL, *newchunk=NULL;
	int height = -23; /* should only be changed upon newline */
	short w=0,h=0;
	WMFont *font;
    
	if(!tPtr->editable) return;
	if(*text == '\n' && tPtr->ignoreNewLine)
		return;

	para = tPtr->currentPara;
	chunk = tPtr->currentChunk;
 	font = (tPtr->monoFont || !chunk)?tPtr->dFont:chunk->font;
	
	if(*text == '\n') {
		int new_top=0;
		if(chunk) { /* there's a chunk (or part of it) to detach from old */
			int current = WMGetTextCurrentChunk(tPtr);
			if(tPtr->tpos <=0) { /* at start of chunk */
				if(current<1) { /* the first chunk... make old para blank */
					newchunk = para->chunks;
					para->chunks = NULL;
					putParagraphOnPixmap(tPtr, para, True);
				} else { /* not first chunk... */
					printf("cut me out \n");
				}	
			} else if(tPtr->tpos < chunk->chars && chunk->type == ctText) {
					 /* not at start of chunk */
				char text[chunk->chars-tPtr->tpos+1];
				int i=0;
				do {
					text[i] = chunk->text[tPtr->tpos+i];    
				} while(++i < chunk->chars-tPtr->tpos);
				chunk->chars -= i;
				newchunk = (tPtr->funcs.createTChunk) (text, i, chunk->font,
					chunk->color, chunk->script, chunk->ul);
				newchunk->next = chunk->next;
				chunk->next = NULL;
				  /* might want to demalloc for LARGE cuts */
		//calcParaExtents(tPtr, para);
				para->height = putParagraphOnPixmap(tPtr, para, True);
				//putParagraphOnPixmap(tPtr, para, True);
			} else if(tPtr->tpos >= chunk->chars) { 
				Chunk *prev;
				WMSetTextCurrentChunk(tPtr, current-1);
				prev = tPtr->currentChunk;
				if(!prev) return;
				newchunk = prev->next;
				prev->next = NULL;
				putParagraphOnPixmap(tPtr, para, True);
			}
		} else newchunk = NULL;

		if(para)  /* the preceeding one */
			new_top = para->bottom;
		
		WMAppendTextStream(tPtr, "\n");
		para = tPtr->currentPara;
		if(!para) return;  
		para->chunks = newchunk;
		tPtr->currentChunk = newchunk;
		tPtr->tpos = 0;
		para->top = new_top;
		calcParaExtents(tPtr, para);
		height = para->height;
	} else {
		if(!para) {
			WMAppendTextStream(tPtr, text);
			para = tPtr->currentPara;
		} else if(!para->chunks || !chunk) {
			//WMPrependTextStream(tPtr, text);
			WMAppendTextStream(tPtr, text);
		} else if(chunk->type == ctImage) {
			WMPrependTextStream(tPtr, text);

printf("\n\nprepe\n\n");
		} else {
			if(tPtr->tpos > chunk->chars) {
printf("\n\nmore\n\n");
				tPtr->tpos = chunk->chars;
			}

			if(chunk->chars+1 >= chunk->mallocedSize) {
				chunk->mallocedSize = reqBlockSize(chunk->chars+1);
				chunk->text = wrealloc(chunk->text, chunk->mallocedSize);
			}
		
			memmove(&(chunk->text[tPtr->tpos+1]), &chunk->text[tPtr->tpos],
				chunk->chars-tPtr->tpos+1);
			w = WMWidthOfString(font, text, 1); 
			memmove(&chunk->text[tPtr->tpos], text, 1);
 			chunk->chars++;
 			tPtr->tpos++;
//doc->clickstart.cursor.x += 
//WMWidthOfString(chunk->fmt->font, text,len);
		}
	}
                

	if(para) {
		affectNextParas(tPtr, para, height);
		putParagraphOnPixmap(tPtr, para, True);
		drawDocumentPartsOnPixmap(tPtr, False);
		updateScrollers(tPtr);
paintText(tPtr);
		//cursorToTextPosition(tPtr, tPtr->clicked.x+w, tPtr->clicked.y);
//check for "sneppah tahw" with blank paras...
//paintText(tPtr);
	}

}

static void
selectRegion(Text *tPtr, int x, int y)
{
	tPtr->sRect.pos.x = WMIN(tPtr->clicked.x, x);
	tPtr->sRect.size.width = abs(tPtr->clicked.x-x);
	tPtr->sRect.pos.y = WMIN(tPtr->clicked.y, y);
	if(tPtr->sRect.pos.y<0) tPtr->sRect.pos.y=0;
	tPtr->sRect.size.height = abs(tPtr->clicked.y-y);

/*
	while(y>tPtr->visibleH && tPtr->vpos < tPtr->docHeight-tPtr->visibleH) {
		WMRefreshText(tPtr, tPtr->vpos+16, tPtr->hpos);
	}
*/
//printf("%d %d \n", y, tPtr->vpos);

//foreach para in selection...
	drawDocumentPartsOnPixmap(tPtr, True);
	paintText(tPtr);
}

    
    


#define WM_EMACSKEYMASK   ControlMask
#define WM_EMACSKEY_LEFT  XK_b
#define WM_EMACSKEY_RIGHT XK_f
#define WM_EMACSKEY_HOME  XK_a
#define WM_EMACSKEY_END   XK_e
#define WM_EMACSKEY_BS    XK_h
#define WM_EMACSKEY_DEL   XK_d

static  void
handleTextKeyPress(Text *tPtr, XEvent *event)
{
	char buffer[2];
	KeySym ksym;
	int control_pressed = False;

	if(!tPtr->editable) return;

	if (((XKeyEvent *) event)->state & WM_EMACSKEYMASK)
		control_pressed = True;
    buffer[XLookupString(&event->xkey, buffer, 1, &ksym, NULL)] = '\0';
    
    switch(ksym) {

		case XK_Right:
		case XK_Left:
			if(tPtr->currentChunk) {
				short w;
				Chunk *chunk = tPtr->currentChunk;
				if(chunk->type == ctText) {
					WMFont *font = (tPtr->monoFont)?tPtr->dFont:chunk->font;
					if(ksym==XK_Right) {
						short pos = (tPtr->tpos<chunk->chars)?tPtr->tpos+1:
							chunk->chars;
						w = WMWidthOfString(font,&chunk->text[pos],1); 
					} else {
						short pos = (tPtr->tpos>0)?tPtr->tpos-1:0;
						w = WMWidthOfString(font,&chunk->text[pos],1); 
					}
				} else { w = chunk->pixmap->width; }
				if(ksym==XK_Right) w = -w;
				cursorToTextPosition(tPtr, tPtr->clicked.x-w, tPtr->clicked.y);
			} else {
				if(ksym==XK_Right) ksym = XK_Down;
				else ksym = XK_Up;
				goto noCChunk;
			}
		break;

		case XK_Down:
		case XK_Up: 
noCChunk: { short h = tPtr->clheight-2;
			if(ksym==XK_Down) h = -h;
			cursorToTextPosition(tPtr, tPtr->clicked.x, tPtr->clicked.y-h);
		} break;

		case XK_BackSpace:
			deleteTextInteractively(tPtr, dtBackSpace);
		break;

		case XK_Delete:
		case XK_KP_Delete:
			deleteTextInteractively(tPtr, dtDelete);
		break;

		case XK_Return:
			buffer[0] = '\n';
		default:
		if(buffer[0] != '\0' && (buffer[0] == '\n' || !iscntrl(buffer[0])))
			insertTextInteractively(tPtr, buffer);
		else if(control_pressed && ksym==XK_r)
		{Bool i = !tPtr->rulerShown; WMShowTextRuler(tPtr, i);
			tPtr->rulerShown = i; }   
	}

}



static void
pasteText(WMView *view, Atom selection, Atom target, Time timestamp,
	void *cdata, WMData *data)
{     
	Text *tPtr = (Text *)view->self;
	char *str; 
    
	
	tPtr->waitingForSelection = False;
	if(data) {
		str = (char*)WMDataBytes(data);
		if(tPtr->tpos<1) WMPrependTextStream(tPtr, str);
		else WMAppendTextStream(tPtr, str);
		WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
	} else {
		int n;
		str = XFetchBuffer(tPtr->view->screen->display, &n, 0);
		if(str) {
			str[n] = 0;
			if(tPtr->tpos<1) WMPrependTextStream(tPtr, str);
			else WMAppendTextStream(tPtr, str);
			XFree(str);
			WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
		}
	}
}


static void
releaseSelection(Text *tPtr)
{
	Paragraph *para = tPtr->paragraphs;
	Chunk *chunk;
	while(para) {
		chunk = para->chunks;
		while(chunk) {
			chunk->selected = False;
			chunk = chunk->next;
			} 
		para = para->next;
	}
	WMDeleteSelectionHandler(tPtr->view, XA_PRIMARY, CurrentTime);
	tPtr->ownsSelection = False;
	drawDocumentPartsOnPixmap(tPtr, True);
	paintText(tPtr);
}


static WMData*
requestHandler(WMView *view, Atom selection, Atom target, 
	void *cdata, Atom *type)
{   
	Text *tPtr = view->self;
	int count;
	Display *dpy = tPtr->view->screen->display;
	Atom _TARGETS;
	Atom TEXT = XInternAtom(dpy, "TEXT", False);
	Atom COMPOUND_TEXT = XInternAtom(dpy, "COMPOUND_TEXT", False);
	WMData *data = NULL;
    

	if(!tPtr->ownsSelection || !tPtr->paragraphs) return NULL;
//printf("got here\n");

	if (target == XA_STRING || target == TEXT || target == COMPOUND_TEXT) {
//for bleh in selection...
		char *s = NULL;
		Paragraph *para  = tPtr->paragraphs;
		Chunk *chunk = NULL;
		char pixmap[] = "[pixmap]";
		Bool first=True;
		short len;
	
		while(para) {
			chunk = para->chunks;
			while(chunk) {

				if(chunk->selected && chunk->type == ctText) {
					len = chunk->chars; //chunk->sEnd - chunk->sStart;
					if(len>0) {
						s = wmalloc(len+1);
						if(s) {
							memcpy(s, &chunk->text[0*chunk->sStart], len);
							s[len] = 0;
							if(first) { 
								data = WMCreateDataWithBytes(s, strlen(s));
								first = False;
							} else {
printf("append: %c %d\n", *s, strlen(s));
								WMAppendDataBytes(data, s, strlen(s));
							}
							//gdbFree(s);
					} } }
#if 0
printf("len is %d [%d %d] %d \n", len, chunk->sStart, chunk->sEnd,
chunk->chars);
#endif
				chunk = chunk->next;
			} 
			para = para->next;
		}

		if(data) { 
			WMSetDataFormat(data, 8);
			*type = target;
		}
		return data;
	}   

#if 0
	 _TARGETS = XInternAtom(dpy, "TARGETS", False);
	if (target == _TARGETS) {
		Atom *ptr = wmalloc(4 * sizeof(Atom));
		ptr[0] = _TARGETS;
		ptr[1] = XA_STRING;
		ptr[2] = TEXT;
		ptr[3] = COMPOUND_TEXT;
        
		data = WMCreateDataWithBytes(ptr, 4*4);
		WMSetDataFormat(data, 32);
    
		*type = target;
		return data;
	}
#endif
    
	return NULL;
    
}   


static void
lostHandler(WMView *view, Atom selection, void *cdata)
{
	WMText *tPtr = (WMText *)view->self;
	releaseSelection(tPtr);
}   

static WMSelectionProcs selectionHandler = {
	requestHandler, lostHandler, NULL };

static void
_notification(void *observerData, WMNotification *notification)
{
	WMText *to = (WMText *)observerData;
    WMText *tw = (WMText *)WMGetNotificationClientData(notification);
    if (to != tw) lostHandler(to->view, XA_PRIMARY, NULL);
}

static void
handleTextEvents(XEvent *event, void *data)
{
	Text *tPtr = (Text *)data;
	Display *dpy = event->xany.display;

	if(tPtr->waitingForSelection) return;

	switch (event->type) {
		case KeyPress:
			if(!tPtr->editable || tPtr->buttonHeld) {
				XBell(dpy, 0);
				return;
			}
			if(tPtr->ownsSelection) releaseSelection(tPtr);
			//if (tPtr->waitingForSelection) return;
			if(tPtr->focused) {
#if 0
				XGrabPointer(dpy, W_VIEW(tPtr)->window, False, 
					PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
					GrabModeAsync, GrabModeAsync, None, 
					W_VIEW(tPtr)->screen->invisibleCursor, CurrentTime);
				tPtr->pointerGrabbed = True;
#endif
				handleTextKeyPress(tPtr, event);
			} break;

		case MotionNotify:
			if(tPtr->pointerGrabbed) {
				tPtr->pointerGrabbed = False;
				XUngrabPointer(dpy, CurrentTime);
			}
			if((event->xmotion.state & Button1Mask)) {
				selectRegion(tPtr, event->xmotion.x, event->xmotion.y);
				if(!tPtr->ownsSelection) { 
					WMCreateSelectionHandler(tPtr->view, XA_PRIMARY,
						event->xbutton.time, &selectionHandler, NULL);
					tPtr->ownsSelection = True;
				}
				break;
			}
		case ButtonPress: 
			if(event->xbutton.button == Button1) { 
				if(tPtr->ownsSelection) releaseSelection(tPtr);
				cursorToTextPosition(tPtr, event->xmotion.x, event->xmotion.y);
				if (tPtr->pointerGrabbed) {
					tPtr->pointerGrabbed = False;
					XUngrabPointer(dpy, CurrentTime);
					break;
				}   
			}
			if(!tPtr->focused) {
				WMSetFocusToWidget(tPtr);
              	tPtr->focused = True;
				break;
			} 	
			if(event->xbutton.button == 4) 
				WMScrollText(tPtr, -16);
			else if(event->xbutton.button == 5) 
				WMScrollText(tPtr, 16);
				 
			break;

			case ButtonRelease:
				tPtr->buttonHeld = False;
				if (tPtr->pointerGrabbed) {
					tPtr->pointerGrabbed = False;
					XUngrabPointer(dpy, CurrentTime);
					break;
				}   
				if(event->xbutton.button == 4 || event->xbutton.button == 5)
					break;
				if(event->xbutton.button == Button2 && tPtr->editable) {
					char *text = NULL;
					int n;
					if(!WMRequestSelection(tPtr->view, XA_PRIMARY, XA_STRING,
						event->xbutton.time, pasteText, NULL)) {
						text = XFetchBuffer(tPtr->view->screen->display, &n, 0);
						if(text) {
							text[n] = 0;
							WMAppendTextStream(tPtr, text);
							XFree(text);
							WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
						} else tPtr->waitingForSelection = True;
			} } break;

	}

}


static void 
handleNonTextEvents(XEvent *event, void *data)
{
	Text *tPtr = (Text *)data;

	switch(event->type) {
		case Expose: 
			if(!event->xexpose.count && tPtr->view->flags.realized)
				paintText(tPtr);
		break;

		case FocusIn: 
			if (W_FocusedViewOfToplevel(W_TopLevelOfView(tPtr->view))!=tPtr->view)
				return;
			tPtr->focused = True;
			//cursor...paintText(tPtr);
		break;

		case FocusOut:
			tPtr->focused = False;
			//cursor...paintText(tPtr);
        break;

		case DestroyNotify:
			printf("destroy");
		//for(...)WMRemoveTextParagraph(tPtr, para);
        break;

	}

//printf("handleNonTextEvents\n");
}



static void
rulerCallBack(WMWidget *w, void *self)
{
	Text *tPtr = (Text *)self;
	short which;
    
	if(tPtr->currentPara) {
		Paragraph *para = tPtr->currentPara;
		para->fmargin = WMGetRulerMargin(tPtr->ruler, WRulerFirst);
		para->bmargin = WMGetRulerMargin(tPtr->ruler, WRulerBody);
		para->rmargin = WMGetRulerMargin(tPtr->ruler, WRulerRight);
		affectNextParas(tPtr, para, -23);
		putParagraphOnPixmap(tPtr, para, True);
	}
#if 0
	which = WMGetReleasedRulerMargin(tPtr->ruler);
	if(which !=  WRulerDocLeft &&  which != WRulerRight 
		/* && Selection.para.count > 0 */ ) { 
		printf(""
		"//for(i=0; i<Selection.para.count; i++) {"
			"affect"
			"//calcParaExtents(tPtr, para);}\n");
	} else {
		WMRefreshText(tPtr, 0, 0);
	}
#endif
}


static void
rulerMoveCallBack(WMWidget *w, void *self)
{
	Text *tPtr = (Text *)self;
	short rmargin = WMGetRulerMargin(tPtr->ruler, WRulerRight);
    
    
	if(WMGetGrabbedRulerMargin(tPtr->ruler) == WRulerLeft) {
		short lmargin = WMGetRulerMargin(tPtr->ruler, WRulerDocLeft);
		XClearArea(tPtr->view->screen->display, tPtr->view->window, 
			22, 42, lmargin-21, tPtr->visibleH, True);
	} else if(WMGetGrabbedRulerMargin(tPtr->ruler) == WRulerRight &&
		tPtr->docWidth+11 < rmargin) {
		XClearArea(tPtr->view->screen->display, tPtr->view->window,
			rmargin-3, 42, 10, tPtr->visibleH, True);
	}
	paintText(tPtr);
}       
    


/* ------------- non-static functions that are "friends" ------------- */
/* -------------   called as (tPtr->funcs.foo)(bars...)  ------------- */

/* create a new paragraph. Don't do anything with it just yet */
//Paragraph *
void *
createParagraph(short fmargin, short bmargin, short rmargin,
	short *tabstops, short numTabs, WMAlignment alignment)
{
	Paragraph *para = wmalloc(sizeof(Paragraph));
	if(!para) return NULL;

	para->chunks = NULL;
	para->next = NULL;
		    

	para->fmargin = (fmargin>=0)?fmargin:0;
	para->bmargin = (bmargin>=0)?bmargin:0;
	if(rmargin-bmargin >= 100 && rmargin-fmargin >= 100)
		para->rmargin = rmargin;
	else 
		para->rmargin = 100;
	para->tabstops = tabstops;
	para->numTabs = (tabstops)?numTabs:0;

	para->drawbuffer = (Pixmap)NULL;
	para->bulletPix = NULL;
	para->top = para->bottom = 0;
	para->width = para->height = 0;

	para->align = alignment;
	
	return para;
}

/* insert the new paragraph in the tPtr, either right before 
   or after the currentPara.  It's the responsibility of the 
   calling code to set what currentPara is. via WMSetTextCurrentParagraph.
   If currentPara is not set, set it as the first in the document. 
   This function then sets currentPara as _this_ paragraph.
   NOTE: this means careless parser implementors might lose previous 
         paragraphs... but this keeps stuff small and non-buggy :-) */
void
insertParagraph(WMText *tPtr, void *v, InsertType type)
//insertParagraph(WMText *tPtr, Paragraph *para, InsertType type)
{
	Paragraph *tmp;
	Paragraph *para = (Paragraph *)v;
	if(!para || !tPtr) return;

	if(!tPtr->currentPara) {
		tPtr->paragraphs = para;
	} else {
		tmp = tPtr->paragraphs;
		if(type == itAppend) {
			while(tmp->next && tmp != tPtr->currentPara) 
				tmp = tmp->next;

			para->next = tmp->next;
			tmp->next = para;	
		} else { /* must be prepend */
		/* this "prior" member is that "doing things the hard way" 
			I spoke of. See? it's not too bad afterall... */
			Paragraph *prior = NULL;
			while(tmp->next && tmp != tPtr->currentPara) {
				prior = tmp;
				tmp = tmp->next;
			}
 		 	 /* if this is the first */
			if(tmp == tPtr->paragraphs) {
				para->next = tmp;	
				tPtr->paragraphs = para;
			} else {
				prior->next = para;
				para->next = tmp;
	}	}	}	
	tPtr->currentPara = para;

}

/* create a new chunk to contain exactly ONE pixmap */
void *
//Chunk *
createPChunk(WMPixmap *pixmap, short script, ushort ul)
{
	Chunk *chunk;
    
	chunk = wmalloc(sizeof(Chunk));
	if(!chunk)
		return NULL;

	chunk->text = NULL; 
	if(!pixmap)
		chunk->pixmap = NULL; /* if it's NULL, we'll draw the "broken" pixmap...  */
	else chunk->pixmap = WMRetainPixmap(pixmap);
	chunk->chars = 0;
	chunk->mallocedSize = 0;
	chunk->type = ctImage;
	chunk->font = NULL;
	chunk->color = NULL;
	chunk->script = script;
	chunk->ul = ul;
	chunk->selected = False;
	chunk->next = NULL;
	return chunk; 
}


/* create a new chunk to contain some text with the given format */
void *
//Chunk *
createTChunk(char *text, short chars, WMFont *font,
	WMColor *color, short script, ushort ul)
{
	Chunk *chunk;

	if(!text || chars<0 || !font || !color) return NULL;
	chunk = wmalloc(sizeof(Chunk));
	if(!chunk) return NULL;

	chunk->mallocedSize = reqBlockSize(chars);
	chunk->text = (char *)wmalloc(chunk->mallocedSize);
	memcpy(chunk->text, text, chars);
	chunk->pixmap = NULL;
	chunk->chars = chars;
	chunk->type = ctText;
	chunk->font = WMRetainFont(font);
	chunk->color = WMRetainColor(color);
	chunk->script = script;
	chunk->ul = ul;
	chunk->selected = False;
	chunk->next = NULL;

	return chunk; 
}

/* insert the new chunk in the paragraph, either right before 
   or after the currentChunk. It's the responsibility of the 
   calling code to set what currentChunk is via WMSetTextCurrentChunk.
   If currentChunk is not set, set it as the first in the existing 
   paragraph...  if not even that, you lose... try again.
   This function then sets currentChunk as _this_ chunk.
   NOTE: this means careless parser implementors might lose previous 
         paragraphs/chunks... but this keeps stuff small and non-buggy :-) */
void 
insertChunk(WMText *tPtr, void *v, InsertType type)
{
	Chunk *tmp;
	Chunk *chunk = (Chunk *)v;

	if(!tPtr || !chunk) return;

	if(!tPtr->paragraphs) { /* i.e., first chunk via insertTextInteractively */ 
		Paragraph *para = (tPtr->funcs.createParagraph) (0, 0, tPtr->visibleW,
			NULL, 0, WALeft);
		(tPtr->funcs.insertParagraph) (tPtr, para, itAppend);
	}

	if(!tPtr->currentPara)
		return;
	if(!tPtr->currentChunk) { /* there is a current chunk */
		tPtr->currentPara->chunks = chunk;
	} else if(!tPtr->currentPara->chunks) { 
		/* but it's not of this paragraph */
		tPtr->currentPara->chunks = chunk;
	} else {
		tmp = tPtr->currentPara->chunks;

		if(type == itAppend) {
			while(tmp->next && tmp != tPtr->currentChunk) 
				tmp = tmp->next;

			chunk->next = tmp->next;
			tmp->next = chunk;	

		} else { /* must be prepend */
		/* this "prior" member is that "doing things the hard way" 
      	 I spoke of. See? it's not too bad afterall... */
			Chunk *prior = NULL;
			while(tmp->next && tmp != tPtr->currentChunk) {
				prior = tmp;
				tmp = tmp->next;
			}
 		 	 /* if this is the first */
			if(tmp == tPtr->currentPara->chunks) {
				chunk->next = tmp;	
				tPtr->currentPara->chunks = chunk;
			} else { 
				prior->next = chunk;
				chunk->next = tmp;
	} } }
	tPtr->currentChunk = chunk;
 	tPtr->tpos = chunk->chars;
}

	
/* ------------- non-static functions (i.e., APIs) ------------- */
/* -------------   called as WMVerbText[Subject]   ------------- */
	
#define DEFAULT_TEXT_WIDTH      250
#define DEFAULT_TEXT_HEIGHT     200



WMText*
WMCreateText(WMWidget *parent)
{
	Text *tPtr = wmalloc(sizeof(Text));
	if(!tPtr) {
		perror("could not create text widget\n");
		return NULL;
	}   
	memset(tPtr, 0, sizeof(Text));
	tPtr->widgetClass = WC_Text;
	tPtr->view = W_CreateView(W_VIEW(parent));
	if (!tPtr->view) {
		perror("could not create text's view\n");
		wgdbFree(tPtr);
		return NULL;
    }
	tPtr->view->self = tPtr;
	tPtr->view->attribs.cursor = tPtr->view->screen->textCursor;
	tPtr->view->attribFlags |= CWOverrideRedirect | CWCursor;
	W_ResizeView(tPtr->view, DEFAULT_TEXT_WIDTH, DEFAULT_TEXT_HEIGHT);
	tPtr->bg = tPtr->view->screen->white;
	W_SetViewBackgroundColor(tPtr->view, tPtr->bg);

    
	tPtr->ruler = WMCreateRuler(tPtr);
	(W_VIEW(tPtr->ruler))->attribs.cursor = tPtr->view->screen->defaultCursor;
	(W_VIEW(tPtr->ruler))->attribFlags |= CWOverrideRedirect | CWCursor;
	WMMoveWidget(tPtr->ruler, 0, 0);
	WMResizeWidget(tPtr->ruler, W_VIEW(parent)->size.width, 40);
	WMShowRulerTabs(tPtr->ruler, True);
	WMSetRulerAction(tPtr->ruler, rulerCallBack, tPtr);
	WMSetRulerMoveAction(tPtr->ruler, rulerMoveCallBack, tPtr);

	tPtr->vpos = 0;
	tPtr->prevVpos = 0;
	tPtr->vscroller = WMCreateScroller(tPtr); 
	(W_VIEW(tPtr->vscroller))->attribs.cursor = 
		tPtr->view->screen->defaultCursor;
	(W_VIEW(tPtr->vscroller))->attribFlags |= CWOverrideRedirect | CWCursor;
	WMMoveWidget(tPtr->vscroller, 1, 1);
	WMResizeWidget(tPtr->vscroller, 20, tPtr->view->size.height - 2);
	WMSetScrollerArrowsPosition(tPtr->vscroller, WSAMaxEnd);
	WMSetScrollerAction(tPtr->vscroller, scrollersCallBack, tPtr);

	tPtr->hpos = 0;
	tPtr->prevHpos = 0;
	tPtr->hscroller = WMCreateScroller(tPtr); 
	(W_VIEW(tPtr->hscroller))->attribs.cursor = 
		tPtr->view->screen->defaultCursor;
	(W_VIEW(tPtr->hscroller))->attribFlags |= CWOverrideRedirect | CWCursor;
	WMMoveWidget(tPtr->hscroller, 1, tPtr->view->size.height-21);
	WMResizeWidget(tPtr->hscroller, tPtr->view->size.width - 2, 20);
	WMSetScrollerArrowsPosition(tPtr->hscroller, WSAMaxEnd);
	WMSetScrollerAction(tPtr->hscroller, scrollersCallBack, tPtr);

	tPtr->visibleW = tPtr->view->size.width;
	tPtr->visibleH = tPtr->view->size.height;

	tPtr->paragraphs = NULL;
	tPtr->docWidth = 0; 
	tPtr->docHeight = 0; 
	tPtr->dBulletPix = WMCreatePixmapFromXPMData(tPtr->view->screen, 
		default_bullet);
	tPtr->dUnknownImg = WMCreatePixmapFromXPMData(tPtr->view->screen, 
		unk_xpm);
    
	tPtr->sRect.pos.x = tPtr->sRect.pos.y = 0;
	tPtr->sRect.size.width = tPtr->sRect.size.height = 0;
	tPtr->currentPara = NULL;
	tPtr->currentChunk = NULL;
	tPtr->tpos = 0;

	tPtr->parser = NULL;
	tPtr->writer = NULL;
	tPtr->funcs.createParagraph = createParagraph;
	tPtr->funcs.insertParagraph = insertParagraph;
	tPtr->funcs.createPChunk = createPChunk;
	tPtr->funcs.createTChunk = createTChunk;
	tPtr->funcs.insertChunk = insertChunk;

	tPtr->clicked.x = tPtr->clicked.y = -23;
	tPtr->cursor.x = tPtr->cursor.y = -23;

	tPtr->relief = WRSunken;
	tPtr->wrapping = wrWord;
	tPtr->editable = False;
	tPtr->cursorShown = False;
	tPtr->frozen = False;
	tPtr->focused = False;
	tPtr->pointerGrabbed = False;
	tPtr->buttonHeld = False;
	tPtr->ignoreNewLine = False;
	tPtr->waitingForSelection = False;
	tPtr->findingClickPoint = False;
	tPtr->foundClickPoint = False;
	tPtr->ownsSelection = False;
	tPtr->clheight = 0;
	tPtr->clwidth = 0;
	
	tPtr->dFont = WMRetainFont(tPtr->view->screen->normalFont);
	tPtr->dColor = WMBlackColor(tPtr->view->screen);

	tPtr->view->delegate = &_TextViewDelegate;
	WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
		|EnterWindowMask|LeaveWindowMask|FocusChangeMask, 
		handleNonTextEvents, tPtr);
	WMCreateEventHandler(tPtr->view, ButtonReleaseMask|ButtonPressMask
		|KeyReleaseMask|KeyPressMask|Button1MotionMask, 
		handleTextEvents, tPtr);
	
	WMAddNotificationObserver(_notification, tPtr, "_lostOwnership", tPtr);

	WMSetTextMonoFont(tPtr, True);
	WMShowTextRuler(tPtr, False);
	WMSetTextHasHorizontalScroller(tPtr, False);
	WMSetTextHasVerticalScroller(tPtr, True);
//printf("the sizeof chunk is %d\n", sizeof(Chunk));
//printf("the sizeof para is %d\n", sizeof(Paragraph));
//printf("the sizeof text is %d\n", sizeof(Text));
	return tPtr;
}

//WMSetTextBullet()
//WRetainPixmap(tPtr->dBulletPix);

void 
WMRemoveTextParagraph(WMText *tPtr, int which)
{
	Paragraph *prior, *removed;
	if(!tPtr || which<0) return;

	WMSetTextCurrentParagraph(tPtr, which);
	removed = tPtr->currentPara;
	if(!removed) return;
	if(removed->chunks)printf("WMRemoveTextChunks\n");
	if(removed == tPtr->paragraphs || which==0) {
		tPtr->paragraphs = removed->next;
	} else {
		WMSetTextCurrentParagraph(tPtr, which-1);
		prior = tPtr->currentPara;
		if(!prior) return;
		prior->next = removed->next;
	}
	wgdbFree(removed);
// removeChunks
	removed = NULL;
}
	
	
	
/* set what is known as the currentPara in the tPtr. */
/* negative number means: "gib me last chunk" */
void 
WMSetTextCurrentParagraph(WMText *tPtr, int current)
{
	Paragraph *tmp;
	int i=0;

	if(!tPtr || current<0) return;
	if(current == 0) {
    	tPtr->currentPara = tPtr->paragraphs;
		return;
	}   
	tmp = tPtr->paragraphs;
	while(tmp->next && ((current==-23)?1:i++<current)) {
	//while(tmp && i++<current) {
		tmp = tmp->next;
	} tPtr->currentPara = tmp;
	//? want to do this?if(tmp) tPtr->currentChunk = tmp
}


int 
WMGetTextParagraphs(WMText *tPtr)
{
	int current=0;
	Paragraph *tmp;
	if(!tPtr) return 0;
	tmp = tPtr->paragraphs;
	while(tmp) { 
		tmp = tmp->next;
		current++;
	} return current;
}



int 
WMGetTextCurrentParagraph(WMText *tPtr)
{
	int current=-1;
	Paragraph *tmp;

	if(!tPtr) return current;
	if(!tPtr->currentPara) return current;
	if(!tPtr->paragraphs) return current;
	tmp = tPtr->paragraphs;
	while(tmp) { 
		current++;
		if(tmp == tPtr->currentPara)
			break;
		tmp = tmp->next;
	} return current;
}

/* set what is known as the currentChunk within the currently 
selected currentPara (or the first paragraph in the document). */
void 
WMSetTextCurrentChunk(WMText *tPtr, int current)
{
	Chunk *tmp;
	int i=0;

	if(!tPtr) return;
	tPtr->currentChunk = NULL;
	if(!tPtr->currentPara) {
		tPtr->currentPara = tPtr->paragraphs;
		if(!tPtr->currentPara)
			return;
	}
	
	if(current == 0) {
		tPtr->currentChunk = tPtr->currentPara->chunks;
		return;
	}
	tmp = tPtr->currentPara->chunks;
	if(tmp) { 
		while(tmp->next && ((current<0)?1:i++<current)) 
			tmp = tmp->next;
	} tPtr->currentChunk = tmp;
}


void 
WMRemoveTextChunk(WMText *tPtr, int which)
{
	Chunk *prior, *removed;
	Paragraph *para;
	if(!tPtr || which<0) return;
	para = tPtr->currentPara;
	if(!para) return;

	WMSetTextCurrentChunk(tPtr, which);
	removed = tPtr->currentChunk;
	if(!removed) return;
	if(removed == tPtr->currentPara->chunks || which==0) {
		para->chunks = removed->next;
	} else {
		WMSetTextCurrentChunk(tPtr, which-1);
		prior = tPtr->currentChunk;
		if(!prior) return;
		prior->next = removed->next;
	}
	if(removed->type == ctText) {
		wgdbFree(removed->text);
		WMReleaseFont(removed->font);
		WMReleaseColor(removed->color);
	} else {
		WMReleasePixmap(removed->pixmap);
	}
	wgdbFree(removed);
	removed = NULL;
}

int 
WMGetTextCurrentChunk(WMText *tPtr)
{
	int current=0;
	Chunk *tmp;

	if(!tPtr) return 0;
	if(!tPtr->currentChunk) return 0;
	if(!tPtr->currentPara) {
		tPtr->currentPara = tPtr->paragraphs;
		if(!tPtr->currentPara)
			return 0;
	}
	
	tmp = tPtr->currentPara->chunks;
	while(tmp) { 
		if(tmp == tPtr->currentChunk)
			break;
		tmp = tmp->next;
		current++;
	}
	return current;
}

int 
WMGetTextChunks(WMText *tPtr)
{
	short current=0;
	Chunk *tmp;
	if(!tPtr || !tPtr->currentPara) return 0;
	tmp = tPtr->currentPara->chunks;
	while(tmp) { 
		tmp = tmp->next;
		current++;
	} return current;
}

void
WMShowTextRuler(WMText *tPtr, Bool show)
{
	if(!tPtr) return;
	if(tPtr->monoFont) show = False;
			
	tPtr->rulerShown = show;
	if(show) WMMapWidget(tPtr->ruler);
	else WMUnmapWidget(tPtr->ruler); 
	resizeText(tPtr->view->delegate, tPtr->view);
}

Bool
WMGetTextRulerShown(WMText *tPtr)
{   
	if(!tPtr) return False; 
	return tPtr->rulerShown;
}

void 
WMSetTextRulerMargin(WMText *tPtr, char which, short pixels)
{   
	if(!tPtr) return;
	if(tPtr->monoFont) return;
	WMSetRulerMargin(tPtr->ruler, which, pixels);
	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

short 
WMGetTextRulerMargin(WMText *tPtr, char which)
{   
    if(!tPtr) return 0;
	if(tPtr->monoFont)
		return 0;
    return WMGetRulerMargin(tPtr->ruler, which);
}


void 
WMShowTextRulerTabs(WMText *tPtr, Bool show)
{
	if(!tPtr) return;
	if(tPtr->monoFont) return;
	WMShowRulerTabs(tPtr->ruler, show);
} 

void 
WMSetTextMonoFont(WMText *tPtr, Bool mono)
{
	if(!tPtr) return;
	if(mono && tPtr->rulerShown) 
		WMShowTextRuler(tPtr, False);
	
	tPtr->monoFont = mono;
}   

Bool 
WMGetTextMonoFont(WMText *tPtr)
{
	if(!tPtr) return True;
	return tPtr->monoFont;
}

void
WMForceTextFocus(WMText *tPtr)
{
	if(!tPtr) return;
    
	if(tPtr->clicked.x == -23 || tPtr->clicked.y == 23)
		cursorToTextPosition(tPtr, 100, 100); /* anyplace */
	else
		cursorToTextPosition(tPtr, tPtr->clicked.x, tPtr->clicked.y);
} 

	
void
WMSetTextEditable(WMText *tPtr, Bool editable)
{
	if(!tPtr) return;
	tPtr->editable = editable;
}


Bool
WMGetTextEditable(WMText *tPtr)
{   
    if(!tPtr) return 0;
    return tPtr->editable;
}


Bool
WMScrollText(WMText *tPtr, int amount)
{
	Bool scroll=False;
	if(amount == 0 || !tPtr) return;
	if(!tPtr->view->flags.realized) return;
	
	if(amount < 0) {
		if(tPtr->vpos > 0) {
			if(tPtr->vpos > amount) tPtr->vpos += amount;
			else tPtr->vpos=0;
			scroll=True;
	} } else {
		int limit = tPtr->docHeight - tPtr->visibleH;
		if(tPtr->vpos < limit) {
			if(tPtr->vpos < limit-amount) tPtr->vpos += amount;
			else tPtr->vpos = limit;
            scroll = True;
    } }   

	if(scroll && tPtr->vpos != tPtr->prevVpos) {
		updateScrollers(tPtr);
		drawDocumentPartsOnPixmap(tPtr, False);
		paintText(tPtr);
    }   
	tPtr->prevVpos = tPtr->vpos;
    return scroll;
}   

Bool
WMPageText(WMText *tPtr, Bool scrollUp)
{
	if(!tPtr) return;
	if(!tPtr->view->flags.realized) return;

	return WMScrollText(tPtr, scrollUp
			    ? tPtr->visibleH:-tPtr->visibleH);
}

void
WMIgnoreTextNewline(WMText *tPtr, Bool ignore)
{
	if(!tPtr) return;
	tPtr->ignoreNewLine = ignore;
}


void 
WMSetTextHasHorizontalScroller(WMText *tPtr, Bool flag)
{
	if(tPtr) {
		short rh;
		if(tPtr->monoFont)
			return;
		rh = tPtr->rulerShown?40:0;
		tPtr->hasHscroller = flag;
		if(flag) { 
			WMMapWidget(tPtr->hscroller);
			tPtr->visibleH = tPtr->view->size.height-rh-22;
		} else {
			WMUnmapWidget(tPtr->hscroller);
			tPtr->visibleH = tPtr->view->size.height-rh;
		}
		resizeText(tPtr->view->delegate, tPtr->view);
	}
}


void 
WMSetTextHasVerticalScroller(WMText *tPtr, Bool flag)
{
	if(tPtr) {
		tPtr->hasVscroller = flag;
		if(flag) {
			WMMapWidget(tPtr->vscroller);
			tPtr->visibleW = tPtr->view->size.width-22;
			WMSetRulerOffset(tPtr->ruler, 22); /* scrollbar width + 2 */
		} else {
			WMUnmapWidget(tPtr->vscroller);
			tPtr->visibleW = tPtr->view->size.width;
			WMSetRulerOffset(tPtr->ruler, 2);
		}
		resizeText(tPtr->view->delegate, tPtr->view);
	}
}



void
WMRefreshText(WMText *tPtr, int vpos, int hpos)
{   
    
	if(!tPtr)
		return;

	if(tPtr->frozen || !tPtr->view->flags.realized)
		return;
	

	XClearArea(tPtr->view->screen->display, tPtr->view->window, 
		22, (tPtr->rulerShown)?45:5,
		tPtr->visibleW, tPtr->visibleH, True);

	calcDocExtents(tPtr);
/*
printf("vpos:%d tPtr->docHeight%d tPtr->visibleH%d \n",
vpos, tPtr->docHeight, tPtr->visibleH);
*/

	//	tPtr->vpos = vpos;
/*
	if(vpos < 0 || tPtr->docHeight < tPtr->visibleH)
		tPtr->vpos = 0;
	else if(vpos-tPtr->visibleH>tPtr->docHeight)
		tPtr->vpos = vpos-tPtr->docHeight-tPtr->visibleH-tPtr->docHeight;
	else
        tPtr->vpos = tPtr->docHeight-tPtr->visibleH;
*/


	if(hpos < 0 || hpos > tPtr->docWidth)
		tPtr->hpos = 0;
	else
        tPtr->hpos = hpos;

	drawDocumentPartsOnPixmap(tPtr, True);
	updateScrollers(tPtr);
	paintText(tPtr);
}   

/* would be nice to have in WINGs proper... */
static void
changeFontProp(char *fname, char *newprop, short which)
{
	char before[128], prop[128], after[128];
	char *ptr, *bptr;
	int part=0;
	
	if(!fname || !prop)
		return;

	ptr = fname;
	bptr = before;
	while (*ptr) {
		if(*ptr == '-') {
    	    *bptr = 0;
			if(part==which) bptr = prop;
			else if(part==which+1) bptr = after;
    	  	*bptr++ = *ptr;
			part++;
		} else {
    	  *bptr++ = *ptr;
		}  ptr++;
	}*bptr = 0;
	snprintf(fname, 255, "%s-%s%s", before, newprop, after); 
}

/* TODO: put in wfont? */
WMFont *
WMGetFontPlain(WMScreen *scrPtr, WMFont *font) 
{
	WMFont *nfont=NULL;
	if(!scrPtr|| !font)
		return NULL;
	return font;
	 
}

WMFont *
WMGetFontBold(WMScreen *scrPtr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];
	if(!scrPtr || !font)
		return NULL;
	snprintf(fname, 255, font->name);
	changeFontProp(fname, "bold", 2);
	newfont = WMCreateNormalFont(scrPtr, fname);
	if(!newfont)
		newfont = font;
	return newfont;
}

WMFont *
WMGetFontItalic(WMScreen *scrPtr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];
	if(!scrPtr || !font)
		return NULL;
	snprintf(fname, 255, font->name);
	changeFontProp(fname, "o", 3);
	newfont = WMCreateNormalFont(scrPtr, fname);
	if(!newfont)
		newfont = font;
	return newfont;
}

WMFont *
WMGetFontOfSize(WMScreen *scrPtr, WMFont *font, short size) 
{
	WMFont *nfont=NULL;
	if(!scrPtr || !font || size<1)
		return NULL;
	return font;
}
/*                                          */

void
WMFreezeText(WMText *tPtr)
{
	if(!tPtr)
		return;
	tPtr->frozen = True;
}   

void
WMThawText(WMText *tPtr)
{
	if(!tPtr)
		return;
	tPtr->frozen = False;
}   


void
WMSetTextDefaultAlignment(WMText *tPtr, WMAlignment alignment)
{   
	if(!tPtr) return;
	if(tPtr->monoFont) return;

	tPtr->dAlignment = alignment;
	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}



void
WMSetTextBackgroundColor(WMText *tPtr, WMColor *color)
{
	if(!tPtr)
		return;

	if(color)
		tPtr->bg = color;
	else
		tPtr->bg = WMWhiteColor(tPtr->view->screen);

	W_SetViewBackgroundColor(tPtr->view, tPtr->bg);
	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}   

void 
WMSetTextDefaultColor(WMText *tPtr, WMColor *color)
{
	if(!tPtr)
		return;

	if(color)
		tPtr->dColor = color;
	else
		tPtr->dColor = WMBlackColor(tPtr->view->screen);
}

void 
WMSetTextDefaultFont(WMText *tPtr, WMFont *font)
{
	if(!tPtr)
		return;

	if(font)
		tPtr->dFont = font;
	else
		tPtr->dFont = WMRetainFont(tPtr->view->screen->normalFont);
}

void 
WMSetTextUseFixedPitchFont(Text *tPtr, Bool fixed)
{
    if(!tPtr)
        return;
	if(fixed)
		tPtr->dFont = WMCreateFontSet(tPtr->view->screen, 
			"lucidasanstypewriter-12");
	else
		tPtr->dFont = WMRetainFont(tPtr->view->screen->normalFont);
	tPtr->fixedPitch = fixed;
}

void
WMSetTextParser(WMText *tPtr, WMParseAction *parser)
{
    if(!tPtr) return;
	if(tPtr->monoFont) return;
    tPtr->parser = parser;
}   


WMParserActions 
WMGetTextParserActions(WMText *tPtr)
{
	WMParserActions null;
	if(!tPtr) return null;
	return tPtr->funcs;
}


char *
WMGetTextAll(WMText *tPtr)
{
	char *text;
	int length=0;
	int where=0;
	Paragraph *para;
	Chunk *chunk;
    
	if(!tPtr) return NULL;
    
	para = tPtr->paragraphs;
	while(para) {
		chunk = para->chunks;
		while(chunk) {
			if(chunk->type == ctText) {
				if(chunk->text) length += chunk->chars;
			} else {
				printf("getting image \n");
			}
		 	chunk = chunk->next;
		}
	   
		if(tPtr->ignoreNewLine) break;
		length += 4;  // newlines
		para = para->next;
	}
    
	text = wmalloc(length+1);
    
	para = tPtr->paragraphs;
	while(para) {
		chunk = para->chunks;
		while(chunk) {
			if(chunk->type == ctText) {
				if(chunk->text) {
					snprintf(&text[where], chunk->chars+1, "%s", chunk->text);
					where += chunk->chars;
				}
			} else {
				printf("writing image \n");
			}
		 chunk = chunk->next;
		}
		if(tPtr->ignoreNewLine) break;
		snprintf(&text[where++], 2, "\n");
		para = para->next;
	 } text[where] = '\0';
    
    return text;
}   

void
WMSetTextWriter(WMText *tPtr, WMParseAction *writer)
{
    if(!tPtr)
        return;
	if(tPtr->monoFont)
		return;
    tPtr->writer = writer;
}   

