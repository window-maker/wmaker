



#include "WINGsP.h"


char *WMBrowserDidScrollNotification = "WMBrowserDidScrollNotification";


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

    WMBrowserFillColumnProc *fillColumn;
    
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

#define DEFAULT_WIDTH		305
#define DEFAULT_HEIGHT		200
#define DEFAULT_HAS_SCROLLER	True

#define DEFAULT_SEPARATOR	"/"

#define COLUMN_IS_VISIBLE(b, c)	((c) >= (b)->firstVisibleColumn \
				&& (c) < (b)->firstVisibleColumn + (b)->maxVisibleColumns)


static void handleEvents(XEvent *event, void *data);
static void destroyBrowser(WMBrowser *bPtr);

static void setupScroller(WMBrowser *bPtr);

static void scrollToColumn(WMBrowser *bPtr, int column);

static void paintItem(WMList *lPtr, int index, Drawable d, char *text, 
		      int state, WMRect *rect);

static void loadColumn(WMBrowser *bPtr, int column);


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

    bPtr->titleHeight = 20;
    bPtr->flags.isTitled = 1;
    bPtr->maxVisibleColumns = 2;

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


void
WMSetBrowserMaxVisibleColumns(WMBrowser *bPtr, int columns)
{
    if (columns > bPtr->maxVisibleColumns) {

	if (columns > bPtr->columnCount) {
	    int i = columns - bPtr->columnCount;

	    while (i--) {
		WMAddBrowserColumn(bPtr);
	    }
	}

	resizeBrowser(bPtr, bPtr->view->size.width, bPtr->view->size.height);

    } else if (columns < bPtr->maxVisibleColumns) {

	resizeBrowser(bPtr, bPtr->view->size.width, bPtr->view->size.height);
    }

    bPtr->maxVisibleColumns = columns;
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

    XFillRectangle(scr->display, bPtr->view->window, W_GC(scr->darkGray), x, 0,
		   bPtr->columnSize.width, bPtr->titleHeight);
    W_DrawRelief(scr, bPtr->view->window, x, 0,
		 bPtr->columnSize.width, bPtr->titleHeight, WRSunken);

    if (column < bPtr->usedColumnCount && bPtr->titles[column])
	W_PaintText(bPtr->view, bPtr->view->window, scr->boldFont, x, 
		    (bPtr->titleHeight-WMFontHeight(scr->boldFont))/2,
		    bPtr->columnSize.width, WACenter, W_GC(scr->white),
		    False, bPtr->titles[column], strlen(bPtr->titles[column]));
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


WMList*
WMGetBrowserListInColumn(WMBrowser *bPtr, int column)
{
    if (column < 0 || column >= bPtr->usedColumnCount)
	return NULL;
    
    return bPtr->columns[column];
}


void 
WMSetBrowserFillColumnProc(WMBrowser *bPtr, WMBrowserFillColumnProc *proc)
{
    bPtr->fillColumn = proc;
}


int 
WMGetBrowserFirstVisibleColumn(WMBrowser *bPtr)
{
    return bPtr->firstVisibleColumn;
}


static void
removeColumn(WMBrowser *bPtr, int column)
{
    int i;
    WMList **clist;
    char **tlist;

    if (column >= bPtr->usedColumnCount)
	return;

    if (column < bPtr->maxVisibleColumns) {
	int tmp;
#if 0
	/* this code causes bugs */
	int limit;

	if(bPtr->usedColumnCount < bPtr->maxVisibleColumns)
	    limit = bPtr->usedColumnCount;
	else
	    limit = bPtr->maxVisibleColumns;

	for (i=column; i < limit; i++) {
	    if (bPtr->titles[i])
		free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	    
	    WMClearList(bPtr->columns[i]);
	    bPtr->usedColumnCount--;
	}
#else
	for (i=column; i < bPtr->maxVisibleColumns; i++) {
	    if (bPtr->titles[i])
		free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	    
	    WMClearList(bPtr->columns[i]);
	    bPtr->usedColumnCount--;
	}
	tmp = bPtr->columnCount;
	for (i=bPtr->maxVisibleColumns; i < tmp; i++) {
	    if (bPtr->titles[i])
		free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	    
	    WMDestroyWidget(bPtr->columns[i]);
	    bPtr->columns[i] = NULL;
	    bPtr->columnCount--;
	    bPtr->usedColumnCount--;
	}
#endif
    } else {
	int tmp = bPtr->columnCount;
	for (i=column; i < tmp; i++) {
	    if (bPtr->titles[i])
		free(bPtr->titles[i]);
	    bPtr->titles[i] = NULL;
	    
	    WMDestroyWidget(bPtr->columns[i]);
	    bPtr->columns[i] = NULL;
	    bPtr->columnCount--;
	    bPtr->usedColumnCount--;
	}	
    }
    clist = wmalloc(sizeof(WMList*)*bPtr->columnCount);
    tlist = wmalloc(sizeof(char*)*bPtr->columnCount);
    memcpy(clist, bPtr->columns, sizeof(WMList*)*bPtr->columnCount);
    memcpy(tlist, bPtr->titles, sizeof(char*)*bPtr->columnCount);
    free(bPtr->titles);
    free(bPtr->columns);
    bPtr->titles = tlist;
    bPtr->columns = clist;
}



WMListItem*
WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column)
{
    if ((column < 0) || (column > bPtr->columnCount))
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
	bPtr->columnSize.height -= TITLE_SPACING + bPtr->titleHeight;
	colY = TITLE_SPACING + bPtr->titleHeight;
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
	XFillRectangle(scr->display, d, W_GC(scr->white), x, y,
		       width, height);
    else 
	XClearArea(scr->display, d, x, y, width, height, False);

    if (text) {
      	W_PaintText(view, d, scr->normalFont,  x+4, y, width,
		    WALeft, W_GC(scr->black), False, text, strlen(text));
    }

    if (state & WLDSIsBranch) {
	XDrawLine(scr->display, d, W_GC(scr->darkGray), x+width-11, y+3,
		  x+width-6, y+height/2);
	if (state & WLDSSelected)
	    XDrawLine(scr->display, d,W_GC(scr->gray), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	else
	    XDrawLine(scr->display, d,W_GC(scr->white), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	XDrawLine(scr->display, d, W_GC(scr->black), x+width-12, y+3,
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
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn-1);
	}
	break;
	
     case WSDecrementPage:
	if (bPtr->firstVisibleColumn > 0) {
	    newFirst = bPtr->firstVisibleColumn - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst);
	}
	break;

	
     case WSIncrementLine:
	if (LAST_VISIBLE_COLUMN < bPtr->columnCount) {
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn+1);
	}
	break;
	
     case WSIncrementPage:
	if (LAST_VISIBLE_COLUMN < bPtr->columnCount) {
	    newFirst = bPtr->firstVisibleColumn + bPtr->maxVisibleColumns;

	    if (newFirst+bPtr->maxVisibleColumns >= bPtr->columnCount)
		newFirst = bPtr->columnCount - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst);
	}
	break;
	
     case WSKnob:
	{
	    float floatValue;
	    float value = bPtr->columnCount - bPtr->maxVisibleColumns;
	    
	    floatValue = WMGetScrollerValue(bPtr->scroller);
	    
	    floatValue = (floatValue*value)/value;
	    
	    newFirst = floatValue*(float)(bPtr->columnCount - bPtr->maxVisibleColumns);

	    if (bPtr->firstVisibleColumn != newFirst)
		scrollToColumn(bPtr, newFirst);
	    else
		WMSetScrollerParameters(bPtr->scroller, floatValue,
					bPtr->maxVisibleColumns/(float)bPtr->columnCount);

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
WMSetBrowserHasScroller(WMBrowser *bPtr, int hasScroller)
{
    bPtr->flags.hasScroller = hasScroller;
}



Bool
WMSetBrowserPath(WMBrowser *bPtr, char *path)
{
    int i;
    char *str = wstrdup(path);
    char *tmp;
    int item;
    Bool ok = True;
    WMListItem *listItem;

    removeColumn(bPtr, 1);

    i = 0;
    tmp = strtok(str, bPtr->pathSeparator);
    while (tmp) {
	/* select it in the column */
	item = WMFindRowOfListItemWithTitle(bPtr->columns[i], tmp);
	if (item<0) {
	    ok = False;
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

    bPtr->selectedColumn = bPtr->usedColumnCount - 1;

    scrollToColumn(bPtr, bPtr->columnCount-bPtr->maxVisibleColumns);

    return ok;
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
    assert(bPtr->fillColumn);
    
    bPtr->flags.loadingColumn = 1;
    (*bPtr->fillColumn)(bPtr, column);
    bPtr->flags.loadingColumn = 0;
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
scrollToColumn(WMBrowser *bPtr, int column)
{
    int i;
    int x;
    int notify = 0;

    
    if (column != bPtr->firstVisibleColumn)
	notify = 1;

    if (column < 0)
	column = 0;

    x = 0;
    bPtr->firstVisibleColumn = column;
    for (i = 0; i < bPtr->usedColumnCount; i++) {
	if (COLUMN_IS_VISIBLE(bPtr, i)) {
	    WMMoveWidget(bPtr->columns[i], x,
			 WMWidgetView(bPtr->columns[i])->pos.y);
	    if (!WMWidgetView(bPtr->columns[i])->flags.realized)
		WMRealizeWidget(bPtr->columns[i]);
	    WMMapWidget(bPtr->columns[i]);
	    x += bPtr->columnSize.width+COLUMN_SPACING;
	} else {
	    WMUnmapWidget(bPtr->columns[i]);
	}
    }

    /* update the scroller */
    if (bPtr->columnCount > bPtr->maxVisibleColumns) {
	float value, proportion;

	value = bPtr->firstVisibleColumn
	    /(float)(bPtr->columnCount-bPtr->maxVisibleColumns);
	proportion = bPtr->maxVisibleColumns/(float)bPtr->columnCount;
	WMSetScrollerParameters(bPtr->scroller, value, proportion);
    } else {
	WMSetScrollerParameters(bPtr->scroller, 0, 1);
    }
    
    if (bPtr->view->flags.mapped)
	paintBrowser(bPtr);

    if (notify)
	WMPostNotificationName(WMBrowserDidScrollNotification, bPtr, NULL);
}


static void
listCallback(void *self, void *clientData)
{
    WMBrowser *bPtr = (WMBrowser*)clientData;
    WMList *lPtr = (WMList*)self;
    WMListItem *item;
    int i;

    item = WMGetListSelectedItem(lPtr);
    if (!item)
	return;

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
	loadColumn(bPtr, bPtr->usedColumnCount-1);
    }
    if (bPtr->usedColumnCount < bPtr->maxVisibleColumns)
	i = 0;
    else
	i = bPtr->usedColumnCount-bPtr->maxVisibleColumns;
    scrollToColumn(bPtr, i);

    /* call callback for click */
    if (bPtr->action)
	(*bPtr->action)(bPtr, bPtr->clientData);
}


void
WMLoadBrowserColumnZero(WMBrowser *bPtr)
{
    if (!bPtr->flags.loaded) {
	/* create column 0 */
	WMAddBrowserColumn(bPtr);

	loadColumn(bPtr, 0);
	    
	/* make column 0 visible */
	scrollToColumn(bPtr, 0);

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
	scrollToColumn(bPtr, 0);
    else
	scrollToColumn(bPtr, bPtr->usedColumnCount-bPtr->maxVisibleColumns);
    
    WMRemoveListItem(list, row);
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
    WMSetListUserDrawProc(list, paintItem);
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

    for (i=0; i<bPtr->columnCount; i++) {
	if (bPtr->titles[i])
	    free(bPtr->titles[i]);
    }
    free(bPtr->titles);
    
    free(bPtr->pathSeparator);
    
    free(bPtr);
}


