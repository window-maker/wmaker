

#ifndef _WINGS_H_
#define _WINGS_H_

#include <wraster.h>
#include <WUtil.h>
#include <X11/Xlib.h>

#define WINGS_H_VERSION  20000521


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#if 0
}
#endif


typedef unsigned long WMPixel;


typedef struct {
    unsigned int width;
    unsigned int height;
} WMSize;

typedef struct {
    int x;
    int y;
} WMPoint;

typedef struct {
    WMPoint pos;
    WMSize size;
} WMRect;


/* WMRange was moved in WUtil.h */

#define ClientMessageMask	(1L<<30)


/* window stacking level */
enum {
  WMNormalWindowLevel   = 0,
  WMFloatingWindowLevel  = 3,
  WMDockWindowLevel   = 5,
  WMSubmenuWindowLevel  = 10,
  WMMainMenuWindowLevel  = 20
};

/* window attributes */
enum {
  WMBorderlessWindowMask = 0,
  WMTitledWindowMask = 1,
  WMClosableWindowMask = 2,
  WMMiniaturizableWindowMask = 4,
  WMResizableWindowMask = 8
};


/* button types */
typedef enum {
    /* 0 is reserved for internal use */
	WBTMomentaryPush = 1,
	WBTPushOnPushOff = 2,
	WBTToggle = 3,
	WBTSwitch = 4,
	WBTRadio = 5,
	WBTMomentaryChange = 6,
	WBTOnOff = 7,
	WBTMomentaryLight = 8
} WMButtonType;

/* button behaviour masks */
enum {
    WBBSpringLoadedMask = 	(1 << 0),
	WBBPushInMask = 	(1 << 1),
	WBBPushChangeMask = 	(1 << 2),
	WBBPushLightMask = 	(1 << 3),
	WBBStateLightMask = 	(1 << 5),
	WBBStateChangeMask = 	(1 << 6),
	WBBStatePushMask = 	(1 << 7)
};


/* frame title positions */
typedef enum {
    WTPNoTitle,
	WTPAboveTop,
	WTPAtTop,
	WTPBelowTop,
	WTPAboveBottom,
	WTPAtBottom,
	WTPBelowBottom
} WMTitlePosition;


/* relief types */
typedef enum {
    WRFlat,
	WRSimple,
	WRRaised,
	WRSunken,
	WRGroove,
	WRRidge,
	WRPushed
} WMReliefType;


/* alignment types */
typedef enum {
    WALeft,
	WACenter,
	WARight,
	WAJustified		       /* not valid for textfields */
} WMAlignment;


/* image position */
typedef enum {
    WIPNoImage,
    WIPImageOnly,
    WIPLeft,
    WIPRight,
    WIPBelow,
    WIPAbove,
    WIPOverlaps
} WMImagePosition;


/* scroller arrow position */
typedef enum {
    WSAMaxEnd,
	WSAMinEnd,
	WSANone
} WMScrollArrowPosition;

/* scroller parts */
typedef enum {
    WSNoPart,
	WSDecrementPage,
	WSIncrementPage,
	WSDecrementLine,
	WSIncrementLine,
	WSDecrementWheel,
	WSIncrementWheel,
	WSKnob,
	WSKnobSlot
} WMScrollerPart;

/* usable scroller parts */
typedef enum {
    WSUNoParts,
	WSUOnlyArrows,
	WSUAllParts
} WMUsableScrollerParts;

/* matrix types */
typedef enum {
    WMRadioMode,
	WMHighlightMode,
	WMListMode,
	WMTrackMode
} WMMatrixTypes;


typedef enum {
    WTTopTabsBevelBorder,
	WTNoTabsBevelBorder,
	WTNoTabsLineBorder,
	WTNoTabsNoBorder
} WMTabViewType;


/* text movement types */
enum {
    WMIllegalTextMovement,
	WMReturnTextMovement,
	WMEscapeTextMovement,
	WMTabTextMovement,
	WMBacktabTextMovement,
	WMLeftTextMovement,
	WMRightTextMovement,
	WMUpTextMovement,
	WMDownTextMovement
};

/* text field special events */
enum {
    WMInsertTextEvent,
    WMDeleteTextEvent
};


enum {
    WLNotFound = -1       /* element was not found in WMList */
};


/* drag operations */
typedef enum {
    WDOperationNone,
    WDOperationCopy,
    WDOperationMove,
    WDOperationLink,
    WDOperationAsk,
    WDOperationPrivate
} WMDragOperationType;


typedef enum {
	WMGrayModeColorPanel = 1,
 	WMRGBModeColorPanel = 2,
	WMCMYKModeColorPanel = 3,
	WMHSBModeColorPanel = 4,
	WMCustomPaletteModeColorPanel = 5,
	WMColorListModeColorPanel = 6,
	WMWheelModeColorPanel = 7
} WMColorPanelMode;



/* system images */
#define WSIReturnArrow			1
#define WSIHighlightedReturnArrow	2
#define WSIScrollerDimple		3
#define WSIArrowLeft			4
#define WSIHighlightedArrowLeft	        5
#define WSIArrowRight			6
#define WSIHighlightedArrowRight	7
#define WSIArrowUp			8
#define WSIHighlightedArrowUp		9
#define WSIArrowDown			10
#define WSIHighlightedArrowDown		11
#define WSICheckMark			12

enum {
    WLDSSelected = (1 << 16),
	WLDSDisabled = (1 << 17),
	WLDSFocused = (1 << 18),
	WLDSIsBranch = (1 << 19)
};

/* alert panel return values */
enum {
    WAPRDefault = 0,
	WAPRAlternate = 1,
	WAPROther = -1,
	WAPRError = -2
};



/* types of input observers */
enum {
    WIReadMask = (1 << 0),
	WIWriteMask = (1 << 1),
	WIExceptMask = (1 << 2)
};



typedef int W_Class;

enum {
    WC_Window = 0,
    WC_Frame = 1,
    WC_Label = 2,
    WC_Button = 3,
    WC_TextField = 4,
    WC_Scroller	= 5,
    WC_ScrollView = 6,
    WC_List = 7,
    WC_Browser = 8,
    WC_PopUpButton = 9,
    WC_ColorWell = 10,
    WC_Slider = 11,
    WC_Matrix = 12,		       /* not ready */
    WC_SplitView = 13,
    WC_TabView = 14,
    WC_ProgressIndicator = 15,
    WC_MenuView = 16,
    WC_Ruler = 17,
    WC_Text = 18,
    WC_Box = 19
};

/* All widgets must start with the following structure
 * in that order. Used for typecasting to get some generic data */
typedef struct W_WidgetType {
    W_Class widgetClass;
    struct W_View *view;
    
} W_WidgetType;


#define WMWidgetClass(widget)  	(((W_WidgetType*)(widget))->widgetClass)
#define WMWidgetView(widget)   	(((W_WidgetType*)(widget))->view)


/* widgets */

typedef void WMWidget;

typedef struct W_Pixmap WMPixmap;
typedef struct W_Font	WMFont;
typedef struct W_Color	WMColor;

typedef struct W_Screen WMScreen;

typedef struct W_View WMView;

typedef struct W_Window WMWindow;
typedef struct W_Frame WMFrame;
typedef struct W_Button WMButton;
typedef struct W_Label WMLabel;
typedef struct W_TextField WMTextField;
typedef struct W_Scroller WMScroller;
typedef struct W_ScrollView WMScrollView;
typedef struct W_List WMList;
typedef struct W_Browser WMBrowser;
typedef struct W_PopUpButton WMPopUpButton;
typedef struct W_ProgressIndicator WMProgressIndicator;
typedef struct W_ColorWell WMColorWell;
typedef struct W_Slider WMSlider;
typedef struct W_Matrix WMMatrix;      /* not ready */
typedef struct W_SplitView WMSplitView;
typedef struct W_TabView WMTabView;
typedef struct W_Ruler WMRuler;
typedef struct W_Text WMText;
typedef struct W_Box WMBox;


/* not widgets */
typedef struct W_TabViewItem WMTabViewItem;
typedef struct W_MenuItem WMMenuItem;


typedef struct W_FilePanel WMFilePanel;
typedef WMFilePanel WMOpenPanel;
typedef WMFilePanel WMSavePanel;

typedef struct W_FontPanel WMFontPanel;

typedef struct W_ColorPanel WMColorPanel;


/* item for WMList */
typedef struct WMListItem {
    char *text;
    void *clientData;		       /* ptr for user clientdata. */

    unsigned int uflags:16;	       /* flags for the user */
    unsigned int selected:1;
    unsigned int disabled:1;
    unsigned int isBranch:1;
    unsigned int loaded:1;
} WMListItem;

/* struct for message panel */
typedef struct WMAlertPanel {
    WMWindow *win;		       /* window */
    WMButton *defBtn;		       /* default button */
    WMButton *altBtn;		       /* alternative button */
    WMButton *othBtn;		       /* other button */
    WMLabel *iLbl;		       /* icon label */
    WMLabel *tLbl;		       /* title label */
    WMLabel *mLbl;		       /* message label */
    WMFrame *line;		       /* separator */
    short result;		       /* button that was pushed */
    short done;

    KeyCode retKey;
    KeyCode escKey;
} WMAlertPanel;


typedef struct WMInputPanel {
    WMWindow *win;		       /* window */
    WMButton *defBtn;		       /* default button */
    WMButton *altBtn;		       /* alternative button */
    WMLabel *tLbl;		       /* title label */
    WMLabel *mLbl;		       /* message label */
    WMTextField *text;		       /* text field */
    short result;		       /* button that was pushed */
    short done;

    KeyCode retKey;
    KeyCode escKey;
} WMInputPanel;




/* WMRuler: */
typedef struct {
    WMArray  *tabs;             /* a growable array of tabstops */
    unsigned short left;        /* left margin marker */
    unsigned short right;       /* right margin marker */
    unsigned short first;       /* indentation marker for first line only */
    unsigned short body;        /* body indentation marker */
    unsigned short retainCount; 
} WMRulerMargins;
/* All indentation and tab markers are _relative_ to the left margin marker */


typedef void *WMHandlerID;

typedef void WMInputProc(int fd, int mask, void *clientData);

typedef void WMEventProc(XEvent *event, void *clientData);

typedef void WMEventHook(XEvent *event);

/* self is set to the widget from where the callback is being called and
 * clientData to the data set to with WMSetClientData() */
typedef void WMAction(WMWidget *self, void *clientData);

/* same as WMAction, but for stuff that arent widgets */
typedef void WMAction2(void *self, void *clientData);


typedef void WMCallback(void *data);

typedef void WMDropDataCallback(WMView *view, WMData *data);

/* delegate method like stuff */
typedef void WMListDrawProc(WMList *lPtr, int index, Drawable d, char *text,
			    int state, WMRect *rect);

/*
typedef void WMSplitViewResizeSubviewsProc(WMSplitView *sPtr,
					   unsigned int oldWidth,
					   unsigned int oldHeight);
*/

typedef void WMSplitViewConstrainProc(WMSplitView *sPtr, int dividerIndex,
				      int *minSize, int *maxSize);

typedef WMWidget *WMMatrixCreateCellProc(WMMatrix *mPtr);




typedef struct WMBrowserDelegate {
    void *data;

    void (*createRowsForColumn)(struct WMBrowserDelegate *self,
				WMBrowser *sender, int column, WMList *list);

    char* (*titleOfColumn)(struct WMBrowserDelegate *self, WMBrowser *sender,
			   int column);

    void (*didScroll)(struct WMBrowserDelegate *self, WMBrowser *sender);

    void (*willScroll)(struct WMBrowserDelegate *self, WMBrowser *sender);
} WMBrowserDelegate;


typedef struct WMTextFieldDelegate {
    void *data;

    void (*didBeginEditing)(struct WMTextFieldDelegate *self,
                            WMNotification *notif);

    void (*didChange)(struct WMTextFieldDelegate *self, 
                      WMNotification *notif);

    void (*didEndEditing)(struct WMTextFieldDelegate *self,
                          WMNotification *notif);

    Bool (*shouldBeginEditing)(struct WMTextFieldDelegate *self,
                               WMTextField *tPtr);

    Bool (*shouldEndEditing)(struct WMTextFieldDelegate *self,
                             WMTextField *tPtr);
} WMTextFieldDelegate;


typedef struct WMTextDelegate {
    void *data;

    Bool (*didDoubleClickOnPicture)(struct WMTextDelegate *self,
                            void *description);

} WMTextDelegate;



typedef struct WMTabViewDelegate {
    void *data;

    void (*didChangeNumberOfItems)(struct WMTabViewDelegate *self,
				   WMTabView *tabView);

    void (*didSelectItem)(struct WMTabViewDelegate *self, WMTabView *tabView,
                          WMTabViewItem *item);

    Bool (*shouldSelectItem)(struct WMTabViewDelegate *self, WMTabView *tabView, 
                             WMTabViewItem *item);

    void (*willSelectItem)(struct WMTabViewDelegate *self, WMTabView *tabView,
                           WMTabViewItem *item);
} WMTabViewDelegate;




typedef void WMSelectionCallback(WMView *view, Atom selection, Atom target,
				 Time timestamp, void *cdata, WMData *data);


typedef struct WMSelectionProcs {
    WMData* (*convertSelection)(WMView *view, Atom selection, Atom target,
				void *cdata, Atom *type);
    void (*selectionLost)(WMView *view, Atom selection, void *cdata);
    void (*selectionDone)(WMView *view, Atom selection, Atom target,
			  void *cdata);
} WMSelectionProcs;


typedef struct W_DraggingInfo WMDraggingInfo;


typedef struct W_DragSourceProcs {
    unsigned (*draggingSourceOperation)(WMView *self, Bool local);
    void (*beganDragImage)(WMView *self, WMPixmap *image, WMPoint point);
    void (*endedDragImage)(WMView *self, WMPixmap *image, WMPoint point,
			   Bool deposited);
    WMData* (*fetchDragData)(WMView *self, char *type);
/*    Bool (*ignoreModifierKeysWhileDragging)(WMView *view);*/
} WMDragSourceProcs;



typedef struct W_DragDestinationProcs {
    unsigned (*draggingEntered)(WMView *self, WMDraggingInfo *info);
    unsigned (*draggingUpdated)(WMView *self, WMDraggingInfo *info);
    void (*draggingExited)(WMView *self, WMDraggingInfo *info);
    Bool (*prepareForDragOperation)(WMView *self, WMDraggingInfo *info);
    Bool (*performDragOperation)(WMView *self, WMDraggingInfo *info);
    void (*concludeDragOperation)(WMView *self, WMDraggingInfo *info);
} WMDragDestinationProcs;



/* ...................................................................... */


WMRange wmkrange(int start, int count);

WMPoint wmkpoint(int x, int y);

WMSize wmksize(unsigned int width, unsigned int height);


/* ....................................................................... */



void WMInitializeApplication(char *applicationName, int *argc, char **argv);

void WMSetResourcePath(char *path);

/* don't free the returned string */
char *WMGetApplicationName();

/* Try to locate resource file. ext may be NULL */
char *WMPathForResourceOfType(char *resource, char *ext);

WMScreen *WMCreateScreenWithRContext(Display *display, int screen, 
				     RContext *context);

WMScreen *WMCreateScreen(Display *display, int screen);

WMScreen *WMCreateSimpleApplicationScreen(Display *display);

void WMScreenMainLoop(WMScreen *scr);


RContext *WMScreenRContext(WMScreen *scr);

Display *WMScreenDisplay(WMScreen *scr);

int WMScreenDepth(WMScreen *scr);



void WMSetApplicationIconImage(WMScreen *app, WMPixmap *icon);

WMPixmap *WMGetApplicationIconImage(WMScreen *app);

void WMSetApplicationIconWindow(WMScreen *scr, Window window);

void WMSetFocusToWidget(WMWidget *widget);

WMEventHook *WMHookEventHandler(WMEventHook *handler);

int WMHandleEvent(XEvent *event);

Bool WMScreenPending(WMScreen *scr);

void WMCreateEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

void WMDeleteEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

int WMIsDoubleClick(XEvent *event);

int WMIsTripleClick(XEvent *event);

void WMNextEvent(Display *dpy, XEvent *event);

void WMMaskEvent(Display *dpy, long mask, XEvent *event);

WMHandlerID WMAddTimerHandler(int milliseconds, WMCallback *callback, 
			      void *cdata);

void WMDeleteTimerWithClientData(void *cdata);

void WMDeleteTimerHandler(WMHandlerID handlerID);

WMHandlerID WMAddIdleHandler(WMCallback *callback, void *cdata);

void WMDeleteIdleHandler(WMHandlerID handlerID);

WMHandlerID WMAddInputHandler(int fd, int condition, WMInputProc *proc, 
			      void *clientData);

void WMDeleteInputHandler(WMHandlerID handlerID);



Bool WMCreateSelectionHandler(WMView *view, Atom selection, Time timestamp,
			      WMSelectionProcs *procs, void *cdata);

void WMDeleteSelectionHandler(WMView *view, Atom selection, Time timestamp);

Bool WMRequestSelection(WMView *view, Atom selection, Atom target, 
			Time timestamp, WMSelectionCallback *callback,
			void *cdata);


/* ....................................................................... */

void WMSetViewDragSourceProcs(WMView *view, WMDragSourceProcs *procs);

void WMDragImageFromView(WMView *view, WMPixmap *image, char *dataTypes[],
			 WMPoint atLocation, WMSize mouseOffset, XEvent *event,
			 Bool slideBack);

void WMRegisterViewForDraggedTypes(WMView *view, char *acceptedTypes[]);

void WMUnregisterViewDraggedTypes(WMView *view);

void WMSetViewDragDestinationProcs(WMView *view, WMDragDestinationProcs *procs);


WMPoint WMGetDraggingInfoImageLocation(WMDraggingInfo *info);

/* ....................................................................... */

WMFont *WMCreateFontSet(WMScreen *scrPtr, char *fontName);

WMFont *WMCreateNormalFont(WMScreen *scrPtr, char *fontName);

WMFont *WMCreateFont(WMScreen *scrPtr, char *fontName);

WMFont *WMRetainFont(WMFont *font);

void WMReleaseFont(WMFont *font);

unsigned int WMFontHeight(WMFont *font);

/*
WMFont *WMUserFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMUserFixedPitchFontOfSize(WMScreen *scrPtr, int size);
*/


void WMSetWidgetDefaultFont(WMScreen *scr, WMFont *font);

void WMSetWidgetDefaultBoldFont(WMScreen *scr, WMFont *font);

WMFont *WMSystemFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMBoldSystemFontOfSize(WMScreen *scrPtr, int size);

XFontSet WMGetFontFontSet(WMFont *font);

WMFont * WMNormalizeFont(WMScreen *scr, WMFont *font);

WMFont * WMStrengthenFont(WMScreen *scr, WMFont *font);

WMFont * WMUnstrengthenFont(WMScreen *scr, WMFont *font);

WMFont * WMEmphasizeFont(WMScreen *scr, WMFont *font);

WMFont * WMUnemphasizeFont(WMScreen *scr, WMFont *font);

WMFont * WMGetFontOfSize(WMScreen *scr, WMFont *font, int size);

/* ....................................................................... */

WMPixmap *WMRetainPixmap(WMPixmap *pixmap);

void WMReleasePixmap(WMPixmap *pixmap);

WMPixmap *WMCreatePixmap(WMScreen *scrPtr, int width, int height, int depth,
			 Bool masked);

WMPixmap *WMCreatePixmapFromXPixmaps(WMScreen *scrPtr, Pixmap pixmap, 
				     Pixmap mask, int width, int height,
				     int depth);

WMPixmap *WMCreatePixmapFromRImage(WMScreen *scrPtr, RImage *image, 
				   int threshold);

WMPixmap *WMCreatePixmapFromXPMData(WMScreen *scrPtr, char **data);

WMSize WMGetPixmapSize(WMPixmap *pixmap);

WMPixmap *WMCreatePixmapFromFile(WMScreen *scrPtr, char *fileName);

WMPixmap *WMCreateBlendedPixmapFromFile(WMScreen *scrPtr, char *fileName,
					RColor *color);

void WMDrawPixmap(WMPixmap *pixmap, Drawable d, int x, int y);

Pixmap WMGetPixmapXID(WMPixmap *pixmap);

Pixmap WMGetPixmapMaskXID(WMPixmap *pixmap);

WMPixmap *WMGetSystemPixmap(WMScreen *scr, int image);

/* ....................................................................... */


WMColor *WMDarkGrayColor(WMScreen *scr);

WMColor *WMGrayColor(WMScreen *scr);

WMColor *WMBlackColor(WMScreen *scr);

WMColor *WMWhiteColor(WMScreen *scr);

void WMSetColorInGC(WMColor *color, GC gc);

GC WMColorGC(WMColor *color);

WMPixel WMColorPixel(WMColor *color);

void WMPaintColorSwatch(WMColor *color, Drawable d, int x, int y, 
			unsigned int width, unsigned int height);

void WMReleaseColor(WMColor *color);

WMColor *WMRetainColor(WMColor *color);

WMColor *WMCreateRGBColor(WMScreen *scr, unsigned short red, 
			  unsigned short green, unsigned short blue,
			  Bool exact);

WMColor *WMCreateNamedColor(WMScreen *scr, char *name, Bool exact);

unsigned short WMRedComponentOfColor(WMColor *color);

unsigned short WMGreenComponentOfColor(WMColor *color);

unsigned short WMBlueComponentOfColor(WMColor *color);

char *WMGetColorRGBDescription(WMColor *color);

/* ....................................................................... */


void WMDrawString(WMScreen *scr, Drawable d, GC gc, WMFont *font, int x, 
		  int y, char *text, int length);

void WMDrawImageString(WMScreen *scr, Drawable d, GC gc, WMFont *font, int x, 
		       int y, char *text, int length);

int WMWidthOfString(WMFont *font, char *text, int length);



/* ....................................................................... */

WMScreen *WMWidgetScreen(WMWidget *w);

unsigned int WMScreenWidth(WMScreen *scr);

unsigned int WMScreenHeight(WMScreen *scr);

void WMUnmapWidget(WMWidget *w);

void WMMapWidget(WMWidget *w);

Bool WMWidgetIsMapped(WMWidget *w);

void WMRaiseWidget(WMWidget *w);

void WMLowerWidget(WMWidget *w);

void WMMoveWidget(WMWidget *w, int x, int y);

void WMResizeWidget(WMWidget *w, unsigned int width, unsigned int height);

void WMSetWidgetBackgroundColor(WMWidget *w, WMColor *color);

void WMMapSubwidgets(WMWidget *w);

void WMUnmapSubwidgets(WMWidget *w);

void WMRealizeWidget(WMWidget *w);

void WMDestroyWidget(WMWidget *widget);

void WMHangData(WMWidget *widget, void *data);

void *WMGetHangedData(WMWidget *widget);

unsigned int WMWidgetWidth(WMWidget *w);

unsigned int WMWidgetHeight(WMWidget *w);

Window WMWidgetXID(WMWidget *w);

Window WMViewXID(WMView *view);

void WMRedisplayWidget(WMWidget *w);

void WMSetViewNotifySizeChanges(WMView *view, Bool flag);

WMSize WMGetViewSize(WMView *view);

WMPoint WMGetViewPosition(WMView *view);

WMPoint WMGetViewScreenPosition(WMView *view);

WMWidget *WMWidgetOfView(WMView *view);

/* notifications */
extern char *WMViewSizeDidChangeNotification;

extern char *WMViewRealizedNotification;

extern char *WMFontPanelDidChangeNotification;


/* ....................................................................... */

void WMSetBalloonTextForView(char *text, WMView *view);

void WMSetBalloonTextAlignment(WMScreen *scr, WMAlignment alignment);

void WMSetBalloonFont(WMScreen *scr, WMFont *font);

void WMSetBalloonTextColor(WMScreen *scr, WMColor *color);

void WMSetBalloonDelay(WMScreen *scr, int delay);

void WMSetBalloonEnabled(WMScreen *scr, Bool flag);


/* ....................................................................... */

WMWindow *WMCreateWindow(WMScreen *screen, char *name);

WMWindow *WMCreateWindowWithStyle(WMScreen *screen, char *name, int style);

WMWindow *WMCreatePanelWithStyleForWindow(WMWindow *owner, char *name, 
					  int style);

WMWindow *WMCreatePanelForWindow(WMWindow *owner, char *name);

void WMChangePanelOwner(WMWindow *win, WMWindow *newOwner);

void WMSetWindowTitle(WMWindow *wPtr, char *title);

void WMSetWindowMiniwindowTitle(WMWindow *win, char *title);

void WMSetWindowMiniwindowImage(WMWindow *win, WMPixmap *pixmap);

void WMSetWindowCloseAction(WMWindow *win, WMAction *action, void *clientData);

void WMSetWindowInitialPosition(WMWindow *win, int x, int y);

void WMSetWindowUserPosition(WMWindow *win, int x, int y);

void WMSetWindowAspectRatio(WMWindow *win, int minX, int minY,
			    int maxX, int maxY);

void WMSetWindowMaxSize(WMWindow *win, unsigned width, unsigned height);

void WMSetWindowMinSize(WMWindow *win, unsigned width, unsigned height);

void WMSetWindowBaseSize(WMWindow *win, unsigned width, unsigned height);

void WMSetWindowResizeIncrements(WMWindow *win, unsigned wIncr, unsigned hIncr);

void WMSetWindowLevel(WMWindow *win, int level);

void WMSetWindowDocumentEdited(WMWindow *win, Bool flag);

void WMCloseWindow(WMWindow *win);

/* ....................................................................... */

void WMSetButtonAction(WMButton *bPtr, WMAction *action, void *clientData);

#define WMCreateCommandButton(parent) \
	WMCreateCustomButton((parent), WBBSpringLoadedMask\
					|WBBPushInMask\
					|WBBPushLightMask\
					|WBBPushChangeMask)

#define WMCreateRadioButton(parent) \
	WMCreateButton((parent), WBTRadio)

#define WMCreateSwitchButton(parent) \
	WMCreateButton((parent), WBTSwitch)

WMButton *WMCreateButton(WMWidget *parent, WMButtonType type);

WMButton *WMCreateCustomButton(WMWidget *parent, int behaviourMask);

void WMSetButtonImageDefault(WMButton *bPtr);

void WMSetButtonImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonAltImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonImagePosition(WMButton *bPtr, WMImagePosition position);

void WMSetButtonFont(WMButton *bPtr, WMFont *font);

void WMSetButtonTextAlignment(WMButton *bPtr, WMAlignment alignment);

void WMSetButtonText(WMButton *bPtr, char *text);

void WMSetButtonAltText(WMButton *bPtr, char *text);

void WMSetButtonTextColor(WMButton *bPtr, WMColor *color);

void WMSetButtonAltTextColor(WMButton *bPtr, WMColor *color);

void WMSetButtonDisabledTextColor(WMButton *bPtr, WMColor *color);

void WMSetButtonSelected(WMButton *bPtr, int isSelected);

int WMGetButtonSelected(WMButton *bPtr);

void WMSetButtonBordered(WMButton *bPtr, int isBordered);

void WMSetButtonEnabled(WMButton *bPtr, Bool flag);

void WMSetButtonImageDimsWhenDisabled(WMButton *bPtr, Bool flag);

void WMSetButtonTag(WMButton *bPtr, int tag);

void WMGroupButtons(WMButton *bPtr, WMButton *newMember);

void WMPerformButtonClick(WMButton *bPtr);

void WMSetButtonContinuous(WMButton *bPtr, Bool flag);

void WMSetButtonPeriodicDelay(WMButton *bPtr, float delay, float interval);

/* ....................................................................... */

WMLabel *WMCreateLabel(WMWidget *parent);

void WMSetLabelWraps(WMLabel *lPtr, Bool flag);

void WMSetLabelImage(WMLabel *lPtr, WMPixmap *image);

WMPixmap *WMGetLabelImage(WMLabel *lPtr);

void WMSetLabelImagePosition(WMLabel *lPtr, WMImagePosition position);
	
void WMSetLabelTextAlignment(WMLabel *lPtr, WMAlignment alignment);

void WMSetLabelRelief(WMLabel *lPtr, WMReliefType relief);
	
void WMSetLabelText(WMLabel *lPtr, char *text);

void WMSetLabelFont(WMLabel *lPtr, WMFont *font);

void WMSetLabelTextColor(WMLabel *lPtr, WMColor *color);

/* ....................................................................... */

WMFrame *WMCreateFrame(WMWidget *parent);

void WMSetFrameTitlePosition(WMFrame *fPtr, WMTitlePosition position);

void WMSetFrameRelief(WMFrame *fPtr, WMReliefType relief);

void WMSetFrameTitle(WMFrame *fPtr, char *title);

/* ....................................................................... */

WMTextField *WMCreateTextField(WMWidget *parent);

void WMInsertTextFieldText(WMTextField *tPtr, char *text, int position);

void WMDeleteTextFieldRange(WMTextField *tPtr, WMRange range);

/* you can free the returned string */
char *WMGetTextFieldText(WMTextField *tPtr);

void WMSetTextFieldText(WMTextField *tPtr, char *text);

void WMSetTextFieldAlignment(WMTextField *tPtr, WMAlignment alignment);

void WMSetTextFieldFont(WMTextField *tPtr, WMFont *font);

WMFont *WMGetTextFieldFont(WMTextField *tPtr);

void WMSetTextFieldBordered(WMTextField *tPtr, Bool bordered);

void WMSetTextFieldBeveled(WMTextField *tPtr, Bool flag);

Bool WMGetTextFieldEditable(WMTextField *tPtr);

void WMSetTextFieldEditable(WMTextField *tPtr, Bool flag);

void WMSetTextFieldSecure(WMTextField *tPtr, Bool flag);

void WMSelectTextFieldRange(WMTextField *tPtr, WMRange range);

void WMSetTextFieldCursorPosition(WMTextField *tPtr, unsigned int position);

void WMSetTextFieldNextTextField(WMTextField *tPtr, WMTextField *next);

void WMSetTextFieldPrevTextField(WMTextField *tPtr, WMTextField *prev);

void WMSetTextFieldDelegate(WMTextField *tPtr, WMTextFieldDelegate *delegate);


extern char *WMTextDidChangeNotification;
extern char *WMTextDidBeginEditingNotification;
extern char *WMTextDidEndEditingNotification;

/* ....................................................................... */

WMScroller *WMCreateScroller(WMWidget *parent);

void WMSetScrollerParameters(WMScroller *sPtr, float floatValue, 
			     float knobProportion);

float WMGetScrollerKnobProportion(WMScroller *sPtr);

float WMGetScrollerValue(WMScroller *sPtr);

WMScrollerPart WMGetScrollerHitPart(WMScroller *sPtr);

void WMSetScrollerAction(WMScroller *sPtr, WMAction *action, void *clientData);

void WMSetScrollerArrowsPosition(WMScroller *sPtr, 
				 WMScrollArrowPosition position);

extern char *WMScrollerDidScrollNotification;

/* ....................................................................... */

WMList *WMCreateList(WMWidget *parent);

void WMSetListAllowMultipleSelection(WMList *lPtr, Bool flag);

void WMSetListAllowEmptySelection(WMList *lPtr, Bool flag);

#define WMAddListItem(lPtr, text) WMInsertListItem((lPtr), -1, (text))

WMListItem *WMInsertListItem(WMList *lPtr, int row, char *text);

void WMSortListItems(WMList *lPtr);

void WMSortListItemsWithComparer(WMList *lPtr, WMCompareDataProc *func);

int WMFindRowOfListItemWithTitle(WMList *lPtr, char *title);

WMListItem *WMGetListItem(WMList *lPtr, int row);

WMArray *WMGetListItems(WMList *lPtr);

void WMRemoveListItem(WMList *lPtr, int row);

void WMSelectListItem(WMList *lPtr, int row);

void WMUnselectListItem(WMList *lPtr, int row);

/* This will select all items in range, and deselect all the others */
void WMSetListSelectionToRange(WMList *lPtr, WMRange range);

/* This will select all items in range, leaving the others as they are */
void WMSelectListItemsInRange(WMList *lPtr, WMRange range);

void WMSelectAllListItems(WMList *lPtr);

void WMUnselectAllListItems(WMList *lPtr);

void WMSetListUserDrawProc(WMList *lPtr, WMListDrawProc *proc);

void WMSetListUserDrawItemHeight(WMList *lPtr, unsigned short height);

int WMGetListItemHeight(WMList *lPtr);

/* don't free the returned data */
WMArray* WMGetListSelectedItems(WMList *lPtr);

/*
 * For the following 2 functions, in case WMList allows multiple selection,
 * the first item in the list of selected items, respective its row number,
 * will be returned.
 */

/* don't free the returned data */
WMListItem* WMGetListSelectedItem(WMList *lPtr);

int WMGetListSelectedItemRow(WMList *lPtr);

void WMSetListAction(WMList *lPtr, WMAction *action, void *clientData);

void WMSetListDoubleAction(WMList *lPtr, WMAction *action, void *clientData);

void WMClearList(WMList *lPtr);

int WMGetListNumberOfRows(WMList *lPtr);

void WMSetListPosition(WMList *lPtr, int row);

void WMSetListBottomPosition(WMList *lPtr, int row);

int WMGetListPosition(WMList *lPtr);

Bool WMListAllowsMultipleSelection(WMList *lPtr);

Bool WMListAllowsEmptySelection(WMList *lPtr);


extern char *WMListDidScrollNotification;
extern char *WMListSelectionDidChangeNotification;

/* ....................................................................... */

WMBrowser *WMCreateBrowser(WMWidget *parent);

void WMSetBrowserAllowMultipleSelection(WMBrowser *bPtr, Bool flag);

void WMSetBrowserAllowEmptySelection(WMBrowser *bPtr, Bool flag);

void WMSetBrowserPathSeparator(WMBrowser *bPtr, char *separator);

void WMSetBrowserTitled(WMBrowser *bPtr, Bool flag);

void WMLoadBrowserColumnZero(WMBrowser *bPtr);

int WMAddBrowserColumn(WMBrowser *bPtr);

void WMRemoveBrowserItem(WMBrowser *bPtr, int column, int row);

void WMSetBrowserMaxVisibleColumns(WMBrowser *bPtr, int columns);

void WMSetBrowserColumnTitle(WMBrowser *bPtr, int column, char *title);

WMListItem *WMInsertBrowserItem(WMBrowser *bPtr, int column, int row, char *text, Bool isBranch);

void WMSortBrowserColumn(WMBrowser *bPtr, int column);

void WMSortBrowserColumnWithComparer(WMBrowser *bPtr, int column,
                                     WMCompareDataProc *func);

/* Don't free the returned string. */
char* WMSetBrowserPath(WMBrowser *bPtr, char *path);

/* free the returned string */
char *WMGetBrowserPath(WMBrowser *bPtr);

/* free the returned string */
char *WMGetBrowserPathToColumn(WMBrowser *bPtr, int column);

/* free the returned array */
WMArray* WMGetBrowserPaths(WMBrowser *bPtr);

void WMSetBrowserAction(WMBrowser *bPtr, WMAction *action, void *clientData);

void WMSetBrowserDoubleAction(WMBrowser *bPtr, WMAction *action, 
			      void *clientData);

WMListItem *WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column);

int WMGetBrowserFirstVisibleColumn(WMBrowser *bPtr);

int WMGetBrowserSelectedColumn(WMBrowser *bPtr);

int WMGetBrowserSelectedRowInColumn(WMBrowser *bPtr, int column);

int WMGetBrowserNumberOfColumns(WMBrowser *bPtr);

int WMGetBrowserMaxVisibleColumns(WMBrowser *bPtr);

WMList *WMGetBrowserListInColumn(WMBrowser *bPtr, int column);

void WMSetBrowserDelegate(WMBrowser *bPtr, WMBrowserDelegate *delegate);

Bool WMBrowserAllowsMultipleSelection(WMBrowser *bPtr);

Bool WMBrowserAllowsEmptySelection(WMBrowser *bPtr);

void WMSetBrowserHasScroller(WMBrowser *bPtr, int hasScroller);

/* ....................................................................... */


Bool WMMenuItemIsSeparator(WMMenuItem *item);

WMMenuItem *WMCreateMenuItem(void);

void WMDestroyMenuItem(WMMenuItem *item);

Bool WMGetMenuItemEnabled(WMMenuItem *item);

void WMSetMenuItemEnabled(WMMenuItem *item, Bool flag);

char *WMGetMenuItemShortcut(WMMenuItem *item);

unsigned WMGetMenuItemShortcutModifierMask(WMMenuItem *item);

void WMSetMenuItemShortcut(WMMenuItem *item, char *shortcut);

void WMSetMenuItemShortcutModifierMask(WMMenuItem *item, unsigned mask);

void *WMGetMenuItemRepresentedObject(WMMenuItem *item);

void WMSetMenuItemRepresentedObject(WMMenuItem *item, void *object);

void WMSetMenuItemAction(WMMenuItem *item, WMAction *action, void *data);

WMAction *WMGetMenuItemAction(WMMenuItem *item);

void *WMGetMenuItemData(WMMenuItem *item);

void WMSetMenuItemTitle(WMMenuItem *item, char *title);

char *WMGetMenuItemTitle(WMMenuItem *item);

void WMSetMenuItemState(WMMenuItem *item, int state);

int WMGetMenuItemState(WMMenuItem *item);

void WMSetMenuItemPixmap(WMMenuItem *item, WMPixmap *pixmap);

WMPixmap *WMGetMenuItemPixmap(WMMenuItem *item);

void WMSetMenuItemOnStatePixmap(WMMenuItem *item, WMPixmap *pixmap);

WMPixmap *WMGetMenuItemOnStatePixmap(WMMenuItem *item);

void WMSetMenuItemOffStatePixmap(WMMenuItem *item, WMPixmap *pixmap);

WMPixmap *WMGetMenuItemOffStatePixmap(WMMenuItem *item);

void WMSetMenuItemMixedStatePixmap(WMMenuItem *item, WMPixmap *pixmap);

WMPixmap *WMGetMenuItemMixedStatePixmap(WMMenuItem *item);

/*void WMSetMenuItemSubmenu(WMMenuItem *item, WMMenu *submenu);
 

WMMenu *WMGetMenuItemSubmenu(WMMenuItem *item);

Bool WMGetMenuItemHasSubmenu(WMMenuItem *item);
 */

/* ....................................................................... */

WMPopUpButton *WMCreatePopUpButton(WMWidget *parent);

void WMSetPopUpButtonAction(WMPopUpButton *sPtr, WMAction *action, 
			    void *clientData);

void WMSetPopUpButtonPullsDown(WMPopUpButton *bPtr, Bool flag);

WMMenuItem *WMAddPopUpButtonItem(WMPopUpButton *bPtr, char *title);

WMMenuItem *WMInsertPopUpButtonItem(WMPopUpButton *bPtr, int index, 
				    char *title);

void WMRemovePopUpButtonItem(WMPopUpButton *bPtr, int index);

void WMSetPopUpButtonItemEnabled(WMPopUpButton *bPtr, int index, Bool flag);

Bool WMGetPopUpButtonItemEnabled(WMPopUpButton *bPtr, int index);

void WMSetPopUpButtonSelectedItem(WMPopUpButton *bPtr, int index);

int WMGetPopUpButtonSelectedItem(WMPopUpButton *bPtr);

void WMSetPopUpButtonText(WMPopUpButton *bPtr, char *text);

/* don't free the returned data */
char *WMGetPopUpButtonItem(WMPopUpButton *bPtr, int index);

WMMenuItem *WMGetPopUpButtonMenuItem(WMPopUpButton *bPtr, int index);


int WMGetPopUpButtonNumberOfItems(WMPopUpButton *bPtr);

void WMSetPopUpButtonEnabled(WMPopUpButton *bPtr, Bool flag);

Bool WMGetPopUpButtonEnabled(WMPopUpButton *bPtr);

/* ....................................................................... */

WMProgressIndicator *WMCreateProgressIndicator(WMWidget *parent);

void WMSetProgressIndicatorMinValue(WMProgressIndicator *progressindicator, int value);

void WMSetProgressIndicatorMaxValue(WMProgressIndicator *progressindicator, int value);

void WMSetProgressIndicatorValue(WMProgressIndicator *progressindicator, int value);

int WMGetProgressIndicatorMinValue(WMProgressIndicator *progressindicator);

int WMGetProgressIndicatorMaxValue(WMProgressIndicator *progressindicator);

int WMGetProgressIndicatorValue(WMProgressIndicator *progressindicator);


/* ....................................................................... */

WMColorPanel *WMGetColorPanel(WMScreen *scrPtr);

void WMFreeColorPanel(WMColorPanel *panel);

void WMShowColorPanel(WMColorPanel *panel);

void WMCloseColorPanel(WMColorPanel *panel);

void WMSetColorPanelColor(WMColorPanel *panel, WMColor *color);

WMColor *WMGetColorPanelColor(WMColorPanel *panel);

void WMSetColorPanelPickerMode(WMColorPanel *panel, WMColorPanelMode mode);

void WMSetColorPanelAction(WMColorPanel *panel, WMAction2 *action, void *data);

extern char *WMColorPanelColorChangedNotification;

/* ....................................................................... */

WMColorWell *WMCreateColorWell(WMWidget *parent);

void WMSetColorWellColor(WMColorWell *cPtr, WMColor *color);

WMColor *WMGetColorWellColor(WMColorWell *cPtr);

void WSetColorWellBordered(WMColorWell *cPtr, Bool flag);


extern char *WMColorWellDidChangeNotification;


/* ...................................................................... */

WMScrollView *WMCreateScrollView(WMWidget *parent);

void WMResizeScrollViewContent(WMScrollView *sPtr, unsigned int width,
			       unsigned int height);

void WMSetScrollViewHasHorizontalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewHasVerticalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

void WMSetScrollViewRelief(WMScrollView *sPtr, WMReliefType type);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

WMRect WMGetScrollViewVisibleRect(WMScrollView *sPtr);

WMScroller *WMGetScrollViewHorizontalScroller(WMScrollView *sPtr);

WMScroller *WMGetScrollViewVerticalScroller(WMScrollView *sPtr);

void WMSetScrollViewLineScroll(WMScrollView *sPtr, int amount);

void WMSetScrollViewPageScroll(WMScrollView *sPtr, int amount);

/* ....................................................................... */

WMSlider *WMCreateSlider(WMWidget *parent);

int WMGetSliderMinValue(WMSlider *slider);

int WMGetSliderMaxValue(WMSlider *slider);

int WMGetSliderValue(WMSlider *slider);

void WMSetSliderMinValue(WMSlider *slider, int value);

void WMSetSliderMaxValue(WMSlider *slider, int value);

void WMSetSliderValue(WMSlider *slider, int value);

void WMSetSliderContinuous(WMSlider *slider, Bool flag);

void WMSetSliderAction(WMSlider *slider, WMAction *action, void *data);

void WMSetSliderKnobThickness(WMSlider *sPtr, int thickness);

void WMSetSliderImage(WMSlider *sPtr, WMPixmap *pixmap);

/* ....................................................................... */


WMSplitView *WMCreateSplitView(WMWidget *parent);
Bool WMGetSplitViewVertical(WMSplitView *sPtr);
void WMSetSplitViewVertical(WMSplitView *sPtr, Bool flag);

int WMGetSplitViewSubviewsCount(WMSplitView *sPtr); /* ??? remove ??? */

WMView *WMGetSplitViewSubviewAt(WMSplitView *sPtr, int index);

/* remove the first subview == view */
void WMRemoveSplitViewSubview(WMSplitView *sPtr, WMView *view);

void WMRemoveSplitViewSubviewAt(WMSplitView *sPtr, int index);


void WMAddSplitViewSubview(WMSplitView *sPtr, WMView *subview);

void WMAdjustSplitViewSubviews(WMSplitView *sPtr);

void WMSetSplitViewConstrainProc(WMSplitView *sPtr, 
				 WMSplitViewConstrainProc *proc);

/*
void WMSetSplitViewResizeSubviewsProc(WMSplitView *sPtr, 
				      WMSplitViewResizeSubviewsProc *proc);
*/

int WMGetSplitViewDividerThickness(WMSplitView *sPtr);

/* ...................................................................... */

WMRuler *WMCreateRuler (WMWidget *parent);

WMRulerMargins *WMGetRulerMargins(WMRuler *rPtr);

void WMSetRulerMargins(WMRuler *rPtr, WMRulerMargins margins);

Bool WMIsMarginEqualToMargin(WMRulerMargins *aMargin,
    WMRulerMargins *anotherMargin);

int WMGetGrabbedRulerMargin(WMRuler *rPtr);

int WMGetReleasedRulerMargin(WMRuler *rPtr);

int WMGetRulerOffset(WMRuler *rPtr);

void WMSetRulerOffset(WMRuler *rPtr, int pixels);

void WMSetRulerMoveAction(WMRuler *rPtr, WMAction *action, void *clientData);

void WMSetRulerReleaseAction(WMRuler *rPtr, WMAction *action, void *clientData);

/* ....................................................................... */


#define WMCreateText(parent) WMCreateTextForDocumentType \
    ((parent), (NULL), (NULL))

WMText *WMCreateTextForDocumentType(WMWidget *parent, 
	WMAction *parser, WMAction *writer);

void WMSetTextDelegate(WMText *tPtr, WMTextDelegate *delegate);

void WMFreezeText(WMText *tPtr); 

#define WMRefreshText(tPtr) WMThawText((tPtr)) 

void WMThawText(WMText *tPtr);

int WMScrollText(WMText *tPtr, int amount);

int WMPageText(WMText *tPtr, Bool direction);

void WMSetTextHasHorizontalScroller(WMText *tPtr, Bool shouldhave);

void WMSetTextHasVerticalScroller(WMText *tPtr, Bool shouldhave);

void WMSetTextHasRuler(WMText *tPtr, Bool shouldhave);

void WMShowTextRuler(WMText *tPtr, Bool show);

int WMGetTextRulerShown(WMText *tPtr);

void WMSetTextEditable(WMText *tPtr, Bool editable);

int WMGetTextEditable(WMText *tPtr);

void WMSetTextUsesMonoFont(WMText *tPtr, Bool mono);

int WMGetTextUsesMonoFont(WMText *tPtr);

void WMSetTextIndentNewLines(WMText *tPtr, Bool indent);

void WMSetTextIgnoresNewline(WMText *tPtr, Bool ignore);

int WMGetTextIgnoresNewline(WMText *tPtr);

void WMSetTextDefaultFont(WMText *tPtr, WMFont *font);

WMFont * WMGetTextDefaultFont(WMText *tPtr);

void WMSetTextDefaultColor(WMText *tPtr, WMColor *color);

WMColor * WMGetTextDefaultColor(WMText *tPtr);

void WMSetTextRelief(WMText *tPtr, WMReliefType relief);

void WMSetTextForegroundColor(WMText *tPtr, WMColor *color);

void WMSetTextBackgroundColor(WMText *tPtr, WMColor *color);

void WMSetTextBackgroundPixmap(WMText *tPtr, WMPixmap *pixmap);

void WMPrependTextStream(WMText *tPtr, char *text);

void WMAppendTextStream(WMText *tPtr, char *text);

/* free the text */
char * WMGetTextStream(WMText *tPtr);

/* free the text */
char * WMGetTextSelectedStream(WMText *tPtr);

/* destroy the array */
WMArray * WMGetTextObjects(WMText *tPtr);

/* destroy the array */
WMArray* WMGetTextSelectedObjects(WMText *tPtr);

void WMSetTextSelectionColor(WMText *tPtr, WMColor *color);

WMColor *WMGetTextSelectionColor(WMText *tPtr);

void WMSetTextSelectionFont(WMText *tPtr, WMFont *font);

WMFont *WMGetTextSelectionFont(WMText *tPtr);

void WMSetTextSelectionUnderlined(WMText *tPtr, int underlined);

int WMGetTextSelectionUnderlined(WMText *tPtr);

void WMSetTextAlignment(WMText *tPtr, WMAlignment alignment);

Bool WMFindInTextStream(WMText *tPtr, char *needle, Bool direction,
    Bool caseSensitive);

Bool WMReplaceTextSelection(WMText *tPtr, char *replacement);


/* parser related stuff... use only if implementing a new parser */

void *WMCreateTextBlockWithObject(WMText *tPtr, WMWidget *w, char *description,
    WMColor *color, unsigned short first, unsigned short extraInfo);
    
void *WMCreateTextBlockWithPixmap(WMText *tPtr, WMPixmap *p, char *description, 
	WMColor *color, unsigned short first, unsigned short extraInfo);

void *WMCreateTextBlockWithText(WMText *tPtr, char *text, WMFont *font,
    WMColor *color, unsigned short first, unsigned short length);
    
void WMSetTextBlockProperties(WMText *tPtr, void *vtb, unsigned int first,
    unsigned int kanji, unsigned int underlined, int script,
	WMRulerMargins *margins);
    
/* do NOT free the margins */
void WMGetTextBlockProperties(WMText *tPtr, void *vtb, unsigned int *first,
    unsigned int *kanji, unsigned int *underlined, int *script,
	WMRulerMargins *margins);

int WMGetTextInsertType(WMText *tPtr);

int WMGetTextBlocks(WMText *tPtr);

void WMSetCurrentTextBlock(WMText *tPtr, int current);

int WMGetCurrentTextBlock(WMText *tPtr); 

void WMPrependTextBlock(WMText *tPtr, void *vtb);

void WMAppendTextBlock(WMText *tPtr, void *vtb);

void *WMRemoveTextBlock(WMText *tPtr);

void WMDestroyTextBlock(WMText *tPtr, void *vtb);

/* ....................................................................... */


WMTabView *WMCreateTabView(WMWidget *parent);

void WMSetTabViewType(WMTabView *tPtr, WMTabViewType type);

void WMSetTabViewFont(WMTabView *tPtr, WMFont *font);

void WMAddItemInTabView(WMTabView *tPtr, WMTabViewItem *item);

void WMInsertItemInTabView(WMTabView *tPtr, int index, WMTabViewItem *item);

void WMRemoveTabViewItem(WMTabView *tPtr, WMTabViewItem *item);

WMTabViewItem *WMAddTabViewItemWithView(WMTabView *tPtr, WMView *view, 
					int identifier, char *label);

WMTabViewItem *WMTabViewItemAtPoint(WMTabView *tPtr, int x, int y);

void WMSelectFirstTabViewItem(WMTabView *tPtr);

void WMSelectLastTabViewItem(WMTabView *tPtr);

void WMSelectNextTabViewItem(WMTabView *tPtr);

void WMSelectPreviousTabViewItem(WMTabView *tPtr);

WMTabViewItem *WMGetSelectedTabViewItem(WMTabView *tPtr);

void WMSelectTabViewItem(WMTabView *tPtr, WMTabViewItem *item);

void WMSelectTabViewItemAtIndex(WMTabView *tPtr, int index);

void WMSetTabViewDelegate(WMTabView *tPtr, WMTabViewDelegate *delegate);


WMTabViewItem *WMCreateTabViewItemWithIdentifier(int identifier);

int WMGetTabViewItemIdentifier(WMTabViewItem *item);

void WMSetTabViewItemLabel(WMTabViewItem *item, char *label);

char *WMGetTabViewItemLabel(WMTabViewItem *item);

void WMSetTabViewItemView(WMTabViewItem *item, WMView *view);

WMView *WMGetTabViewItemView(WMTabViewItem *item);

void WMDestroyTabViewItem(WMTabViewItem *item);


/* ....................................................................... */

WMBox *WMCreateBox(WMWidget *parent);

void WMSetBoxBorderWidth(WMBox *box, unsigned width);

void WMAddBoxSubview(WMBox *bPtr, WMView *view, Bool expand, Bool fill,
		     int minSize, int maxSize, int space);

void WMAddBoxSubviewAtEnd(WMBox *bPtr, WMView *view, Bool expand, Bool fill,
			  int minSize, int maxSize, int space);

void WMSetBoxHorizontal(WMBox *box, Bool flag);

/* ....................................................................... */

int WMRunAlertPanel(WMScreen *app, WMWindow *owner, char *title, char *msg, 
		    char *defaultButton, char *alternateButton, 
		    char *otherButton);

/* you can free the returned string */
char *WMRunInputPanel(WMScreen *app, WMWindow *owner, char *title, char *msg,
		      char *defaultText, char *okButton, char *cancelButton);

WMAlertPanel *WMCreateAlertPanel(WMScreen *app, WMWindow *owner, char *title,
				 char *msg, char *defaultButton, 
				 char *alternateButton, char *otherButton);

WMInputPanel *WMCreateInputPanel(WMScreen *app, WMWindow *owner, char *title,
				 char *msg, char *defaultText, char *okButton, 
				 char *cancelButton);

void WMDestroyAlertPanel(WMAlertPanel *panel);

void WMDestroyInputPanel(WMInputPanel *panel);

/* ....................................................................... */

/* only 1 instance per WMScreen */
WMOpenPanel *WMGetOpenPanel(WMScreen *scrPtr);

WMSavePanel *WMGetSavePanel(WMScreen *scrPtr);

void WMSetFilePanelCanChooseDirectories(WMFilePanel *panel, Bool flag);

void WMSetFilePanelCanChooseFiles(WMFilePanel *panel, Bool flag);

void WMSetFilePanelAutoCompletion(WMFilePanel *panel, Bool flag);

void WMSetFilePanelDirectory(WMFilePanel *panel, char *path);

/* you can free the returned string */
char *WMGetFilePanelFileName(WMFilePanel *panel);

void WMFreeFilePanel(WMFilePanel *panel);

int WMRunModalFilePanelForDirectory(WMFilePanel *panel, WMWindow *owner,
                                    char *path, char *name, char **fileTypes);

void WMSetFilePanelAccessoryView(WMFilePanel *panel, WMView *view);

WMView *WMGetFilePanelAccessoryView(WMFilePanel *panel);


/* ...................................................................... */

/* only 1 instance per WMScreen */
WMFontPanel *WMGetFontPanel(WMScreen *scr);

void WMShowFontPanel(WMFontPanel *panel);

void WMHideFontPanel(WMFontPanel *panel);

void WMSetFontPanelFont(WMFontPanel *panel, WMFont *font);

/* you can free the returned string */
char *WMGetFontPanelFontName(WMFontPanel *panel);

WMFont *WMGetFontPanelFont(WMFontPanel *panel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

