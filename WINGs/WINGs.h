

#ifndef _WINGS_H_
#define _WINGS_H_

#include <wraster.h>
#include <X11/Xlib.h>

#define WINGS_H_VERSION  980930






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

typedef struct {
    int position;
    int count;
} WMRange;


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
	WMJustified		       /* not valid for textfields */
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
    WMMRadioMode,
	WMMHighlightMode,
	WMMListMode,
	WMMTrackMode
} WMMatrixTypes;


/* text movement types */
enum {
    WMIllegalTextMovement,
	WMReturnTextMovement,
	WMTabTextMovement,
	WMBacktabTextMovement,
	WMLeftTextMovement,
	WMRightTextMovement,
	WMUpTextMovement,
	WMDownTextMovement
};


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
    WC_SplitView = 13
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
typedef struct W_ColorWell WMColorWell;
typedef struct W_Slider WMSlider;
typedef struct W_Matrix WMMatrix;      /* not ready */
typedef struct W_SplitView WMSplitView;

/* not widgets */
typedef struct W_FilePanel WMFilePanel;
typedef WMFilePanel WMOpenPanel;
typedef WMFilePanel WMSavePanel;

typedef struct W_FontPanel WMFontPanel;


/* item for WMList */
typedef struct WMListItem {
    char *text;
    void *clientData;		       /* ptr for user clientdata. */
        
    struct WMListItem *nextPtr;
    
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
} WMInputPanel;







typedef void *WMHandlerID;

typedef void WMInputProc(int fd, int mask, void *clientData);

typedef void WMEventProc(XEvent *event, void *clientData);

typedef void WMEventHook(XEvent *event);

/* self is set to the widget from where the callback is being called and
 * clientData to the data set to with WMSetClientData() */
typedef void WMAction(WMWidget *self, void *clientData);


typedef void WMCallback(void *data);


/* delegate method like stuff */
typedef void WMFreeDataProc(void *data);

typedef void WMListDrawProc(WMList *lPtr, Drawable d, char *text, int state,
			    WMRect *rect);

/*
typedef void WMSplitViewResizeSubviewsProc(WMSplitView *sPtr,
					   unsigned int oldWidth,
					   unsigned int oldHeight);
*/

typedef void WMSplitViewConstrainProc(WMSplitView *sPtr, int dividerIndex,
				      int *minCoordinate, int *maxCoordinate);

typedef WMWidget *WMMatrixCreateCellProc(WMMatrix *mPtr);


typedef void WMBrowserFillColumnProc(WMBrowser *bPtr, int column);


/* ....................................................................... */

void WMInitializeApplication(char *applicationName, int *argc, char **argv);

void WMSetApplicationDataPath(char *path);

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

void WMSetFocusToWidget(WMWidget *widget);

WMEventHook *WMHookEventHandler(WMEventHook *handler);

int WMHandleEvent(XEvent *event);

Bool WMScreenPending(WMScreen *scr);

void WMCreateEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

void WMDeleteEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

int WMIsDoubleClick(XEvent *event);

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

/* ....................................................................... */
/*
void WMDragImageFromView(WMView *view, WMPixmap *image, WMPoint atLocation,
			 WMSize mouseOffset, XEvent *event, Bool slideBack);

*/

/* ....................................................................... */

WMFont *WMCreateFont(WMScreen *scrPtr, char *fontName);

WMFont *WMCreateFontInDefaultEncoding(WMScreen *scrPtr, char *fontName);

WMFont *WMRetainFont(WMFont *font);

void WMReleaseFont(WMFont *font);

unsigned int WMFontHeight(WMFont *font);

/*
WMFont *WMUserFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMUserFixedPitchFontOfSize(WMScreen *scrPtr, int size);
*/

WMFont *WMSystemFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMBoldSystemFontOfSize(WMScreen *scrPtr, int size);

XFontSet WMGetFontFontSet(WMFont *font);

/* ....................................................................... */

WMPixmap *WMRetainPixmap(WMPixmap *pixmap);

void WMReleasePixmap(WMPixmap *pixmap);

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

void WMUnmapWidget(WMWidget *w);

void WMMapWidget(WMWidget *w);

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

WMWidget *WMWidgetOfView(WMView *view);

/* notifications */
extern char *WMViewSizeDidChangeNotification;

extern char *WMViewRealizedNotification;

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

void WMSetButtonImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonAltImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonImagePosition(WMButton *bPtr, WMImagePosition position);

void WMSetButtonFont(WMButton *bPtr, WMFont *font);

void WMSetButtonTextAlignment(WMButton *bPtr, WMAlignment alignment);

void WMSetButtonText(WMButton *bPtr, char *text);

void WMSetButtonAltText(WMButton *bPtr, char *text);

void WMSetButtonSelected(WMButton *bPtr, int isSelected);

int WMGetButtonSelected(WMButton *bPtr);

void WMSetButtonBordered(WMButton *bPtr, int isBordered);

void WMSetButtonEnabled(WMButton *bPtr, Bool flag);

void WMSetButtonTag(WMButton *bPtr, int tag);

void WMGroupButtons(WMButton *bPtr, WMButton *newMember);

void WMPerformButtonClick(WMButton *bPtr);

void WMSetButtonContinuous(WMButton *bPtr, Bool flag);

void WMSetButtonPeriodicDelay(WMButton *bPtr, float delay, float interval);

/* ....................................................................... */

WMLabel *WMCreateLabel(WMWidget *parent);

void WMSetLabelWraps(WMLabel *lPtr, Bool flag);

void WMSetLabelImage(WMLabel *lPtr, WMPixmap *image);

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

void WMSetTextFieldBordered(WMTextField *tPtr, Bool bordered);

void WMSetTextFieldEnabled(WMTextField *tPtr, Bool flag);

void WMSetTextFieldSecure(WMTextField *tPtr, Bool flag);


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

/* ....................................................................... */

WMList *WMCreateList(WMWidget *parent);

#define WMAddListItem(lPtr, text) WMInsertListItem((lPtr), -1, (text))

WMListItem *WMInsertListItem(WMList *lPtr, int row, char *text);

WMListItem *WMAddSortedListItem(WMList *lPtr, char *text);

int WMFindRowOfListItemWithTitle(WMList *lPtr, char *title);

WMListItem *WMGetListItem(WMList *lPtr, int row);

void WMRemoveListItem(WMList *lPtr, int row);

void WMSelectListItem(WMList *lPtr, int row);

void WMSetListUserDrawProc(WMList *lPtr, WMListDrawProc *proc);

void WMSetListUserDrawItemHeight(WMList *lPtr, unsigned short height);

/* don't free the returned data */
WMListItem *WMGetListSelectedItem(WMList *lPtr);

int WMGetListSelectedItemRow(WMList *lPtr);

void WMSetListAction(WMList *lPtr, WMAction *action, void *clientData);

void WMSetListDoubleAction(WMList *lPtr, WMAction *action, void *clientData);

void WMClearList(WMList *lPtr);

int WMGetListNumberOfRows(WMList *lPtr);

void WMSetListPosition(WMList *lPtr, int row);

void WMSetListBottomPosition(WMList *lPtr, int row);

int WMGetListPosition(WMList *lPtr);

/* ....................................................................... */

WMBrowser *WMCreateBrowser(WMWidget *parent);

void WMSetBrowserPathSeparator(WMBrowser *bPtr, char *separator);

void WMSetBrowserTitled(WMBrowser *bPtr, Bool flag);

void WMLoadBrowserColumnZero(WMBrowser *bPtr);

int WMAddBrowserColumn(WMBrowser *bPtr);

void WMRemoveBrowserItem(WMBrowser *bPtr, int column, int row);

void WMSetBrowserMaxVisibleColumns(WMBrowser *bPtr, int columns);

void WMSetBrowserColumnTitle(WMBrowser *bPtr, int column, char *title);

WMListItem *WMAddSortedBrowserItem(WMBrowser *bPtr, int column, char *text, Bool isBranch);

WMListItem *WMInsertBrowserItem(WMBrowser *bPtr, int column, int row, char *text, Bool isBranch);

Bool WMSetBrowserPath(WMBrowser *bPtr, char *path);

/* you can free the returned string */
char *WMGetBrowserPath(WMBrowser *bPtr);
/* you can free the returned string */
char *WMGetBrowserPathToColumn(WMBrowser *bPtr, int column);

void WMSetBrowserFillColumnProc(WMBrowser *bPtr,WMBrowserFillColumnProc *proc);

void WMSetBrowserAction(WMBrowser *bPtr, WMAction *action, void *clientData);

WMListItem *WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column);

int WMGetBrowserFirstVisibleColumn(WMBrowser *bPtr);

int WMGetBrowserSelectedColumn(WMBrowser *bPtr);

int WMGetBrowserSelectedRowInColumn(WMBrowser *bPtr, int column);

int WMGetBrowserNumberOfColumns(WMBrowser *bPtr);

WMList *WMGetBrowserListInColumn(WMBrowser *bPtr, int column);

extern char *WMBrowserDidScrollNotification;

/* ....................................................................... */

WMPopUpButton *WMCreatePopUpButton(WMWidget *parent);

void WMSetPopUpButtonAction(WMPopUpButton *sPtr, WMAction *action, 
			    void *clientData);

void WMSetPopUpButtonPullsDown(WMPopUpButton *bPtr, Bool flag);

void WMAddPopUpButtonItem(WMPopUpButton *bPtr, char *title);

void WMInsertPopUpButtonItem(WMPopUpButton *bPtr, int index, char *title);

void WMRemovePopUpButtonItem(WMPopUpButton *bPtr, int index);

void WMSetPopUpButtonItemEnabled(WMPopUpButton *bPtr, int index, Bool flag);

void WMSetPopUpButtonSelectedItem(WMPopUpButton *bPtr, int index);

int WMGetPopUpButtonSelectedItem(WMPopUpButton *bPtr);

void WMSetPopUpButtonText(WMPopUpButton *bPtr, char *text);

/* don't free the returned data */
char *WMGetPopUpButtonItem(WMPopUpButton *bPtr, int index);

int WMGetPopUpButtonNumberOfItems(WMPopUpButton *bPtr);

/* ....................................................................... */

WMColorWell *WMCreateColorWell(WMWidget *parent);

void WMSetColorWellColor(WMColorWell *cPtr, WMColor *color);

WMColor *WMGetColorWellColor(WMColorWell *cPtr);

/* ...................................................................... */

WMScrollView *WMCreateScrollView(WMWidget *parent);

void WMResizeScrollViewContent(WMScrollView *sPtr, unsigned int width,
			       unsigned int height);

void WMSetScrollViewHasHorizontalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewHasVerticalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

void WMSetScrollViewRelief(WMScrollView *sPtr, WMReliefType type);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

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

/* ....................................................................... */

/* only supports 2 subviews */
WMSplitView *WMCreateSplitView(WMWidget *parent);

void WMAddSplitViewSubview(WMSplitView *sPtr, WMView *subview);

void WMAdjustSplitViewSubviews(WMSplitView *sPtr);

void WMSetSplitViewConstrainProc(WMSplitView *sPtr, 
				 WMSplitViewConstrainProc *proc);

/*
void WMSetSplitViewResizeSubviewsProc(WMSplitView *sPtr, 
				      WMSplitViewResizeSubviewsProc *proc);
*/

int WMGetSplitViewDividerThickness(WMSplitView *sPtr);


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

void WMSetFilePanelCanChooseDirectories(WMFilePanel *panel, int flag);

void WMSetFilePanelCanChooseFiles(WMFilePanel *panel, int flag);

void WMSetFilePanelDirectory(WMFilePanel *panel, char *path);

/* you can free the returned string */
char *WMGetFilePanelFileName(WMFilePanel *panel);

void WMFreeFilePanel(WMFilePanel *panel);

int WMRunModalOpenPanelForDirectory(WMFilePanel *panel, WMWindow *owner,
				    char *path, char *name, char **fileTypes);

int WMRunModalSavePanelForDirectory(WMFilePanel *panel, WMWindow *owner, 
				    char *path, char *name);

/* ...................................................................... */

/* only 1 instance per WMScreen */
WMFontPanel *WMGetFontPanel(WMScreen *scr);

void WMShowFontPanel(WMFontPanel *panel);

void WMHideFontPanel(WMFontPanel *panel);

void WMSetFontPanelFont(WMFontPanel *panel, WMFont *font);

/* you can free the returned string */
char *WMGetFontPanelFontName(WMFontPanel *panel);

WMFont *WMGetFontPanelFont(WMFontPanel *panel);

#endif

