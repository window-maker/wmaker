#ifndef _WINGSP_H_
#define _WINGSP_H_


#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include "WINGs.h"
#include "WUtil.h"

#if WINGS_H_VERSION < 990222
#error There_is_an_old_WINGs.h_file_somewhere_in_your_system._Please_remove_it.
#endif

#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DOUBLE_BUFFER



#define WC_UserWidget	128



#define SCROLLER_WIDTH	 20

/* internal messages */
#define WM_UPDATE_COLORWELL	130


#define WM_USER_MESSAGE	1024


#define SETUP_INTERNAL_MESSAGE(event, scrPtr) \
		event.xclient.type=ClientMessage;\
		event.xclient.display=scrPtr->display;\
		event.xclient.send_event=False;\
		event.xclient.serial=0;\
		event.xclient.format=32;\
		event.xclient.message_type=scrPtr->internalMessage;


typedef struct W_Font {
    struct W_Screen *screen;

    union {
	XFontSet set;
	XFontStruct *normal;
    } font;
    short height;
    short y;
    short refCount;
    char *name;
    unsigned int notFontSet:1;
} W_Font;


typedef struct W_Pixmap {
    struct W_Screen *screen;
    Pixmap pixmap;
    Pixmap mask;
    unsigned short width;
    unsigned short height;
    short depth;
    short refCount;
} W_Pixmap;


typedef struct W_Color {
    struct W_Screen *screen;

    XColor color;
    short refCount;
    GC gc;
    struct {
	unsigned int exact:1;
    } flags;
} W_Color;


typedef struct W_FocusInfo {
    struct W_View *toplevel;
    struct W_View *focused;	       /* view that has the focus in this toplevel */
    struct W_FocusInfo *next;
} W_FocusInfo;

typedef struct W_Screen {
    Display *display;
    int screen;
    int depth;

    Colormap colormap;
    
    Visual *visual;
    
    Time lastEventTime;
    
    Window rootWin;

    struct W_View *rootView;
    
    RContext *rcontext;

    /* application related */

    W_FocusInfo *focusInfo;

    struct W_Pixmap *applicationIcon;

    struct W_Window *windowList;       /* list of windows in the app */

    Window groupLeader;		       /* the leader of the application */
    				       /* also used for other things */

    struct W_SelectionHandlers *selectionHandlerList;

    struct {
	unsigned int hasAppIcon:1;
	unsigned int simpleApplication:1;
    } aflags;

    WMOpenPanel *sharedOpenPanel;
    WMSavePanel *sharedSavePanel;

    struct W_FontPanel *sharedFontPanel;

    struct W_ColorPanel *sharedColorPanel;

    Pixmap stipple;

    /* colors */
    W_Color *white;
    W_Color *black;
    W_Color *gray;
    W_Color *darkGray;

    GC stippleGC;
    
    GC copyGC;
    GC clipGC;
    
    GC monoGC;			       /* GC for 1bpp visuals */
    
    GC xorGC;

    GC ixorGC;			       /* IncludeInferiors XOR */

    GC textFieldGC;
    
    W_Font *normalFont;

    W_Font *boldFont;

    WMHashTable *fontCache;


    struct W_Balloon *balloon;

    
    struct W_Pixmap *checkButtonImageOn;
    struct W_Pixmap *checkButtonImageOff;
    
    struct W_Pixmap *radioButtonImageOn;
    struct W_Pixmap *radioButtonImageOff;

    struct W_Pixmap *buttonArrow;
    struct W_Pixmap *pushedButtonArrow;

    struct W_Pixmap *scrollerDimple;
    
    struct W_Pixmap *upArrow;
    struct W_Pixmap *downArrow;
    struct W_Pixmap *leftArrow;
    struct W_Pixmap *rightArrow;
    
    struct W_Pixmap *hiUpArrow;
    struct W_Pixmap *hiDownArrow;
    struct W_Pixmap *hiLeftArrow;
    struct W_Pixmap *hiRightArrow;

    struct W_Pixmap *pullDownIndicator;
    struct W_Pixmap *popUpIndicator;

    struct W_Pixmap *checkMark;

    struct W_Pixmap *homeIcon;
    struct W_Pixmap *altHomeIcon;

    struct W_Pixmap *magnifyIcon;
/*    struct W_Pixmap *altMagnifyIcon;*/
    struct W_Pixmap *wheelIcon;
    struct W_Pixmap *grayIcon;
    struct W_Pixmap *rgbIcon;
    struct W_Pixmap *cmykIcon;
    struct W_Pixmap *hsbIcon;
    struct W_Pixmap *customPaletteIcon;
    struct W_Pixmap *colorListIcon;
	
    struct W_Pixmap *defaultObjectIcon;

    Cursor defaultCursor;
    
    Cursor textCursor;

    Cursor invisibleCursor;

    Atom internalMessage;	       /* for ClientMessage */
    
    Atom attribsAtom;		       /* GNUstepWindowAttributes */
    
    Atom deleteWindowAtom;	       /* WM_DELETE_WINDOW */
    
    Atom protocolsAtom;		       /* _XA_WM_PROTOCOLS */
    
    Atom clipboardAtom;		       /* CLIPBOARD */


    /* stuff for detecting double-clicks */
    Time lastClickTime;		       /* time of last mousedown event */
    Window lastClickWindow;	       /* window of the last mousedown */

    struct W_View *modalView;
    unsigned modal:1;
    unsigned ignoreNextDoubleClick:1;
} W_Screen;



typedef struct W_View {
    struct W_Screen *screen;

    WMWidget *self;		       /* must point to the widget the
					* view belongs to */
    
    Window window;

    WMSize size;

    WMPoint pos;
    
    struct W_View *nextFocusChain;     /* next/prev in focus chain */
    struct W_View *prevFocusChain;
        
    struct W_View *parent;	       /* parent WMView */
    
    struct W_View *childrenList;       /* first in list of child windows */
    
    struct W_View *nextSister;	       /* next on parent's children list */
    
    struct W_EventHandler *handlerList;/* list of event handlers for this window */

    unsigned long attribFlags;
    XSetWindowAttributes attribs;
    
    void *hangedData;		       /* data holder for user program */
#if 0
    struct W_DragSourceProcs *dragSourceProcs;
    struct W_DragDestinationProcs *dragDestinationProcs;
    int helpContext;
#endif

    struct {
	unsigned int realized:1;
	unsigned int mapped:1;
	unsigned int parentDying:1;
	unsigned int dying:1;	       /* the view is being destroyed */
	unsigned int topLevel:1;       /* is a top level window */
	unsigned int root:1;	       /* is the root window */
	unsigned int mapWhenRealized:1;/* map the view when it's realized */
	unsigned int alreadyDead:1;    /* view was freed */

	unsigned int dontCompressMotion:1;   /* motion notify event compress */
	unsigned int notifySizeChanged:1;
	unsigned int dontCompressExpose:1;   /* will compress all expose
					      events into one */
	/* toplevel only */
	unsigned int worksWhenModal:1;
	unsigned int pendingRelease1:1;
	unsigned int pendingRelease2:1;
	unsigned int pendingRelease3:1;
	unsigned int pendingRelease4:1;
	unsigned int pendingRelease5:1;
    } flags;
    
    int refCount;
} W_View;


typedef struct W_EventHandler {
    unsigned long eventMask;
    
    WMEventProc *proc;
    
    void *clientData;
    
    struct W_EventHandler *nextHandler;
} W_EventHandler;



typedef struct W_ViewProcedureTable {
    void (*setBackgroundColor)(WMWidget*, WMColor *color);
    void (*resize)(WMWidget*, unsigned int, unsigned int);
    void (*move)(WMWidget*, int, int);
} W_ViewProcedureTable;



typedef struct _WINGsConfiguration {
    char *systemFont;
    char *boldSystemFont;
    unsigned doubleClickDelay;
} _WINGsConfiguration;

_WINGsConfiguration WINGsConfiguration;



#define CHECK_CLASS(widget, class) assert(W_CLASS(widget)==(class))


#define W_CLASS(widget)  	(((W_WidgetType*)(widget))->widgetClass)
#define W_VIEW(widget)   	(((W_WidgetType*)(widget))->view)

#define W_VIEW_REALIZED(view)	(view)->flags.realized
#define W_VIEW_MAPPED(view)	(view)->flags.mapped

#define W_PIXEL(c)		(c)->color.pixel

#define W_FONTID(f)		(f)->font->fid

#define W_DRAWABLE(scr)		(scr)->rcontext->drawable



W_View *W_GetViewForXWindow(Display *display, Window window);

W_View *W_CreateView(W_View *parent);

W_View *W_CreateTopView(W_Screen *screen);


W_View *W_CreateRootView(W_Screen *screen);

void W_DestroyView(W_View *view);

void W_RealizeView(W_View *view);

void W_ReparentView(W_View *view, W_View *newParent, int x, int y);

void W_MapView(W_View *view);

void W_MapSubviews(W_View *view);

void W_UnmapSubviews(W_View *view);

W_View *W_TopLevelOfView(W_View *view);

void W_UnmapView(W_View *view);

void W_MoveView(W_View *view, int x, int y);

void W_ResizeView(W_View *view, unsigned int width, unsigned int height);

void W_SetViewBackgroundColor(W_View *view, WMColor *color);

void W_DrawRelief(W_Screen *scr, Drawable d, int x, int y, unsigned int width,
		  unsigned int height, WMReliefType relief);


void W_CleanUpEvents(W_View *view);

void W_CallDestroyHandlers(W_View *view);

void W_PaintTextAndImage(W_View *view, int wrap, GC textGC, W_Font *font,
			 WMReliefType relief, char *text,
			 WMAlignment alignment, W_Pixmap *image, 
			 WMImagePosition position, GC backGC, int ofs);

void W_PaintText(W_View *view, Drawable d, WMFont *font,  int x, int y,
		 int width, WMAlignment alignment, GC gc,
		 int wrap, char *text, int length);

int W_GetTextHeight(WMFont *font, char *text, int width, int wrap);


int W_TextWidth(WMFont *font, char *text, int length);


void W_BroadcastMessage(W_View *targetParent, XEvent *event);

void W_DispatchMessage(W_View *target, XEvent *event);

Bool W_CheckInternalMessage(W_Screen *scr, XClientMessageEvent *cev, int event);

void W_SetFocusOfToplevel(W_View *toplevel, W_View *view);

W_View *W_FocusedViewOfToplevel(W_View *view);

void W_SetFocusOfTopLevel(W_View *toplevel, W_View *view);

void W_ReleaseView(WMView *view);

WMView *W_RetainView(WMView *view);

void W_InitApplication(WMScreen *scr);

void W_InitNotificationCenter(void);

W_Class W_RegisterUserWidget(W_ViewProcedureTable *procTable);

void W_RedisplayView(WMView *view);

Bool W_ApplicationInitialized(void);

char *W_GetTextSelection(WMScreen *scr, Atom selection);

void W_HandleSelectionEvent(XEvent *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WINGSP_H_ */

void W_FlushASAPNotificationQueue();

void W_FlushIdleNotificationQueue();

struct W_Balloon *W_CreateBalloon(WMScreen *scr);

void W_BalloonHandleEnterView(WMView *view);

void W_BalloonHandleLeaveView(WMView *view);
