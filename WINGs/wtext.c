/*
 *  WINGs WMText multi-line/font/color/graphic text widget
 *
 *  Copyright (c) 1999-2000 Nwanua Elumeze
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* README README README README README README README
 * Nwanua: dont use // style comments please!
 * It doesnt work in lots of compilers out there :/
 * -Alfredo
 * README README README README README README README
 */

#include "WINGsP.h"
#include <X11/keysym.h>
#include <X11/Xatom.h>


//_______
//TODO: 

#if 0
    
use currentTextBlock and neighbours for fast paint and layout

WMGetTextStreamAll...  WMGetTextStream WMGetTextSelection(if(selected) )

the bitfield arrangement in this code assumes a little-endian 
   machine...  might need a __BIG_ENDIAN__ define for arranging 
   the bitfields efficiently for those big boys. 

make a file named fontman.c, put that kind of
stuff in there and not put the APIs in WINGs.h
WMGetFontItalic() should some day be part of the font manager
instead, just put a bunch of extern WMGetFontbla in the top of wtext.c
  
#endif

//_______




/* a Section is a section of a TextBlock that describes what parts
 	  of a TextBlock has be layout on which "line"... 
   o  this greatly aids redraw, scroll and selection. 
   o  this is created during layoutLine, but may be later modified.
   o  there may be many regions per TextBlock, hence the array */
typedef struct {
	int x, y;				/* where to draw it from */
	int w, h;				/* it's width and height (to aid selection) */
	int _y;
	unsigned short begin, end;	/* what part of the text block */
} Section;


/* a TextBlock is a doubly-linked list of TextBlocks containing:
	o text for the block, color and font
    o or a pointer to the widget and the (text) description for its graphic
	o but NOT both */

typedef struct _TextBlock {
	struct _TextBlock *next;	/* next text block in linked list */
	struct _TextBlock *prior;	/* prior text block in linked list */

	char *text;					/* pointer to 8- or 16-bit text */
								/* or to the object's description */
	union {
		WMFont *font;			/* the font */
		WMWidget *widget;		/* the embedded widget */
	} d;						/* description */

	WMColor *color;				/* the color */
	Section *sections;		/* the region for layouts (a growable array) */
								/* an _array_! of size _nsections_ */

	unsigned short used;		/* number of chars in this block */
	unsigned short allocated;	/* size of allocation (in chars) */

	unsigned int first:1;		/* first TextBlock in paragraph */
	unsigned int blank:1;		/* ie. blank paragraph */
	unsigned int kanji:1;		/* is of 16-bit characters or not */
	unsigned int graphic:1;		/* embedded object or text: text=0 */
	unsigned int underlined:1;	/* underlined or not */
	unsigned int nsections:8;	/* over how many "lines" a TexBlock wraps */
			 int script:8;		/* script in points: negative for subscript */
	unsigned int marginN:10;	/* which of the margins in WMText to use */
	unsigned int RESERVED:1;
} TextBlock;


/* somehow visible.h  beats the hell outta visible.size.height :-) */
typedef struct {
	unsigned int y;
	unsigned int x;
	unsigned int h;
	unsigned int w;
} myRect;


typedef struct W_Text {
	W_Class widgetClass;	/* the class number of this widget */
	W_View  *view;			/* the view referring to this instance */

	WMRuler *ruler;			/* the ruler subwiget to manipulate paragraphs */

	WMScroller *vS;			/* the vertical scroller */
	int vpos;				/* the current vertical position */
	int prevVpos;			/* the previous vertical position */

	WMScroller *hS;			/* the horizontal scroller */
	int hpos;				/* the current horizontal position */
	int prevHpos;			/* the previous horizontal position */
							/* in short: tPtr->hS?nowrap:wrap  */

	WMFont *dFont;          /* the default font */
	WMColor *dColor;        /* the default color */
	WMPixmap *dBulletPix;   /* the default pixmap for bullets */

	GC bgGC;				/* the background GC to draw with */
	GC fgGC;				/* the foreground GC to draw with */
	Pixmap db;				/* the buffer on which to draw */

	WMRulerMargins *margins;/* a (growable) array of margins to be used */
							/*	by the various TextBlocks */

	myRect visible;			/* the actual rectangle that can be drawn into */
	myRect sel;				/* the selection rectangle */
	int docWidth;			/* the width of the entire document */
	int docHeight;			/* the height of the entire document */


	TextBlock *firstTextBlock;
	TextBlock *lastTextBlock;
	TextBlock *currentTextBlock;

	
	WMBag		*gfxItems;	/* a nice bag containing graphic items */

	WMPoint clicked;		/* where in the _document_ was clicked */
	unsigned short tpos;	/* the character position in the currentTextBlock */
	unsigned short RESERVED;/* space taker upper... */
	

	WMAction *parser;
	WMAction *writer;

	struct {
		unsigned int monoFont:1;	/* whether to ignore formats */
		unsigned int focused:1;     /* whether this instance has input focus */
		unsigned int editable:1;	/* "silly user, you can't edit me" */
		unsigned int ownsSelection:1; /* "I ownz the current selection!" */
		unsigned int pointerGrabbed:1;/* "heh, gib me pointer" */
		unsigned int buttonHeld:1;	/* the user is holding down the button */
		unsigned int waitingForSelection:1;	/* dum dee dumm... */
		unsigned int extendSelection:1;	/* shift-drag to select more regions */

		unsigned int rulerShown:1;	/* whether the ruler is shown or not */
		unsigned int frozen:1;		/* whether screen updates are to be made */
		unsigned int cursorShown:1;	/* whether to show the cursor */
		unsigned int clickPos:1;	/* clicked before=0/after=1 a graphic: */
									/* within counts as after too */

		unsigned int ignoreNewLine:1;/* "bleh XK_Return" ignore it when typed */
		unsigned int laidOut:1;		/* have the TextBlocks all been laid out */
		unsigned int prepend:1; 	/* prepend=1, append=0 (for parsers) */
		WMAlignment alignment:2;	/* the alignment for text */
		WMReliefType relief:3;		/* the relief to display with */
		unsigned int RESERVED:4;
		unsigned int nmargins:10;	/* the number of margin arrays */
	} flags;
} Text;

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


/* done purely for speed ... mostly same as WMWidthOfString */
static inline unsigned int 
myWidthOfString(WMFont *font, char *text, unsigned int length)
{
	if (font->notFontSet)
		return XTextWidth(font->font.normal, text, length);
	else {
		XRectangle rect, AIXsucks;
		XmbTextExtents(font->font.set, text, length, &AIXsucks, &rect);
		return rect.width;
	}
}


static void 
paintText(Text *tPtr)
{   
	TextBlock *tb = tPtr->firstTextBlock;
	WMFont *font;
	GC gc, greyGC;
	char *text;
	int len, y, c, s, done=False;
	int prev_y=-23;
	WMScreen *scr = tPtr->view->screen;
	Display *dpy = tPtr->view->screen->display;
	Window win = tPtr->view->window;
	
	

	if(!tPtr->view->flags.realized || !tPtr->db)
		return;

	XFillRectangle(dpy, tPtr->db, tPtr->bgGC, 
		0, 0, tPtr->visible.w, tPtr->visible.h);


	tb = tPtr->firstTextBlock;
    if(!tb)
      goto _copy_area;
        

	if(tPtr->flags.ownsSelection) {
		greyGC = WMColorGC(WMGrayColor(scr));
			//XFillRectangle(dpy, tPtr->db, greyGC,
//			tPtr->sel.x, tPtr->sel.y-tPtr->vpos, tPtr->sel.w, tPtr->sel.h);
	//	XDrawRectangle(dpy, tPtr->db, tPtr->fgGC, 
	//		tPtr->sel.x, tPtr->sel.y-tPtr->vpos, tPtr->sel.w, tPtr->sel.h);
	}

	done = False;
	while(!done && tb) {

		if(!tb->sections || (!tPtr->flags.monoFont && tb->graphic))  {
			tb = tb->next;
			continue;
		}

		for(s=0; s<tb->nsections && !done; s++) {


			if(tb->sections[s]._y  > tPtr->vpos + tPtr->visible.h) {
				done = True;
				break;
			}

			if( tb->sections[s].y + tb->sections[s].h < tPtr->vpos) 
				continue; 

			if(tPtr->flags.monoFont) {
				font = tPtr->dFont;
				gc = tPtr->fgGC;
			} else {
				font = tb->d.font;
				gc = WMColorGC(tb->color);
			}

			if(tPtr->flags.ownsSelection) {

				if(prev_y != tb->sections[s]._y
					&& (tb->sections[s]._y >= tPtr->sel.y) 
					&& (tb->sections[s]._y + tb->sections[s].h 
						<= tPtr->sel.y + tPtr->sel.h)) {
					XFillRectangle(dpy, tPtr->db, greyGC,
						tPtr->visible.x, 
						tPtr->visible.y + tb->sections[s]._y - tPtr->vpos, 
						tPtr->visible.w,  tb->sections[s].h);

				} else if( prev_y != tb->sections[s]._y
					&& (tb->sections[s]._y <= tPtr->sel.y)
					&& (tb->sections[s]._y + tb->sections[s].h 
						>= tPtr->sel.y)
					&& (tPtr->sel.x >= tb->sections[s].x)
					&& (tPtr->sel.y + tPtr->sel.h
						>= tb->sections[s]._y + tb->sections[s].h)) {
					XFillRectangle(dpy, tPtr->db, greyGC,
						tPtr->clicked.x, 
						tPtr->visible.y + tb->sections[s]._y - tPtr->vpos,
						tPtr->visible.w - tPtr->sel.x, tb->sections[s].h);

				} else if(prev_y != tb->sections[s]._y
					&& (tb->sections[s]._y <= tPtr->sel.y + tPtr->sel.h)
					&& (tb->sections[s]._y >= tPtr->sel.y)) {  
					XFillRectangle(dpy, tPtr->db, greyGC,
						tPtr->visible.x, 
						tPtr->visible.y + tb->sections[s]._y - tPtr->vpos,
						tPtr->sel.x + tPtr->sel.w -tPtr->visible.x,	
						tb->sections[s].h);

				} else if( prev_y != tb->sections[s]._y
					&& (tb->sections[s]._y <= tPtr->sel.y)
					&& (tb->sections[s]._y + tb->sections[s].h 
						>= tPtr->sel.y + tPtr->sel.h)  ) { 
					XFillRectangle(dpy, tPtr->db, greyGC,
						tPtr->sel.x, 
						tPtr->visible.y + tb->sections[s]._y - tPtr->vpos,
						tPtr->sel.w,tb->sections[s].h);
				}
			
			}

			prev_y = tb->sections[s]._y;

			len = tb->sections[s].end - tb->sections[s].begin;
			text = &(tb->text[tb->sections[s].begin]);
			y = tb->sections[s].y - tPtr->vpos;
			WMDrawString(scr, tPtr->db, gc, font, 
				tb->sections[s].x, y, text, len);

			if(tb->underlined) { 
				XDrawLine(dpy, tPtr->db, gc, 	
					tb->sections[s].x, y + font->y + 1,
					tb->sections[s].x + tb->sections[s].w, y + font->y + 1);
			}
						
		}

		tb = (!done? tb->next : NULL);

	}
			
 	c = WMGetBagItemCount(tPtr->gfxItems);
	if(c > 0 && !tPtr->flags.monoFont) {
		int j; 
		WMWidget *wdt;
		for(j=0; j<c; j++) {
			tb = (TextBlock *)WMGetFromBag(tPtr->gfxItems, j);
			if(!tb || !tb->sections)
				continue;
			wdt = tb->d.widget;
			if(tb->sections[0]._y + tb->sections[0].h 
					<= tPtr->vpos 
				|| tb->sections[0]._y  
					>= tPtr->vpos + tPtr->visible.h ) {
		
				if((W_VIEW(wdt))->flags.mapped) {
					WMUnmapWidget(wdt);
				}
			} else {
				if(!(W_VIEW(wdt))->flags.mapped) {
					WMMapWidget(wdt);
					XLowerWindow(dpy,
						(W_VIEW(wdt))->window);
				}
				
				if(tPtr->flags.ownsSelection && 0
					//&& (tb->sections[s]._y >= tPtr->sel.y) 
					//&& (tb->sections[s]._y + tb->sections[s].h 
					){ //	<= tPtr->sel.y + tPtr->sel.h)) {
					XFillRectangle(dpy, tPtr->db, greyGC,
						tb->sections[0].x, tb->sections[0].y - tPtr->vpos, 
						tb->sections[0].w, tb->sections[0].h);
				}

				WMMoveWidget(wdt, 3 + tb->sections[0].x + tPtr->visible.x, 
					tb->sections[0].y - tPtr->vpos);

				if(tb->underlined) { 
					XDrawLine(dpy, tPtr->db, WMColorGC(tb->color), 
					tb->sections[0].x,
					tb->sections[0].y + WMWidgetHeight(wdt) + 1,
					tb->sections[0].x + tb->sections[0].w,
					tb->sections[0].y + WMWidgetHeight(wdt) + 1);
		} } } }
	

_copy_area:



	XCopyArea(dpy, tPtr->db, win, tPtr->bgGC, 
		0, 0,
        tPtr->visible.w, tPtr->visible.h, 
		tPtr->visible.x, tPtr->visible.y);

	W_DrawRelief(scr, win, 0, 0,
		tPtr->view->size.width, tPtr->view->size.height, 
		tPtr->flags.relief);		

	if(tPtr->ruler && tPtr->flags.rulerShown)
		XDrawLine(dpy, win, 
			tPtr->fgGC, 2, 42, 
			tPtr->view->size.width-4, 42);

/*
	XFillRectangle(tPtr->view->screen->display, tPtr->view->window,
		tPtr->bgGC,
		2, tPtr->view->size.height-3, 
		tPtr->view->size.width-4, 3);
*/

}


static  void
cursorToTextPosition(Text *tPtr, int x, int y)
{
	TextBlock *tb = NULL;
	int done=False, s, len, _w, _y, dir=1; /* 1 == "down" */
	WMFont *font;
	char *text;


	y += tPtr->vpos - tPtr->visible.y;
	if(y<0) y = 0;

	x -= tPtr->visible.x-2; 
	if(x<0) x=0;

	tPtr->clicked.x = x;
	tPtr->clicked.y = y;

	/* first, which direction?, most likely, newly clicked 
		position will be  close to previous */
	tb = tPtr->currentTextBlock;
	if(!tb) 
		tb = tPtr->firstTextBlock;
	if(!tb || !tb->sections) 
		return;

	if(y < tb->sections[0].y)
		dir = 0; /* "up" */

//tb = tPtr->firstTextBlock;
//dir = 1;


	if(y == tb->sections[0].y)
		goto _doneV;  /* yeah yeah, goto, whatever... :-P */

	/* get the first section of the first TextBlock based on v. position */
	done = False;
	while(!done && tb) { 
		if(tPtr->flags.monoFont && tb->graphic) {
			tb = tb->next;
			continue;
		}

printf("tb %p t[%c] blank%d  graphic %d\n", tb, 
*tb->text, tb->blank, tb->graphic);
		if(!tb->sections) {
			printf("we have a bad thing!\n");
			exit(1);
		}
		s = (dir? 0 : tb->nsections-1);
		while( (dir? (s<tb->nsections) : (s>=0) )) {
			if( y >= tb->sections[s]._y 
				&& y <= tb->sections[s]._y + tb->sections[s].h) { 
					done = True;
					break;
			} else 
				dir? s++ : s--;
		}
		if(!done) tb = (dir ? tb->next : tb->prior);
	}

_doneV:
	/* we have the line, which TextBlock on that line is it? */
	if(tb) 
		_y = tb->sections[s]._y;

	while(tb) { 
		if(!tb->sections)
			break;
		if(_y != tb->sections[s]._y)	
			break;

		if(tb->graphic) {
			_w = WMWidgetWidth(tb->d.widget);
		} else {
			text = &(tb->text[tb->sections[s].begin]);
			len = tb->sections[s].end - tb->sections[s].begin;
			font = tb->d.font;
			_w = myWidthOfString(font, text, len);
		}
		if(tb->sections[s].x + _w >= x)
			break;
		s = 0; 
		tb = tb->next;
	}
		
	/* we have said TextBlock, now where within it? */
	if(tb && !tb->graphic) {
		int begin = tb->sections[s].begin;
		int end = tb->sections[s].end;
		int i=0;
		len = end-begin;
		text = &(tb->text[begin]);
		font = tb->d.font;

		_w = x - tb->sections[s].x;

		i = 0;
		while(i<len && myWidthOfString(font, text, i+1) <  _w)
			i++;
		
		i += begin;
		tPtr->tpos = (i<tb->used)?i:tb->used;
	}
		
//	if(!tb) 
		//tb = tPtr->firstTextBlock;
	tPtr->currentTextBlock = tb;

	if(tb &&tb->graphic) printf("graphic\n");
}

static void
updateScrollers(Text *tPtr)
{   

	if(tPtr->vS) {
		if(tPtr->docHeight < tPtr->visible.h) {
			WMSetScrollerParameters(tPtr->vS, 0, 1);
			tPtr->vpos = 0;
		} else {   
			float vmax = (float)(tPtr->docHeight);
			WMSetScrollerParameters(tPtr->vS,
				((float)tPtr->vpos)/(vmax - (float)tPtr->visible.h),
				(float)tPtr->visible.h/vmax);
		}       
	} else tPtr->vpos = 0;
    
	if(tPtr->hS)
		;
} 

static void
scrollersCallBack(WMWidget *w, void *self)
{   
	Text *tPtr = (Text *)self;
    Bool scroll = False;
    Bool dimple = False;
	int which;

	if(!tPtr->view->flags.realized) return;

	if(w == tPtr->vS) {
		float vmax; 
		int height; 
		vmax = (float)(tPtr->docHeight);
		height = tPtr->visible.h;

 		which = WMGetScrollerHitPart(tPtr->vS);
		switch(which) { 
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
				tPtr->vpos = WMGetScrollerValue(tPtr->vS)
					* (float)(tPtr->docHeight - height);
					scroll = True;
			break; 
            
			case WSKnobSlot:
			case WSNoPart:
printf("WSNoPart, WSKnobSlot\n");
#if 0
float vmax = (float)(tPtr->docHeight);
((float)tPtr->vpos)/(vmax - (float)tPtr->visible.h), 
(float)tPtr->visible.h/vmax;
dimple =where mouse is.
#endif
			break;
		} 
		scroll = (tPtr->vpos != tPtr->prevVpos);
		tPtr->prevVpos = tPtr->vpos;
	}
		
	if(w == tPtr->hS)
		;
    
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
		if(which == WSDecrementLine || which == WSIncrementLine)
			updateScrollers(tPtr);
		paintText(tPtr);
	}
}



typedef struct {
    TextBlock *tb;
	unsigned short begin, end;	/* what part of the text block */
	
} myLineItems;


static int
layOutLine(Text *tPtr, myLineItems *items, int nitems, int x, int y,
int pwidth, WMAlignment align)
{   
	int i, j=0;  /* j = justification */
	int line_width = 0, line_height=0, max_descent=0;
	WMFont *font;
	char *text;
	int len;
	TextBlock *tb;
	Bool gfx=0;
	TextBlock *tbsame=NULL;

	for(i=0; i<nitems; i++) {
		tb = items[i].tb;

		if(tb->graphic) { 
			if(!tPtr->flags.monoFont) {
				WMWidget *wdt = tb->d.widget;
				line_height = WMAX(line_height, WMWidgetHeight(wdt));
				if(align != WALeft) 
					line_width += WMWidgetWidth(wdt);
				gfx = True;
			}
			
		} else { 
			font = (tPtr->flags.monoFont)?tPtr->dFont : tb->d.font;
			max_descent = WMAX(max_descent, font->height-font->y); 
			line_height = WMAX(line_height, font->height); //+font->height-font->y);
			text = &(tb->text[items[i].begin]);
			len = items[i].end - items[i].begin;
			if(align != WALeft) 
				line_width += myWidthOfString(font,	text, len);
		}
	}
	
	if(align == WARight) {
		j = pwidth - line_width;
	} else if (align == WACenter) {
		j = (int) ((float)(pwidth - line_width))/2.0;
	}   
	if(gfx)
		y+=10;

	for(i=0; i<nitems; i++) {
		tb = items[i].tb;

		if(tbsame == tb) {  /*extend it, since it's on same line */
			tb->sections[tb->nsections-1].end = items[i].end;
		} else {
			tb->sections = wrealloc(tb->sections,
				(++tb->nsections)*sizeof(Section));
			tb->sections[tb->nsections-1]._y = y;
			tb->sections[tb->nsections-1].x = x+j;
			tb->sections[tb->nsections-1].h = line_height;
			tb->sections[tb->nsections-1].begin = items[i].begin;
			tb->sections[tb->nsections-1].end = items[i].end;
		}
		

		if(tb->graphic) { 
			if(!tPtr->flags.monoFont) {
				WMWidget *wdt = tb->d.widget;
				tb->sections[tb->nsections-1].y = 1 +max_descent +
					y + line_height - WMWidgetHeight(wdt);
				tb->sections[tb->nsections-1].w = WMWidgetWidth(wdt);
				x += tb->sections[tb->nsections-1].w;
			}
		} else {
			font = (tPtr->flags.monoFont)? tPtr->dFont : tb->d.font;
			len = items[i].end - items[i].begin;

			text = &(tb->text[items[i].begin]);

			tb->sections[tb->nsections-1].y = y+line_height-font->y;
			tb->sections[tb->nsections-1].w = 
				myWidthOfString(font,  
					&(tb->text[tb->sections[tb->nsections-1].begin]), 
					tb->sections[tb->nsections-1].end -
					tb->sections[tb->nsections-1].begin);

			x += myWidthOfString(font,  text, len); 
		} 

		tbsame = tb;
	}

	return line_height+(gfx?10:0);
					
}


static void 
output(char *ptr, int len)
{   
    char s[len+1]; 
    memcpy(s, ptr, len);
    s[len] = 0; 
    printf(" s is [%s] (%d)\n",  s, strlen(s));
}



#define MAX_TB_PER_LINE 64

static void
layOutDocument(Text *tPtr) 
{
	TextBlock *tb;
	myLineItems items[MAX_TB_PER_LINE];
	WMAlignment align = WALeft;
	WMFont *font;
	Bool lhc = !tPtr->flags.laidOut; /* line height changed? */
	int prev_y;

	int nitems=0, x=0, y=0, line_width = 0, width=0;
	int pwidth = tPtr->visible.w - tPtr->visible.x; 
		
	char *start=NULL, *mark=NULL;
	int begin, end;

	if(!(tb = tPtr->firstTextBlock)) {
		printf("clear view... *pos=0\n");
		return;
	}

	if(0&&tPtr->flags.laidOut) {
		tb = tPtr->currentTextBlock;
		if(tb->sections && tb->nsections>0) 
			prev_y = tb->sections[tb->nsections-1]._y;
y+=10;
printf("1 prev_y %d \n", prev_y); 

		/* search backwards for textblocks on same line */
		while(tb) {
			if(!tb->sections || tb->nsections<1) {
				tb = tPtr->firstTextBlock;
				break;
			}
			if(tb->sections[tb->nsections-1]._y != prev_y) {
				tb = tb->next;
				break;
			}
		//	prev_y = tb->sections[tb->nsections-1]._y;
			tb = tb->prior;
		}
		y = 0;//tb->sections[tb->nsections-1]._y;
printf("2 prev_y %d \n\n", tb->sections[tb->nsections-1]._y); 
	}


	while(tb) {

		if(tb->sections && tb->nsections>0) {
			wfree(tb->sections);
			tb->sections = NULL;
			tb->nsections = 0;
		} 

		if(tb->first) {
			y += layOutLine(tPtr, items, nitems, x, y, pwidth, align);
			x = 0;//tPtr->visible.x+2;
			nitems = 0;
			line_width = 0;
		}
			
		if(tb->graphic) { 
			if(!tPtr->flags.monoFont) { 
				width = WMWidgetWidth(tb->d.widget);
				if(width > pwidth)printf("rescale graphix to fit?\n");
				line_width += width;
				if(line_width >= pwidth - x 
				|| nitems >= MAX_TB_PER_LINE) {
					y += layOutLine(tPtr, items, nitems, x, y, 
						pwidth, align);
					nitems = 0;
					x = 0;//tPtr->visible.x+2;
					line_width = width;
				}

    			items[nitems].tb = tb;
    			items[nitems].begin = 0;
    			items[nitems].end = 0;
				nitems++;
			}

		} else if((start = tb->text)) {
			begin = end = 0;
			font = tPtr->flags.monoFont?tPtr->dFont:tb->d.font;

			while(start) {
				mark = strchr(start, ' ');
				if(mark) {
					end += (int)(mark-start)+1;
					start = mark+1;
				} else {
					end += strlen(start);
					start = mark;
				}

				if(end-begin > 0) {
					
					width = myWidthOfString(font, 
						&tb->text[begin], end-begin);

					if(width > pwidth) { /* break this tb up */
						char *t = &tb->text[begin];
						int l=end-begin, i=0;
						do { 
							width = myWidthOfString(font, t, ++i);
						} while (width < pwidth && i < l);  
						end = begin+i;
						if(start) // and since (nil)-4 = 0xfffffffd
							 start -= l-i;	
					
					}

					line_width += width;
				}

				if((line_width >= pwidth - x) 
				|| nitems >= MAX_TB_PER_LINE) { 
					y += layOutLine(tPtr, items, nitems, x, y, 
						pwidth, align); 
					line_width = width;
					x = 0; //tPtr->visible.x+2;
					nitems = 0;
				}

				items[nitems].tb = tb;
				items[nitems].begin = begin;
				items[nitems].end = end;
				nitems++;

				begin = end;
			}
		}
		tb = tb->next;
	}


	if(nitems > 0) 
		y += layOutLine(tPtr, items, nitems, x, y, pwidth, align);               
	if(lhc) {
		tPtr->docHeight = y+10;
		updateScrollers(tPtr);
	}
	tPtr->flags.laidOut = True;
		
}


static void
textDidResize(W_ViewDelegate *self, WMView *view)
{   
	Text *tPtr = (Text *)view->self;
	unsigned short w = WMWidgetWidth(tPtr);
	unsigned short h = WMWidgetHeight(tPtr);
	unsigned short rh = 0, vw = 0;

	if(tPtr->ruler && tPtr->flags.rulerShown) { 
		WMMoveWidget(tPtr->ruler, 20, 2);
		WMResizeWidget(tPtr->ruler, w - 22, 40);
		rh = 40;
	}   
		
	if(tPtr->vS) { 
		WMMoveWidget(tPtr->vS, 1, rh + 2);
		WMResizeWidget(tPtr->vS, 20, h - rh - 3);
		vw = 20;
		WMSetRulerOffset(tPtr->ruler, 22);
	} else WMSetRulerOffset(tPtr->ruler, 2);

	if(tPtr->hS) {
		if(tPtr->vS) {
			WMMoveWidget(tPtr->hS, vw, h - 21);
			WMResizeWidget(tPtr->hS, w - vw - 1, 20);
		} else {
			WMMoveWidget(tPtr->hS, vw+1, h - 21);
			WMResizeWidget(tPtr->hS, w - vw - 2, 20);
		}
	}

	tPtr->visible.x = (tPtr->vS)?22:0;
	tPtr->visible.y = (tPtr->ruler && tPtr->flags.rulerShown)?43:3;
	tPtr->visible.w = tPtr->view->size.width - tPtr->visible.x - 12;
	tPtr->visible.h = tPtr->view->size.height - tPtr->visible.y;
	tPtr->visible.h -= (tPtr->hS)?20:0;

	tPtr->margins[0].left = tPtr->margins[0].right = tPtr->visible.x;
	tPtr->margins[0].body = tPtr->visible.x;
	tPtr->margins[0].right = tPtr->visible.w;

	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);

	if(tPtr->db) {
		//if(tPtr->view->flags.realized)
		//XFreePixmap(tPtr->view->screen->display, tPtr->db);
	}

		//if(size did not change
		if(tPtr->visible.w < 10) tPtr->visible.w = 10;
		if(tPtr->visible.h < 10) tPtr->visible.h = 10;
	
		tPtr->db = XCreatePixmap(tPtr->view->screen->display, 
			tPtr->view->window, tPtr->visible.w, 
			tPtr->visible.h, tPtr->view->screen->depth);
	
	paintText(tPtr);
}

W_ViewDelegate _TextViewDelegate =
{
	NULL,
	NULL,
	textDidResize,
	NULL,
};

/* nice, divisble-by-16 memory */
static inline unsigned short
reqBlockSize(unsigned short requested)
{
	return requested+16-(requested%16);
}

static void
deleteTextInteractively(Text *tPtr, KeySym ksym)
{
	printf("deleting %ld\n", ksym);
}
	
static void
insertTextInteractively(Text *tPtr, char *text, int len)
{
	TextBlock *tb;

//	Chunk *tb=NULL, *newtb=NULL;
	int height = -23; /* should only be changed upon newline */
	int w=0;
	WMFont *font;
	char *mark = NULL;
    
	if(!tPtr->flags.editable || len < 1 || !text
		|| (*text == '\n' && tPtr->flags.ignoreNewLine))
		 return;

	tb = tPtr->currentTextBlock;
	if(!tb) {
		WMAppendTextStream(tPtr, text);
		WMRefreshText(tPtr, 0, 0);
		return;
	}

	if(tb->graphic)
		return;

	if(len > 1) { 
		mark = strchr(text, '\n');
		if(mark) {
			len = (int)(mark-text);
			mark++;
		}
		if(len<1 && mark) {
			printf("problem pasting text %d\n", len);
			len = strlen(text);
			mark = NULL;
		}
	}
		
 	font = (tPtr->flags.monoFont || !tb)?tPtr->dFont:tb->d.font;
	
#if 0
	if(*text == '\n') {
		int new_top=0;
		if(tb) { /* there's a tb (or part of it) to detach from old */
			int current = WMGetTextCurrentChunk(tPtr);
			if(tPtr->tpos <=0) { /* at start of tb */
				if(current<1) { /* the first tb... make old para blank */
					newtb = para->tbs;
					para->tbs = NULL;
					putParagraphOnPixmap(tPtr, para, True);
				} else { /* not first tb... */
					printf("cut me out \n");
				}	
			} else if(tPtr->tpos < tb->chars && tb->type == ctText) {
					 /* not at start of tb */
				char text[tb->chars-tPtr->tpos+1];
				int i=0;
				do {
					text[i] = tb->text[tPtr->tpos+i];    
				} while(++i < tb->chars-tPtr->tpos);
				tb->chars -= i;
				newtb = (tPtr->funcs.createTChunk) (text, i, tb->font,
					tb->color, tb->script, tb->ul);
				newtb->next = tb->next;
				tb->next = NULL;
				  /* might want to demalloc for LARGE cuts */
		//calcParaExtents(tPtr, para);
				para->height = putParagraphOnPixmap(tPtr, para, True);
				//putParagraphOnPixmap(tPtr, para, True);
			} else if(tPtr->tpos >= tb->chars) { 
				Chunk *prev;
				WMSetTextCurrentChunk(tPtr, current-1);
				prev = tPtr->currentChunk;
				if(!prev) return;
				newtb = prev->next;
				prev->next = NULL;
				putParagraphOnPixmap(tPtr, para, True);
			}
		} else newtb = NULL;

		if(para)  /* the preceeding one */
			new_top = para->bottom;
		
		WMAppendTextStream(tPtr, "\n");
		para = tPtr->currentPara;
		if(!para) return;  
		para->tbs = newtb;
		tPtr->currentChunk = newtb;
		tPtr->tpos = 0;
		para->top = new_top;
		calcParaExtents(tPtr, para);
		height = para->height;
	} else {
		if(!para) {
			WMAppendTextStream(tPtr, text);
			para = tPtr->currentPara;
		} else if(!para->tbs || !tb) {
			//WMPrependTextStream(tPtr, text);
			WMAppendTextStream(tPtr, text);
		} else if(tb->type == ctImage) {
			WMPrependTextStream(tPtr, text);

printf("\n\nprepe\n\n");
		} else {
			if(tPtr->tpos > tb->chars) {
printf("\n\nmore\n\n");
				tPtr->tpos = tb->chars;
			}
#endif

printf("len is %d\n", len);
			if(tb->used+len >= tb->allocated) {
				tb->allocated = reqBlockSize(tb->used+len);
printf("ralloced %d\n", tb->allocated);
				tb->text = wrealloc(tb->text, tb->allocated);
			}
		
			if(tb->blank) {
				memmove(tb->text, text, len);
				tb->used = len;
 				tPtr->tpos = len;
				tb->blank = False;
			} else {
				memmove(&(tb->text[tPtr->tpos+len]), &tb->text[tPtr->tpos],
					tb->used-tPtr->tpos+1);
				memmove(&tb->text[tPtr->tpos], text, len);
 				tb->used += len;
 				tPtr->tpos += len;
			}
				w = myWidthOfString(font, text, len); 
			 

	if(mark) { 
		WMAppendTextStream(tPtr, mark);
		WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
printf("paste: use  prev/post chunk's fmt...\n");
	} else {
		layOutDocument(tPtr);
		paintText(tPtr);
	}
#if 0
//doc->clickstart.cursor.x += 
//myWidthOfString(tb->fmt->font, text,len);
		}
	}
#endif
}


static void 
selectRegion(Text *tPtr, int x, int y)
{
	if(x < 0 || y < 0) 
		return;
	y += tPtr->vpos;
	if(y>10) y -= 10;  /* the original offset */

	x -= tPtr->visible.x-2;
	if(x<0) x=0;

    tPtr->sel.x = WMAX(0, WMIN(tPtr->clicked.x, x));
    tPtr->sel.w = abs(tPtr->clicked.x - x);
    tPtr->sel.y = WMAX(0, WMIN(tPtr->clicked.y, y));
    tPtr->sel.h = abs(tPtr->clicked.y - y);

	tPtr->flags.ownsSelection = True;
	paintText(tPtr);
}


static void
pasteText(WMView *view, Atom selection, Atom target, Time timestamp,
	void *cdata, WMData *data)
{     
	Text *tPtr = (Text *)view->self;
	char *str; 
    
	tPtr->flags.waitingForSelection = False;

	if(data) {
		str = (char*)WMDataBytes(data);
			insertTextInteractively(tPtr, str, strlen(str));
	} else {
		int n;
		str = XFetchBuffer(tPtr->view->screen->display, &n, 0);
		if(str) {
			str[n] = 0;
			insertTextInteractively(tPtr, str, n);
			XFree(str);
		}
	}
}

static  void
releaseSelection(Text *tPtr)
{
	printf("I have %d selection\n", 1);
	tPtr->flags.ownsSelection = False;
	paintText(tPtr);
}

static WMData *
requestHandler(WMView *view, Atom selection, Atom target, 
	void *cdata, Atom *type)
{
	Text *tPtr = view->self;
	int count;
	Display *dpy = tPtr->view->screen->display;
	Atom TEXT = XInternAtom(dpy, "TEXT", False);
	Atom COMPOUND_TEXT = XInternAtom(dpy, "COMPOUND_TEXT", False);
	
	*type = target;
	if (target == XA_STRING || target == TEXT || target == COMPOUND_TEXT)
		return WMGetTextSelected(tPtr);
	else  { 
		WMData *data = WMCreateDataWithBytes("bleh", 4);
		return data;
	}

	return NULL;
}
		

static void
lostHandler(WMView *view, Atom selection, void *cdata)
{
	releaseSelection((WMText *)view->self);
}   

static WMSelectionProcs selectionHandler = {
	requestHandler, lostHandler, NULL 
};

static void
_notification(void *observerData, WMNotification *notification)
{
	WMText *to = (WMText *)observerData;
    WMText *tw = (WMText *)WMGetNotificationClientData(notification);
    if (to != tw) 
		lostHandler(to->view, XA_PRIMARY, NULL);
}

static  void
handleTextKeyPress(Text *tPtr, XEvent *event)
{
	char buffer[2];
	KeySym ksym;
	int control_pressed = False;
//	int h=1;

	if (((XKeyEvent *) event)->state & ControlMask)
		control_pressed = True;
    buffer[XLookupString(&event->xkey, buffer, 1, &ksym, NULL)] = '\0';
    
    switch(ksym) {
		case XK_Right:
		case XK_Left:
		case XK_Down:
		case XK_Up: 
			printf("arrows %ld\n", ksym);
		break;

		case XK_BackSpace:
		case XK_Delete:
		case XK_KP_Delete:
			deleteTextInteractively(tPtr, ksym);
		break;


		case XK_Return:
			buffer[0] = '\n';
		default:
		if(buffer[0] != '\0' && (buffer[0] == '\n' || !control_pressed))
			insertTextInteractively(tPtr, buffer, 1);
		else if(control_pressed && ksym==XK_r) {
		//	Bool i = !tPtr->rulerShown; WMShowTextRuler(tPtr, i);
		//	tPtr->rulerShown = i;
printf("toggle ruler\n");
		 }   
		else if(control_pressed && buffer[0] == '') 
				XBell(tPtr->view->screen->display, 0);
	}

	if(tPtr->flags.ownsSelection) 
		releaseSelection(tPtr);
}


static void
handleWidgetPress(XEvent *event, void *data)
{
	TextBlock *tb = (TextBlock *)data;
	Text *tPtr;
	WMWidget *w;
	if(!tb)
		return;
#if 0
	/* this little bit of nastiness here saves a boatload of trouble */
	w = (WMWidget *)(W_VIEW((W_VIEW(tb->d.widget))->parent))->self;
	//if(W_CLASS(w) != WC_TextField && W_CLASS(w) != WC_Text) 
	if( (((W_WidgetType*)(w))->widgetClass) != WC_Text) 
		return;
	*tPtr = (Text*)w;
	printf("%p clicked on tb %p wif: (%c)%c", tPtr, tb,	
		tPtr->firstTextBlock->text[0], tPtr->firstTextBlock->text[1]);
	output(tb->text, tb->used);
#endif
}

static void
handleActionEvents(XEvent *event, void *data)
{
	Text *tPtr = (Text *)data;
	Display *dpy = event->xany.display;
	KeySym ksym;
	

	if(tPtr->flags.waitingForSelection) 
		return;

	switch (event->type) {
		case KeyPress:
			ksym = XLookupKeysym((XKeyEvent*)event, 0);  
			if(ksym == XK_Shift_R || ksym == XK_Shift_L) { 
				tPtr->flags.extendSelection = True;
				return;
			} 
			if(!tPtr->flags.editable || tPtr->flags.buttonHeld) {
				XBell(dpy, 0);
				return;
			}

			if (tPtr->flags.waitingForSelection) 
				return;
			if(tPtr->flags.focused) {
				XGrabPointer(dpy, W_VIEW(tPtr)->window, False, 
					PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
					GrabModeAsync, GrabModeAsync, None, 
					W_VIEW(tPtr)->screen->invisibleCursor, CurrentTime);
				tPtr->flags.pointerGrabbed = True;
				handleTextKeyPress(tPtr, event);
				
			} break;

		case KeyRelease:
			ksym = XLookupKeysym((XKeyEvent*)event, 0);  
			if(ksym == XK_Shift_R || ksym == XK_Shift_L) {
				tPtr->flags.extendSelection = False;
				return;
			//end modify flag so selection can be extended
			} 
		break;


		case MotionNotify:
			if(tPtr->flags.pointerGrabbed) {
				tPtr->flags.pointerGrabbed = False;
				XUngrabPointer(dpy, CurrentTime);
			}
			if((event->xmotion.state & Button1Mask)) {
				if(!tPtr->flags.ownsSelection) { 
					WMCreateSelectionHandler(tPtr->view, XA_PRIMARY,
						event->xbutton.time, &selectionHandler, NULL);
					tPtr->flags.ownsSelection = True;
				}
				selectRegion(tPtr, event->xmotion.x, event->xmotion.y);
			}
		break;


		case ButtonPress: 
			tPtr->flags.buttonHeld = True;
			if(tPtr->flags.extendSelection) {
				selectRegion(tPtr, event->xmotion.x, event->xmotion.y);
				return;
			}
			if(event->xbutton.button == Button1) { 
				if(tPtr->flags.ownsSelection) 
					releaseSelection(tPtr);
				cursorToTextPosition(tPtr, event->xmotion.x, event->xmotion.y);
				if (tPtr->flags.pointerGrabbed) {
					tPtr->flags.pointerGrabbed = False;
					XUngrabPointer(dpy, CurrentTime);
					break;
				}   
			}
			if(!tPtr->flags.focused) {
				WMSetFocusToWidget(tPtr);
              	tPtr->flags.focused = True;
				break;
			} 	

			if(event->xbutton.button == WINGsConfiguration.mouseWheelDown) 
				WMScrollText(tPtr, -16);
			else if(event->xbutton.button == WINGsConfiguration.mouseWheelUp) 
				WMScrollText(tPtr, 16);
			break;

			case ButtonRelease:
				tPtr->flags.buttonHeld = False;
				if (tPtr->flags.pointerGrabbed) {
					tPtr->flags.pointerGrabbed = False;
					XUngrabPointer(dpy, CurrentTime);
					break;
				}   
				if(event->xbutton.button == WINGsConfiguration.mouseWheelDown
					|| event->xbutton.button == WINGsConfiguration.mouseWheelUp)
					break;

				if(event->xbutton.button == Button2 && tPtr->flags.editable) {
					char *text = NULL;
					int n;
					if(!WMRequestSelection(tPtr->view, XA_PRIMARY, XA_STRING,
						event->xbutton.time, pasteText, NULL)) {
						text = XFetchBuffer(tPtr->view->screen->display, &n, 0);
						if(text) {
							text[n] = 0;
							insertTextInteractively(tPtr, text, n-1);
							XFree(text);
						} else tPtr->flags.waitingForSelection = True;
				} }

			 break;

	}

}


static void 
handleEvents(XEvent *event, void *data)
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
			tPtr->flags.focused = True;
		break;

		case FocusOut: 
			tPtr->flags.focused = False;
        break;

		case DestroyNotify:
			printf("destroy");
		//for(...)WMRemoveTextParagraph(tPtr, para);
        break;

	}
}



static void
clearText(Text *tPtr)
{
	void *tb;
	if(!tPtr->firstTextBlock) 
		return;

	while(tPtr->currentTextBlock) 
		WMDestroyTextBlock(tPtr, WMRemoveTextBlock(tPtr));

	printf("yadda clearText\n");

	printf("remove the document\n");
	tPtr->firstTextBlock = NULL;
	tPtr->currentTextBlock = NULL;
	tPtr->lastTextBlock = NULL;
	//WMThawText(tPtr);
	WMRefreshText(tPtr, 0, 0);
}


static void
insertPlainText(WMText *tPtr, char *text) 
{
	char *start, *mark; 
	void *tb = NULL;
    

	if(!text) {
		clearText(tPtr);
		return;
	}


	start = text;
	while(start) {
		mark = strchr(start, '\n');
		if(mark) {
			tb = WMCreateTextBlockWithText(start, tPtr->dFont, 
				tPtr->dColor, True, (int)(mark-start));
			start = mark+1;
		} else {
			if(start && strlen(start)) {
				tb = WMCreateTextBlockWithText(start, tPtr->dFont,
					tPtr->dColor, False, strlen(start));
			} else tb = NULL;
			start = mark;
		}

		if(tPtr->flags.prepend) 
			WMPrependTextBlock(tPtr, tb);
		else
			WMAppendTextBlock(tPtr, tb);
	}
	return;

}
	





WMText *
WMCreateText(WMWidget *parent)
{
	Text *tPtr = wmalloc(sizeof(Text));
	if(!tPtr) {
		printf("could not create text widget\n");
		return NULL;
	}

#if 0
   printf("sizeof:\n");
    printf(" TextBlock %d\n", sizeof(TextBlock));
    printf(" TextBlock *%d\n", sizeof(TextBlock *));
    printf(" Section %d\n", sizeof(Section));
    printf(" char * %d\n", sizeof(char *));
    printf(" void * %d\n", sizeof(void *));
    printf(" short %d\n", sizeof(short));
    printf(" Text %d\n", sizeof(Text));
#endif

	memset(tPtr, 0, sizeof(Text));
	tPtr->widgetClass = WC_Text;
	tPtr->view = W_CreateView(W_VIEW(parent));
	if (!tPtr->view) {
		perror("could not create text's view\n");
		free(tPtr);
		return NULL;
    }
	tPtr->view->self = tPtr;
	tPtr->view->attribs.cursor = tPtr->view->screen->textCursor;
	tPtr->view->attribFlags |= CWOverrideRedirect | CWCursor;
	W_ResizeView(tPtr->view, 250, 200);
	tPtr->bgGC = WMColorGC(tPtr->view->screen->white);
	tPtr->fgGC = WMColorGC(tPtr->view->screen->black);
	W_SetViewBackgroundColor(tPtr->view, tPtr->view->screen->white);

	tPtr->ruler = NULL;
    tPtr->vS = NULL;
    tPtr->hS = NULL;

	tPtr->dFont = NULL; 
	//tPtr->dFont = WMCreateFont(tPtr->view->screen, 
	 //	"-*-fixed-medium-r-normal--26-*-*-*-*-*-*-*");
	//"-sony-fixed-medium-r-normal--24-230-75-75-c-120-jisx0201.1976-0");
	//	"-*-times-bold-r-*-*-12-*-*-*-*-*-*-*,"
     //   "-*-fixed-medium-r-normal-*-12-*");
    if (!tPtr->dFont)
		tPtr->dFont = WMRetainFont(tPtr->view->screen->normalFont);

	tPtr->dColor = WMBlackColor(tPtr->view->screen);

	tPtr->view->delegate = &_TextViewDelegate;

	WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
		|EnterWindowMask|LeaveWindowMask|FocusChangeMask, 
		handleEvents, tPtr);

	WMCreateEventHandler(tPtr->view, ButtonReleaseMask|ButtonPressMask
		|KeyReleaseMask|KeyPressMask|Button1MotionMask, 
		handleActionEvents, tPtr);
	
	WMAddNotificationObserver(_notification, tPtr, "_lostOwnership", tPtr);
	

	tPtr->firstTextBlock = NULL;
	tPtr->lastTextBlock = NULL;
	tPtr->currentTextBlock = NULL;
	tPtr->tpos = 0;

	tPtr->gfxItems = WMCreateArrayBag(4);

	tPtr->parser = NULL;
	tPtr->writer = NULL;

	tPtr->sel.x = tPtr->sel.y = 2;
	tPtr->sel.w = tPtr->sel.h = 0;

	tPtr->clicked.x = tPtr->clicked.y = 2;

	tPtr->visible.x = tPtr->visible.y = 2;
	tPtr->visible.h = tPtr->view->size.height;
	tPtr->visible.w = tPtr->view->size.width - 12;
	
	tPtr->docWidth = 0;
	tPtr->docHeight = 0;
	tPtr->dBulletPix = WMCreatePixmapFromXPMData(tPtr->view->screen,
		default_bullet);
	tPtr->db = (Pixmap) NULL;

	tPtr->margins = wmalloc(sizeof(WMRulerMargins));
	tPtr->margins[0].left = tPtr->margins[0].right = tPtr->visible.x;
	tPtr->margins[0].body = tPtr->visible.x;
	tPtr->margins[0].right = tPtr->visible.w;

	tPtr->flags.nmargins = 1;
	tPtr->flags.rulerShown = False;
	tPtr->flags.monoFont = !True;	
	tPtr->flags.focused = False;
	tPtr->flags.editable  = True;
	tPtr->flags.ownsSelection  = False;
	tPtr->flags.pointerGrabbed  = False;
	tPtr->flags.buttonHeld = False;
	tPtr->flags.waitingForSelection  = False;
	tPtr->flags.extendSelection = False;
	tPtr->flags.rulerShown  = False;
	tPtr->flags.frozen  = False;
	tPtr->flags.cursorShown = True;
	tPtr->flags.clickPos = 1;
	tPtr->flags.ignoreNewLine = False;
	tPtr->flags.laidOut = False;
	tPtr->flags.prepend = False;
	tPtr->flags.relief = WRFlat;
	tPtr->flags.alignment = WALeft;

	return tPtr;
}


void 
WMPrependTextStream(WMText *tPtr, char *text) 
{
	if(!tPtr)
		return;
	//check for "{\rtf0" in the text...
	//insertRTF
	//else 
	tPtr->flags.prepend = True;
	if(text && tPtr->parser)
		(tPtr->parser) (tPtr, (void *) text);
	else
		insertPlainText(tPtr, text);

}

void 
WMAppendTextStream(WMText *tPtr, char *text) 
{
	if(!tPtr)
		return;
	//check for "{\rtf0" in the text...
	//insertRTF
	//else 
	tPtr->flags.prepend = False;
	if(text && tPtr->parser)
		(tPtr->parser) (tPtr, (void *) text);
	else
		insertPlainText(tPtr, text);

}

WMData * 
WMGetTextSelected(WMText *tPtr)
{	
	WMData *data = NULL;
	TextBlock *tb;
	
	if(!tPtr)
		return NULL;

 	//tb = tPtr->firstTextBlock;
 	tb = tPtr->currentTextBlock;
	if(!tb)
		return NULL;

	data = WMCreateDataWithBytes(tb->text, tb->used);
	if(data) 
		WMSetDataFormat(data, 8);
	return data;
}

void *
WMCreateTextBlockWithObject(WMWidget *w, char *description, WMColor *color, 
	unsigned short first, unsigned short reserved)
{
	TextBlock *tb;
	unsigned short length;

	if(!w || !description || !color) 
		return NULL;

	tb = wmalloc(sizeof(TextBlock));
	if(!tb)
		 return NULL;

	length = strlen(description);
	tb->text = (char *)wmalloc(length);
	memset(tb->text, 0, length);
	memcpy(tb->text, description, length);
	tb->used = length;
	tb->blank = False;
	tb->d.widget = w;	
	tb->color = WMRetainColor(color);
	tb->marginN = 0;
	tb->allocated = 0;
	tb->first = first;
	tb->kanji = False;
	tb->graphic = True;
	tb->underlined = False;
	tb->script = 0;
	tb->sections = NULL;
	tb->nsections = 0;
	tb->prior = NULL;
	tb->next = NULL;

	return tb;
}
	
void *
WMCreateTextBlockWithText(char *text, WMFont *font, WMColor *color, 
	unsigned short first, unsigned short length)
{
	TextBlock *tb;

	if(!font || !color) 
		return NULL;

	tb = wmalloc(sizeof(TextBlock));
	if(!tb)
		 return NULL;

	tb->allocated = reqBlockSize(length);
	tb->text = (char *)wmalloc(tb->allocated);
	memset(tb->text, 0, tb->allocated);

	if(length < 1|| !text ) { // || *text == '\n') {
		*tb->text = ' ';
		tb->used = 1;
		tb->blank = True;
	} else {
		memcpy(tb->text, text, length);
		tb->used = length;
		tb->blank = False;
	}

	tb->d.font = WMRetainFont(font);
	tb->color = WMRetainColor(color);
	tb->marginN = 0;
	tb->first = first;
	tb->kanji = False;
	tb->graphic = False;
	tb->underlined = False;
	tb->script = 0;
	tb->sections = NULL;
	tb->nsections = 0;
	tb->prior = NULL;
	tb->next = NULL;
	return tb;
}

void 
WMSetTextBlockProperties(void *vtb, unsigned int first, 
	unsigned int kanji, unsigned int underlined, int script, 
	unsigned int marginN)
{
	TextBlock *tb = (TextBlock *) vtb;
	if(!tb)
		return;

	tb->first = first;
	tb->kanji = kanji;
	tb->underlined = underlined;
	tb->script = script;
	tb->marginN = marginN;
}
	
void 
WMGetTextBlockProperties(void *vtb, unsigned int *first, 
	unsigned int *kanji, unsigned int *underlined, int *script, 
	unsigned int *marginN)
{
	TextBlock *tb = (TextBlock *) vtb;
	if(!tb)
		return;

	if(first) *first = tb->first;
	if(kanji) *kanji = tb->kanji;
	if(underlined) *underlined = tb->underlined;
	if(script) *script = tb->script;
	if(marginN) *marginN = tb->marginN;
}
	


void 
WMPrependTextBlock(WMText *tPtr, void *vtb)
{
	TextBlock *tb = (TextBlock *)vtb;


	if(!tPtr || !tb)
		return;

	if(tb->graphic) {
		WMWidget *w = tb->d.widget;
		WMCreateEventHandler(W_VIEW(w), ButtonPressMask,
			handleWidgetPress, tb);
		//if(W_CLASS(w) != WC_TextField && W_CLASS(w) != WC_Text) {
		if(W_CLASS(w) != WC_TextField && 
				(((W_WidgetType*)(w))->widgetClass) != WC_Text) {
			(W_VIEW(w))->attribs.cursor = tPtr->view->screen->defaultCursor;
			(W_VIEW(w))->attribFlags |= CWOverrideRedirect | CWCursor;
		}
		WMPutInBag(tPtr->gfxItems, (void *)tb);
		WMRealizeWidget(w);
	}

	if(!tPtr->lastTextBlock || !tPtr->firstTextBlock) {
		tb->next = tb->prior = NULL;
		tPtr->lastTextBlock = tPtr->firstTextBlock 
			= tPtr->currentTextBlock = tb;
		return;
	}

	tb->next = tPtr->currentTextBlock;
	tb->prior = tPtr->currentTextBlock->prior;
	if(tPtr->currentTextBlock->prior)
		tPtr->currentTextBlock->prior->next = tb;

	tPtr->currentTextBlock->prior = tb;
	if(!tb->prior)
		tPtr->firstTextBlock = tb;

	tPtr->currentTextBlock = tb;
}
	

void 
WMAppendTextBlock(WMText *tPtr, void *vtb)
{
	TextBlock *tb = (TextBlock *)vtb;

	if(!tPtr || !tb)
		return;

	if(tb->graphic) {
		WMWidget *w = tb->d.widget;
		WMCreateEventHandler(W_VIEW(w), ButtonPressMask,
			handleWidgetPress, tb);
		//if(W_CLASS(w) != WC_TextField && W_CLASS(w) != WC_Text) {
		if(W_CLASS(w) != WC_TextField && 
				(((W_WidgetType*)(w))->widgetClass) != WC_Text) {
			(W_VIEW(w))->attribs.cursor = tPtr->view->screen->defaultCursor;
			(W_VIEW(w))->attribFlags |= CWOverrideRedirect | CWCursor;
		}
		WMPutInBag(tPtr->gfxItems, (void *)tb);
		WMRealizeWidget(w);
	}

	if(!tPtr->lastTextBlock || !tPtr->firstTextBlock) {
		tb->next = tb->prior = NULL;
		tPtr->lastTextBlock = tPtr->firstTextBlock 
			= tPtr->currentTextBlock = tb;
		return;
	}

	tb->next = tPtr->currentTextBlock->next;
	tb->prior = tPtr->currentTextBlock;
	if(tPtr->currentTextBlock->next)
		tPtr->currentTextBlock->next->prior = tb;
	
	tPtr->currentTextBlock->next = tb;

	if(!tb->next)
		tPtr->lastTextBlock = tb;

	tPtr->currentTextBlock = tb;
}

void  *
WMRemoveTextBlock(WMText *tPtr) 
{
	TextBlock *tb = NULL;

	if(!tPtr || !tPtr->firstTextBlock || !tPtr->lastTextBlock
			|| !tPtr->currentTextBlock) {
		printf("cannot remove non existent TextBlock!\b");
		return tb;
	}

	tb = tPtr->currentTextBlock;
	if(tb->graphic) {
		WMDeleteEventHandler(W_VIEW(tb->d.widget), ButtonPressMask,
			handleWidgetPress, tb);
		WMRemoveFromBag(tPtr->gfxItems, (void *)tb);
		WMUnmapWidget(tb->d.widget);
	}
	
	if(tPtr->currentTextBlock == tPtr->firstTextBlock) {
		if(tPtr->currentTextBlock->next) 
			tPtr->currentTextBlock->next->prior = NULL;

		tPtr->firstTextBlock = tPtr->currentTextBlock->next;
		tPtr->currentTextBlock = tPtr->firstTextBlock;

	} else if(tPtr->currentTextBlock == tPtr->lastTextBlock) {
		tPtr->currentTextBlock->prior->next = NULL;
		tPtr->lastTextBlock = tPtr->currentTextBlock->prior;
		tPtr->currentTextBlock = tPtr->lastTextBlock;
	} else {
		tPtr->currentTextBlock->prior->next = tPtr->currentTextBlock->next;
		tPtr->currentTextBlock->next->prior = tPtr->currentTextBlock->prior;
		tPtr->currentTextBlock = tPtr->currentTextBlock->next;
	}
	
	return (void *)tb;
}

void 
WMDestroyTextBlock(WMText *tPtr, void *vtb)
{
	TextBlock *tb = (TextBlock *)vtb;
	if(!tPtr || !tb)
		return;

	if(tb->graphic) {
return;
		WMDestroyWidget(tb->d.widget);
		wfree(tb->d.widget);
	} else {
		WMReleaseFont(tb->d.font);
	}
		
	WMReleaseColor(tb->color);
	if(tb->sections && tb->nsections > 0)
		wfree(tb->sections);
	wfree(tb->text);
	wfree(tb);
}
	

void 
WMRefreshText(WMText *tPtr, int vpos, int hpos)
{
	//TextBlock *tb;

	if(!tPtr || vpos<0 || hpos<0)
		return;

	tPtr->flags.laidOut = False;
	layOutDocument(tPtr);
	updateScrollers(tPtr);
	paintText(tPtr);


}

void
WMSetTextForegroundColor(WMText *tPtr, WMColor *color)
{
	if(!tPtr)
		return;

	if(color) 
		tPtr->fgGC = WMColorGC(color);
	else
		tPtr->fgGC = WMColorGC(WMBlackColor(tPtr->view->screen));

	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

void
WMSetTextBackgroundColor(WMText *tPtr, WMColor *color)
{
	if(!tPtr)
		return;

	if(color) {
		tPtr->bgGC = WMColorGC(color);
		W_SetViewBackgroundColor(tPtr->view, color);
	} else {
		tPtr->bgGC = WMColorGC(WMWhiteColor(tPtr->view->screen));
		W_SetViewBackgroundColor(tPtr->view, 
			WMWhiteColor(tPtr->view->screen));
	}

	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

void
WMSetTextRelief(WMText *tPtr, WMReliefType relief)
{
	if(!tPtr)
		 return;
	tPtr->flags.relief = relief;
	paintText(tPtr);
}

void 
WMSetTextHasHorizontalScroller(WMText *tPtr, Bool shouldhave)
{
	if(!tPtr) 
		return;

	if(shouldhave && !tPtr->hS) {
		tPtr->hS = WMCreateScroller(tPtr); 
		(W_VIEW(tPtr->hS))->attribs.cursor = tPtr->view->screen->defaultCursor;
		(W_VIEW(tPtr->hS))->attribFlags |= CWOverrideRedirect | CWCursor;
		WMSetScrollerArrowsPosition(tPtr->hS, WSAMaxEnd);
		WMSetScrollerAction(tPtr->hS, scrollersCallBack, tPtr);
		WMRealizeWidget(tPtr->hS);
		WMMapWidget(tPtr->hS);
	} else if(!shouldhave && tPtr->hS) {
		WMUnmapWidget(tPtr->hS);
		WMDestroyWidget(tPtr->hS);
		tPtr->hS = NULL;
	}

	tPtr->hpos = 0;
	tPtr->prevHpos = 0;
	textDidResize(tPtr->view->delegate, tPtr->view);
}


void 
WMSetTextHasVerticalScroller(WMText *tPtr, Bool shouldhave)
{
	if(!tPtr) 
		return;
		
	if(shouldhave && !tPtr->vS) {
		tPtr->vS = WMCreateScroller(tPtr); 
		(W_VIEW(tPtr->vS))->attribs.cursor = tPtr->view->screen->defaultCursor;
		(W_VIEW(tPtr->vS))->attribFlags |= CWOverrideRedirect | CWCursor;
		WMSetScrollerArrowsPosition(tPtr->vS, WSAMaxEnd);
		WMSetScrollerAction(tPtr->vS, scrollersCallBack, tPtr);
		WMRealizeWidget(tPtr->vS);
		WMMapWidget(tPtr->vS);
	} else if(!shouldhave && tPtr->vS) {
		WMUnmapWidget(tPtr->vS);
		WMDestroyWidget(tPtr->vS);
		tPtr->vS = NULL;
	}

	tPtr->vpos = 0;
	tPtr->prevVpos = 0;
	textDidResize(tPtr->view->delegate, tPtr->view);
}

	

Bool
WMScrollText(WMText *tPtr, int amount)
{
	Bool scroll=False;
	if(!tPtr)
		return False;
	if(amount == 0 || !tPtr->view->flags.realized)
		return False;
	
	if(amount < 0) {
		if(tPtr->vpos > 0) {
			if(tPtr->vpos > amount) tPtr->vpos += amount;
			else tPtr->vpos=0;
			scroll=True;
	} } else {
		int limit = tPtr->docHeight - tPtr->visible.h;
		if(tPtr->vpos < limit) {
			if(tPtr->vpos < limit-amount) tPtr->vpos += amount;
			else tPtr->vpos = limit;
            scroll = True;
    } }   

	if(scroll && tPtr->vpos != tPtr->prevVpos) {
		updateScrollers(tPtr);
		paintText(tPtr);
    }   
	tPtr->prevVpos = tPtr->vpos;
    return scroll;
}   

Bool 
WMPageText(WMText *tPtr, Bool direction)
{
	if(!tPtr) return False;
	if(!tPtr->view->flags.realized) return False;

	return WMScrollText(tPtr, direction?tPtr->visible.h:-tPtr->visible.h);
}


void
WMSetTextUseMonoFont(WMText *tPtr, Bool mono)
{
	if(!tPtr)
		return;
	if(mono && tPtr->flags.rulerShown)
		;//WMShowTextRuler(tPtr, False);

	tPtr->flags.monoFont = mono;
	WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

Bool
WMGetTextUsesMonoFont(WMText *tPtr)
{
	if(!tPtr)
		return True;
	return tPtr->flags.monoFont;
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

WMFont *
WMGetTextDefaultFont(WMText *tPtr)
{
	if(!tPtr)
		return NULL;
	else
		return tPtr->dFont;
}

void
WMSetTextParser(WMText *tPtr, WMAction *parser)
{
	if(!tPtr) 
		return;
	tPtr->parser = parser;
}


void
WMSetTextWriter(WMText *tPtr, WMAction *writer)
{
	if(!tPtr)
		return;
	tPtr->writer = writer;
}

int 
WMGetTextInsertType(WMText *tPtr)
{
	if(!tPtr)
		return 0;
	return tPtr->flags.prepend;
}
	


