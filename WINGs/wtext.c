/*
 *  WINGs WMText: multi-line/font/color/graphic text widget
 *
 *  Copyright (c) 1999-2000 Nwanua Elumeze <nwanua@windowmaker.org>
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


#include "WINGsP.h"
#include <X11/keysym.h>
#include <X11/Xatom.h>

#define DO_BLINK 0


/* TODO: 
 *
 * -  FIX wrap... long lines that don't fit are not char wrapped yet.
 * -  FIX html parser:  1. <b>foo</i>  should STILL BE BOLD!
 * -                    2.  " foo > bar "  should not confuse it.
 * -  change Array stuffs to WMArray
 * -  assess danger of destroying widgets whose actions link to other pages
 * -  integrate WMConvertFontTo* functions into WINGs proper (fontpanel)?
 * -  change cursor shape around pixmaps
 * -  redo blink code to reduce paint event... use pixmap buffer...
 * -  add paragraph support (full) and '\n' code in getStream..
 * -  use currentTextBlock and neighbours for fast paint and layout
 * -  replace copious uses of Refreshtext with appropriate layOut()...
 * -  WMFindInTextStream should also highlight found text...
 * -  add full support for Horizontal Scroll
*/


/* a Section is a section of a TextBlock that describes what parts
       of a TextBlock has been laid out on which "line"... 
   o  this greatly aids redraw, scroll and selection. 
   o  this is created during layoutLine, but may be later modified.
   o  there may be many Sections per TextBlock, hence the array */
typedef struct {
    unsigned int x, y;       /* where to draw it from */
    unsigned short w, h;     /* its width and height  */
    unsigned short begin;    /* where the layout begins */
    unsigned short end ;     /* where it ends */
    unsigned short last:1;   /* is it the last section on a "line"? */
    unsigned int _y:31;      /* the "line" it and other textblocks are on */
} Section;


/* a TextBlock is a doubly-linked list of TextBlocks containing:
    o text for the block, color and font
    o or a pointer to the pixmap
    o OR a pointer to the widget and the (text) description for its graphic
*/

typedef struct _TextBlock {
    struct _TextBlock *next;    /* next text block in linked list */
    struct _TextBlock *prior;   /* prior text block in linked list */

    char *text;                 /* pointer to text (could be kanji) */
                                /* or to the object's description */
    union {
        WMFont *font;           /* the font */
        WMWidget *widget;       /* the embedded widget */
        WMPixmap *pixmap;       /* the pixmap */
    } d;                        /* description */

    unsigned short used;        /* number of chars in this block */
    unsigned short allocated;   /* size of allocation (in chars) */
    WMColor *color;             /* the color */

    Section *sections;          /* the region for layouts (a growable array) */
                                /* an _array_! of size _nsections_ */
    
    unsigned short s_begin;     /* where the selection begins */
    unsigned short s_end;       /* where it ends */

    unsigned int first:1;       /* first TextBlock in paragraph */
    unsigned int blank:1;       /* ie. blank paragraph */
    unsigned int kanji:1;       /* is of 16-bit characters or not */
    unsigned int graphic:1;     /* graphic or text: text=0 */
    unsigned int object:1;      /* embedded object or pixmap */
    unsigned int underlined:1;  /* underlined or not */
    unsigned int selected:1;    /* selected or not */
    unsigned int nsections:8;   /* over how many "lines" a TextBlock wraps */
             int script:8;      /* script in points: negative for subscript */
    unsigned int marginN:8;     /* which of the margins in the tPtr to use */
    unsigned int nClicks:2;     /* single, double, triple clicks */
    unsigned int RESERVED:7;
} TextBlock;


/* somehow visible.h  beats the hell outta visible.size.height :-) */
typedef struct {
    unsigned int y;
    unsigned int x;
    unsigned int h;
    unsigned int w;
} myRect;


typedef struct W_Text {
    W_Class widgetClass;        /* the class number of this widget */
    W_View  *view;              /* the view referring to this instance */

    WMRuler *ruler;             /* the ruler widget to manipulate paragraphs */

    WMScroller *vS;             /* the vertical scroller */
    unsigned int vpos;          /* the current vertical position */
    unsigned int prevVpos;      /* the previous vertical position */

    WMScroller *hS;             /* the horizontal scroller */
    unsigned int hpos;          /* the current horizontal position */
    unsigned int prevHpos;      /* the previous horizontal position */

    WMFont *dFont;              /* the default font */
    WMColor *dColor;            /* the default color */
    WMPixmap *dBulletPix;       /* the default pixmap for bullets */

    GC bgGC;                    /* the background GC to draw with */
    GC fgGC;                    /* the foreground GC to draw with */
    Pixmap db;                  /* the buffer on which to draw */

    myRect visible;             /* the actual rectangle that can be drawn into */
    myRect cursor;              /* the position and (height) of cursor */
    myRect sel;                 /* the selection rectangle */

    WMPoint clicked;            /* where in the _document_ was clicked */

    unsigned short tpos;        /* the position in the currentTextBlock */
    unsigned short docWidth;    /* the width of the entire document */
    unsigned int docHeight;     /* the height of the entire document */

    TextBlock *firstTextBlock;
    TextBlock *lastTextBlock;
    TextBlock *currentTextBlock;

    WMArray *gfxItems;           /* a nice array of graphic items */

#if DO_BLINK
    WMHandlerID timerID;       /* for nice twinky-winky */
#endif
    
    WMAction *parser;
    WMAction *writer;
    WMTextDelegate *delegate;
    Time lastClickTime;

    WMRulerMargins *margins;      /* an array of margins */

    unsigned int nMargins:8;    /* the total number of margins in use */
    struct {
        unsigned int monoFont:1;    /* whether to ignore formats */
        unsigned int focused:1;     /* whether this instance has input focus */
        unsigned int editable:1;    /* "silly user, you can't edit me" */
        unsigned int ownsSelection:1; /* "I ownz the current selection!" */
        unsigned int pointerGrabbed:1;/* "heh, gib me pointer" */
        unsigned int buttonHeld:1;    /* the user is holding down the button */
        unsigned int extendSelection:1;  /* shift-drag to select more regions */

        unsigned int rulerShown:1;   /* whether the ruler is shown or not */
        unsigned int frozen:1;       /* whether screen updates are to be made */
        unsigned int cursorShown:1;  /* whether to show the cursor */
        unsigned int clickPos:1;     /* clicked before=0 or after=1 a graphic: */
                                     /*    (within counts as after too) */

        unsigned int horizOnDemand:1;/* if a large image should appear*/
        unsigned int needsRefresh:1; /* in case of Append/Deletes */
        unsigned int ignoreNewLine:1;/* turn it into a ' ' in streams > 1 */
        unsigned int laidOut:1;      /* have the TextBlocks all been laid out */
        unsigned int waitingForSelection:1; /* I don't wanna wait in vain... */
        unsigned int prepend:1;      /* prepend=1, append=0 (for parsers) */
        unsigned int parsingHTML:1;  /* how to interpret a stream of text */
        WMAlignment alignment:2;     /* the alignment for text */
        WMReliefType relief:3;       /* the relief to display with */
        unsigned int RESERVED:2;
    } flags;
} Text;


#define NOTIFY(T,C,N,A) { WMNotification *notif = WMCreateNotification(N,T,A);\
            if ((T)->delegate && (T)->delegate->C)\
                (*(T)->delegate->C)((T)->delegate,notif);\
            WMPostNotification(notif);\
            WMReleaseNotification(notif);}



/*
 * A hack to speed up caseless_equal.  Thanks to Quincey Koziol for
 * developing it for the "chimera" folks so I could use it  7 years later ;-)
 * Constraint: nothing but '\0' may map to 0
 */
static unsigned char map_table[256] = {
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,99,100,101,102,103,104,105,
106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,
92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,
130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,
148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,
166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,
202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,
238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

void HTMLParser(Text *tPtr, char *stream);
#define MAX_TOKEN_SIZE 255
#define MAX_TEXT_SIZE 1023

#define TOLOWER(x)  (map_table[(int)x])

#define ISALNUM(x) ( ((x>='0') && (x<='9')) \
    || ((x>='a') && (x<='z'))  || ((x>='A') && x<='Z'))

static void 
output(char *ptr, int len)
{   
    char s[len+1]; 
    memcpy(s, ptr, len);
    s[len] = 0; 
    /* printf(" s is [%s] (%d)\n",  s, strlen(s)); */
    printf("[%s]\n",  s);
}


/* tb->text doesn't necessarily end in '\0' hmph! (strchr) */
static inline char *
mystrchr(char *s, char needle, unsigned short len)
{
    char *haystack = s;

    if (!haystack || len < 1)
        return NULL;

    while ( (int) (haystack - s) < len ) {
        if (*haystack == needle)    
            return haystack; 
        haystack++;
    }
    return NULL;
}

/* tb->text doesn't necessarily end in '\0' hmph! (strstr) */
static inline char *
mystrstr(char *haystack, char *needle, unsigned short len)
{
    if (!haystack || !needle || len < 1) {
        return NULL;
    } else { 
        char *s = wmalloc(len+1);
        char *r;
    
        memcpy(s, haystack, len);
        s[len] = 0;
        r = (strstr(s, needle));
        wfree(s);
        return r;
   }
} 

static int
mystrcasecmp(const unsigned char *s1, const unsigned char *s2, 
    int len, Bool caseSensitive)
{
    if (!*s1 || !*s2 || len<1)
        return 0;

    while (*s2 != '\0' && len>0) {
        if ( (caseSensitive? (*s1) : TOLOWER (*s1)) 
            != (caseSensitive? (*s2) : TOLOWER (*s2)))
            return 0;

        s1++;
        s2++;
        len--;
    }
    return (*s1=='\0' || !ISALNUM(*s1))?1:0;
}



#if DO_BLINK
#define CURSOR_BLINK_ON_DELAY   600
#define CURSOR_BLINK_OFF_DELAY  400
#endif

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

static void 
handleEvents(XEvent *event, void *data);


static int 
getMarginNumber(Text *tPtr, WMRulerMargins *margins)
{
    unsigned int i=0;
    
    for(i=0; i < tPtr->nMargins; i++) {

        if(WMIsMarginEqualToMargin(&tPtr->margins[i], margins))
            return i;
    }

    return -1;
}
    


static int 
newMargin(Text *tPtr, WMRulerMargins *margins)
{
    int n;

    if (!margins) { 
        tPtr->margins[0].retainCount++;
        return 0;
    }

    n = getMarginNumber(tPtr, margins);

    if (n == -1) { 

        tPtr->margins = wrealloc(tPtr->margins,
            (++tPtr->nMargins)*sizeof(WMRulerMargins));

        n = tPtr->nMargins-1;
        tPtr->margins[n].left = margins->left;
        tPtr->margins[n].first = margins->first;
        tPtr->margins[n].body = margins->body;
        tPtr->margins[n].right = margins->right;
        /* for each tab... */
        tPtr->margins[n].retainCount = 1;
    } else {
        tPtr->margins[n].retainCount++;
    }

    return n;
}
        
static Bool 
sectionWasSelected(Text *tPtr, TextBlock *tb, XRectangle *rect, int s) 
{
    unsigned short i, w, lw, selected = False, extend = False;
    myRect sel; 
    

    /* if selection rectangle completely encloses the section */
    if ((tb->sections[s]._y >= tPtr->visible.y + tPtr->sel.y) 
        && (tb->sections[s]._y + tb->sections[s].h 
            <= tPtr->visible.y + tPtr->sel.y + tPtr->sel.h) ) { 
        sel.x = 0;
        sel.w = tPtr->visible.w;
        selected = extend = True;

    /* or if it starts on a line and then goes further down */
    } else if ((tb->sections[s]._y <= tPtr->visible.y + tPtr->sel.y) 
        && (tb->sections[s]._y + tb->sections[s].h 
            <= tPtr->visible.y + tPtr->sel.y + tPtr->sel.h)  
        && (tb->sections[s]._y + tb->sections[s].h 
            >= tPtr->visible.y + tPtr->sel.y) ) { 
        sel.x = WMAX(tPtr->sel.x, tPtr->clicked.x);
        sel.w = tPtr->visible.w;
        selected = extend = True;

    /* or if it begins before a line, but ends on it */
    } else if ((tb->sections[s]._y >= tPtr->visible.y + tPtr->sel.y) 
        && (tb->sections[s]._y + tb->sections[s].h 
            >= tPtr->visible.y + tPtr->sel.y + tPtr->sel.h)  
        && (tb->sections[s]._y 
            <= tPtr->visible.y + tPtr->sel.y + tPtr->sel.h) ) { 

        if (1||tPtr->sel.x + tPtr->sel.w > tPtr->clicked.x)
            sel.w = tPtr->sel.x + tPtr->sel.w;
        else 
            sel.w = tPtr->sel.x;

        sel.x = 0;
        selected = True;

    /* or if the selection rectangle lies entirely within a line */
    } else if ((tb->sections[s]._y <= tPtr->visible.y + tPtr->sel.y) 
        && (tPtr->sel.w >= 2)  
        && (tb->sections[s]._y + tb->sections[s].h 
            >= tPtr->visible.y + tPtr->sel.y + tPtr->sel.h) ) { 
        sel.x = tPtr->sel.x;
        sel.w = tPtr->sel.w;
        selected = True;
    }
  
    if (selected) { 
        selected = False;
       
        /* if not within (modified) selection rectangle */
        if ( tb->sections[s].x > sel.x + sel.w
            || tb->sections[s].x + tb->sections[s].w < sel.x)
            return False;

        if (tb->graphic) { 
            if ( tb->sections[s].x + tb->sections[s].w <= sel.x + sel.w 
                && tb->sections[s].x >= sel.x) { 
                rect->width = tb->sections[s].w;
                rect->x = tb->sections[s].x;
                selected = True;
            }
        } else { 

            i = tb->sections[s].begin; 
            lw = 0;

            if (0&& tb->sections[s].x >= sel.x) { 
                tb->s_begin = tb->sections[s].begin;
                goto _selEnd;
            }

            while (++i <= tb->sections[s].end) {

                w = WMWidthOfString(tb->d.font,  &(tb->text[i-1]), 1);
                lw += w;

                if (lw + tb->sections[s].x >= sel.x
                    || i == tb->sections[s].end ) {
                    lw -= w;
                    i--;
                    tb->s_begin = (tb->selected? WMIN(tb->s_begin, i) : i);
                    break;
                }
            }
 
            if (i > tb->sections[s].end) {
                printf("WasSelected: (i > tb->sections[s].end) \n");
                return False;
            }

_selEnd:    rect->x = tb->sections[s].x + lw;
            lw = 0;
            while(++i <= tb->sections[s].end) {

                w = WMWidthOfString(tb->d.font,  &(tb->text[i-1]), 1);
                lw += w;

                if (lw + rect->x >= sel.x + sel.w 
                    || i == tb->sections[s].end ) {

                    if  (i != tb->sections[s].end) { 
                        lw -= w;
                        i--;
                    }
             
                    rect->width =  lw;
                    if (tb->sections[s].last && sel.x + sel.w
                        >= tb->sections[s].x + tb->sections[s].w
                        && extend ) { 
                        rect->width  += (tPtr->visible.w - rect->x - lw);
                    }

                    tb->s_end = (tb->selected? WMAX(tb->s_end, i) : i);
                    selected = True;
                    break;
    } } } }

    if (selected) { 
        rect->y = tb->sections[s]._y - tPtr->vpos;
        rect->height = tb->sections[s].h;
if(tb->graphic) { printf("graphic s%d h%d\n", s,tb->sections[s].h);}
    }
    return selected;

}
            
static void 
setSelectionProperty(WMText *tPtr, WMFont *font, WMColor *color)
{
    TextBlock *tb;
    int isFont=False;
    
    if((font && color) || (!font && !color))
        return;

    if(font && !color)
        isFont = True;

    tb = tPtr->firstTextBlock;
    if (!tb || !tPtr->flags.ownsSelection)
        return;

    while (tb) {
        if (tPtr->flags.monoFont || tb->selected) { 
            
            if (tPtr->flags.monoFont || (tb->s_end - tb->s_begin == tb->used) 
                || tb->graphic) { 

                if(isFont) { 
                    if(!tb->graphic) {
                        WMReleaseFont(tb->d.font);
                        tb->d.font = WMRetainFont(font);
                    }
                } else {
                    WMReleaseColor(tb->color);
                    tb->color = WMRetainColor(color);
                }

            } else if (tb->s_end <= tb->used && tb->s_begin < tb->s_end) {

                TextBlock *otb = tb;

                TextBlock *ntb = (TextBlock *) 
                    WMCreateTextBlockWithText(tPtr, 
                        &(tb->text[tb->s_begin]),
                        (isFont?font:tb->d.font), 
                        (isFont?tb->color:color), 
                        False, (tb->s_end - tb->s_begin));
          
                if (ntb) { 
                    ntb->selected = True;
                    ntb->s_begin = 0;
                    ntb->s_end = ntb->used;
                    tPtr->currentTextBlock = tb;
                    WMAppendTextBlock(tPtr, ntb);
                    tb = tPtr->currentTextBlock;
                }

                if (otb->used - otb->s_end > 0) { 
                    ntb = (TextBlock *) 
                        WMCreateTextBlockWithText(tPtr, 
                            &(otb->text[otb->s_end]), otb->d.font, otb->color,
                            False, otb->used - otb->s_end);

                    if (ntb) { 
                        ntb->selected = False;
                        WMAppendTextBlock(tPtr, ntb);
                        tb = tPtr->currentTextBlock;
                    }
                }

                otb->selected = False;
                otb->used = otb->s_begin;
            }
        }
 
        tb = tb->next;
    }

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}


static void 
removeSelection(Text *tPtr)
{
    TextBlock *tb = NULL;

    if (!(tb = tPtr->firstTextBlock))
        return;

    while (tb) {
        if (tb->selected) { 

            if ( (tb->s_end - tb->s_begin == tb->used) || tb->graphic) { 
                tPtr->currentTextBlock = tb;
                WMDestroyTextBlock(tPtr, WMRemoveTextBlock(tPtr));
                tb = tPtr->currentTextBlock;
                if (tb)
                    tPtr->tpos = 0;
                continue;

            } else if (tb->s_end <= tb->used) {
                memmove(&(tb->text[tb->s_begin]),
                    &(tb->text[tb->s_end]), tb->used - tb->s_end);
                tb->used -= (tb->s_end - tb->s_begin);
                tb->selected = False;
                tPtr->tpos = tb->s_begin;
            }
                
        }
 
        tb = tb->next;
    }

}
    
static void 
paintText(Text *tPtr)
{   
    TextBlock *tb;
    WMFont *font;
    GC gc, greyGC;
    char *text;
    int len, y, c, s, done=False, prev_y=-23;
    WMScreen *scr = tPtr->view->screen;
    Display *dpy = tPtr->view->screen->display;
    Window win = tPtr->view->window;

    if (!tPtr->view->flags.realized || !tPtr->db || tPtr->flags.frozen)
        return;

    XFillRectangle(dpy, tPtr->db, tPtr->bgGC, 
       0, 0, tPtr->visible.w, tPtr->visible.h);

    if(!(tb = tPtr->firstTextBlock))
      goto _copy_area;
        
    if (tPtr->flags.ownsSelection) 
        greyGC = WMColorGC(WMGrayColor(scr));

    done = False;

    /* first, place all text that can be viewed */
    while (!done && tb) {

        if (tb->graphic) {
            tb = tb->next;
            continue;
        }

        tb->selected = False;

        for(s=0; s<tb->nsections && !done; s++) {

            if (tb->sections[s]._y  > tPtr->vpos + tPtr->visible.h) {
                done = True;
                break;
            }

            if ( tb->sections[s].y + tb->sections[s].h < tPtr->vpos) 
                continue; 

            if (tPtr->flags.monoFont) {
                font = tPtr->dFont;
                gc = tPtr->fgGC;
            } else {
                font = tb->d.font;
                gc = WMColorGC(tb->color);
            }

            if (tPtr->flags.ownsSelection) {
                XRectangle rect;

                if ( sectionWasSelected(tPtr, tb, &rect, s)) {
                    tb->selected = True;
                    XFillRectangle(dpy, tPtr->db, greyGC, 
                        rect.x, rect.y, rect.width, rect.height);
                }
            }

            prev_y = tb->sections[s]._y;

            len = tb->sections[s].end - tb->sections[s].begin;
            text = &(tb->text[tb->sections[s].begin]);
            y = tb->sections[s].y - tPtr->vpos;
            WMDrawString(scr, tPtr->db, gc, font, 
                tb->sections[s].x - tPtr->hpos, y, text, len);

            if (tb->underlined) { 
                XDrawLine(dpy, tPtr->db, gc,     
                    tb->sections[s].x - tPtr->hpos, 
                    y + font->y + 1,
                    tb->sections[s].x + tb->sections[s].w - tPtr->hpos,
                    y + font->y + 1);
            }
                        
        }

        tb = (!done? tb->next : NULL);

    }
            
    /* now , show all graphic items that can be viewed */
    c = WMGetArrayItemCount(tPtr->gfxItems);
    if (c > 0 && !tPtr->flags.monoFont) {
        int j, h;

        for(j=0; j<c; j++) {
            tb = (TextBlock *) WMGetFromArray(tPtr->gfxItems, j);
            if (tb->sections[0]._y + tb->sections[0].h <= tPtr->vpos 
                || tb->sections[0]._y >= tPtr->vpos + tPtr->visible.h ) {
        
                if(tb->object) {
                    if ((W_VIEW(tb->d.widget))->flags.mapped) {
                        WMUnmapWidget(tb->d.widget);
                    }
                }
            } else {
                if(tb->object) {
                    if (!(W_VIEW(tb->d.widget))->flags.mapped) {
                        if (!(W_VIEW(tb->d.widget))->flags.realized)
                            WMRealizeWidget(tb->d.widget);
                        WMMapWidget(tb->d.widget);
                        WMLowerWidget(tb->d.widget);
                    }
                }
                
                if (tPtr->flags.ownsSelection) {
                    XRectangle rect;

                    if ( sectionWasSelected(tPtr, tb, &rect, s)) {
                        tb->selected = True;
                        XFillRectangle(dpy, tPtr->db, greyGC,
                            rect.x, rect.y, rect.width, rect.height);
                    }
                }

                if(tb->object) {
                    WMMoveWidget(tb->d.widget, 
                        tb->sections[0].x - tPtr->hpos, 
                        tb->sections[0].y - tPtr->vpos);
                    h = WMWidgetHeight(tb->d.widget) + 1;

                } else {
                    WMDrawPixmap(tb->d.pixmap, tPtr->db, 
                        tb->sections[0].x - tPtr->hpos, 
                        tb->sections[0].y - tPtr->vpos);
                    h = tb->d.pixmap->height + 1;

                }

                if (tb->underlined) { 
                    XDrawLine(dpy, tPtr->db, WMColorGC(tb->color), 
                    tb->sections[0].x,
                    tb->sections[0].y + h,
                    tb->sections[0].x + tb->sections[0].w,
                    tb->sections[0].y + h);
      } } } } 


_copy_area:
    if (tPtr->flags.editable && tPtr->flags.cursorShown 
        && tPtr->cursor.x != -23 && tPtr->flags.focused) { 
        int y = tPtr->cursor.y - tPtr->vpos;
        XDrawLine(dpy, tPtr->db, tPtr->fgGC,
            tPtr->cursor.x, y,
            tPtr->cursor.x, y + tPtr->cursor.h);
    }

    XCopyArea(dpy, tPtr->db, win, tPtr->bgGC, 0, 0,
        tPtr->visible.w, tPtr->visible.h, 
        tPtr->visible.x, tPtr->visible.y);

    W_DrawRelief(scr, win, 0, 0,
        tPtr->view->size.width, tPtr->view->size.height, 
        tPtr->flags.relief);        

    if (tPtr->ruler && tPtr->flags.rulerShown)
        XDrawLine(dpy, win, tPtr->fgGC, 
            2, 42, tPtr->view->size.width-4, 42);

}


#if DO_BLINK

static void
blinkCursor(void *data)
{
    Text *tPtr = (Text*)data;

    if (tPtr->flags.cursorShown) {
        tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_OFF_DELAY, 
            blinkCursor, data);
    } else {
        tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_ON_DELAY, 
            blinkCursor, data);
    }
    paintText(tPtr);
    tPtr->flags.cursorShown = !tPtr->flags.cursorShown;
}
#endif

static TextBlock *
getFirstNonGraphicBlockFor(TextBlock *tb, short dir)
{
    if (!tb)
        return NULL;
    while (tb) {
        if (!tb->graphic)
            break;
        tb = (dir? tb->next : tb->prior);
    }

    return tb;
}
        

static void
updateCursorPosition(Text *tPtr)
{
    TextBlock *tb = NULL;
    int x, y, h, s;

    if(tPtr->flags.needsRefresh)
        WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);

    if (! (tb = tPtr->currentTextBlock)) {
        if (! (tb = tPtr->firstTextBlock)) {
            tPtr->tpos = 0;
            tPtr->cursor.h = tPtr->dFont->height;
            tPtr->cursor.y = 2;
            tPtr->cursor.x = 2;
            return;
        }
    }

    if(tb->graphic) {
        y = tb->sections[0].y;
        h = tb->sections[0].h;
        x = tb->sections[0].x;
    } else {
        if(tPtr->tpos > tb->used)
            tPtr->tpos = tb->used;

        for(s=0; s<tb->nsections; s++) {

            if(tPtr->tpos >= tb->sections[s].begin
                && tPtr->tpos <= tb->sections[s].end)
                break;
        }
    
       y = tb->sections[s]._y;
       h = tb->sections[s].h;
       x = tb->sections[s].x + WMWidthOfString(
           (tPtr->flags.monoFont?tPtr->dFont:tb->d.font), 
           &tb->text[tb->sections[s].begin], 
           tPtr->tpos - tb->sections[s].begin);
    }

    tPtr->cursor.y = y;
    tPtr->cursor.h = h;
    tPtr->cursor.x = x;
    paintText(tPtr);
}


static  void
cursorToTextPosition(Text *tPtr, int x, int y)
{
    TextBlock *tb = NULL;
    int done=False, s, pos, len, _w, _y, dir=1; /* 1 == "down" */
    char *text;

    if(tPtr->flags.needsRefresh)
        WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);

    y += (tPtr->vpos - tPtr->visible.y);
    if (y<0) 
        y = 0;

    x -= (tPtr->visible.x - 2); 
    if (x<0) 
        x=0;

    /* clicked is relative to document, not window... */
    tPtr->clicked.x = x;
    tPtr->clicked.y = y;

    if (! (tb = tPtr->currentTextBlock)) {
        if (! (tb = tPtr->firstTextBlock)) {
            tPtr->tpos = 0;
            tPtr->cursor.h = tPtr->dFont->height;
            tPtr->cursor.y = 2;
            tPtr->cursor.x = 2;
            return;
        }
    }

    /* first, which direction? Most likely, newly clicked 
        position will be close to previous */
    dir = !(y <= tb->sections[0].y);
    if ( ( y <= tb->sections[0]._y + tb->sections[0].h )
        && (y >= tb->sections[0]._y ) ) {
        /* if it's on the same line */
        if(x < tb->sections[0].x)   
            dir = 0;
        if(x >=  tb->sections[0].x) 
            dir = 1;
    }

tb = tPtr->firstTextBlock;
dir = 1;
        
    if (tPtr->flags.monoFont && tb->graphic) {
        tb = getFirstNonGraphicBlockFor(tb, 1);
        if (!tb) {
            tPtr->currentTextBlock = 
                (dir? tPtr->lastTextBlock : tPtr->firstTextBlock);
            tPtr->tpos = 0;
            return;
        }
    }

    s = (dir? 0 : tb->nsections-1);
    if ( y >= tb->sections[s]._y 
        && y <= tb->sections[s]._y + tb->sections[s].h) { 
            goto _doneV;
    }

    /* get the first section of the TextBlock that lies about 
        the vertical click point */
    done = False;
    while (!done && tb) { 

        if (tPtr->flags.monoFont && tb->graphic) {
            if(tb->next)
                tb = tb->next;
            continue;
        }

        s = (dir? 0 : tb->nsections-1);
        while (!done && (dir? (s<tb->nsections) : (s>=0) )) {

            if ( (dir?  (y <= tb->sections[s]._y + tb->sections[s].h) :
                    ( y >= tb->sections[s]._y ) ) ) {
                    done = True;
                } else {
                    dir? s++ : s--;
                }
        }
                    
        if (!done) {
            if ( (dir? tb->next : tb->prior))  {
                tb = (dir ? tb->next : tb->prior);
            } else {
                pos = tb->used;
                break; /* goto _doneH; */
            }
        }
    }

    
    if (s<0 || s>=tb->nsections) {
        s = (dir? tb->nsections-1 : 0);
    }

_doneV:
    /* we have the line, which TextBlock on that line is it? */
    pos = 0;
    if (tPtr->flags.monoFont && tb->graphic)
        tb = getFirstNonGraphicBlockFor(tb, dir);
    if (tb)  {
if ((dir? tb->sections[s].x >= x : tb->sections[s].x < x))
         goto   _doneH;

#if 0
        if(tb->blank) {
            _w = 0;
printf("blank\n");
        } else {
            text = &(tb->text[tb->sections[s].begin]);
            len = tb->sections[s].end - tb->sections[s].begin;
            _w = WMWidthOfString(tb->d.font, text, len);
        }
printf("here %d %d \n", tb->sections[s].x + _w, x);
        if ((dir? tb->sections[s].x + _w < x : tb->sections[s].x + _w >= x))  { 
            pos = tb->sections[s].end;
            tPtr->cursor.x = tb->sections[s].x + _w;
            goto _doneH;
        } 
#endif
        _y = tb->sections[s]._y;
    }

    while (tb) { 

        if (tPtr->flags.monoFont && tb->graphic) {
            tb = (dir ? tb->next : tb->prior);
            continue;
        }

        if (dir) {
            if (tb->graphic) {
                if(tb->object)
                    _w = WMWidgetWidth(tb->d.widget);
                else 
                    _w = tb->d.pixmap->width;
            } else {
                text = &(tb->text[tb->sections[s].begin]);
                len = tb->sections[s].end - tb->sections[s].begin;
                _w = WMWidthOfString(tb->d.font, text, len);
                if (tb->sections[s].x + _w >= x)
                    break;
                
            }
        } else { 
            if (tb->sections[s].x  <= x)
                break;
        }

        if ((dir? tb->next : tb->prior)) {
            TextBlock *nxt = (dir? tb->next : tb->prior);
            if (tPtr->flags.monoFont && nxt->graphic) {
                nxt = getFirstNonGraphicBlockFor(nxt, dir);
                if (!nxt) {
                    pos = 0;
                    tPtr->cursor.x = tb->sections[s].x;
                    goto _doneH;
                }
            }

            if (_y != nxt->sections[0]._y) {
                /* this must be the last/first on this line. stop */
                pos = (dir? tb->sections[s].end : 0);
                tPtr->cursor.x = tb->sections[s].x;
                if (!tb->blank) { 
                    if (tb->graphic) {
                        if(tb->object)
                            tPtr->cursor.x += WMWidgetWidth(tb->d.widget);
                        else 
                            tPtr->cursor.x += tb->d.pixmap->width;
                    } else if (pos > tb->sections[s].begin) {
                        tPtr->cursor.x += 
                        WMWidthOfString(tb->d.font, 
                            &(tb->text[tb->sections[s].begin]),
                            pos - tb->sections[s].begin);
                    }
                }
                goto _doneH;
            }
        }

        if ( (dir? tb->next : tb->prior))  {
            tb = (dir ? tb->next : tb->prior);
        } else { 
            done = True;
            break;
        }

        if (tb) 
            s = (dir? 0 : tb->nsections-1);
    }
        
    /* we have said TextBlock, now where within it? */
    if (tb && !tb->graphic) {
        WMFont *f = tb->d.font;
        len = tb->sections[s].end - tb->sections[s].begin;
        text = &(tb->text[tb->sections[s].begin]);

        _w = x - tb->sections[s].x;
        pos = 0;

        while (pos<len && WMWidthOfString(f, text, pos+1) <  _w)
            pos++;

        tPtr->cursor.x = tb->sections[s].x + 
            (pos? WMWidthOfString(f, text, pos) : 0);

        pos += tb->sections[s].begin;
_doneH:  
        tPtr->tpos = (pos<tb->used)? pos : tb->used;
    }
        
    if (!tb)
        printf("this app will surely crash :-)\n");

    tPtr->currentTextBlock = tb;
    tPtr->cursor.h = tb->sections[s].h;
    tPtr->cursor.y = tb->sections[s]._y;
    
}

static void 
autoSelectText(Text *tPtr, int clicks) 
{
    int x, y;
    TextBlock *tb;
    int start, end;
    char *mark = NULL;

    if(!(tb = tPtr->currentTextBlock))
        return;

     if(clicks == 2) {

        if(tb->text[tPtr->tpos] == ' ')	
			return;

        tPtr->sel.y = tPtr->cursor.y+5;
        tPtr->sel.h = tPtr->cursor.h-10;

        if(tb->graphic) { 
            tPtr->sel.x = tb->sections[0].x;
            tPtr->sel.w = tb->sections[0].w;
        } else {
            WMFont *font = tPtr->flags.monoFont?tPtr->dFont:tb->d.font;

            start = tPtr->tpos;
            while(start > 0 &&  tb->text[start-1] != ' ')
                start--;

            x = tPtr->cursor.x + 0*WMWidthOfString(font, &tb->text[start], 1);
            if(tPtr->tpos > start){ 
output(&tb->text[start], tPtr->tpos - start);
 				x -= WMWidthOfString(font, &tb->text[start], 
                tPtr->tpos - start);
            }
            tPtr->sel.x = (x<0?0:x);

            if((mark = mystrchr(&tb->text[start], ' ', tb->used-start))) {
                tPtr->sel.w = WMWidthOfString(font, &tb->text[start],
                    (int)(mark - &tb->text[start]));
            } else { 
                tPtr->sel.w = tPtr->docWidth - tPtr->sel.x;
            }
        }

    } else if(clicks == 3) { 
          tPtr->sel.x = tPtr->visible.x;
          tPtr->sel.w = tPtr->docWidth;
	}

    tPtr->flags.ownsSelection = True;
    paintText(tPtr);

}

static void
updateScrollers(Text *tPtr)
{   

    if (tPtr->flags.frozen)
        return;

    if (tPtr->vS) {
        if (tPtr->docHeight < tPtr->visible.h) {
            WMSetScrollerParameters(tPtr->vS, 0, 1);
            tPtr->vpos = 0;
        } else {   
            float hmax = (float)(tPtr->docHeight);
            WMSetScrollerParameters(tPtr->vS,
                ((float)tPtr->vpos)/(hmax - (float)tPtr->visible.h),
                (float)tPtr->visible.h/hmax);
        }       
    } else tPtr->vpos = 0;
    
    if (tPtr->hS) { 
        if (tPtr->docWidth < tPtr->visible.w) {
            WMSetScrollerParameters(tPtr->hS, 0, 1);
            tPtr->hpos = 0;
        } else {   
            float wmax = (float)(tPtr->docWidth);
            WMSetScrollerParameters(tPtr->hS,
                ((float)tPtr->hpos)/(wmax - (float)tPtr->visible.w),
                (float)tPtr->visible.w/wmax);
        }       
    } else tPtr->hpos = 0;
} 

static void
scrollersCallBack(WMWidget *w, void *self)
{   
    Text *tPtr = (Text *)self;
    Bool scroll = False;
    Bool dimple = False;
    int which;

    if (!tPtr->view->flags.realized || tPtr->flags.frozen) 
        return;

    if (w == tPtr->vS) {
        int height; 
        height = tPtr->visible.h;

         which = WMGetScrollerHitPart(tPtr->vS);
        switch(which) { 
            case WSDecrementLine:
                if (tPtr->vpos > 0) {
                    if (tPtr->vpos>16) tPtr->vpos-=16;
                    else tPtr->vpos=0;
                    scroll=True;
            }break;
            case WSIncrementLine: {
                int limit = tPtr->docHeight - height;
                if (tPtr->vpos < limit) {
                    if (tPtr->vpos<limit-16) tPtr->vpos+=16;
                    else tPtr->vpos=limit;
                    scroll = True;
            }}break;
            case WSDecrementPage:
                tPtr->vpos -= height;

                if (tPtr->vpos < 0)
                    tPtr->vpos = 0;
                dimple = True;
                scroll = True;
                printf("dimple needs to jump to mouse location ;-/\n");
                break;
            case WSIncrementPage:
                tPtr->vpos += height;
                if (tPtr->vpos > (tPtr->docHeight - height))
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
float hmax = (float)(tPtr->docHeight);
((float)tPtr->vpos)/(hmax - (float)tPtr->visible.h), 
(float)tPtr->visible.h/hmax;
dimple =where mouse is.
#endif
            break;
        } 
        scroll = (tPtr->vpos != tPtr->prevVpos);
        tPtr->prevVpos = tPtr->vpos;
    }
        
    if (w == tPtr->hS) {
        int width = tPtr->visible.w;

        which = WMGetScrollerHitPart(tPtr->hS);
        switch(which) { 
            case WSDecrementLine:
                if (tPtr->hpos > 0) {
                    if (tPtr->hpos>16) tPtr->hpos-=16;
                    else tPtr->hpos=0;
                    scroll=True;
            }break;
            case WSIncrementLine: {
                int limit = tPtr->docWidth - width;
                if (tPtr->hpos < limit) {
                    if (tPtr->hpos<limit-16) tPtr->hpos+=16;
                    else tPtr->hpos=limit;
                    scroll = True;
            }}break;
            case WSDecrementPage:
                tPtr->hpos -= width;

                if (tPtr->hpos < 0)
                    tPtr->hpos = 0;
                dimple = True;
                scroll = True;
                printf("dimple needs to jump to mouse location ;-/\n");
                break;
            case WSIncrementPage:
                tPtr->hpos += width;
                if (tPtr->hpos > (tPtr->docWidth - width))
                    tPtr->hpos = tPtr->docWidth - width;
                dimple = True;
                scroll = True;
                printf("dimple needs to jump to mouse location ;-/\n");
            break;
            
            
            case WSKnob:
                tPtr->hpos = WMGetScrollerValue(tPtr->hS)
                    * (float)(tPtr->docWidth - width);
                    scroll = True;
            break; 
            
            case WSKnobSlot:
            case WSNoPart:
printf("WSNoPart, WSKnobSlot\n");
#if 0
float wmax = (float)(tPtr->docWidth);
((float)tPtr->vpos)/(wmax - (float)tPtr->visible.w), 
(float)tPtr->visible.w/wmax;
dimple =where mouse is.
#endif
            break;
        } 
        scroll = (tPtr->hpos != tPtr->prevHpos);
        tPtr->prevHpos = tPtr->hpos;
    }
    
    if (scroll) {
/*
        if (0&&dimple) { 
            if (tPtr->rulerShown) 
                XClearArea(tPtr->view->screen->display, tPtr->view->window, 22, 47,
                    tPtr->view->size.width-24, tPtr->view->size.height-49, True);
            else    
                XClearArea(tPtr->view->screen->display, tPtr->view->window, 22, 2,
                    tPtr->view->size.width-24, tPtr->view->size.height-4, True);
             }
*/
        if (dimple || which == WSDecrementLine || which == WSIncrementLine)
            updateScrollers(tPtr);
        paintText(tPtr);
    }
}



typedef struct {
    TextBlock *tb;
    unsigned short begin, end;    /* what part of the text block */
} myLineItems;


static int
layOutLine(Text *tPtr, myLineItems *items, int nitems, int x, int y)
{   
    int i, j=0, lw = 0, line_height=0, max_d=0, len, n;
    WMFont *font;
    char *text;
    TextBlock *tb;
    TextBlock *tbsame=NULL;

    if(!items || nitems == 0)
        return 0;

    for(i=0; i<nitems; i++) {
        tb = items[i].tb;

        if (tb->graphic) { 
            if (!tPtr->flags.monoFont) {
                if(tb->object) {
                    WMWidget *wdt = tb->d.widget;
                    line_height = WMAX(line_height, WMWidgetHeight(wdt));
                    if (tPtr->flags.alignment != WALeft) 
                        lw += WMWidgetWidth(wdt);
                } else {
                    line_height = WMAX(line_height, tb->d.pixmap->height + max_d);
                    if (tPtr->flags.alignment != WALeft) 
                        lw += tb->d.pixmap->width;
                }
            }
            
        } else { 
            font = (tPtr->flags.monoFont)?tPtr->dFont : tb->d.font;
            max_d = WMAX(max_d, abs(font->height-font->y)); 
            line_height = WMAX(line_height, font->height + max_d);
            text = &(tb->text[items[i].begin]);
            len = items[i].end - items[i].begin;
            if (tPtr->flags.alignment != WALeft) 
                lw += WMWidthOfString(font, text, len);
        }
    }
    
    if (tPtr->flags.alignment == WARight) {
        j = tPtr->visible.w - lw;
    } else if (tPtr->flags.alignment == WACenter) {
        j = (int) ((float)(tPtr->visible.w - lw))/2.0;
    }   

    for(i=0; i<nitems; i++) {
        tb = items[i].tb;

        if (tbsame == tb) {  /*extend it, since it's on same line */
            tb->sections[tb->nsections-1].end = items[i].end;
            n = tb->nsections-1;
        } else {
            tb->sections = wrealloc(tb->sections,
                (++tb->nsections)*sizeof(Section));
            n = tb->nsections-1;
            tb->sections[n]._y = y + max_d;
            tb->sections[n].x = x+j;
            tb->sections[n].h = line_height;
            tb->sections[n].begin = items[i].begin;
            tb->sections[n].end = items[i].end;

            if (tb->graphic && tb->object) {
                tb->sections[n].x += tPtr->visible.x;
                tb->sections[n].y += tPtr->visible.y;
            }
        }

        tb->sections[n].last = (i+1 == nitems);

        if (tb->graphic) { 
            if (!tPtr->flags.monoFont) {
                if(tb->object) {
                    WMWidget *wdt = tb->d.widget;
                    tb->sections[n].y = max_d + y 
                        + line_height - WMWidgetHeight(wdt);
                    tb->sections[n].w = WMWidgetWidth(wdt);
                } else {
                    tb->sections[n].y = y + max_d;
                    tb->sections[n].w = tb->d.pixmap->width;
                }
                x += tb->sections[n].w;
            }
        } else {
            font = (tPtr->flags.monoFont)? tPtr->dFont : tb->d.font;
            len = items[i].end - items[i].begin;
            text = &(tb->text[items[i].begin]);

            tb->sections[n].y = y+line_height-font->y;
            tb->sections[n].w = 
                WMWidthOfString(font,  
                    &(tb->text[tb->sections[n].begin]), 
                    tb->sections[n].end - tb->sections[n].begin);

            x += WMWidthOfString(font,  text, len); 
        } 

        tbsame = tb;
    }

    return line_height;
                    
}


static void
layOutDocument(Text *tPtr) 
{
    TextBlock *tb;
    myLineItems *items = NULL;
    unsigned int itemsSize=0, nitems=0;
    WMFont *font;
    unsigned int x, y=0, prev_y, lw = 0, width=0, bmargin;

        
    Bool lhc = !tPtr->flags.laidOut; /* line height changed? */

    char *start=NULL, *mark=NULL;
    unsigned int begin, end;

    if (tPtr->flags.frozen)
        return;

    if (!(tb = tPtr->firstTextBlock)) 
        return;

    tPtr->docWidth = tPtr->visible.w;
    x = 0; /* tPtr->margins[tb->marginN].first; */
printf("x:%d\n", x);
    bmargin = tPtr->margins[tb->marginN].body;

    if (0&&tPtr->flags.laidOut) {
        tb = tPtr->currentTextBlock;
        if (tb->sections && tb->nsections>0) 
            prev_y = tb->sections[tb->nsections-1]._y;
y+=10;
printf("1 prev_y %d \n", prev_y); 

        /* search backwards for textblocks on same line */
        while (tb) {
            if (!tb->sections || tb->nsections<1) {
                tb = tPtr->firstTextBlock;
                break;
            }
            if (tb->sections[tb->nsections-1]._y != prev_y) {
                tb = tb->next;
                break;
            }
        /*    prev_y = tb->sections[tb->nsections-1]._y; */
            tb = tb->prior;
        }
        y = 0;  /* tb->sections[tb->nsections-1]._y; */
printf("2 prev_y %d \n\n", tb->sections[tb->nsections-1]._y); 
    }


    while (tb) {

        if (tb->sections && tb->nsections>0) {
            wfree(tb->sections);
            tb->sections = NULL;
            tb->nsections = 0;
        } 

        if (tb->first && tb != tPtr->firstTextBlock) {
            y += layOutLine(tPtr, items, nitems, x, y);
            x = 0*tPtr->margins[tb->marginN].first;
            bmargin = tPtr->margins[tb->marginN].body;
            nitems = 0;
            lw = 0;
        }
            
        if (tb->graphic) { 
            if (!tPtr->flags.monoFont) { 
                if(tb->object)
                    width = WMWidgetWidth(tb->d.widget);
                else
                    width = tb->d.pixmap->width;

                if (width > tPtr->docWidth) 
                    tPtr->docWidth = width;

                lw += width;
                if (lw >= tPtr->visible.w - x ) { 
                    y += layOutLine(tPtr, items, nitems, x, y);
                    nitems = 0;
                    x = 0*bmargin;
                    lw = width;
                }

                if(nitems + 1> itemsSize) {
                    items = wrealloc(items, 
                        (++itemsSize)*sizeof(myLineItems));
                }
                    
                items[nitems].tb = tb;
                items[nitems].begin = 0;
                items[nitems].end = 0;
                nitems++;
            }

        } else if ((start = tb->text)) {
            begin = end = 0;
            font = tPtr->flags.monoFont?tPtr->dFont:tb->d.font;

            while (start) {
                mark = mystrchr(start, ' ', tb->used);
                if (mark) {
                    end += (int)(mark-start)+1;
                    start = mark+1;
                } else {
                    end += strlen(start);
                    start = mark;
                }

                if (end > tb->used)
                    end = tb->used;

                if (end-begin > 0) {
                    
                    width = WMWidthOfString(font, 
                        &tb->text[begin], end-begin);

                    /* if it won't fit,  break it up */
                    if (width > tPtr->visible.w) { 
                        char *t = &tb->text[begin];
                        int l=end-begin, i=0;
                        do { 
                            width = WMWidthOfString(font, t, ++i);
                        } while (width < tPtr->visible.w && i < l);  
                        end = begin+i;
                        if (start) 
                             start -= l-i;    
                    }

                    lw += width;
                }

                if (lw >= tPtr->visible.w - x) {
                    y += layOutLine(tPtr, items, nitems, x, y); 
                    lw = width;
                    x = bmargin;
                    nitems = 0;
                }

                if(nitems + 1 > itemsSize) {
                    items = wrealloc(items, 
                        (++itemsSize)*sizeof(myLineItems));
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


    if (nitems > 0) 
        y += layOutLine(tPtr, items, nitems, x, y);               
    if (lhc) {
        tPtr->docHeight = y+10;
        updateScrollers(tPtr);
    }

    if(tPtr->docWidth > tPtr->visible.w && !tPtr->hS) {
        XEvent event;
            
        tPtr->flags.horizOnDemand = True;
        WMSetTextHasHorizontalScroller((WMText*)tPtr, True);
        event.type = Expose;
        handleEvents(&event, (void *)tPtr);

    } else if(tPtr->docWidth <= tPtr->visible.w 
        && tPtr->hS && tPtr->flags.horizOnDemand ) {
        tPtr->flags.horizOnDemand = False;
        WMSetTextHasHorizontalScroller((WMText*)tPtr, False);
    }
    tPtr->flags.laidOut = True;
        
    
    if(items && itemsSize > 0)
        wfree(items);
}


static void
textDidResize(W_ViewDelegate *self, WMView *view)
{   
    Text *tPtr = (Text *)view->self;
    unsigned short w = tPtr->view->size.width;
    unsigned short h = tPtr->view->size.height;
    unsigned short rh = 0, vw = 0;

    if (tPtr->ruler && tPtr->flags.rulerShown) { 
        WMMoveWidget(tPtr->ruler, 2, 2);
        WMResizeWidget(tPtr->ruler, w - 4, 40);
        rh = 40;
    }   
        
    if (tPtr->vS) { 
        WMMoveWidget(tPtr->vS, 1, rh + 1);
        WMResizeWidget(tPtr->vS, 20, h - rh - 2);
        vw = 20;
        WMSetRulerOffset(tPtr->ruler,22);
    } else WMSetRulerOffset(tPtr->ruler, 2);

    if (tPtr->hS) {
        if (tPtr->vS) {
            WMMoveWidget(tPtr->hS, vw, h - 21);
            WMResizeWidget(tPtr->hS, w - vw - 1, 20);
        } else {
            WMMoveWidget(tPtr->hS, vw+1, h - 21);
            WMResizeWidget(tPtr->hS, w - vw - 2, 20);
        }
    }

    tPtr->visible.x = (tPtr->vS)?24:4;
    tPtr->visible.y = (tPtr->ruler && tPtr->flags.rulerShown)?43:3;
    tPtr->visible.w = tPtr->view->size.width - tPtr->visible.x - 8;
    tPtr->visible.h = tPtr->view->size.height - tPtr->visible.y;
    tPtr->visible.h -= (tPtr->hS)?20:0;
    tPtr->margins[0].right = tPtr->visible.w;

    if (tPtr->view->flags.realized) {

        if (tPtr->db) {
            XFreePixmap(tPtr->view->screen->display, tPtr->db);
            tPtr->db = (Pixmap) NULL;
        }

        if (tPtr->visible.w < 40) 
            tPtr->visible.w = 40;
        if (tPtr->visible.h < 20) 
            tPtr->visible.h = 20;
    
        if(!tPtr->db) { 
            tPtr->db = XCreatePixmap(tPtr->view->screen->display, 
                tPtr->view->window, tPtr->visible.w, 
                tPtr->visible.h, tPtr->view->screen->depth);
        }
    }
    
    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

W_ViewDelegate _TextViewDelegate =
{
    NULL,
    NULL,
    textDidResize,
    NULL,
};

/* nice, divisble-by-16 blocks */
static inline unsigned short
reqBlockSize(unsigned short requested)
{
    return requested + 16 - (requested%16);
}


static void
clearText(Text *tPtr)
{
    if (!tPtr->firstTextBlock) 
        return;

    while (tPtr->currentTextBlock) 
        WMDestroyTextBlock(tPtr, WMRemoveTextBlock(tPtr));

    tPtr->firstTextBlock = NULL;
    tPtr->currentTextBlock = NULL;
    tPtr->lastTextBlock = NULL;
}

static void
deleteTextInteractively(Text *tPtr, KeySym ksym)
{
    TextBlock *tb = tPtr->currentTextBlock;
    Bool back = (Bool) (ksym == XK_BackSpace);
    Bool done = 1;

    if (!tPtr->flags.editable || tPtr->flags.buttonHeld) {
        XBell(tPtr->view->screen->display, 0);
        return;
    }

    if (!tb)
        return;

    tPtr->flags.needsRefresh = True;

    if (tPtr->flags.ownsSelection) {
        removeSelection(tPtr);
        return;
    }

    if (back && tPtr->tpos < 1) {
        if (tb->prior) {
            tb = tb->prior;    
            tb->first = False;
            tPtr->tpos = tb->used;
            tPtr->currentTextBlock = tb;
            done = 1;
        }
    }

    if ( (tb->used > 0) &&  ((back?tPtr->tpos > 0:1))
        && (tPtr->tpos <= tb->used) && !tb->graphic) {
        if (back)
            tPtr->tpos--;
        memmove(&(tb->text[tPtr->tpos]),
            &(tb->text[tPtr->tpos + 1]), tb->used - tPtr->tpos);
        tb->used--;
        done = 0;
    }

    if ( (back? (tPtr->tpos < 1 && !done) : ( tPtr->tpos >= tb->used))
        || tb->graphic) {

        TextBlock *sibling = (back? tb->prior : tb->next);

        if(tb->used == 0 || tb->graphic) 
            WMDestroyTextBlock(tPtr, WMRemoveTextBlock(tPtr));

        if (sibling) { 
            tPtr->currentTextBlock = sibling;
            tPtr->tpos = (back? sibling->used : 0);
        }
    }

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}
    

static void
insertTextInteractively(Text *tPtr, char *text, int len)
{
    TextBlock *tb;
    char *newline = NULL;
    
    if (!tPtr->flags.editable || tPtr->flags.buttonHeld) {
        XBell(tPtr->view->screen->display, 0);
        return;
    }

    if (len < 1 || !text)
        return;


    if(tPtr->flags.ignoreNewLine && *text == '\n' && len == 1)
        return;

    if (tPtr->flags.ownsSelection) 
        removeSelection(tPtr);

    tPtr->flags.needsRefresh = True;

    if (tPtr->flags.ignoreNewLine) {
        int i;
        for(i=0; i<len; i++) {
            if (text[i] == '\n') 
                text[i] = ' ';
        }
    }

    tb = tPtr->currentTextBlock;
    if (!tb || tb->graphic) {
        text[len] = 0;
        WMAppendTextStream(tPtr, text);
        if (tPtr->currentTextBlock) { 
            tPtr->tpos = tPtr->currentTextBlock->used;
        }
        WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
        return;
    }

    if ((newline = mystrchr(text, '\n', len))) {
        int nlen = (int)(newline-text);
        int s = tb->used - tPtr->tpos;
        char save[s];

        if (!tb->blank && nlen>0) {
            if (s > 0) { 
                memcpy(save, &tb->text[tPtr->tpos], s);
                tb->used -= (tb->used - tPtr->tpos);
            }
            text[nlen] = 0;
            insertTextInteractively(tPtr, text, nlen);
            newline++;
            WMAppendTextStream(tPtr, newline);
            if (s>0)
                insertTextInteractively(tPtr, save, s);

        } else { 
            if (tPtr->tpos>0 && tPtr->tpos < tb->used 
                && !tb->graphic && tb->text) { 

                void *ntb = WMCreateTextBlockWithText(
                    tPtr, &tb->text[tPtr->tpos],
                    tb->d.font, tb->color, True, tb->used - tPtr->tpos);
                tb->used = tPtr->tpos;
                WMAppendTextBlock(tPtr, ntb);
                tPtr->tpos = 0;
            } else if (tPtr->tpos == tb->used || tPtr->tpos == 0) { 
                void *ntb = WMCreateTextBlockWithText(tPtr, 
                    NULL, tb->d.font, tb->color, True, 0);

                if (tPtr->tpos>0)
                    WMAppendTextBlock(tPtr, ntb);
                else
                    WMPrependTextBlock(tPtr, ntb);
                tPtr->tpos = 1;
                 
            }
        }
    } else { 

        if (tb->used + len >= tb->allocated) {
            tb->allocated = reqBlockSize(tb->used+len);
            tb->text = wrealloc(tb->text, tb->allocated);
        }
        
        if (tb->blank) {
            memcpy(tb->text, text, len);
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

    }

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}


static void 
selectRegion(Text *tPtr, int x, int y)
{

    if (x < 0 || y < 0) 
        return;

    y += (tPtr->flags.rulerShown? 40: 0);
    y += tPtr->vpos;
    if (y>10) 
       y -= 10;  /* the original offset */

    x -= tPtr->visible.x-2;
    if (x<0)
        x=0;

    tPtr->sel.x = WMAX(0, WMIN(tPtr->clicked.x, x));
    tPtr->sel.w = abs(tPtr->clicked.x - x);
    tPtr->sel.y = WMAX(0, WMIN(tPtr->clicked.y, y));
    tPtr->sel.h = abs(tPtr->clicked.y - y);

    tPtr->flags.ownsSelection = True;
    paintText(tPtr);
}


static  void
releaseSelection(Text *tPtr)
{
    TextBlock *tb = tPtr->firstTextBlock;

    while(tb) {
        tb->selected = False;
        tb = tb->next;
    }
    tPtr->flags.ownsSelection = False;
    WMDeleteSelectionHandler(tPtr->view, XA_PRIMARY,
        CurrentTime);

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}


WMData*
requestHandler(WMView *view, Atom selection, Atom target, void *cdata,
    Atom *type)
{
    Text *tPtr = view->self;
    Display *dpy = tPtr->view->screen->display;
    Atom _TARGETS; 
    Atom TEXT = XInternAtom(dpy, "TEXT", False);
    Atom COMPOUND_TEXT = XInternAtom(dpy, "COMPOUND_TEXT", False);
    WMData *data = NULL;


    if (target == XA_STRING || target == TEXT || target == COMPOUND_TEXT) {
        char *text = WMGetTextSelected(tPtr);
    
        if (text) {
printf("got text [%s]\n", text);
            data = WMCreateDataWithBytes(text, strlen(text));
               WMSetDataFormat(data, 8);
        }
        *type = target;
        return data;
    }   else printf("didn't get it\n");

    _TARGETS = XInternAtom(dpy, "TARGETS", False);
    if (target == _TARGETS) {
        Atom *ptr;
        
        ptr = wmalloc(4 * sizeof(Atom));
        ptr[0] = _TARGETS;
        ptr[1] = XA_STRING;
        ptr[2] = TEXT;
        ptr[3] = COMPOUND_TEXT;
        
        data = WMCreateDataWithBytes(ptr, 4*4);
        WMSetDataFormat(data, 32);
    
        *type = target;
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
ownershipObserver(void *observerData, WMNotification *notification)
{
    if (observerData != WMGetNotificationClientData(notification))
        lostHandler(WMWidgetView(observerData), XA_PRIMARY, NULL);
}


static void
fontChanged(void *observerData, WMNotification *notification)
{
    WMText *tPtr = (WMText *) observerData;
    WMFont *font = (WMFont *)WMGetNotificationClientData(notification);
printf("fontChanged\n");

    if(!tPtr || !font)
        return;

    if (tPtr->flags.ownsSelection)
        WMSetTextSelectionFont(tPtr, font);
}


static  void
handleTextKeyPress(Text *tPtr, XEvent *event)
{
    char buffer[2];
    KeySym ksym;
    int control_pressed = False;
    TextBlock *tb = NULL;

    if (((XKeyEvent *) event)->state & ControlMask)
        control_pressed = True;
    buffer[XLookupString(&event->xkey, buffer, 1, &ksym, NULL)] = 0;
    
    switch(ksym) {

        case XK_Left: 
        if(!(tb = tPtr->currentTextBlock))
            break;
            if(tb->graphic) 
                goto L_imaGFX;
            
            if(tPtr->tpos==0) { 
L_imaGFX:       if(tb->prior) {
                    tPtr->currentTextBlock = tb->prior;
                    tPtr->tpos = tPtr->currentTextBlock->used -1;
                } else tPtr->tpos = 0;
            } else tPtr->tpos--;
            updateCursorPosition(tPtr);
         break;

        case XK_Right:  
printf("foind %d \n", WMFindInTextStream(tPtr, "It", 1, 0));
return;
        if(!(tb = tPtr->currentTextBlock))
            break;
            if(tb->graphic) 
                goto R_imaGFX;
           if(tPtr->tpos == tb->used) {
R_imaGFX:      if(tb->next) {
                    tPtr->currentTextBlock = tb->next;
                    tPtr->tpos = 1;
                } else tPtr->tpos = tb->used;
            } else  tPtr->tpos++;
            updateCursorPosition(tPtr);
        break;
            
        case XK_Down:
            cursorToTextPosition(tPtr, tPtr->cursor.x + tPtr->visible.x, 
                tPtr->clicked.y + tPtr->cursor.h - tPtr->vpos);
            paintText(tPtr);        
            break;

        case XK_Up:  
            cursorToTextPosition(tPtr, tPtr->cursor.x + tPtr->visible.x, 
                tPtr->visible.y + tPtr->cursor.y - tPtr->vpos - 3);
            paintText(tPtr);        
        break;

        case XK_BackSpace:
        case XK_Delete:
        case XK_KP_Delete:
            deleteTextInteractively(tPtr, ksym);
            updateCursorPosition(tPtr);
        break;

        case XK_Control_R :
        case XK_Control_L :
            control_pressed = True;
        break;

        case XK_Return:
            buffer[0] = '\n';
        default:
        if (buffer[0] != 0 && !control_pressed) {
            insertTextInteractively(tPtr, buffer, 1);
            updateCursorPosition(tPtr);

        } else if (control_pressed && ksym==XK_r) {
            Bool i = !tPtr->flags.rulerShown; 
            WMShowTextRuler(tPtr, i);
            tPtr->flags.rulerShown = i;
         }   
        else if (control_pressed && buffer[0] == '') 
            XBell(tPtr->view->screen->display, 0);
    }

    if (!control_pressed && tPtr->flags.ownsSelection) 
        releaseSelection(tPtr);
}

static void
handleWidgetPress(XEvent *event, void *data)
{
    TextBlock *tb = (TextBlock *)data;
    Text *tPtr;
    WMWidget *w;

    if (!tb)
        return;
    /* this little bit of nastiness here saves a boatload of trouble */
    w = (WMWidget *)(((W_VIEW(tb->d.widget))->parent)->self);
    if (W_CLASS(w) != WC_Text) 
        return;
    tPtr = (Text*)w;
    tPtr->currentTextBlock = getFirstNonGraphicBlockFor(tb, 1);
    if (!tPtr->currentTextBlock)
        tPtr->currentTextBlock = tb;
    tPtr->tpos = 0;
    output(tPtr->currentTextBlock->text, tPtr->currentTextBlock->used);
#if 0
    if (!tPtr->flags.focused) {
        WMSetFocusToWidget(tPtr);
      tPtr->flags.focused = True;
    }     
#endif

}

static void
handleActionEvents(XEvent *event, void *data)
{
    Text *tPtr = (Text *)data;
    Display *dpy = event->xany.display;
    KeySym ksym;
    

    switch (event->type) {
        case KeyPress:
            ksym = XLookupKeysym((XKeyEvent*)event, 0);  
            if (ksym == XK_Shift_R || ksym == XK_Shift_L) { 
                tPtr->flags.extendSelection = True;
                return;
            } 

            if (tPtr->flags.focused) {
                XGrabPointer(dpy, W_VIEW(tPtr)->window, False, 
                    PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                    GrabModeAsync, GrabModeAsync, None, 
                    tPtr->view->screen->invisibleCursor, CurrentTime);
                tPtr->flags.pointerGrabbed = True;
                handleTextKeyPress(tPtr, event);
                
            } break;

        case KeyRelease:
            ksym = XLookupKeysym((XKeyEvent*)event, 0);  
            if (ksym == XK_Shift_R || ksym == XK_Shift_L) {
                tPtr->flags.extendSelection = False;
                return;
            /* end modify flag so selection can be extended */
            } 
        break;


        case MotionNotify:

            if (tPtr->flags.pointerGrabbed) {
                tPtr->flags.pointerGrabbed = False;
                XUngrabPointer(dpy, CurrentTime);
            }
   
            if(tPtr->flags.waitingForSelection)
                break;

            if ((event->xmotion.state & Button1Mask)) {
                if (!tPtr->flags.ownsSelection) { 
                    WMCreateSelectionHandler(tPtr->view, 
                        XA_PRIMARY, event->xbutton.time, 
                        &selectionHandler, NULL);
                    tPtr->flags.ownsSelection = True;
                }
                selectRegion(tPtr, event->xmotion.x, event->xmotion.y);
            }
        break;


        case ButtonPress: 

            tPtr->flags.buttonHeld = True;

            if (tPtr->flags.pointerGrabbed) {
                tPtr->flags.pointerGrabbed = False;
                XUngrabPointer(dpy, CurrentTime);
                break;
            }   

            if (tPtr->flags.waitingForSelection) 
                break;

            if (tPtr->flags.extendSelection) {
                selectRegion(tPtr, event->xmotion.x, event->xmotion.y);
                return;
            }

            
            if (event->xbutton.button == Button1) { 

                if(WMIsDoubleClick(event)) {
printf("lastClickTime: %d \n", event->xbutton.time -tPtr->lastClickTime);
                    autoSelectText(tPtr, 2);
                    tPtr->lastClickTime = event->xbutton.time;
                    break;
                } 
if(0&&event->xbutton.time - tPtr->lastClickTime
                <  1.5*WINGsConfiguration.doubleClickDelay) {
                  ;//  autoSelectText(tPtr, 3);
                    break;
                }

//WMGetTextStreamIntoArray(tPtr);

                if (!tPtr->flags.focused) {
                    WMSetFocusToWidget(tPtr);
                      tPtr->flags.focused = True;
                }    

                if (tPtr->flags.ownsSelection) 
                    releaseSelection(tPtr);

                tPtr->lastClickTime = event->xbutton.time;
                cursorToTextPosition(tPtr, event->xmotion.x, event->xmotion.y);
                paintText(tPtr);
           } 

            if (event->xbutton.button 
                == WINGsConfiguration.mouseWheelDown)  {
                WMScrollText(tPtr, -16);
                break;
            }

            if (event->xbutton.button 
                == WINGsConfiguration.mouseWheelUp)  {
                WMScrollText(tPtr, 16);
                break;
            }

            if (event->xbutton.button == Button2) {
                char *text = NULL;
                int n;

                if (!tPtr->flags.editable) { 
                    XBell(dpy, 0);
                    break;
                }

#if 0
                if (!WMRequestSelection(tPtr->view, XA_PRIMARY, XA_STRING,
                    event->xbutton.time, pasteText, NULL)) {
#endif
{

                    text = XFetchBuffer(tPtr->view->screen->display, &n, 0);

                       if (text) {
                        text[n] = 0;

                        if (tPtr->parser) 
                            (tPtr->parser) (tPtr, (void *) text);
                        else 
                            insertTextInteractively(tPtr, text, n);
                    
                        XFree(text);
                        NOTIFY(tPtr, didChange, WMTextDidChangeNotification,
                              (void*)WMInsertTextEvent);

                    } else {
                        tPtr->flags.waitingForSelection = True;
                    }
                } 
                break;
            }


        case ButtonRelease:

            tPtr->flags.buttonHeld = False;

            if (tPtr->flags.pointerGrabbed) {
                tPtr->flags.pointerGrabbed = False;
                XUngrabPointer(dpy, CurrentTime);
                break;
            }   

            if (tPtr->flags.waitingForSelection) 
                break;
        } 
        
}


static void 
handleEvents(XEvent *event, void *data)
{
    Text *tPtr = (Text *)data;

    switch(event->type) {
        case Expose: 

          if (event->xexpose.count!=0)
              break;

            if(tPtr->hS) { 
                if (!(W_VIEW(tPtr->hS))->flags.realized)
                    WMRealizeWidget(tPtr->hS);
                if (!((W_VIEW(tPtr->hS))->flags.mapped)) 
                    WMMapWidget(tPtr->hS);
            }

            if(tPtr->vS) { 
                if (!(W_VIEW(tPtr->vS))->flags.realized)
                    WMRealizeWidget(tPtr->vS);
                if (!((W_VIEW(tPtr->vS))->flags.mapped))
                    WMMapWidget(tPtr->vS);
            }

            if(tPtr->ruler) {
                if (!(W_VIEW(tPtr->ruler))->flags.realized)
                    WMRealizeWidget(tPtr->ruler);

                if (!((W_VIEW(tPtr->ruler))->flags.mapped) 
                    && tPtr->flags.rulerShown) 
                    WMMapWidget(tPtr->ruler);
            }

            if(!tPtr->db) 
                textDidResize(tPtr->view->delegate, tPtr->view);

             paintText(tPtr);
        break;

        case FocusIn: 
            if (W_FocusedViewOfToplevel(W_TopLevelOfView(tPtr->view))
                != tPtr->view)
                return;
            tPtr->flags.focused = True;
#if DO_BLINK
            if (tPtr->flags.editable && !tPtr->timerID) {
                tPtr->timerID = WMAddTimerHandler(12+0*CURSOR_BLINK_ON_DELAY,
                    blinkCursor, tPtr);
            }
#endif

        break;

        case FocusOut: 
            tPtr->flags.focused = False;
            paintText(tPtr);
#if DO_BLINK
            if (tPtr->timerID) {
                WMDeleteTimerHandler(tPtr->timerID);
                tPtr->timerID = NULL;
            }
#endif
        break;


        case DestroyNotify:
            clearText(tPtr);
            if(tPtr->hS)
                WMDestroyWidget(tPtr->hS);
            if(tPtr->vS)
                WMDestroyWidget(tPtr->vS);
            if(tPtr->ruler)
                WMDestroyWidget(tPtr->ruler);
            if(tPtr->db)
                XFreePixmap(tPtr->view->screen->display, tPtr->db);
            if(tPtr->gfxItems)
                WMFreeArray(tPtr->gfxItems);
#if DO_BLINK
            if (tPtr->timerID)
                WMDeleteTimerHandler(tPtr->timerID);
#endif
            WMReleaseFont(tPtr->dFont);
            WMReleaseColor(tPtr->dColor);
            WMDeleteSelectionHandler(tPtr->view, XA_PRIMARY, CurrentTime);
            WMRemoveNotificationObserver(tPtr);
    
            wfree(tPtr);

        break;

    }
}


static void
insertText(WMText *tPtr, char *stream) 
{
    char *ptr, *begin, *end;
    void *tb = NULL;
    char c;
    char b, e, again=False;
    unsigned long tokenLength, tk;
    unsigned long textLength, tx;
    
    if (!stream) {
        clearText(tPtr);
        return;
    }

WMAppendTextBlock(tPtr, 
WMCreateTextBlockWithText(tPtr, stream, tPtr->dFont, tPtr->dColor,
                     1, strlen(stream)));
return;
    ptr = stream;
    while (ptr) {
        again = False;
        b = '<'; e = '>';
again:
        begin = strchr(ptr, b);
        if (begin) {
            end = strchr(begin+1, e);
            if(end) { 
                if((int)(end-begin)-1 > 1)
                    output(begin+1, (int)(end-begin)-1);
            }
            ptr = begin+1;
        } else if(!again) {
            b = '>'; e = '<';
            again = True;
printf("again!\n");
            goto again;
        } else {
            if (ptr && strlen(ptr)) {
printf("boo\n");
            } 
            ptr = begin;
        }


#if 0
    while( (c=*(stream++))) {
//printf("%c", c);
        if((c == '\n' && tPtr->flags.parsingHTML) || c =='\t')
            //c = ' '; //continue;
            continue;
        if(c == ' ') {
            if(wasspace) 
                continue;
            wasspace = 1;
        }else wasspace = 0;

        if(c == '\n') { 
            parseToken(tPtr, token, -1);

        } else if(c == '<'  && !open) { 
            open=1;
            if(textlen>0) { 
                text[textlen] = 0;
                tb = WMCreateTextBlockWithText(tPtr, text, cfmt.cfont,
                     cfmt.ccolor, cfmt.first, textlen);
                //WMSetTextBlockProperties(tb, cfmt.first, False, (cfmt.u?1:0), 0, cfmt.margins);
                WMAppendTextBlock(tPtr, tb);
                cfmt.first = False;
//printf("%s\n", text);
            }
            textlen = 0;
        } else if(c == '>' && open) {
            token[tk] = 0;
            if(tk>0) parseToken(tPtr, token, tk);
            open=0; 
            tk=0;
        } else {
            if(open) {
                if(tk < MAX_TOKEN_SIZE) token[tk++] = c;
            } else if(textlen < MAX_TEXT_SIZE) text[textlen++] = c;
        }
    }

    if(tk>0) { token[tk] = 0; parseToken(tPtr, token, tk);}
    if(textlen>0) {
        text[textlen] = 0;
        //printf("%s\n", text);
        tb = WMCreateTextBlockWithText(tPtr, text, 
            (WMFont *)WMGetFromArray(cfmt.fonts, 
                WMGetArrayItemCount(cfmt.fonts)-1), 
            (WMColor *)WMGetFromArray(cfmt.colors,
                WMGetArrayItemCount(cfmt.colors)-1), 
            cfmt.first, textlen);
        //WMSetTextBlockProperties(tb, cfmt.first, False, (cfmt.u?1:0), 0, cfmt.margins);
        WMAppendTextBlock(tPtr, tb);
        cfmt.first = False;
    }
#endif
        
}


#if 0
    start = text;
    while (start) {
        mark = strchr(start, '\n');
        if (mark) {
            tb = WMCreateTextBlockWithText(tPtr, 
                start, tPtr->dFont, 
                tPtr->dColor, True, (int)(mark-start));
            start = mark+1;
        } else {
            if (start && strlen(start)) {
                tb = WMCreateTextBlockWithText(tPtr, start, tPtr->dFont,
                    tPtr->dColor, False, strlen(start));
            } else tb = NULL;
            start = mark;
        }

        if (tPtr->flags.prepend) 
            WMPrependTextBlock(tPtr, tb);
        else
            WMAppendTextBlock(tPtr, tb);
    }
    return;
#endif

}


static void
rulerMoveCallBack(WMWidget *w, void *self)
{   
    Text *tPtr = (Text *)self;
    if (!tPtr)    
        return;
    if (W_CLASS(tPtr) != WC_Text)
        return;
    
    paintText(tPtr);
}

    
static void
rulerReleaseCallBack(WMWidget *w, void *self)
{
    Text *tPtr = (Text *)self;
    if (!tPtr)    
        return;
    if (W_CLASS(tPtr) != WC_Text)
        return;

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
    return;
}


static unsigned
draggingEntered(WMView *self, WMDraggingInfo *info)
{
printf("draggingEntered\n");
    return WDOperationCopy;
}   


static unsigned
draggingUpdated(WMView *self, WMDraggingInfo *info)
{
    return WDOperationCopy;
}   


static void
draggingExited(WMView *self, WMDraggingInfo *info)
{
printf("draggingExited\n");
}

static Bool
prepareForDragOperation(WMView *self, WMDraggingInfo *info)
{
    printf("prepareForDragOperation\n");
    return True;
}   


char *badbadbad;

receivedData(WMView *view, Atom selection, Atom target, Time timestamp,
         void *cdata, WMData *data)
{             
badbadbad = wstrdup((char *)WMDataBytes(data));
}


/* when it's done in WINGs, remove this */ 

Bool requestDroppedData(WMView *view, WMDraggingInfo *info, char *type)
{   
    WMScreen *scr = W_VIEW_SCREEN(view);

    if (!WMRequestSelection(scr->dragInfo.destView,
                scr->xdndSelectionAtom,
                XInternAtom(scr->display, type, False),
                scr->dragInfo.timestamp,
                receivedData, &scr->dragInfo)) {
wwarning("could not request data for dropped data");

}

    {
    XEvent ev;
    
    ev.type = ClientMessage;
    ev.xclient.message_type = scr->xdndFinishedAtom;
    ev.xclient.format = 32; 
    ev.xclient.window = info->destinationWindow;
    ev.xclient.data.l[0] = 0;
    ev.xclient.data.l[1] = 0;
    ev.xclient.data.l[2] = 0;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;
    
    XSendEvent(scr->display, info->sourceWindow, False, 0, &ev);
    XFlush(scr->display);
    }
}

static Bool
performDragOperation(WMView *self, WMDraggingInfo *info, WMData *data)
{
    WMColor *color; 
    WMText *tPtr = (WMText *)self->self;
    
    if (!tPtr)
        return True;

    requestDroppedData(tPtr->view, info, "application/X-color");
    color = WMCreateNamedColor(W_VIEW_SCREEN(self), badbadbad, True);
    if(color) {
        WMSetTextSelectionColor(tPtr, color);
        WMReleaseColor(color);
    }
    


    return True;
}   

static void
concludeDragOperation(WMView *self, WMDraggingInfo *info)
{
    printf("concludeDragOperation\n");
}


static WMDragDestinationProcs _DragDestinationProcs = {
    draggingEntered,
    draggingUpdated,
    draggingExited,
    prepareForDragOperation,
    performDragOperation,
    concludeDragOperation
}; 

static void 
releaseArrayData(void *data) 
{
    if(data)
        wfree(data);
}


char *
getStream(WMText *tPtr, int sel, int array)
{
    TextBlock *tb = NULL;
    char *text = NULL;
    unsigned long where = 0;
    
    if (!tPtr)
        return NULL;

    if (!(tb = tPtr->firstTextBlock))
        return NULL;

    /* this might be tricky to get right... not yet  implemented */
    if (tPtr->writer) {
        (tPtr->writer) (tPtr, (void *) text);
        return text;
    }
    
    tb = tPtr->firstTextBlock;
    while (tb) {

        if (!tb->graphic || (tb->graphic && !tPtr->flags.monoFont)) {

            if (!sel || (tb->graphic && tb->selected)) {

                if (!tPtr->flags.ignoreNewLine && (tb->first || tb->blank)) {
                    text = wrealloc(text, where+1);
                    text[where++] = '\n';
                 }

                if(tb->graphic && array) {
                    text = wrealloc(text, where+3);
                    text[where++] = 'G';//0xFA;
                    text[where++] = 'A';//tb->used;
                    text[where++] = 'H';//18+tb->allocated;
                }

                text = wrealloc(text, where+tb->used);
                memcpy(&text[where], tb->text, tb->used);
                where += tb->used;

          
            } else if (sel && tb->selected) {

                if (!tPtr->flags.ignoreNewLine && (tb->first || tb->blank)) { 
                  text = wrealloc(text, where+1);
                  text[where++] = '\n';
                }

                text = wrealloc(text, where+(tb->s_end - tb->s_begin));
                memcpy(&text[where], &tb->text[tb->s_begin], 
                    tb->s_end - tb->s_begin);
                where += tb->s_end - tb->s_begin;            

            }
           
        }
        tb = tb->next;
    }
    
    /* +1 for the end of string, let's be nice */
    text = wrealloc(text, where+1);
    text[where] = 0;
    return text;
}



WMArray * 
getStreamIntoArray(WMText *tPtr, int sel)
{
    WMArray *array = WMCreateArray(4);
    WMData *data;
    char *stream;
    unsigned long loc=0, len;
    unsigned long end;
    int i, count;

   
    stream = getStream(tPtr, sel, 0);
    if(!stream)
        return NULL;
    

printf(stream);
return;
    end = strlen(stream);

#if 0
            data = WMCreateDataWithBytes((void *)(fa+2), len);
            WMSetDataFormat(data, 32);
            WMAddToArray(array, (void *) data);
            start = fa + len + 2;
            
         } else {
             if (start && strlen(start)) {
                 data = WMCreateDataWithBytes((void *)start, strlen(start));
                 WMSetDataFormat(data, 8);
                 WMAddToArray(array, (void *) data);
             }   
             start = fa;
         }   
    }    

    wfree(stream);
    return array;
#endif
    WMFreeArray(array);
}


WMText *
WMCreateText(WMWidget *parent)
{
    Text *tPtr = wmalloc(sizeof(Text));
    if (!tPtr) {
        printf("could not create text widget\n");
        return NULL;
    }


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

    tPtr->dColor = WMWhiteColor(tPtr->view->screen);
    tPtr->bgGC = WMColorGC(tPtr->dColor);
    W_SetViewBackgroundColor(tPtr->view, tPtr->dColor);
    WMReleaseColor(tPtr->dColor);

    tPtr->dColor = WMBlackColor(tPtr->view->screen);
    tPtr->fgGC = WMColorGC(tPtr->dColor);

    tPtr->ruler = NULL;
    tPtr->vS = NULL;
    tPtr->hS = NULL;

    tPtr->dFont = WMRetainFont(WMSystemFontOfSize(tPtr->view->screen, 12));

    tPtr->view->delegate = &_TextViewDelegate;

#if DO_BLINK
    tPtr->timerID = NULL;
#endif

    WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
        |EnterWindowMask|LeaveWindowMask|FocusChangeMask, 
        handleEvents, tPtr);

    WMCreateEventHandler(tPtr->view, ButtonReleaseMask|ButtonPressMask
        |KeyReleaseMask|KeyPressMask|Button1MotionMask, 
        handleActionEvents, tPtr);
    
    WMAddNotificationObserver(ownershipObserver, tPtr, 
        "_lostOwnership", tPtr);
    
    WMSetViewDragDestinationProcs(tPtr->view, &_DragDestinationProcs);
    {
        char *types[2] = {"application/X-color", NULL};
        WMRegisterViewForDraggedTypes(tPtr->view, types);
    }

    WMAddNotificationObserver(fontChanged, tPtr, 
        "WMFontPanelDidChangeNotification", tPtr);

    tPtr->firstTextBlock = NULL;
    tPtr->lastTextBlock = NULL;
    tPtr->currentTextBlock = NULL;
    tPtr->tpos = 0;

    tPtr->gfxItems = WMCreateArray(4);

    tPtr->parser = NULL;
    tPtr->writer = NULL;

    tPtr->sel.x = tPtr->sel.y = 2;
    tPtr->sel.w = tPtr->sel.h = 0;

    tPtr->clicked.x = tPtr->clicked.y = 2;

    tPtr->visible.x = tPtr->visible.y = 2;
    tPtr->visible.h = tPtr->view->size.height;
    tPtr->visible.w = tPtr->view->size.width - 4;
    
    tPtr->cursor.x = -23;

    tPtr->docWidth = 0;
    tPtr->docHeight = 0;
    tPtr->dBulletPix = WMCreatePixmapFromXPMData(tPtr->view->screen,
        default_bullet);
    tPtr->db = (Pixmap) NULL;

    tPtr->margins = WMGetRulerMargins(NULL);
    tPtr->margins->right = tPtr->visible.w;
    tPtr->nMargins = 1;

    tPtr->flags.rulerShown = False;
    tPtr->flags.monoFont = False;    
    tPtr->flags.focused = False;
    tPtr->flags.editable  = True;
    tPtr->flags.ownsSelection  = False;
    tPtr->flags.pointerGrabbed  = False;
    tPtr->flags.buttonHeld = False;
    tPtr->flags.extendSelection = False;
    tPtr->flags.frozen  = False;
    tPtr->flags.cursorShown = True;
    tPtr->flags.clickPos = 1;
    tPtr->flags.horizOnDemand = False;
    tPtr->flags.needsRefresh = False;
    tPtr->flags.ignoreNewLine = False;
    tPtr->flags.laidOut = False;
    tPtr->flags.waitingForSelection = False;
    tPtr->flags.prepend = False;
    tPtr->flags.parsingHTML = False;
    tPtr->flags.relief = WRSunken;
    tPtr->flags.alignment = WALeft;

    return tPtr;
}

void 
WMPrependTextStream(WMText *tPtr, char *text) 
{
    CHECK_CLASS(tPtr, WC_Text);

    if(!text)
        releaseSelection(tPtr);

    tPtr->flags.prepend = True;
    if (text && tPtr->parser)
        (tPtr->parser) (tPtr, (void *) text);
    else
        insertText(tPtr, text);

    tPtr->flags.needsRefresh = True;
}


void 
WMAppendTextStream(WMText *tPtr, char *text) 
{
    CHECK_CLASS(tPtr, WC_Text);

    if(!text)
        releaseSelection(tPtr);

    tPtr->flags.prepend = False;
    if (text && tPtr->parser)
        (tPtr->parser) (tPtr, (void *) text);
    else
        insertText(tPtr, text);

    tPtr->flags.needsRefresh = True;
  
}


char * 
WMGetTextStream(WMText *tPtr)
{    
    CHECK_CLASS(tPtr, WC_Text);
    return getStream(tPtr, 0, 0);
}

char * 
WMGetTextSelected(WMText *tPtr)
{    
    CHECK_CLASS(tPtr, WC_Text);
    return getStream(tPtr, 1, 0);
}

WMArray *
WMGetTextStreamIntoArray(WMText *tPtr)
{
    CHECK_CLASS(tPtr, WC_Text);
    return getStreamIntoArray(tPtr, 0);
}

WMArray *
WMGetTextSelectedIntoArray(WMText *tPtr)
{
    CHECK_CLASS(tPtr, WC_Text);
    return getStreamIntoArray(tPtr, 1);
}


void
WMSetTextDelegate(WMText *tPtr, WMTextDelegate *delegate)
{
    CHECK_CLASS(tPtr, WC_Text);

    tPtr->delegate = delegate;
}


void *
WMCreateTextBlockWithObject(WMText *tPtr, WMWidget *w, 
    char *description, WMColor *color, 
    unsigned short first, unsigned short extraInfo)
{
    TextBlock *tb;
    unsigned short length;

    if (!w || !description || !color) 
        return NULL;

    tb = wmalloc(sizeof(TextBlock));
    if (!tb)
         return NULL;

    length = strlen(description);
    tb->text = (char *)wmalloc(length);
    memset(tb->text, 0, length);
    memcpy(tb->text, description, length);
    tb->used = length;
    tb->blank = False;
    tb->d.widget = w;    
    tb->color = WMRetainColor(color);
    tb->marginN = newMargin(tPtr, NULL);
    tb->allocated = extraInfo;
    tb->first = first;
    tb->kanji = False;
    tb->graphic = True;
    tb->object = True;
    tb->underlined = False;
    tb->selected = False;
    tb->script = 0;
    tb->sections = NULL;
    tb->nsections = 0;
    tb->prior = NULL;
    tb->next = NULL;

    return tb;
}
    

void *
WMCreateTextBlockWithPixmap(WMText *tPtr, WMPixmap *p, 
    char *description, WMColor *color, 
    unsigned short first, unsigned short extraInfo)
{
    TextBlock *tb;
    unsigned short length;

    if (!p || !description || !color) 
        return NULL;

    tb = wmalloc(sizeof(TextBlock));
    if (!tb)
         return NULL;

    length = strlen(description);
    tb->text = (char *)wmalloc(length);
    memset(tb->text, 0, length);
    memcpy(tb->text, description, length);
    tb->used = length;
    tb->blank = False;
    tb->d.pixmap = p;    
    tb->color = WMRetainColor(color);
    tb->marginN = newMargin(tPtr, NULL);
    tb->allocated = extraInfo;
    tb->first = first;
    tb->kanji = False;
    tb->graphic = True;
    tb->object = False;
    tb->underlined = False;
    tb->selected = False;
    tb->script = 0;
    tb->sections = NULL;
    tb->nsections = 0;
    tb->prior = NULL;
    tb->next = NULL;

    return tb;
}
    
void *
WMCreateTextBlockWithText(WMText *tPtr, char *text, WMFont *font, WMColor *color, 
    unsigned short first, unsigned short length)
{
    TextBlock *tb;

    if (!font || !color) 
        return NULL;

    tb = wmalloc(sizeof(TextBlock));
    if (!tb)
         return NULL;

    tb->allocated = reqBlockSize(length);
    tb->text = (char *)wmalloc(tb->allocated);
    memset(tb->text, 0, tb->allocated);

    if (length < 1|| !text ) { 
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
    tb->marginN = newMargin(tPtr, NULL);
    tb->first = first;
    tb->kanji = False;
    tb->graphic = False;
    tb->underlined = False;
    tb->selected = False;
    tb->script = 0;
    tb->sections = NULL;
    tb->nsections = 0;
    tb->prior = NULL;
    tb->next = NULL;
    return tb;
}

void 
WMSetTextBlockProperties(WMText *tPtr, void *vtb, unsigned int first, 
    unsigned int kanji, unsigned int underlined, int script, 
    WMRulerMargins *margins)
{
    TextBlock *tb = (TextBlock *) vtb;
    if (!tb)
        return;

    tb->first = first;
    tb->kanji = kanji;
    tb->underlined = underlined;
    tb->script = script;
    tb->marginN = newMargin(tPtr, margins);
}
    
void 
WMGetTextBlockProperties(WMText *tPtr, void *vtb, unsigned int *first, 
    unsigned int *kanji, unsigned int *underlined, int *script, 
    WMRulerMargins *margins)
{
    TextBlock *tb = (TextBlock *) vtb;
    if (!tb)
        return;

    if (first) *first = tb->first;
    if (kanji) *kanji = tb->kanji;
    if (underlined) *underlined = tb->underlined;
    if (script) *script = tb->script;
    if (margins) margins = &tPtr->margins[tb->marginN];
}
    


void 
WMPrependTextBlock(WMText *tPtr, void *vtb)
{
    TextBlock *tb = (TextBlock *)vtb;

    if (!tPtr || !tb)
        return;

    if (tb->graphic) {
        if(tb->object) {
            WMWidget *w = tb->d.widget;
            WMCreateEventHandler(W_VIEW(w), ButtonPressMask,
                handleWidgetPress, tb);
            if (W_CLASS(w) != WC_TextField && W_CLASS(w) != WC_Text) {
                (W_VIEW(w))->attribs.cursor = tPtr->view->screen->defaultCursor;
                (W_VIEW(w))->attribFlags |= CWOverrideRedirect | CWCursor;
            }
        }
        WMAddToArray(tPtr->gfxItems, (void *)tb);
        tPtr->tpos = 0;
    } else tPtr->tpos = tb->used;

    if (!tPtr->lastTextBlock || !tPtr->firstTextBlock) {
        tb->next = tb->prior = NULL;
        tb->first = True;
        tPtr->lastTextBlock = tPtr->firstTextBlock 
            = tPtr->currentTextBlock = tb;
        return;
    }

    if(!tb->first) {
        tb->marginN = tPtr->currentTextBlock->marginN;
    }

    tb->next = tPtr->currentTextBlock;
    tb->prior = tPtr->currentTextBlock->prior;
    if (tPtr->currentTextBlock->prior)
        tPtr->currentTextBlock->prior->next = tb;

    tPtr->currentTextBlock->prior = tb;
    if (!tb->prior)
        tPtr->firstTextBlock = tb;

    tPtr->currentTextBlock = tb;
}
    

void 
WMAppendTextBlock(WMText *tPtr, void *vtb)
{
    TextBlock *tb = (TextBlock *)vtb;

    if (!tPtr || !tb)
        return;

    if (tb->graphic) {
        if(tb->object) {
            WMWidget *w = tb->d.widget;
            WMCreateEventHandler(W_VIEW(w), ButtonPressMask,
                handleWidgetPress, tb);
            if (W_CLASS(w) != WC_TextField && W_CLASS(w) != WC_Text) {
                (W_VIEW(w))->attribs.cursor = 
                    tPtr->view->screen->defaultCursor;
                (W_VIEW(w))->attribFlags |= CWOverrideRedirect | CWCursor;
            }
        }
        WMAddToArray(tPtr->gfxItems, (void *)tb);
        tPtr->tpos = 0;
    } else tPtr->tpos = tb->used;

    if (!tPtr->lastTextBlock || !tPtr->firstTextBlock) {
        tb->next = tb->prior = NULL;
        tb->first = True;
        tPtr->lastTextBlock = tPtr->firstTextBlock 
            = tPtr->currentTextBlock = tb;
        return;
    }

    if(!tb->first) {
        tb->marginN = tPtr->currentTextBlock->marginN;
    }

    tb->next = tPtr->currentTextBlock->next;
    tb->prior = tPtr->currentTextBlock;
    if (tPtr->currentTextBlock->next)
        tPtr->currentTextBlock->next->prior = tb;
    
    tPtr->currentTextBlock->next = tb;

    if (!tb->next)
        tPtr->lastTextBlock = tb;

    tPtr->currentTextBlock = tb;
}

void  *
WMRemoveTextBlock(WMText *tPtr) 
{
    TextBlock *tb = NULL;

    if (!tPtr || !tPtr->firstTextBlock || !tPtr->lastTextBlock
            || !tPtr->currentTextBlock) {
        printf("cannot remove non existent TextBlock!\b");
        return tb;
    }

    tb = tPtr->currentTextBlock;
    if (tb->graphic) {
        WMRemoveFromArray(tPtr->gfxItems, (void *)tb);

        if(tb->object) {
            WMDeleteEventHandler(W_VIEW(tb->d.widget), ButtonPressMask,
                handleWidgetPress, tb);
            WMUnmapWidget(tb->d.widget);
        }
    }
    
    if (tPtr->currentTextBlock == tPtr->firstTextBlock) {
        if (tPtr->currentTextBlock->next) 
            tPtr->currentTextBlock->next->prior = NULL;

        tPtr->firstTextBlock = tPtr->currentTextBlock->next;
        tPtr->currentTextBlock = tPtr->firstTextBlock;

    } else if (tPtr->currentTextBlock == tPtr->lastTextBlock) {
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
    if (!tPtr || !tb)
        return;

    if (tb->graphic) {
        if(tb->object) {
            /* naturally, there's a danger to destroying 
                 widgets whose action brings us here:
            ie. press a button to destroy it... need to 
            find a safer way. till then... this stays commented out */
            /* WMDestroyWidget(tb->d.widget);
            wfree(tb->d.widget); */
            tb->d.widget = NULL;
        } else {
            WMReleasePixmap(tb->d.pixmap);
            tb->d.pixmap = NULL;
        }
    } else {
        WMReleaseFont(tb->d.font);
    }
        
    WMReleaseColor(tb->color);
    if (tb->sections && tb->nsections > 0)
        wfree(tb->sections);
    wfree(tb->text);
    wfree(tb);
    tb = NULL;
}
    

void 
WMRefreshText(WMText *tPtr, int vpos, int hpos)
{
    if (!tPtr || vpos<0 || hpos<0)
        return;

    if (tPtr->flags.frozen && !tPtr->flags.needsRefresh)
        return;

    if(tPtr->flags.monoFont) {
        int j, c = WMGetArrayItemCount(tPtr->gfxItems);
        TextBlock *tb;

        /* make sure to unmap widgets no matter where they are */
        for(j=0; j<c; j++) {
            if ((tb = (TextBlock *) WMGetFromArray(tPtr->gfxItems, j))) {
                if (tb->object  && ((W_VIEW(tb->d.widget))->flags.mapped))
                    WMUnmapWidget(tb->d.widget);
            }
        }
     }


    if (tPtr->vpos != vpos) {
        if (vpos < 0 || tPtr->docHeight < tPtr->visible.h) {
            tPtr->vpos = 0;
        } else if(tPtr->docHeight - vpos > tPtr->visible.h - tPtr->visible.y) {
            tPtr->vpos = vpos;
        } else {
            tPtr->vpos = tPtr->docHeight - tPtr->visible.h;
        }
    }

    if (tPtr->hpos != hpos) {
        if (hpos < 0 || tPtr->docWidth < tPtr->visible.w) {
            tPtr->hpos = 0;
        } else if(tPtr->docWidth - hpos > tPtr->visible.w - tPtr->visible.x) {
            tPtr->hpos = hpos;
        } else {
            tPtr->hpos = tPtr->docWidth - tPtr->visible.w;
        }
    }


    tPtr->flags.laidOut = False;
    layOutDocument(tPtr);
    updateScrollers(tPtr);
    paintText(tPtr);
    tPtr->flags.needsRefresh = False;

}

void
WMSetTextForegroundColor(WMText *tPtr, WMColor *color)
{
    if (!tPtr)
        return;

    if (color) 
        tPtr->fgGC = WMColorGC(color);
    else
        tPtr->fgGC = WMColorGC(WMBlackColor(tPtr->view->screen));

    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

void
WMSetTextBackgroundColor(WMText *tPtr, WMColor *color)
{
    if (!tPtr)
        return;

    if (color) {
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
    if (!tPtr)
         return;
    tPtr->flags.relief = relief;
    paintText(tPtr);
}

void 
WMSetTextHasHorizontalScroller(WMText *tPtr, Bool shouldhave)
{
    if (!tPtr) 
        return;

    if (shouldhave && !tPtr->hS) {
        tPtr->hS = WMCreateScroller(tPtr); 
        (W_VIEW(tPtr->hS))->attribs.cursor = tPtr->view->screen->defaultCursor;
        (W_VIEW(tPtr->hS))->attribFlags |= CWOverrideRedirect | CWCursor;
        WMSetScrollerArrowsPosition(tPtr->hS, WSAMinEnd);
        WMSetScrollerAction(tPtr->hS, scrollersCallBack, tPtr);
        WMMapWidget(tPtr->hS);
    } else if (!shouldhave && tPtr->hS) {
        WMUnmapWidget(tPtr->hS);
        WMDestroyWidget(tPtr->hS);
        tPtr->hS = NULL;
    }

    tPtr->hpos = 0;
    tPtr->prevHpos = 0;
    textDidResize(tPtr->view->delegate, tPtr->view);
}


void
WMSetTextHasRuler(WMText *tPtr, Bool shouldhave)
{
    if (!tPtr) 
        return;
        
    if(shouldhave && !tPtr->ruler) {
        tPtr->ruler = WMCreateRuler(tPtr);
        (W_VIEW(tPtr->ruler))->attribs.cursor =
            tPtr->view->screen->defaultCursor;      
        (W_VIEW(tPtr->ruler))->attribFlags |= CWOverrideRedirect | CWCursor;
        WMSetRulerReleaseAction(tPtr->ruler, rulerReleaseCallBack, tPtr);
        WMSetRulerMoveAction(tPtr->ruler, rulerMoveCallBack, tPtr);
    } else if(!shouldhave && tPtr->ruler) {
        WMShowTextRuler(tPtr, False);
        WMDestroyWidget(tPtr->ruler);
        tPtr->ruler = NULL;
    }
    textDidResize(tPtr->view->delegate, tPtr->view);
}   

void
WMShowTextRuler(WMText *tPtr, Bool show)
{
    if(!tPtr)
        return;
    if(!tPtr->ruler)
        return;

    if(tPtr->flags.monoFont)
        show = False;
             
    tPtr->flags.rulerShown = show;
    if(show) { 
        WMMapWidget(tPtr->ruler);
    } else {
        WMUnmapWidget(tPtr->ruler);
    }
    
    textDidResize(tPtr->view->delegate, tPtr->view);
}   

Bool
WMGetTextRulerShown(WMText *tPtr)
{
    if(!tPtr)
        return 0;

    if(!tPtr->ruler)
        return 0;

    return tPtr->flags.rulerShown;
}


void 
WMSetTextHasVerticalScroller(WMText *tPtr, Bool shouldhave)
{
    if (!tPtr) 
        return;
        
    if (shouldhave && !tPtr->vS) {
        tPtr->vS = WMCreateScroller(tPtr); 
        (W_VIEW(tPtr->vS))->attribs.cursor = tPtr->view->screen->defaultCursor;
        (W_VIEW(tPtr->vS))->attribFlags |= CWOverrideRedirect | CWCursor;
        WMSetScrollerArrowsPosition(tPtr->vS, WSAMaxEnd);
        WMSetScrollerAction(tPtr->vS, scrollersCallBack, tPtr);
        WMMapWidget(tPtr->vS);
    } else if (!shouldhave && tPtr->vS) {
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
    if (!tPtr)
        return False;
    if (amount == 0 || !tPtr->view->flags.realized)
        return False;
    
    if (amount < 0) {
        if (tPtr->vpos > 0) {
            if (tPtr->vpos > abs(amount)) tPtr->vpos += amount;
            else tPtr->vpos=0;
            scroll=True;
    } } else {
        int limit = tPtr->docHeight - tPtr->visible.h;
        if (tPtr->vpos < limit) {
            if (tPtr->vpos < limit-amount) tPtr->vpos += amount;
            else tPtr->vpos = limit;
            scroll = True;
    } }   

    if (scroll && tPtr->vpos != tPtr->prevVpos) {
        updateScrollers(tPtr);
        paintText(tPtr);
    }   
    tPtr->prevVpos = tPtr->vpos;
    return scroll;
}   

Bool 
WMPageText(WMText *tPtr, Bool direction)
{
    if (!tPtr) return False;
    if (!tPtr->view->flags.realized) return False;

    return WMScrollText(tPtr, direction?tPtr->visible.h:-tPtr->visible.h);
}

void
WMSetTextEditable(WMText *tPtr, Bool editable)
{
    if (!tPtr)
        return;
    tPtr->flags.editable = editable;
}
    
int 
WMGetTextEditable(WMText *tPtr)
{
    if (!tPtr)
        return 0;
    return tPtr->flags.editable;
}

void
WMSetTextIgnoresNewline(WMText *tPtr, Bool ignore)
{
    if (!tPtr)
        return;
    tPtr->flags.ignoreNewLine = ignore;
}

Bool
WMGetTextIgnoresNewline(WMText *tPtr)
{
    if (!tPtr)
        return True;
    return tPtr->flags.ignoreNewLine;
}

void
WMSetTextUsesMonoFont(WMText *tPtr, Bool mono)
{
    if (!tPtr)
        return;
    if (mono && tPtr->flags.rulerShown)
        WMShowTextRuler(tPtr, False);

    tPtr->flags.monoFont = mono;
    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

Bool
WMGetTextUsesMonoFont(WMText *tPtr)
{
    if (!tPtr)
        return True;
    return tPtr->flags.monoFont;
}


void
WMSetTextDefaultFont(WMText *tPtr, WMFont *font)
{
    if (!tPtr)
        return;
        
    WMReleaseFont(tPtr->dFont);
    if (font)
        tPtr->dFont = WMRetainFont(font);
    else
        tPtr->dFont = WMRetainFont(WMSystemFontOfSize(tPtr->view->screen, 12));
}

WMFont *
WMGetTextDefaultFont(WMText *tPtr)
{
    if (!tPtr)
        return NULL;
    else
        return tPtr->dFont;
}

void
WMSetTextAlignment(WMText *tPtr, WMAlignment alignment)
{
    if (!tPtr) 
        return;
    tPtr->flags.alignment = alignment;
    WMRefreshText(tPtr, tPtr->vpos, tPtr->hpos);
}

void
WMSetTextParser(WMText *tPtr, WMAction *parser)
{
    if (!tPtr) 
        return;
    tPtr->parser = parser;
}

void
WMSetTextWriter(WMText *tPtr, WMAction *writer)
{
    if (!tPtr)
        return;
    tPtr->writer = writer;
}

int 
WMGetTextInsertType(WMText *tPtr)
{
    if (!tPtr)
        return 0;
    return tPtr->flags.prepend;
}
    

void 
WMSetTextSelectionColor(WMText *tPtr, WMColor *color)
{
    if (!tPtr || !color)
        return;

    setSelectionProperty(tPtr, NULL, color);
}



void 
WMSetTextSelectionFont(WMText *tPtr, WMFont *font)
{
    if (!tPtr || !font)
        return;
    
    setSelectionProperty(tPtr, font, NULL);
}


void
WMFreezeText(WMText *tPtr) 
{
    if (!tPtr)
        return;

    tPtr->flags.frozen = True;
}

void
WMThawText(WMText *tPtr) 
{
    if (!tPtr)
        return;

    tPtr->flags.frozen = False;
}


Bool 
WMFindInTextStream(WMText *tPtr, char *needle, Bool direction, 
    Bool caseSensitive)
{
    TextBlock *tb = NULL;
    char *haystack = NULL, *mark = NULL;
    unsigned short pos;

    if (!tPtr || !needle)
        return False;

    if (! (tb = tPtr->currentTextBlock)) {
        if (! (tb = ( (direction > 0) ?
                 tPtr->firstTextBlock : tPtr->lastTextBlock) ) ){
            return False;
       }
    } else { 
        if(tb != ((direction>0) ?tPtr->firstTextBlock : tPtr->lastTextBlock))
            tb = (direction>0) ? tb->next : tb->prior;
    }
   
printf("\n%p\n\n", tb);

    while(tb) {
        if (!tb->graphic) { 
    pos = tPtr->tpos;
    if(pos+1 < tb->used)
        pos++;
printf("here\n");
output(tb->text, tb->used);
            mark = strstr(&tb->text[pos], needle); //, tb->used);
            if(mark) { 
                tPtr->currentTextBlock = tb;
                tPtr->tpos = (int)(mark - tb->text);
                updateCursorPosition(tPtr);
                return True;
            }
        }   
        tb = (direction>0) ? tb->next : tb->prior;
        pos = 0;
    }

    return False;
}


typedef struct _currentFormat {
    WMArray *fonts;
    WMArray *colors;
    WMColor *ccolor;
    WMFont *cfont;
    WMRulerMargins margins;
    //WMArray *aligns; // for tables...
    /* the following are "nested" 
        i.e.:  <b><b><i></b><b></i>   
                1  2  1  1   2   0   get it? */
    short i;
    short b;
    short u;
    short fmargin;
    short bmargin;
    short first:1;
    short type:1;
    WMAlignment align:2;
    short ul:3; /* how "nested"... up to 8 levels deep */
    short comment:1; /* ignore text till --> */
    short RESERVED:10;
} CFMT;
CFMT cfmt;



#if 0
getArg(char *t, short type, void *arg)
{
    short d=0;
    while(*(++t) && !d) {
        if(type==0) {
            if(*t>='0' && *t<='9') {
                sscanf(t, "%d", arg);
                while(*t&& (*t<'0' || *t>'9'))
                    t++;
                d=1;
            }
        }
    }
}
#endif
    
static void
parseToken(WMText *tPtr, char *token, short tk)
{
    short open=0; /* 0 starts, 1 closes */
    void *tb= NULL;
    int prepend = WMGetTextInsertType(tPtr);
    WMScreen *scr = tPtr->view->screen;

    if(tk == -1) { 
         WMAppendTextBlock(tPtr, WMCreateTextBlockWithText(tPtr, 
            NULL, cfmt.cfont, cfmt.ccolor, True, 0));
        return;
    }


    while(*token &&  *token == ' ') 
        token++;

        if(*token == '/') {
            token++;
            open = 1;

            while(*token == ' ')
                token++;
        }

    if(!tPtr->flags.parsingHTML) { 
        if(mystrcasecmp(token, "html", 4, False)) {
printf("got HTMLLLL: [%s]\n", token);
            tPtr->flags.parsingHTML = True;
        } else { 
             WMAppendTextBlock(tPtr, WMCreateTextBlockWithText(tPtr, 
                token, cfmt.cfont, cfmt.ccolor, cfmt.first, strlen(token)));
        }

        return;
    }
    

    if(strlen(token)==1) {
    /* nice and fast for small tokens... no need for too much brain 
        power here */
        switch(TOLOWER(*token)) {
            case 'i': 
                if(!open) {
                    cfmt.cfont = WMConvertFontToItalic(scr, cfmt.cfont);
                    WMAddToArray(cfmt.fonts, (void *)cfmt.cfont);
                } else { /*dun wanna remove the baseFont eh? */
                    int count = WMGetArrayItemCount(cfmt.fonts); 
                   if(count>1) {
                        WMDeleteFromArray(cfmt.fonts, count-1);
                    }
                     cfmt.cfont = (WMFont *)WMGetFromArray(cfmt.fonts,
                        WMGetArrayItemCount(cfmt.fonts)-1); 
                }printf("i\n"); break;

            case 'b': 
                if(!open) {
                    cfmt.cfont = WMConvertFontToBold(scr, cfmt.cfont);
                    WMAddToArray(cfmt.fonts, (void *)cfmt.cfont);
                } else { /*dun wanna remove the baseFont eh? */
                    int count = WMGetArrayItemCount(cfmt.fonts); 
                    if(count>1) 
                        WMDeleteFromArray(cfmt.fonts, count-1);
                     cfmt.cfont = (WMFont *)WMGetFromArray(cfmt.fonts,
                        WMGetArrayItemCount(cfmt.fonts)-1); 
                } break;
            case 'p': 
                cfmt.first = True;
                tb = WMCreateTextBlockWithText(tPtr, NULL, cfmt.cfont,     
                    cfmt.ccolor, cfmt.first, 0);
               // WMSetTextBlockProperties(tb, cfmt.first, False, (cfmt.u?1:0), 0, cfmt.margins);
                WMAppendTextBlock(tPtr, tb);
                cfmt.first = False;
                break;
            case 'u': cfmt.u = !open; break;
        }
    } else { 
        if(mystrcasecmp(token, "br", 2, False)) {
                cfmt.first = True;
        }
        else if(mystrcasecmp(token, "ul", 2, False)) {
            if(open) { 
                if(cfmt.ul>1) cfmt.ul--;
            } else cfmt.ul++;
            if(cfmt.ul) {
                cfmt.bmargin = cfmt.ul*30;
                cfmt.fmargin = cfmt.bmargin-10;
            } else cfmt.fmargin = cfmt.bmargin = 0;
        } else if(mystrcasecmp(token, "li", 2, False)) {
                cfmt.first = True;
//change margins... create a new margin....
            //(cfmt.fmargin, cfmt.bmargin, 
        } else if(mystrcasecmp(token, "html", 4, False)) {
            tPtr->flags.parsingHTML = !open;

        } else if(mystrcasecmp(token, "align", 5, False))
            ;//printf("align");
        else if(mystrcasecmp(token, "img", 3, False))  {
            if(!open) {
                char *mark=NULL;
                WMPixmap *pixmap; 
                token+=3;
                while(*token == ' ') token++;
                do { 
                switch(TOLOWER(*token)) {
                case 's':         
                if(TOLOWER(*(1+token)) == 'r' && TOLOWER(*(2+token)) == 'c') {
                mark = strchr(token, '='); 
                if(mark) {
                char img[256], *iptr;
                token = mark+1;
                if(!token) return;
                sscanf(token, "%s", img);
                iptr = img;
                 if(*img == '\"') { img[strlen(img)-1] = 0; iptr++;}
                pixmap = WMCreatePixmapFromFile(scr, iptr);
                if(pixmap) {
                    tb = WMCreateTextBlockWithPixmap(tPtr, pixmap, 
                        iptr, cfmt.ccolor, cfmt.first, 0);
                   // WMSetTextBlockProperties(tb, cfmt.first, 
                     //   False, (cfmt.u?1:0), 0, cfmt.margins);
                    WMAppendTextBlock(tPtr, tb);
                    cfmt.first = False;
                }
                //printf("[%s]\n", iptr);
                } } break; } } while(*(token++));
            }     
        } else if(mystrcasecmp(token, "font", 4, False)) {
#if 0
            if(open) {
                 cfmt.cfont = (WMFont *)WMGetFromArray(cfmt.fonts,
                        WMGetArrayItemCount(cfmt.fonts)-1); 
            } else 
                    (WMColor *)WMGetFromArray(cfmt.colors, 
                WMGetArrayItemCount(cfmt.colors)-1), 
#endif    
        }
        else if(mystrcasecmp(token, "center", 6, False)) {
printf("center\n");
            if(open) cfmt.align = WALeft;
            else cfmt.align = WACenter;
                cfmt.first = True;
//change margins...
        }
    }
        



    //printf("parse token  (%s)[%s]\n", open?"close":"open", token); 
#if 0
    i=0;
    //while(*token && !isspace(*(token))) token++;
//printf("A:%d a:%d z%d Z%d\n", '1', 'a', 'Z', 'z');
    do { 
        if(!mm) {
            if(c>=65 &&  c<=122) { major[i++] = c;
            } else if(c==' ' || c=='='){ major[i] = 0; i=0; mm=1; 
            printf("\nmajor: [%s]", major);}
        } else {
            if(c!=' ') {
                minor[i++] = c;
            } else { minor[i] = 0; i=0; printf("  minor: [%s]  ", minor);}
        }
    }while((c = *(++token)));
#endif
    
    
    //printf("parse token  (%s)[%s]\n", open?"close":"open", token); 
}
    

