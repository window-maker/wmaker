



#include "WINGsP.h"
#include <math.h> /* for : double rint (double) */



typedef struct W_Browser {
    W_Class widgetClass;
    W_View *view;

    char **titles;
    WMList **columns;

    short columnCount;
    short usedColumnCount;	       /* columns actually being used */
    short minColumnWidth;
 
    short maxVisibleColumns;
    short firstVisibleColumn;

    short titleHeight;
    
    short selectedColumn;

    WMSize columnSize;
    

    void *clientData;
    WMAction *action;
    void *doubleClientData;
    WMAction *doubleAction;

    WMBrowserDelegate *delegate;
    
    WMScroller *scroller;

    char *pathSeparator;
    
    struct {
	unsigned int isTitled:1;
	unsigned int allowMultipleSelection:1;
	unsigned int hasScroller:1;
	
	/* */
	unsigned int loaded:1;
	unsigned int loadingColumn:1;
    } flags;
} Browser;


#define COLUMN_SPACING 	4
#define TITLE_SPACING 2

#define DEFAULT_WIDTH                 305
#define DEFAULT_HEIGHT                200
#define DEFAULT_HAS_SCROLLER          True
#define DEFAULT_TITLE_HEIGHT          20
#define DEFAULT_IS_TITLED             True
#define DEFAULT_MAX_VISIBLE_COLUMNS   2
#define DEFAULT_SEPARATOR             "/"

#define MIN_VISIBLE_COLUMNS           1
#define MAX_VISIBLE_COLUMNS           32


#define COLUMN_IS_VISIBLE(b, c)	((c) >= (b)->firstVisibleColumn \
				&& (c) < (b)->firstVisibleColumn + (b)->maxVisibleColumns)


static void handleEvents(XEvent *event, void *data);
static void destroyBrowser(WMBrowser *bPtr);

static void setupScroller(WMBrowser *bPtr);

static void scrollToColumn(WMBrowser *bPtr, int column, Bool updateScroller);

static void paintItem(WMList *lPtr, int index, Drawable d, char *text, 
		      int state, WMRect *rect);

static void loadColumn(WMBrowser *bPtr, int column);

static void removeColumn(WMBrowser *bPtr, int column);

static char*
createTruncatedString(WMFont *font, char *text, int *textLen, int width);

static void resizeBrowser(WMWidget*, unsigned int, unsigned int);

W_ViewProcedureTable _BrowserViewProcedures = {
    NULL,
	resizeBrowser,
	NULL
};



WMBrowser*
WMCreateBrowser(WMWidget *parent)
{
    WMBrowser *bPtr;
    int i;

    wassertrv(parent, NULL);

    bPtr = wmalloc(sizeof(WMBrowser));
    memset(bPtr, 0, sizeof(WMBrowser));

    bPtr->widgetClass = WC_Browser;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;

    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, bPtr);
	
    /* default configuration */
    bPtr->flags.hasScroller = DEFAULT_HAS_SCROLLER;

    bPtr->titleHeight = DEFAULT_TITLE_HEIGHT;
    bPtr->flags.isTitled = DEFAULT_IS_TITLED;
    bPtr->maxVisibleColumns = DEFAULT_MAX_VISIBLE_COLUMNS;

    resizeBrowser(bPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    bPtr->pathSeparator = wstrdup(DEFAULT_SEPARATOR);

    if (bPtr->flags.hasScroller)
	setupScroller(bPtr);

    for (i=0; i<bPtr->maxVisibleColumns; i++) {
	WMAddBrowserColumn(bPtr);
    }
    bPtr->usedColumnCount = 0;
    
    bPtr->selectedColumn = -1;

    return bPtr;
}


int
WMGetBrowserMaxVisibleColumns(WMBrowser *bPtr)
{
    return bPtr->maxVisibleColumns;
}


void
WMSetBrowserMaxVisibleColumns(WMBrowser *bPtr, int columns)
{
    int curMaxVisibleColumns;
    int newFirstVisibleColumn = 0;

    assert ((int) bPtr);
    
    columns = (columns < MIN_VISIBLE_COLUMNS) ? MIN_VISIBLE_COLUMNS : columns;
    columns = (columns > MAX_VISIBLE_COLUMNS) ? MAX_VISIBLE_COLUMNS : columns;
    if (columns == bPtr->maxVisibleColumns) {
    	return;
    }
    curMaxVisibleColumns = bPtr->maxVisibleColumns;
    bPtr->maxVisibleColumns = columns;
    /* browser not loaded */
    if (!bPtr->flags.loaded) {
    	if ((columns > curMaxVisibleColumns) && (columns > bPtr->columnCount)) {
	    int i = columns - bPtr->columnCount;
	    bPtr->usedColumnCount = bPtr->columnCount;
	    while (i--) {
	        WMAddBrowserColumn(bPtr);
	    }
	    bPtr->usedColumnCount = 0;
	}
    /* browser loaded and columns > curMaxVisibleColumns */
    } else if (columns > curMaxVisibleColumns) {
	if (bPtr->usedColumnCount > columns) {
	    newFirstVisibleColumn = bPtr->usedColumnCount - columns;
	}
	if (newFirstVisibleColumn > bPtr->firstVisibleColumn) {
	    newFirstVisibleColumn = bPtr->firstVisibleColumn;
	}
	if (columns > bPtr->columnCount) {
	    int i = columns - bPtr->columnCount;
	    int curUsedColumnCount = bPtr->usedColumnCount;
	    bPtr->usedColumnCount = bPtr->columnCount;
	    while (i--) {
		WMAddBrowserColumn(bPtr);
	    }
	    bPtr->usedColumnCount = curUsedColumnCount;
	}
    /* browser loaded and columns < curMaxVisibleColumns */
    } else {
	newFirstVisibleColumn = bPtr->firstVisibleColumn;
	if (newFirstVisibleColumn + columns >= bPtr->usedColumnCount) {
	    removeColumn(bPtr, newFirstVisibleColumn + columns);
	}
    }
    resizeBrowser(bPtr, bPtr->view->size.width, bPtr->view->size.height);
    if (bPtr->flags.loaded) {
	XClearArea(bPtr->view->screen->display, bPtr->view->window, 0, 0,
		   bPtr->view->size.width, bPtr->titleHeight, False);
	scrollToColumn (bPtr, newFirstVisibleColumn, True);
    }
}


int 
WMGetBrowserNumberOfColumns(WMBrowser *bPtr)
{
    return bPtr->usedColumnCount;
}

void
WMSetBrowserPathSeparator(WMBrowser *bPtr, char *separator)
{
    if (bPtr->pathSeparator)
	free(bPtr->pathSeparator);
    bPtr->pathSeparator = wstrdup(separator);
}



static void
drawTitleOfColumn(WMBrowser *bPtr, int column)
{
    WMScreen *scr = bPtr->view->screen;
    int x;
    
    x=(column-bPtr->firstVisibleColumn)*(bPtr->columnSize.width+COLUMN_SPACING);

    XFillRectangle(scr->display, bPtr->view->window, WMColorGC(scr->darkGray), x, 0,
		   bPtr->columnSize.width, bPtr->titleHeight);
    W_DrawRelief(scr, bPtr->view->window, x, 0,
		 bPtr->columnSize.width, bPtr->titleHeight, WRSunken);

    if (column < bPtr->usedColumnCount && bPtr->titles[column]) {
	int titleLen = strlen(bPtr->titles[column]);
	int widthC = bPtr->columnSize.width-8;

	if (WMWidthOfString(scr->boldFont, bPtr->titles[column], titleLen)
	    > widthC) {     
	    char *titleBuf = createTruncatedString(scr->boldFont,
						   bPtr->titles[column],
						   &titleLen, widthC);
	    W_PaintText(bPtr->view, bPtr->view->window, scr->boldFont, x, 
			(bPtr->titleHeight-WMFontHeight(scr->boldFont))/2,
			bPtr->columnSize.width, WACenter, WMColorGC(scr->white),
			False, titleBuf, titleLen);
	    free (titleBuf);
	} else {
	    W_PaintText(bPtr->view, bPtr->view->window, scr->boldFont, x, 
			(bPtr->titleHeight-WMFontHeight(scr->boldFont))/2,
			bPtr->columnSize.width, WACenter, WMColorGC(scr->white),
			False, bPtr->titles[column], titleLen);
	}
    }
}


WMList*
WMGetBrowserListInColumn(WMBrowser *bPtr, int column)
{
    if (column < 0 || column >= bPtr->usedColumnCount)
	return NULL;
    
    return bPtr->columns[column];
}


void 
WMSetBrowserDelegate(WMBrowser *bPtr, WMBrowserDelegate *delegate)
{
    bPtr->delegate = delegate;
}


int 
WMGetBrowserFirstVisibleColumn(WMBrowser *bPtr)
{
    return bPtr->firstVisibleColumn;
}


static void
removeColumn(WMBrowser *bPtr, int column)
{
    int i, clearEnd, destroyEnd;
    WMList **clist;
    char **tlist;
    
    assert ((int) bPtr);
    
    column = (column < 0) ? 0 : column;
    if (column >= bPtr->columnCount) {
	return;
    }
    if (column < bPtr->maxVisibleColumns) {
	clearEnd = bPtr->maxVisibleColumns;
	destroyEnd = bPtr->columnCount;
	bPtr->columnCount = bPtr->maxVisibleColumns;
    } else {
	clearEnd = column;
	destroyEnd = bPtr->columnCount;
	bPtr->columnCount = column;
    }
    if (column < bPtr->usedColumnCount) {
	bPtr->usedColumnCount = column;
    }
    for (i=column; i < clearEnd; i++) {
	if (bPtr->titles[i]) {
	    free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	}
	WMClearList(bPtr->columns[i]);
    }
    for (;i < destroyEnd; i++) {
	if (bPtr->titles[i]) {
	    free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	}
        WMRemoveNotificationObserverWithName(bPtr,
                                             WMListSelectionDidChangeNotification,
					     bPtr->columns[i]);
	WMDestroyWidget(bPtr->columns[i]);
	bPtr->columns[i] = NULL;
    }
    clist = wmalloc(sizeof(WMList*) * (bPtr->columnCount));
    tlist = wmalloc(sizeof(char*) * (bPtr->columnCount));
    memcpy(clist, bPtr->columns, sizeof(WMList*) * (bPtr->columnCount));
    memcpy(tlist, bPtr->titles, sizeof(char*) * (bPtr->columnCount));
    free(bPtr->titles);
    free(bPtr->columns);
    bPtr->titles = tlist;
    bPtr->columns = clist;
}


WMListItem*
WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column)
{
    if ((column < 0) || (column >= bPtr->usedColumnCount))
	return NULL;

    return WMGetListSelectedItem(bPtr->columns[column]);
}



int
WMGetBrowserSelectedColumn(WMBrowser *bPtr)
{
    return bPtr->selectedColumn;
}


int
WMGetBrowserSelectedRowInColumn(WMBrowser *bPtr, int column)
{
    if (column >= 0 && column < bPtr->columnCount) {
	return WMGetListSelectedItemRow(bPtr->columns[column]);
    } else {
	return -1;
    }
} 


void
WMSetBrowserColumnTitle(WMBrowser *bPtr, int column, char *title)
{
    assert(column >= 0);
    assert(column < bPtr->usedColumnCount);

    if (bPtr->titles[column])
	free(bPtr->titles[column]);

    bPtr->titles[column] = wstrdup(title);
   
    if (COLUMN_IS_VISIBLE(bPtr, column) && bPtr->flags.isTitled) {
	drawTitleOfColumn(bPtr, column);
    }
}


void
WMSetBrowserTitled(WMBrowser *bPtr, Bool flag)
{
    int i;
    int columnX, columnY;

    if (bPtr->flags.isTitled == flag)
	return;

    columnX = 0;
    
    if (!bPtr->flags.isTitled) {
	columnY = TITLE_SPACING + bPtr->titleHeight;

	bPtr->columnSize.height -= columnY;

	for (i=0; i<bPtr->columnCount; i++) {
	    WMResizeWidget(bPtr->columns[i], bPtr->columnSize.width,
			   bPtr->columnSize.height);

	    columnX = WMWidgetView(bPtr->columns[i])->pos.x;

	    WMMoveWidget(bPtr->columns[i], columnX, columnY);
	}
    } else {
	bPtr->columnSize.height += TITLE_SPACING + bPtr->titleHeight;

	for (i=0; i<bPtr->columnCount; i++) {
	    WMResizeWidget(bPtr->columns[i], bPtr->columnSize.width,
			   bPtr->columnSize.height);

	    columnX = WMWidgetView(bPtr->columns[i])->pos.x;

	    WMMoveWidget(bPtr->columns[i], columnX, 0);
	}
    }
    
    bPtr->flags.isTitled = flag;
}


WMListItem*
WMAddSortedBrowserItem(WMBrowser *bPtr, int column, char *text, Bool isBranch)
{
    WMListItem *item;

    if (column < 0 || column >= bPtr->columnCount)
	return NULL;
	
    item = WMAddSortedListItem(bPtr->columns[column], text);
    item->isBranch = isBranch;

    return item;
}



WMListItem*
WMInsertBrowserItem(WMBrowser *bPtr, int column, int row, char *text,
		    Bool isBranch)
{
    WMListItem *item;

    if (column < 0 || column >= bPtr->columnCount)
	return NULL;

    item = WMInsertListItem(bPtr->columns[column], row, text);
    item->isBranch = isBranch;

    return item;
}




static void 
resizeBrowser(WMWidget *w, unsigned int width, unsigned int height)
{
    WMBrowser *bPtr = (WMBrowser*)w;
    int cols = bPtr->maxVisibleColumns;
    int colX, colY;
    int i;

    assert(width > 0);
    assert(height > 0);
    
    bPtr->columnSize.width = (width-(cols-1)*COLUMN_SPACING) / cols;
    bPtr->columnSize.height = height;
    
    if (bPtr->flags.isTitled) {
	colY = TITLE_SPACING + bPtr->titleHeight;
	bPtr->columnSize.height -= colY;
    } else {
	colY = 0;
    }
    
    if (bPtr->flags.hasScroller) {
	bPtr->columnSize.height -= SCROLLER_WIDTH + 4;

	if (bPtr->scroller) {
	    WMResizeWidget(bPtr->scroller, width-2, 1);
	    WMMoveWidget(bPtr->scroller, 1, height-SCROLLER_WIDTH-1);
	}
    }

    colX = 0;
    for (i = 0; i < bPtr->columnCount; i++) {
	WMResizeWidget(bPtr->columns[i], bPtr->columnSize.width,
		       bPtr->columnSize.height);
	
	WMMoveWidget(bPtr->columns[i], colX, colY);
	
	if (COLUMN_IS_VISIBLE(bPtr, i)) {
	    colX += bPtr->columnSize.width+COLUMN_SPACING;
	}
    }

    W_ResizeView(bPtr->view, width, height);
}


static void
paintItem(WMList *lPtr, int index, Drawable d, char *text, int state, 
	  WMRect *rect)
{
    WMView *view = W_VIEW(lPtr);
    W_Screen *scr = view->screen;
    int width, height, x, y;

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
	XFillRectangle(scr->display, d, WMColorGC(scr->white), x, y,
		       width, height);
    else 
	XClearArea(scr->display, d, x, y, width, height, False);

    if (text) {
	/* Avoid overlaping... */
	int textLen = strlen(text);
	int widthC = (state & WLDSIsBranch) ? width-20 : width-8;
	if (WMWidthOfString(scr->normalFont, text, textLen) > widthC) {
	    char *textBuf = createTruncatedString(scr->normalFont,
		                                  text, &textLen, widthC);
            W_PaintText(view, d, scr->normalFont,  x+4, y, widthC,
		    	WALeft, WMColorGC(scr->black), False, textBuf, textLen);
	    free(textBuf);
	} else {
      	    W_PaintText(view, d, scr->normalFont,  x+4, y, widthC,
		    	WALeft, WMColorGC(scr->black), False, text, textLen);
	}
    }

    if (state & WLDSIsBranch) {
	XDrawLine(scr->display, d, WMColorGC(scr->darkGray), x+width-11, y+3,
		  x+width-6, y+height/2);
	if (state & WLDSSelected)
	    XDrawLine(scr->display, d,WMColorGC(scr->gray), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	else
	    XDrawLine(scr->display, d,WMColorGC(scr->white), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	XDrawLine(scr->display, d, WMColorGC(scr->black), x+width-12, y+3,
		  x+width-12, y+height-5);
    }
}


static void
scrollCallback(WMWidget *scroller, void *self)
{
    WMBrowser *bPtr = (WMBrowser*)self;
    WMScroller *sPtr = (WMScroller*)scroller;
    int newFirst;
#define LAST_VISIBLE_COLUMN  bPtr->firstVisibleColumn+bPtr->maxVisibleColumns

    switch (WMGetScrollerHitPart(sPtr)) {
     case WSDecrementLine:
	if (bPtr->firstVisibleColumn > 0) {
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn-1, True);
	}
	break;
	
     case WSDecrementPage:
	if (bPtr->firstVisibleColumn > 0) {
	    newFirst = bPtr->firstVisibleColumn - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst, True);
	}
	break;

	
     case WSIncrementLine:
	if (LAST_VISIBLE_COLUMN < bPtr->usedColumnCount) {
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn+1, True);
	}
	break;
	
     case WSIncrementPage:
	if (LAST_VISIBLE_COLUMN < bPtr->usedColumnCount) {
	    newFirst = bPtr->firstVisibleColumn + bPtr->maxVisibleColumns;

	    if (newFirst+bPtr->maxVisibleColumns >= bPtr->columnCount)
		newFirst = bPtr->columnCount - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst, True);
	}
	break;
	
     case WSKnob:
	{
	    double floatValue;
	    double value = bPtr->columnCount - bPtr->maxVisibleColumns;

	    floatValue = WMGetScrollerValue(bPtr->scroller);

	    floatValue = (floatValue*value)/value;

	    newFirst = rint(floatValue*(float)(bPtr->columnCount - bPtr->maxVisibleColumns));

	    if (bPtr->firstVisibleColumn != newFirst)
		scrollToColumn(bPtr, newFirst, False);
/*	    else
		WMSetScrollerParameters(bPtr->scroller, floatValue,
					bPtr->maxVisibleColumns/(float)bPtr->columnCount);
 */

	}
	break;

     case WSKnobSlot:
     case WSNoPart:
	/* do nothing */
	break;
    }
#undef LAST_VISIBLE_COLUMN
}


static void
setupScroller(WMBrowser *bPtr)
{
    WMScroller *sPtr;
    int y;

    y = bPtr->view->size.height - SCROLLER_WIDTH - 1;
    
    sPtr = WMCreateScroller(bPtr);
    WMSetScrollerAction(sPtr, scrollCallback, bPtr);
    WMMoveWidget(sPtr, 1, y);
    WMResizeWidget(sPtr, bPtr->view->size.width-2, SCROLLER_WIDTH);
    
    bPtr->scroller = sPtr;
    
    WMMapWidget(sPtr);
}


void
WMSetBrowserAction(WMBrowser *bPtr, WMAction *action, void *clientData)
{
    bPtr->action = action;
    bPtr->clientData = clientData;
}


void
WMSetBrowserDoubleAction(WMBrowser *bPtr, WMAction *action, void *clientData)
{
    bPtr->doubleAction = action;
    bPtr->doubleClientData = clientData;
}


void
WMSetBrowserHasScroller(WMBrowser *bPtr, int hasScroller)
{
    bPtr->flags.hasScroller = hasScroller;
}



char*
WMSetBrowserPath(WMBrowser *bPtr, char *path)
{
    int i;
    char *str = wstrdup(path);
    char *tmp, *retPtr = NULL;
    int item;
    WMListItem *listItem;

    /* WMLoadBrowserColumnZero must be call first */
    if (!bPtr->flags.loaded) {
	return False;
    }

    removeColumn(bPtr, 1);

    WMSelectListItem(bPtr->columns[0], -1);
    WMSetListPosition(bPtr->columns[0], 0);

    i = 0;
    tmp = strtok(str, bPtr->pathSeparator);
    while (tmp) {
	/* select it in the column */
        item = WMFindRowOfListItemWithTitle(bPtr->columns[i], tmp);
	if (item<0) {
            retPtr = &path[(int)(tmp - str)];
	    break;
	}
	WMSelectListItem(bPtr->columns[i], item);
	WMSetListPosition(bPtr->columns[i], item);

	listItem = WMGetListItem(bPtr->columns[i], item);
	if (!listItem || !listItem->isBranch) {
	    break;
	} 

	/* load next column */
	WMAddBrowserColumn(bPtr);

	loadColumn(bPtr, i+1);
	
	tmp = strtok(NULL, bPtr->pathSeparator);

	i++;
    }
    free(str);

    for (i = bPtr->usedColumnCount - 1;
    	 (i > -1) && !WMGetListSelectedItem(bPtr->columns[i]);
	 i--);

    bPtr->selectedColumn = i;
    
    if (bPtr->columnCount < bPtr->maxVisibleColumns) {
	int i = bPtr->maxVisibleColumns - bPtr->columnCount;
	int curUsedColumnCount = bPtr->usedColumnCount;
	bPtr->usedColumnCount = bPtr->columnCount;
	while (i--) {
	    WMAddBrowserColumn(bPtr);
	}
	bPtr->usedColumnCount = curUsedColumnCount;
    }

    scrollToColumn(bPtr, bPtr->columnCount - bPtr->maxVisibleColumns, True);

    return retPtr;
}


char*
WMGetBrowserPath(WMBrowser *bPtr)
{
    return WMGetBrowserPathToColumn(bPtr, bPtr->columnCount);
}



char*
WMGetBrowserPathToColumn(WMBrowser *bPtr, int column)
{
    int i, size;
    char *path;
    WMListItem *item;
    
    if (column >= bPtr->usedColumnCount)
	column = bPtr->usedColumnCount-1;

    if (column < 0) {
        return wstrdup(bPtr->pathSeparator);
    }

    /* calculate size of buffer */
    size = 0;
    for (i = 0; i <= column; i++) {
	item = WMGetListSelectedItem(bPtr->columns[i]);
	if (!item)
	    break;
	size += strlen(item->text);
    }
    
    /* get the path */
    path = wmalloc(size+(column+1)*strlen(bPtr->pathSeparator)+1);
    /* ignore first / */
    *path = 0;
    for (i = 0; i <= column; i++) {
	strcat(path, bPtr->pathSeparator);
	item = WMGetListSelectedItem(bPtr->columns[i]);
	if (!item)
	    break;
	strcat(path, item->text);
    }

    return path;
}


static void
loadColumn(WMBrowser *bPtr, int column)
{
    assert(bPtr->delegate);
    assert(bPtr->delegate->createRowsForColumn);

    bPtr->flags.loadingColumn = 1;
    (*bPtr->delegate->createRowsForColumn)(bPtr->delegate, bPtr, column,
					   bPtr->columns[column]);
    bPtr->flags.loadingColumn = 0;

    if (bPtr->delegate->titleOfColumn) {
	char *title;

	title = (*bPtr->delegate->titleOfColumn)(bPtr->delegate, bPtr, column);

	if (bPtr->titles[column])
	    free(bPtr->titles[column]);

	bPtr->titles[column] = title;
    }
}


static void
paintBrowser(WMBrowser *bPtr)
{
    int i;

    if (!bPtr->view->flags.mapped)
	return;

    W_DrawRelief(bPtr->view->screen, bPtr->view->window, 0, 
		 bPtr->view->size.height-SCROLLER_WIDTH-2,
		 bPtr->view->size.width, 22, WRSunken);
    
    if (bPtr->flags.isTitled) {
	for (i=0; i<bPtr->maxVisibleColumns; i++) {
	    drawTitleOfColumn(bPtr, i+bPtr->firstVisibleColumn);
	}
    }
}


static void
handleEvents(XEvent *event, void *data)
{
    WMBrowser *bPtr = (WMBrowser*)data;

    CHECK_CLASS(data, WC_Browser);


    switch (event->type) {
     case Expose:
	paintBrowser(bPtr);
	break;
	
     case DestroyNotify:
	destroyBrowser(bPtr);
	break;
	
    }
}



static void
scrollToColumn(WMBrowser *bPtr, int column, Bool updateScroller)
{
    int i;
    int x;
    int notify = 0;

    
    if (column != bPtr->firstVisibleColumn) {
	notify = 1;
    }

    if (column < 0)
	column = 0;

    if (notify && bPtr->delegate && bPtr->delegate->willScroll) {
	(*bPtr->delegate->willScroll)(bPtr->delegate, bPtr);
    }

    x = 0;
    bPtr->firstVisibleColumn = column;
    for (i = 0; i < bPtr->columnCount; i++) {
	if (COLUMN_IS_VISIBLE(bPtr, i)) {
	    WMMoveWidget(bPtr->columns[i], x,
			 WMWidgetView(bPtr->columns[i])->pos.y);
	    if (!WMWidgetView(bPtr->columns[i])->flags.realized)
		WMRealizeWidget(bPtr->columns[i]);
	    WMMapWidget(bPtr->columns[i]);
	    x += bPtr->columnSize.width + COLUMN_SPACING;
	} else {
	    WMUnmapWidget(bPtr->columns[i]);
	}
    }

    /* update the scroller */
    if (updateScroller) {
	if (bPtr->columnCount > bPtr->maxVisibleColumns) {
	    float value, proportion;

	    value = bPtr->firstVisibleColumn
		/(float)(bPtr->columnCount-bPtr->maxVisibleColumns);
	    proportion = bPtr->maxVisibleColumns/(float)bPtr->columnCount;
	    WMSetScrollerParameters(bPtr->scroller, value, proportion);
	} else {
	    WMSetScrollerParameters(bPtr->scroller, 0, 1);
	}
    }
    
    if (bPtr->view->flags.mapped)
	paintBrowser(bPtr);

    if (notify && bPtr->delegate && bPtr->delegate->didScroll) {
	(*bPtr->delegate->didScroll)(bPtr->delegate, bPtr);
    }
}


static void
listCallback(void *self, void *clientData)
{
    WMBrowser *bPtr = (WMBrowser*)clientData;
    WMList *lPtr = (WMList*)self;
    WMListItem *item;
    static WMListItem *oldItem = NULL;
    int i;

    item = WMGetListSelectedItem(lPtr);
    if (!item) {
        oldItem = item;
        return;
    }

    if (oldItem != item) {
        for (i=0; i<bPtr->columnCount; i++) {
            if (lPtr == bPtr->columns[i])
                break;
        }
        assert(i<bPtr->columnCount);

	bPtr->selectedColumn = i;

        /* columns at right must be cleared */
        removeColumn(bPtr, i+1);
        /* open directory */
        if (item->isBranch) {
            WMAddBrowserColumn(bPtr);
        }
        if (bPtr->usedColumnCount < bPtr->maxVisibleColumns)
            i = 0;
        else
            i = bPtr->usedColumnCount-bPtr->maxVisibleColumns;
        scrollToColumn(bPtr, i, True);
	if (item->isBranch) {
            loadColumn(bPtr, bPtr->usedColumnCount-1);
	}
    }
  

    /* call callback for click */
    if (bPtr->action)
	(*bPtr->action)(bPtr, bPtr->clientData);

    oldItem = item;
}


static void
listDoubleCallback(void *self, void *clientData)
{
    WMBrowser *bPtr = (WMBrowser*)clientData;
    WMList *lPtr = (WMList*)self;
    WMListItem *item;

    item = WMGetListSelectedItem(lPtr);
    if (!item)
	return;

    /* call callback for double click */
    if (bPtr->doubleAction)
	(*bPtr->doubleAction)(bPtr, bPtr->doubleClientData);
}


void
WMLoadBrowserColumnZero(WMBrowser *bPtr)
{
    if (!bPtr->flags.loaded) {
	/* create column 0 */
	WMAddBrowserColumn(bPtr);

	loadColumn(bPtr, 0);
	    
	/* make column 0 visible */
	scrollToColumn(bPtr, 0, True);

	bPtr->flags.loaded = 1;
    } 
}


void
WMRemoveBrowserItem(WMBrowser *bPtr, int column, int row)
{
    WMList *list;

    if (column < 0 || column >= bPtr->usedColumnCount)
	return;

    list = WMGetBrowserListInColumn(bPtr, column);

    if (row < 0 || row >= WMGetListNumberOfRows(list))
	return;

    removeColumn(bPtr, column+1);
    if (bPtr->usedColumnCount < bPtr->maxVisibleColumns)
	scrollToColumn(bPtr, 0, True);
    else
	scrollToColumn(bPtr, bPtr->usedColumnCount-bPtr->maxVisibleColumns,
		       True);

    WMRemoveListItem(list, row);
}


static void
listSelectionObserver(void *observerData, WMNotification *notification)
{
    WMBrowser *bPtr = (WMBrowser*)observerData;
    int column, item = (int)WMGetNotificationClientData(notification);
    WMList *lPtr = (WMList*)WMGetNotificationObject(notification);

    for (column = 0; column < bPtr->usedColumnCount; column++)
        if (bPtr->columns[column] == lPtr)
            break;

    /* this can happen when a list is being cleared with WMClearList
     * after the column was removed */
    if (column >= bPtr->usedColumnCount) {
	return;
    }

    if (item < 0)
        column--;

    bPtr->selectedColumn = column;
}


int
WMAddBrowserColumn(WMBrowser *bPtr)
{
    WMList *list;
    WMList **clist;
    char **tlist;
    int colY;
    int index;

    
    if (bPtr->usedColumnCount < bPtr->columnCount) {
	return bPtr->usedColumnCount++;
    }

    bPtr->usedColumnCount++;

    if (bPtr->flags.isTitled) {
	colY = TITLE_SPACING + bPtr->titleHeight;
    } else {
	colY = 0;
    }

    index = bPtr->columnCount;
    bPtr->columnCount++;
    clist = wmalloc(sizeof(WMList*)*bPtr->columnCount);
    tlist = wmalloc(sizeof(char*)*bPtr->columnCount);
    memcpy(clist, bPtr->columns, sizeof(WMList*)*(bPtr->columnCount-1));
    memcpy(tlist, bPtr->titles, sizeof(char*)*(bPtr->columnCount-1));
    if (bPtr->columns)
	free(bPtr->columns);
    if (bPtr->titles)
	free(bPtr->titles);
    bPtr->columns = clist;
    bPtr->titles = tlist;

    bPtr->titles[index] = NULL;

    list = WMCreateList(bPtr);
    WMSetListAction(list, listCallback, bPtr);
    WMSetListDoubleAction(list, listDoubleCallback, bPtr);
    WMSetListUserDrawProc(list, paintItem);
    WMAddNotificationObserver(listSelectionObserver, bPtr,
			      WMListSelectionDidChangeNotification, list);

    bPtr->columns[index] = list;

    WMResizeWidget(list, bPtr->columnSize.width, bPtr->columnSize.height);
    WMMoveWidget(list, (bPtr->columnSize.width+COLUMN_SPACING)*index, colY);
    if (COLUMN_IS_VISIBLE(bPtr, index)) 
	WMMapWidget(list);

    /* update the scroller */
    if (bPtr->columnCount > bPtr->maxVisibleColumns) {
	float value, proportion;

	value = bPtr->firstVisibleColumn
	    /(float)(bPtr->columnCount-bPtr->maxVisibleColumns);
	proportion = bPtr->maxVisibleColumns/(float)bPtr->columnCount;
	WMSetScrollerParameters(bPtr->scroller, value, proportion);
    }

    return index;
}



static void
destroyBrowser(WMBrowser *bPtr)
{
    int i;

    for (i = 0; i < bPtr->columnCount; i++) {
	if (bPtr->titles[i])
	    free(bPtr->titles[i]);
    }
    free(bPtr->titles);
    
    free(bPtr->pathSeparator);
    
    WMRemoveNotificationObserver(bPtr);
    
    free(bPtr);
}


static char*
createTruncatedString(WMFont *font, char *text, int *textLen, int width)
{
    int dLen = WMWidthOfString(font, ".", 1);
    char *textBuf = (char*)wmalloc((*textLen)+4);

    if (width >= 3*dLen) {
	int dddLen = 3*dLen;
	int tmpTextLen = *textLen;
	
	strcpy(textBuf, text);
	while (tmpTextLen
	       && (WMWidthOfString(font, textBuf, tmpTextLen)+dddLen > width))
	    tmpTextLen--;
	strcpy(textBuf+tmpTextLen, "...");
	*textLen = tmpTextLen+3;
    } else if (width >= 2*dLen) {
	strcpy(textBuf, "..");
	*textLen = 2;
    } else if (width >= dLen) {
	strcpy(textBuf, ".");
	*textLen = 1;
    } else {
	*textBuf = '\0';
	*textLen = 0;
    }
    return textBuf;
}
