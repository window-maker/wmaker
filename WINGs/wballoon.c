



#include "WINGsP.h"

#include <X11/extensions/shape.h>


typedef struct W_Balloon {
    W_View *view;

    WMHashTable *table;		       /* Table from view ptr to text */

    WMColor *backColor;
    WMColor *textColor;
    WMFont *font;

    WMHandlerID timer; 		       /* timer for showing balloon */
    
    WMHandlerID noDelayTimer;

    int delay;

    Window forWindow;		       /* window for which the balloon
					* is being show in the moment */

    struct {
	WMAlignment alignment:2;
	unsigned enabled:1;
	unsigned noDelay:1;
    } flags;
} Balloon;



#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		14
#define DEFAULT_ALIGNMENT	WALeft
#define DEFAULT_DELAY		500

#define NO_DELAY_DELAY		150


static void destroyBalloon(Balloon *bPtr);


static void handleEvents(XEvent *event, void *data);

static void showText(Balloon *bPtr, int x, int y, int w, int h, char *text);


struct W_Balloon*
W_CreateBalloon(WMScreen *scr)
{
    Balloon *bPtr;

    bPtr = wmalloc(sizeof(Balloon));
    memset(bPtr, 0, sizeof(Balloon));

    bPtr->view = W_CreateTopView(scr);
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;

    bPtr->view->attribFlags |= CWOverrideRedirect;
    bPtr->view->attribs.override_redirect = True;

    bPtr->textColor = WMRetainColor(bPtr->view->screen->black);

    WMCreateEventHandler(bPtr->view, StructureNotifyMask, handleEvents, bPtr);

    W_ResizeView(bPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    bPtr->flags.alignment = DEFAULT_ALIGNMENT;

    bPtr->table = WMCreateHashTable(WMIntHashCallbacks);

    bPtr->delay = DEFAULT_DELAY;

    bPtr->flags.enabled = 1;

    return bPtr;
}



void
WMSetBalloonTextAlignment(WMScreen *scr, WMAlignment alignment)
{
    scr->balloon->flags.alignment = alignment;

}


void
WMSetBalloonTextForView(char *text, WMView *view)
{
    char *oldText = NULL;
    WMScreen *scr = view->screen;

    if (text) {
	oldText = WMHashInsert(scr->balloon->table, view, wstrdup(text));
    } else {
	oldText = WMHashGet(scr->balloon->table, view);

	WMHashRemove(scr->balloon->table, view);
    }

    if (oldText) {
	free(oldText);
    }
}


void
WMSetBalloonFont(WMScreen *scr, WMFont *font)
{
    Balloon *bPtr = scr->balloon;
    
    if (bPtr->font!=NULL)
	WMReleaseFont(bPtr->font);

    if (font)
	bPtr->font = WMRetainFont(font);
    else
	bPtr->font = NULL;
}


void
WMSetBalloonTextColor(WMScreen *scr, WMColor *color)
{
    Balloon *bPtr = scr->balloon;

    if (bPtr->textColor)
	WMReleaseColor(bPtr->textColor);

    bPtr->textColor = WMRetainColor(color);
}


void
WMSetBalloonDelay(WMScreen *scr, int delay)
{
    scr->balloon->delay = delay;
}


void
WMSetBalloonEnabled(WMScreen *scr, Bool flag)
{
    scr->balloon->flags.enabled = flag;

    W_UnmapView(scr->balloon->view);
}


static void
clearNoDelay(void *data)
{
    Balloon *bPtr = (Balloon*)data;

    bPtr->flags.noDelay = 0;
    bPtr->noDelayTimer = NULL;
}


void
W_BalloonHandleLeaveView(WMView *view)
{
    Balloon *bPtr = view->screen->balloon;

    if (bPtr->view->flags.mapped && bPtr->forWindow == view->window) {
	W_UnmapView(bPtr->view);

	if (bPtr->timer)
	    WMDeleteTimerHandler(bPtr->timer);

	bPtr->timer = NULL;

	bPtr->noDelayTimer = WMAddTimerHandler(NO_DELAY_DELAY, clearNoDelay, 
					       bPtr);
    }
    bPtr->forWindow = None;
}


static void
showBalloon(void *data)
{
    char *text;
    WMView *view = (WMView*)data;
    Balloon *bPtr = view->screen->balloon;
    int x, y;
    Window foo;

    bPtr->timer = NULL;

    text = WMHashGet(bPtr->table, view);
    if (!text)
	return;

    XTranslateCoordinates(view->screen->display, view->window,
			  view->screen->rootWin, 0, 0, &x, &y, &foo);

    if (!bPtr->view->flags.realized)
	W_RealizeView(bPtr->view);

    showText(bPtr, x, y, view->size.width, view->size.height, text);

    bPtr->forWindow = view->window;

    bPtr->flags.noDelay = 1;
}



void
W_BalloonHandleEnterView(WMView *view)
{
    Balloon *bPtr = view->screen->balloon;
    char *text;

    if (!bPtr->flags.enabled)
	return;

    text = WMHashGet(bPtr->table, view);
    if (!text) {
	bPtr->forWindow = None;

	if (bPtr->view->flags.realized)
	    W_UnmapView(bPtr->view);

	return;
    }

    if (bPtr->timer)
	WMDeleteTimerHandler(bPtr->timer);

    if (bPtr->noDelayTimer)
	WMDeleteTimerHandler(bPtr->noDelayTimer);
    bPtr->noDelayTimer = NULL;

    if (bPtr->flags.noDelay) {
	bPtr->timer = NULL;

	showBalloon(view);
    } else {
	bPtr->timer = WMAddTimerHandler(bPtr->delay, showBalloon, view);
    }
}


#define TOP	0
#define BOTTOM	1
#define LEFT	0
#define RIGHT	2

#define TLEFT	(TOP|LEFT)
#define TRIGHT 	(TOP|RIGHT)
#define BLEFT	(BOTTOM|LEFT)
#define BRIGHT	(BOTTOM|RIGHT)



#define 	SPACE	12


static void
drawBalloon(Display *dpy, Pixmap pix, GC gc, int x, int y, int w, int h, 
	    int side)
{
    int rad = h*3/10;
    XPoint pt[3];
    
    XFillArc(dpy, pix, gc, x, y, rad, rad, 90*64, 90*64);
    XFillArc(dpy, pix, gc, x, y+h-1-rad, rad, rad, 180*64, 90*64);
    
    XFillArc(dpy, pix, gc, x+w-1-rad, y, rad, rad, 0*64, 90*64);
    XFillArc(dpy, pix, gc, x+w-1-rad, y+h-1-rad, rad, rad, 270*64, 90*64);
    
    XFillRectangle(dpy, pix, gc, x, y+rad/2, w, h-rad);
    XFillRectangle(dpy, pix, gc, x+rad/2, y, w-rad, h);

    if (side & BOTTOM) {
	pt[0].y = y+h-1;
	pt[1].y = y+h-1+SPACE;
	pt[2].y = y+h-1;
    } else {
	pt[0].y = y;
	pt[1].y = y-SPACE;
	pt[2].y = y;
    }
    if (side & RIGHT) {
	pt[0].x = x+w-h+2*h/16;
	pt[1].x = x+w-h+11*h/16;
	pt[2].x = x+w-h+7*h/16;
    } else {
	pt[0].x = x+h-2*h/16;
	pt[1].x = x+h-11*h/16;
	pt[2].x = x+h-7*h/16;
    }
    XFillPolygon(dpy, pix, gc, pt, 3, Convex, CoordModeOrigin);
}


static Pixmap
makePixmap(WMScreen *scr, int width, int height, int side, Pixmap *mask)
{
    Display *dpy = WMScreenDisplay(scr);
    Pixmap bitmap;
    Pixmap pixmap;
    int x, y;
    WMColor *black = WMBlackColor(scr);
    WMColor *white = WMWhiteColor(scr);

    bitmap = XCreatePixmap(dpy, scr->rootWin, width+SPACE, height+SPACE, 1);

    XSetForeground(dpy, scr->monoGC, 0); 
    XFillRectangle(dpy, bitmap, scr->monoGC, 0, 0, width+SPACE, height+SPACE);

    pixmap = XCreatePixmap(dpy, scr->rootWin, width+SPACE, height+SPACE,
			   scr->depth);

    XFillRectangle(dpy, pixmap, WMColorGC(black), 0, 0, 
		   width+SPACE, height+SPACE);

    if (side & BOTTOM) {
	y = 0;
    } else {
	y = SPACE;
    }
    x = 0;

    XSetForeground(dpy, scr->monoGC, 1);
    drawBalloon(dpy, bitmap, scr->monoGC, x, y, width, height, side);
    drawBalloon(dpy, pixmap, WMColorGC(white), x+1, y+1, width-2, height-2,
		side);

    *mask = bitmap;

    WMReleaseColor(black);
    WMReleaseColor(white);

    return pixmap;
}


static void
showText(Balloon *bPtr, int x, int y, int h, int w, char *text)
{
    WMScreen *scr = bPtr->view->screen;
    Display *dpy = WMScreenDisplay(scr);
    int width;
    int height;
    Pixmap pixmap;
    Pixmap mask;
    WMFont *font = bPtr->font ? bPtr->font : scr->normalFont;
    int textHeight;
    int side = 0;
    int ty;
    int bx, by;

    {
	int w;
	char *ptr, *ptr2;
	
	ptr = text;
	width = 0;
	while (ptr && ptr2) {
	    ptr2 = strchr(ptr, '\n');
	    if (ptr2) {
		w = WMWidthOfString(font, ptr, ptr2 - ptr);
	    } else {
		w = WMWidthOfString(font, ptr, strlen(ptr));
	    }
	    if (w > width)
		width = w;
	    ptr = ptr2 + 1;
	}
    }
    
    width += 16;

    textHeight = W_GetTextHeight(font, text, width, False);
    
    height = textHeight + 4;
    
    if (height < 16)
	height = 16;
    if (width < height)
	width = height;

    if (x + width > scr->rootView->size.width) {
	side = RIGHT;
	bx = x - width + w/2;
	if (bx < 0)
	    bx = 0;
    } else {
	side = LEFT;
	bx = x + w/2;
    }
    if (bx + width > scr->rootView->size.width)
	bx = scr->rootView->size.width - width;

    if (y - (height + SPACE) < 0) {
	side |= TOP;
	by = y+h-1;
	ty = SPACE;
    } else {
	side |= BOTTOM;
	by = y - (height + SPACE);
	ty = 0;
    }
    pixmap = makePixmap(scr, width, height, side, &mask);

    W_PaintText(bPtr->view, pixmap, font, 8, ty + (height - textHeight)/2,
		width, bPtr->flags.alignment,
		WMColorGC(bPtr->textColor ? bPtr->textColor : scr->black),
		False, text, strlen(text));

    XSetWindowBackgroundPixmap(dpy, bPtr->view->window, pixmap);

    W_ResizeView(bPtr->view, width, height+SPACE);

    XFreePixmap(dpy, pixmap);

    XShapeCombineMask(dpy, bPtr->view->window, ShapeBounding, 0, 0, mask,
		      ShapeSet);
    XFreePixmap(dpy, mask);

    W_MoveView(bPtr->view, bx, by);

    W_MapView(bPtr->view);
}


static void
handleEvents(XEvent *event, void *data)
{
    Balloon *bPtr = (Balloon*)data;

    switch (event->type) {	
     case DestroyNotify:
	destroyBalloon(bPtr);
	break;
    }
}


static void
destroyBalloon(Balloon *bPtr)
{
    WMHashEnumerator e;
    char *str;

    e = WMEnumerateHashTable(bPtr->table);

    while ((str = WMNextHashEnumeratorItem(&e))) {
	free(str);
    }
    WMFreeHashTable(bPtr->table);

    if (bPtr->textColor)
	WMReleaseColor(bPtr->textColor);

    if (bPtr->font)
	WMReleaseFont(bPtr->font);

    free(bPtr);
}
