
/* WMRuler: nifty ruler widget for WINGs  (OK, for WMText ;-) */
/* Copyleft (>) 1999, 2000 Nwanua Elumeze <nwanua@colorado.edu> */


#include <WINGsP.h>
#include <WUtil.h>
#include <WMaker.h>

#include "wruler.h"

#define DEFAULT_X_OFFSET 22
#define MIN_DOC_WIDTH 200
#define DEFAULT_RULER_WIDTH   240
#define DEFAULT_RULER_HEIGHT  40 


/* a pixel is defined here as 1/8 of an "inch" */
/* an "inch" is not the British unit inch... just so you know :-P */

typedef struct W_Ruler {
    W_Class widgetClass;
    W_View  *view;

    W_View  *pview;
    
    void *clientData;
    WMAction *action, *moveaction;

	 struct {
        int left, body, first, right, dleft;
    } margins;
	int which_marker;

	int end;
	int pressed;
	int offset;

	struct { 
		unsigned int showtabs:1;
		unsigned int buttonPressed:1;
	} flags;
	
} Ruler;


static void
resizeRuler(W_ViewDelegate *self, WMView *view)
{   
    Ruler    *rPtr = (Ruler *)view->self;
    
    CHECK_CLASS(rPtr, WC_Ruler);
    
    W_ResizeView(rPtr->view, view->size.width, 
		DEFAULT_RULER_HEIGHT); /* more like DEFACTO_RULER_HEIGHT ;-) */

	rPtr->end  = view->size.width;
	rPtr->margins.right = view->size.width;
}   
    

W_ViewDelegate _RulerViewDelegate =
{
    NULL,
    NULL,
    resizeRuler,
    NULL,   
    NULL
};  

static void 
paintRuler(Ruler *rPtr)
{
	GC gc;
	XPoint  points[6];
	WMFont *font;
	int i, j, xpos;
	char c[3];
	int actual_x;
	
	CHECK_CLASS(rPtr, WC_Ruler);

	if(!rPtr->view->flags.mapped)
		return;

 	gc = WMColorGC(WMBlackColor(rPtr->view->screen));
	font = WMSystemFontOfSize(rPtr->view->screen, 8);


	WMDrawString(rPtr->view->screen, rPtr->view->window, gc, 
		font, rPtr->margins.dleft+2, 25, "0   inches", 10);

	/* marker ticks */
	j=0;
	for(i=80; i<rPtr->view->size.width; i+=80) {
		if(j<10)
			snprintf(c,3,"%d",++j);
		else
			snprintf(c,3,"%2d",++j);
		WMDrawString(rPtr->view->screen, rPtr->view->window, gc, 
			font, rPtr->margins.dleft+2+i, 25, c, 2);
	}

	if(rPtr->flags.showtabs){
    	points[0].y = 9;
    	points[1].y = 15;
    	points[2].y = 20;
	}

	for(i=0; i<rPtr->view->size.width; i+=40) {
		XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
			gc, rPtr->margins.dleft+i, 21, rPtr->margins.dleft+i, 31);
		
		if(rPtr->flags.showtabs){
    		points[0].x = rPtr->margins.dleft+i+40;
    		points[1].x = points[0].x+6;
    		points[2].x = points[0].x;
			XFillPolygon (rPtr->view->screen->display, rPtr->view->window,
 				WMColorGC(WMDarkGrayColor(rPtr->view->screen)),
				points, 3, Convex, CoordModeOrigin);

			XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
 				WMColorGC(WMDarkGrayColor(rPtr->view->screen)),
				rPtr->margins.dleft+i, 18, rPtr->margins.dleft+i, 21);
		}
	}
		
	for(i=0; i<rPtr->view->size.width; i+=20)
		XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
			gc, rPtr->margins.dleft+i, 21, rPtr->margins.dleft+i, 27);
		
	for(i=0; i<rPtr->view->size.width-20; i+=10)
		XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
			gc, rPtr->margins.dleft+i, 21, rPtr->margins.dleft+i, 24);
		
	/* guide the end marker in future drawings till next resize */
	rPtr->end = i+12;

	/* base line */
	XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
		gc, rPtr->margins.dleft, 21, rPtr->margins.left+i-10, 21);


	/* Marker for right margin
       
         /|
        / | 
       /__|
          |
          |          */
 
	xpos = rPtr->margins.right;
	XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
 		WMColorGC(WMDarkGrayColor(rPtr->view->screen)), xpos, 10, xpos, 20);

    points[0].x = xpos+1;
    points[0].y = 2;
    points[1].x = points[0].x-6;
    points[1].y = 9;
    points[2].x = points[0].x-6;
    points[2].y = 11;
    points[3].x = points[0].x;
    points[3].y = 11;
	XFillPolygon (rPtr->view->screen->display, rPtr->view->window,
		gc, points, 4, Convex, CoordModeOrigin);
		

	/* Marker for left margin
       
       |\
       | \
       |__\
       |
       |          */
 
	xpos = rPtr->margins.left;
	XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
		WMColorGC(WMDarkGrayColor(rPtr->view->screen)), xpos, 10, xpos, 20);

    points[0].x = xpos;
    points[0].y = 3;
    points[1].x = points[0].x+6;
    points[1].y = 10;
    points[2].x = points[0].x+6;
    points[2].y = 11;
    points[3].x = points[0].x;
    points[3].y = 11;
	XFillPolygon (rPtr->view->screen->display, rPtr->view->window,
		gc, points, 4, Convex, CoordModeOrigin);

/*
    points[3].x = 

	/* Marker for first line only 
         _____
	     |___|
           |
         
	*/

	xpos = rPtr->margins.first + rPtr->margins.dleft;
	XFillRectangle(rPtr->view->screen->display, rPtr->view->window,
	gc, xpos-5, 7, 11, 5);

	XDrawLine(rPtr->view->screen->display, rPtr->view->window, 
		gc, xpos, 10, xpos, 20);


	/* Marker for rest of body 
          _____
          \   /
	       \./      */


	xpos = rPtr->margins.body + rPtr->margins.dleft;
    points[0].x = xpos-5;
    points[0].y = 14;
    points[1].x = points[0].x+11;
    points[1].y = 14;
    points[2].x = points[0].x+5;
    points[2].y = 20;
	XFillPolygon (rPtr->view->screen->display, rPtr->view->window,
		gc, points, 3, Convex, CoordModeOrigin);


	if(!rPtr->flags.buttonPressed)
		return;

	/* actual_x is used as a shortcut is all... */
	switch(rPtr->which_marker){
		case WRulerLeft: actual_x = rPtr->margins.left; break;
		case WRulerBody: 
			actual_x = rPtr->margins.body + rPtr->margins.dleft; 
		break;
		case WRulerFirst: 
			actual_x = rPtr->margins.first + rPtr->margins.dleft; 
		break;
		case WRulerRight: actual_x = rPtr->margins.right; break; 
		default:  return;
	}

		
 	gc = WMColorGC(WMDarkGrayColor(rPtr->view->screen)),

	XSetLineAttributes(rPtr->view->screen->display, gc, 1, 
		LineOnOffDash, CapNotLast, JoinMiter);

	XDrawLine(rPtr->pview->screen->display, rPtr->pview->window, 
 		gc, actual_x+1, 45, actual_x+1, rPtr->pview->size.height-5);

	XSetLineAttributes(rPtr->view->screen->display, gc, 1, 
		LineSolid, CapNotLast, JoinMiter);
	
         
}

static int
whichMarker(Ruler *rPtr, int x, int y)
{
	CHECK_CLASS(rPtr, WC_Ruler);

	if(y>22)
		return;

	if(x<0)
		return;

	if( rPtr->margins.dleft-x >= -6 && y <= 9 && 
		rPtr->margins.dleft-x <=0  && y>=4)
			return WRulerLeft;

	if( rPtr->margins.right-x >= -1 && y <= 11 && 
		rPtr->margins.right-x <=5  && y>=4)
			return WRulerRight;

	x -= rPtr->margins.dleft;
	if(x<0)
		return;

	if( rPtr->margins.first-x <= 4   && y<=12  && 
		rPtr->margins.first-x >= -5  && y>=5)
			return WRulerFirst;



	if( rPtr->margins.body-x <= 4 &&  y<=19  && 
		rPtr->margins.body-x >= -5  && y>=15)
			return WRulerBody;


	/* both first and body? */
	if( rPtr->margins.first-x <= 4 && rPtr->margins.first-x >= -5 
		&& rPtr->margins.body-x <= 4 && rPtr->margins.body-x >= -5
		&& y>=13 && y<=14) 
			return WRulerBoth;
	
	return -1;
}
	
static Bool
verifyMarkerMove(Ruler *rPtr, int x)
{
	if(!rPtr)
		return;

	switch(rPtr->which_marker){
		case WRulerLeft: 
		if(x < rPtr->margins.right-MIN_DOC_WIDTH &&
			rPtr->margins.body + x <= rPtr->margins.right-MIN_DOC_WIDTH && 
			rPtr->margins.first + x <= rPtr->margins.right-MIN_DOC_WIDTH)  
			rPtr->margins.left = x;
		else return False;
		break;


		case WRulerBody: 
		if(x < rPtr->margins.right-MIN_DOC_WIDTH && x >= rPtr->margins.left)
			rPtr->margins.body = x - rPtr->margins.dleft;
		else return False;
		break;

		case WRulerFirst: 
		if(x < rPtr->margins.right-MIN_DOC_WIDTH && x >= rPtr->margins.left)
			rPtr->margins.first = x - rPtr->margins.dleft;
		else return False;
 		break;

		case WRulerRight: 
		if( x >= rPtr->margins.first+MIN_DOC_WIDTH &&
			x >= rPtr->margins.body+MIN_DOC_WIDTH &&
			x >= rPtr->margins.left+MIN_DOC_WIDTH &&  
			x <= rPtr->end) 
				rPtr->margins.right = x;  
		else return False;
		break; 

		case WRulerBoth: 
			if( x < rPtr->margins.right-MIN_DOC_WIDTH && x >= rPtr->margins.left) {
				rPtr->margins.first = x - rPtr->margins.dleft;
				rPtr->margins.body = x - rPtr->margins.dleft;
			} else return False;
		break;

		default : return False;
	}

	return True;
}
	

static void
moveMarker(Ruler *rPtr, int x)
{

	CHECK_CLASS(rPtr, WC_Ruler);

	if(x < rPtr->offset || x > rPtr->end)
		return;

/* restrictive ticks */
/*
	if(x%10-2 != 0)  
		return;
*/
	if(!verifyMarkerMove(rPtr, x))
		return;

	/* clear around the motion area */
#if 0
	XClearWindow(rPtr->view->screen->display, rPtr->view->window);
#else

	XClearArea(rPtr->view->screen->display, rPtr->view->window, 
		x-10, 3, 20, rPtr->view->size.height-4, False);

#endif

#if 0
	{ //while(1) {
	if  (QLength(rPtr->view->screen->display) > 0 ) {
		XEvent *    event;
		while(QLength(rPtr->view->screen->display) > 0 ) {
			XNextEvent(rPtr->view->screen->display, event);
			if(event->type == MotionNotify ||
				event->type == ButtonRelease)
						break;
		}
	}
	}
#endif
		


	rPtr->pressed = x;

	if(rPtr->moveaction)
		(rPtr->moveaction)(rPtr, rPtr->clientData);

	paintRuler(rPtr);
}

static void
handleEvents(XEvent *event, void *data)
{
	Ruler *rPtr = (Ruler*)data;

	CHECK_CLASS(rPtr, WC_Ruler);


	switch (event->type) {
	
		case Expose:
			if(event->xexpose.count)
				break;
			if(rPtr->view->flags.realized)
				paintRuler(rPtr);
		break;

		case ButtonPress:
			if(event->xbutton.button != Button1)
				return;

			rPtr->flags.buttonPressed = True;
			rPtr->which_marker = 
				whichMarker(rPtr, event->xmotion.x, 
					event->xmotion.y);

#if 0
			/* clear around the clicked area */
			if(rPtr->which_marker>=0 && rPtr->which_marker <= 4) {

				XClearArea(rPtr->view->screen->display, rPtr->view->window, 
					WMGetRulerMargin(rPtr, rPtr->which_marker)-10, 2, 20, 21, True);

				paintRuler(rPtr);
			}
#endif
			paintRuler(rPtr);
		break;

		case ButtonRelease:
			if(event->xbutton.button != Button1)
				return;

			rPtr->flags.buttonPressed = False;
			rPtr->pressed = event->xmotion.x;
			if(rPtr->which_marker == 0) {
				rPtr->margins.dleft = rPtr->margins.left;
			
				/* clear entire (moved) ruler */
				XClearArea(rPtr->view->screen->display, 
					rPtr->view->window, 0, 0, rPtr->view->size.width,
					rPtr->view->size.height, True);
			}
				
			if(rPtr->action)
				(rPtr->action)(rPtr, rPtr->clientData);

			paintRuler(rPtr);
		break;

		case MotionNotify:
			if(rPtr->flags.buttonPressed && 
				(event->xmotion.state & Button1Mask)) {
					moveMarker(rPtr, event->xmotion.x);
			}
		break;

	}
}


WMRuler *
WMCreateRuler(WMWidget *parent)
{
	Ruler *rPtr = wmalloc(sizeof(Ruler));

	memset(rPtr, 0, sizeof(Ruler));
    
	rPtr->widgetClass = WC_Ruler;
    
	rPtr->view = W_CreateView(W_VIEW(parent));
	if (!rPtr->view) {
		free(rPtr);
		return NULL;
	}
	rPtr->view->self = rPtr;

	W_ResizeView(rPtr->view, DEFAULT_RULER_WIDTH, DEFAULT_RULER_HEIGHT);

	WMCreateEventHandler(rPtr->view, ExposureMask|StructureNotifyMask
		|EnterWindowMask|LeaveWindowMask|FocusChangeMask 
 		|ButtonReleaseMask|ButtonPressMask|KeyReleaseMask
		|KeyPressMask|Button1MotionMask, handleEvents, rPtr);

	rPtr->view->delegate = &_RulerViewDelegate;

	rPtr->which_marker = WRulerLeft;  /* none */	
	rPtr->pressed = 0;	


	rPtr->offset = DEFAULT_X_OFFSET;
 	rPtr->margins.left = DEFAULT_X_OFFSET;
 	rPtr->margins.dleft = DEFAULT_X_OFFSET; 
 	rPtr->margins.body = 0;  /* relative */
 	rPtr->margins.first = 0;  /* relative */
 	rPtr->margins.right = 300+DEFAULT_X_OFFSET;
	rPtr->end = 320+DEFAULT_X_OFFSET;

	rPtr->flags.showtabs = False;
	rPtr->flags.buttonPressed = False;

	rPtr->pview = W_VIEW(parent);

    return rPtr;
}        

	
void
WMShowRulerTabs(WMRuler *rPtr, Bool show)
{   
	if(!rPtr)
		return;
    CHECK_CLASS(rPtr, WC_Ruler);
	rPtr->flags.showtabs = show;
}

void
WMSetRulerMargin(WMRuler *rPtr, int which, int pixels)
{
	if(!rPtr)
		return;
		

 	if(pixels<rPtr->offset)
		pixels =  rPtr->offset; 

	rPtr->which_marker = which;
	if(!verifyMarkerMove(rPtr, pixels))
		return;

	rPtr->pressed = pixels;
	if(which == WRulerLeft)
		rPtr->margins.dleft = rPtr->margins.left;

	if(!rPtr->view->flags.realized)
		return;

	XClearArea(rPtr->view->screen->display, 
		rPtr->view->window, 0, 0, rPtr->view->size.width,
		rPtr->view->size.height, True);

	paintRuler(rPtr);
}

int
WMGetRulerMargin(WMRuler *rPtr, int which)
{
	if(!rPtr)
		return;

	CHECK_CLASS(rPtr, WC_Ruler);
	
	switch(which) {
		case WRulerLeft: return rPtr->margins.left; break;
		case WRulerBody: return rPtr->margins.body; break;
		case WRulerFirst: return rPtr->margins.first; break;
		case WRulerRight: return rPtr->margins.right; break;
		case WRulerBoth: return rPtr->margins.body; break;
		case WRulerDocLeft: return rPtr->margins.dleft; break;
		default: return rPtr->margins.dleft;
	}
}

/* _which_ one was released */
int 
WMGetReleasedRulerMargin(WMRuler *rPtr)
{
	if(!rPtr)
		return WRulerLeft; 
	CHECK_CLASS(rPtr, WC_Ruler);
	if(rPtr->which_marker != -1)
		return rPtr->which_marker;
	else
		return WRulerLeft;
}

/* _which_ one is being grabbed */
int 
WMGetGrabbedRulerMargin(WMRuler *rPtr)
{
	if(!rPtr)
		return WRulerLeft; 
	CHECK_CLASS(rPtr, WC_Ruler);
	if(rPtr->which_marker != -1)
		return rPtr->which_marker;
	else
		return WRulerLeft;
}



void
WMSetRulerOffset(WMRuler *rPtr, int pixels)
{
	if(!rPtr || pixels<0)
		return;
	CHECK_CLASS(rPtr, WC_Ruler);
	rPtr->offset = pixels;
}

int 
WMGetRulerOffset(WMRuler *rPtr)
{
	if(!rPtr)
		return;
	CHECK_CLASS(rPtr, WC_Ruler);
	return rPtr->offset;
}


void
WMSetRulerAction(WMRuler *rPtr, WMAction *action, void *clientData)
{
	if(!rPtr)
		return;

    CHECK_CLASS(rPtr, WC_Ruler);
    rPtr->action = action;
    rPtr->clientData = clientData;
}

void
WMSetRulerMoveAction(WMRuler *rPtr, WMAction *moveaction, void *clientData)
{
	if(!rPtr)
		return;

    CHECK_CLASS(rPtr, WC_Ruler);
    rPtr->moveaction = moveaction;
    rPtr->clientData = clientData;
}

