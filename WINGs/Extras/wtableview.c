

#include <WINGs/WINGsP.h>
#include <X11/cursorfont.h>

#include "wtableview.h"


const char *WMTableViewSelectionDidChangeNotification = "WMTableViewSelectionDidChangeNotification";


struct W_TableColumn {
    WMTableView *table;
    WMWidget *titleW;
    char *title;
    int width;
    int minWidth;
    int maxWidth;
    
    void *id;

    WMTableColumnDelegate *delegate;

    unsigned resizable:1;
    unsigned editable:1;
};



static void handleResize(W_ViewDelegate *self, WMView *view);

static void rearrangeHeader(WMTableView *table);

static WMRange rowsInRect(WMTableView *table, WMRect rect);


WMTableColumn *WMCreateTableColumn(char *title)
{
    WMTableColumn *col = wmalloc(sizeof(WMTableColumn));
    
    col->table = NULL;
    col->titleW = NULL;
    col->width = 50;
    col->minWidth = 5;
    col->maxWidth = 0;
    
    col->id = NULL;
    
    col->title = wstrdup(title);
    
    col->delegate = NULL;
    
    col->resizable = 1;
    col->editable = 0;
        
    return col;
}


void WMSetTableColumnId(WMTableColumn *column, void *id)
{
    column->id = id;
}


void *WMGetTableColumnId(WMTableColumn *column)
{
    return column->id;
}


void WMSetTableColumnWidth(WMTableColumn *column, unsigned width)
{
    if (column->maxWidth == 0)
	column->width = WMAX(column->minWidth, width);
    else
	column->width = WMAX(column->minWidth, WMIN(column->maxWidth, width));

    if (column->table) {
	rearrangeHeader(column->table);
    }
}


void WMSetTableColumnDelegate(WMTableColumn *column, 
			      WMTableColumnDelegate *delegate)
{
    column->delegate = delegate;
}


void WMSetTableColumnConstraints(WMTableColumn *column, 
				 unsigned minWidth, unsigned maxWidth)
{
    wassertr(maxWidth == 0 || minWidth <= maxWidth);
    
    column->minWidth = minWidth;
    column->maxWidth = maxWidth;
    
    if (column->width < column->minWidth)
	WMSetTableColumnWidth(column, column->minWidth);
    else if (column->width > column->maxWidth && column->maxWidth != 0)
	WMSetTableColumnWidth(column, column->maxWidth);
}


void WMSetTableColumnEditable(WMTableColumn *column, Bool flag)
{
    column->editable = flag;
}


WMTableView *WMGetTableColumnTableView(WMTableColumn *column)
{
    return column->table;
}



struct W_TableView {
    W_Class widgetClass;
    WMView *view;

    WMFrame *header;
    
    WMLabel *corner;
    
    WMScrollView *scrollView;
    WMView *tableView;

    WMArray *columns;
    WMArray *splitters;

    WMArray *selectedRows;

    int tableWidth;
    
    int rows;
    
    GC gridGC;
    WMColor *gridColor;
    
    Cursor splitterCursor;
    
    void *dataSource;

    WMTableViewDelegate *delegate;

    WMAction *action;
    void *clientData;
    
    void *clickedColumn;
    int clickedRow;
    
    int editingRow;
    
    unsigned headerHeight;
    
    unsigned rowHeight;

    unsigned dragging:1;
    unsigned drawsGrid:1;
    unsigned canSelectRow:1;
    unsigned canSelectMultiRows:1;
    unsigned canDeselectRow:1;
};

static W_Class tableClass = 0;


static W_ViewDelegate viewDelegate = {
    NULL,
	NULL,
	handleResize,
	NULL,
	NULL
};





static void handleEvents(XEvent *event, void *data);
static void handleTableEvents(XEvent *event, void *data);


static void scrollObserver(void *self, WMNotification *notif)
{
    WMTableView *table = (WMTableView*)self;
    WMRect rect;
    int i, x;

    rect = WMGetScrollViewVisibleRect(table->scrollView);
    
    x = 0;
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	WMTableColumn *column;
	WMView *splitter;
	
	column = WMGetFromArray(table->columns, i);
	
	WMMoveWidget(column->titleW, x - rect.pos.x, 0);
		
	x += W_VIEW_WIDTH(WMWidgetView(column->titleW)) + 1;
	
	splitter = WMGetFromArray(table->splitters, i);
	W_MoveView(splitter, x - rect.pos.x - 1, 0);	
    }
}


static void splitterHandler(XEvent *event, void *data)
{
    WMTableColumn *column = (WMTableColumn*)data;
    WMTableView *table = column->table;
    int done = 0;
    int cx, ox, offsX;
    WMPoint pos;
    WMScreen *scr = WMWidgetScreen(table);
    GC gc = scr->ixorGC;
    Display *dpy = WMScreenDisplay(scr);
    int h = WMWidgetHeight(table) - 22;
    Window w = WMViewXID(table->view);
    
    pos = WMGetViewPosition(WMWidgetView(column->titleW));

    offsX = pos.x + column->width;
    
    ox = cx = offsX;
    
    XDrawLine(dpy, w, gc, cx+20, 0, cx+20, h);

    while (!done) {
	XEvent ev;

	WMMaskEvent(dpy, ButtonMotionMask|ButtonReleaseMask, &ev);

	switch (ev.type) {
	 case MotionNotify:
	    ox = cx;
	    	    
	    if (column->width + ev.xmotion.x < column->minWidth)
		cx = pos.x + column->minWidth;
	    else if (column->maxWidth > 0 
		&& column->width + ev.xmotion.x > column->maxWidth)
		cx = pos.x + column->maxWidth;
	    else
		cx = offsX + ev.xmotion.x;

	    XDrawLine(dpy, w, gc, ox+20, 0, ox+20, h);	    
	    XDrawLine(dpy, w, gc, cx+20, 0, cx+20, h);
	    break;

	 case ButtonRelease:
	    column->width = cx - pos.x;
	    done = 1;
	    break;
	}
    }
    
    XDrawLine(dpy, w, gc, cx+20, 0, cx+20, h);
    rearrangeHeader(table);    
}



WMTableView *WMCreateTableView(WMWidget *parent)
{
    WMTableView *table = wmalloc(sizeof(WMTableView));
    WMScreen *scr = WMWidgetScreen(parent);
    
    memset(table, 0, sizeof(WMTableView));

    if (!tableClass) {
	tableClass = W_RegisterUserWidget();
    }
    table->widgetClass = tableClass;
    
    table->view = W_CreateView(W_VIEW(parent));
    if (!table->view) 
	goto error;
    table->view->self = table;
        
    table->view->delegate = &viewDelegate;
    
    table->headerHeight = 20;
    
    table->scrollView = WMCreateScrollView(table);
    if (!table->scrollView)
	goto error;
    WMResizeWidget(table->scrollView, 10, 10);
    WMSetScrollViewHasVerticalScroller(table->scrollView, True);
    WMSetScrollViewHasHorizontalScroller(table->scrollView, True);
    {
	WMScroller *scroller;
	scroller = WMGetScrollViewHorizontalScroller(table->scrollView);
	WMAddNotificationObserver(scrollObserver, table,
				  WMScrollerDidScrollNotification,
				  scroller);
    }
    WMMoveWidget(table->scrollView, 1, 2+table->headerHeight);    
    WMMapWidget(table->scrollView);

    table->header = WMCreateFrame(table);
    WMMoveWidget(table->header, 22, 2);
    WMMapWidget(table->header);
    WMSetFrameRelief(table->header, WRFlat);
    
    table->corner = WMCreateLabel(table);
    WMResizeWidget(table->corner, 20, table->headerHeight);
    WMMoveWidget(table->corner, 2, 2);
    WMMapWidget(table->corner);
    WMSetLabelRelief(table->corner, WRRaised);
    WMSetWidgetBackgroundColor(table->corner, scr->darkGray);
    

    table->tableView = W_CreateView(W_VIEW(parent));
    if (!table->tableView)
	goto error;
    table->tableView->self = table;
    W_ResizeView(table->tableView, 100, 1000);
    W_MapView(table->tableView);
    
    table->tableView->flags.dontCompressExpose = 1;
    
    table->gridColor = WMCreateNamedColor(scr, "#cccccc", False);
 /*   table->gridColor = WMGrayColor(scr);*/
    
    {
	XGCValues gcv;
	
	gcv.foreground = WMColorPixel(table->gridColor);
	gcv.dashes = 1;
	gcv.line_style = LineOnOffDash;
	table->gridGC = XCreateGC(WMScreenDisplay(scr), W_DRAWABLE(scr),
				  GCForeground, &gcv);
    }
    
    table->editingRow = -1;
    table->clickedRow = -1;
        
    table->drawsGrid = 1;
    table->rowHeight = 16;

    WMSetScrollViewLineScroll(table->scrollView, table->rowHeight);
    
    table->tableWidth = 1;
    
    table->columns = WMCreateArray(4);
    table->splitters = WMCreateArray(4);

    table->selectedRows = WMCreateArray(16);
    
    table->splitterCursor = XCreateFontCursor(WMScreenDisplay(scr),
					      XC_sb_h_double_arrow);
    
    table->canSelectRow = 1;
    
    WMCreateEventHandler(table->view, ExposureMask|StructureNotifyMask,
                         handleEvents, table);

    WMCreateEventHandler(table->tableView, ExposureMask|ButtonPressMask|
			 ButtonReleaseMask|ButtonMotionMask,
                         handleTableEvents, table);

    WMSetScrollViewContentView(table->scrollView, table->tableView);
    
    return table;
    
 error:
    if (table->scrollView) 
	WMDestroyWidget(table->scrollView);
    if (table->tableView)
	W_DestroyView(table->tableView);
    if (table->view)
	W_DestroyView(table->view);
    wfree(table);
    return NULL;
}


void WMAddTableViewColumn(WMTableView *table, WMTableColumn *column)
{
    int width;
    int i;
    WMScreen *scr = WMWidgetScreen(table);
    int count;
    
    column->table = table;

    WMAddToArray(table->columns, column);
        
    if (!column->titleW) {
	column->titleW = WMCreateLabel(table);
	WMSetLabelRelief(column->titleW, WRRaised);
	WMSetLabelFont(column->titleW, scr->boldFont);
	WMSetLabelTextColor(column->titleW, scr->white);
	WMSetWidgetBackgroundColor(column->titleW, scr->darkGray);
	WMSetLabelText(column->titleW, column->title);
	W_ReparentView(WMWidgetView(column->titleW),
		       WMWidgetView(table->header), 0, 0);
	if (W_VIEW_REALIZED(table->view))
	    WMRealizeWidget(column->titleW);
	WMMapWidget(column->titleW);
    }
    
    {
	WMView *splitter = W_CreateView(WMWidgetView(table->header));
	
	W_SetViewBackgroundColor(splitter, WMWhiteColor(scr));

	if (W_VIEW_REALIZED(table->view))
	    W_RealizeView(splitter);

	W_ResizeView(splitter, 2, table->headerHeight-1);
	W_MapView(splitter);

	W_SetViewCursor(splitter, table->splitterCursor);
	WMCreateEventHandler(splitter, ButtonPressMask|ButtonReleaseMask,
			     splitterHandler, column);

	WMAddToArray(table->splitters, splitter);
    }

    rearrangeHeader(table);
}


void WMSetTableViewHeaderHeight(WMTableView *table, unsigned height)
{
    table->headerHeight = height;
    
    handleResize(NULL, table->view);
}


void WMSetTableViewDelegate(WMTableView *table, WMTableViewDelegate *delegate)
{
    table->delegate = delegate;
}


void WMSetTableViewAction(WMTableView *table, WMAction *action, void *clientData)
{
    table->action = action;
    
    table->clientData = clientData;
}


void *WMGetTableViewClickedColumn(WMTableView *table)
{
    return table->clickedColumn;
}


int WMGetTableViewClickedRow(WMTableView *table)
{
    return table->clickedRow;
}


WMArray *WMGetTableViewSelectedRows(WMTableView *table)
{
    return table->selectedRows;
}


WMView *WMGetTableViewDocumentView(WMTableView *table)
{
    return table->tableView;
}


void *WMTableViewDataForCell(WMTableView *table, WMTableColumn *column, 
			     int row)
{
    return (*table->delegate->valueForCell)(table->delegate, column, row);
}


void WMSetTableViewDataForCell(WMTableView *table, WMTableColumn *column, 
			       int row, void *data)
{
    (*table->delegate->setValueForCell)(table->delegate, column, row, data);
}


WMRect WMTableViewRectForCell(WMTableView *table, WMTableColumn *column, 
			      int row)
{
    WMRect rect;
    int i;
    
    rect.pos.x = 0;
    rect.pos.y = row * table->rowHeight;
    rect.size.height = table->rowHeight;
    
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	WMTableColumn *col;
	col = WMGetFromArray(table->columns, i);
	
	if (col == column) {
	    rect.size.width = col->width;
	    break;
	}
	
	rect.pos.x += col->width;
    }
    return rect;
}


void WMSetTableViewDataSource(WMTableView *table, void *source)
{
    table->dataSource = source;
}


void *WMGetTableViewDataSource(WMTableView *table)
{
    return table->dataSource;
}


void WMSetTableViewBackgroundColor(WMTableView *table, WMColor *color)
{
    W_SetViewBackgroundColor(table->tableView, color);
}


void WMSetTableViewGridColor(WMTableView *table, WMColor *color)
{
    WMReleaseColor(table->gridColor);
    table->gridColor = WMRetainColor(color);
    XSetForeground(WMScreenDisplay(WMWidgetScreen(table)), table->gridGC, 
		   WMColorPixel(color));
}



void WMSetTableViewRowHeight(WMTableView *table, int height)
{
    table->rowHeight = height;
    
    WMRedisplayWidget(table);
}


void WMScrollTableViewRowToVisible(WMTableView *table, int row)
{
    WMScroller *scroller;
    WMRange range;
    WMRect rect;
    int newY, tmp;
    
    rect = WMGetScrollViewVisibleRect(table->scrollView);
    range = rowsInRect(table, rect);

    scroller = WMGetScrollViewVerticalScroller(table->scrollView);

    if (row < range.position) {
	newY = row * table->rowHeight - rect.size.height / 2;
    } else if (row >= range.position + range.count) {
	newY = row * table->rowHeight - rect.size.height / 2;
    } else {
	return;
    }
    tmp = table->rows*table->rowHeight - rect.size.height;
    newY = WMAX(0, WMIN(newY, tmp));
    WMScrollViewScrollPoint(table->scrollView, rect.pos.x, newY);
}
      


static void drawGrid(WMTableView *table, WMRect rect)
{
    WMScreen *scr = WMWidgetScreen(table);
    Display *dpy = WMScreenDisplay(scr);
    int i;
    int y1, y2;
    int x1, x2;
    int xx;
    Drawable d = W_VIEW_DRAWABLE(table->tableView);
    GC gc = table->gridGC;
    
#if 0
    char dashl[1] = {1};
    
    XSetDashes(dpy, gc, 0, dashl, 1);
    
    y1 = (rect.pos.y/table->rowHeight - 1)*table->rowHeight;
    y2 = y1 + (rect.size.height/table->rowHeight+2)*table->rowHeight;
#endif
    y1 = 0;
    y2 = W_VIEW_HEIGHT(table->tableView);

    xx = 0;
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	WMTableColumn *column;
	
	if (xx >= rect.pos.x && xx <= rect.pos.x+rect.size.width) {
	    XDrawLine(dpy, d, gc, xx, y1, xx, y2);
	}
	column = WMGetFromArray(table->columns, i);
	xx += column->width;
    }
    XDrawLine(dpy, d, gc, xx, y1, xx, y2);
    
    
    x1 = rect.pos.x;
    x2 = WMIN(x1 + rect.size.width, xx);
    
    if (x2 <= x1)
	return;
#if 0
    XSetDashes(dpy, gc, (rect.pos.x&1), dashl, 1);
#endif
    
    y1 = rect.pos.y - rect.pos.y%table->rowHeight;
    y2 = y1 + rect.size.height + table->rowHeight;
    
    for (i = y1; i <= y2; i += table->rowHeight) {
	XDrawLine(dpy, d, gc, x1, i, x2, i);
    }    
}


static WMRange columnsInRect(WMTableView *table, WMRect rect)
{
    WMTableColumn *column;
    int width;
    int i , j;
    int totalColumns = WMGetArrayItemCount(table->columns);
    WMRange range;
    
    j = 0;
    width = 0;
    for (i = 0; i < totalColumns; i++) {
	column = WMGetFromArray(table->columns, i);
	if (j == 0) {
	    if (width <= rect.pos.x && width + column->width > rect.pos.x) {
		range.position = i;
		j = 1;
	    }
	} else {
	    range.count++;
	    if (width > rect.pos.x + rect.size.width) {
		break;
	    }
	}
	width += column->width;
    }
    range.count = WMAX(1, WMIN(range.count, totalColumns - range.position));
    return range;
}


static WMRange rowsInRect(WMTableView *table, WMRect rect)
{
    WMRange range;
    int rh = table->rowHeight;
    int dif;
    
    dif = rect.pos.y % rh;
    
    range.position = WMAX(0, (rect.pos.y - dif) / rh);
    range.count = WMAX(1, WMIN((rect.size.height + dif) / rh, table->rows));

    return range;
}


static void drawRow(WMTableView *table, int row, WMRect clipRect)
{
    int i;
    WMRange cols = columnsInRect(table, clipRect);
    WMTableColumn *column;

    for (i = cols.position; i < cols.position+cols.count; i++) {
	column = WMGetFromArray(table->columns, i);

	wassertr(column->delegate && column->delegate->drawCell);
	
	if (WMFindInArray(table->selectedRows, NULL, (void*)row) != WANotFound)
	    (*column->delegate->drawSelectedCell)(column->delegate, column, row);
	else
	    (*column->delegate->drawCell)(column->delegate, column, row);
    }
}


static void drawFullRow(WMTableView *table, int row)
{
    int i;
    WMTableColumn *column;
       
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	column = WMGetFromArray(table->columns, i);

	wassertr(column->delegate && column->delegate->drawCell);
	
	if (WMFindInArray(table->selectedRows, NULL, (void*)row) != WANotFound)
	    (*column->delegate->drawSelectedCell)(column->delegate, column, row);
	else
	    (*column->delegate->drawCell)(column->delegate, column, row);
    }
}


static void setRowSelected(WMTableView *table, unsigned row, Bool flag)
{
    int repaint = 0;

    if (WMFindInArray(table->selectedRows, NULL, (void*)row) != WANotFound) {
	if (!flag) {
	    WMRemoveFromArray(table->selectedRows, (void*)row);
	    repaint = 1;
	}
    } else {
	if (flag) {
	    WMAddToArray(table->selectedRows, (void*)row);
	    repaint = 1;
	}
    }
    if (repaint && row < table->rows) {
	drawFullRow(table, row);
    }
}


static void repaintTable(WMTableView *table, int x, int y, 
			 int width, int height)
{
    WMRect rect;
    WMRange rows;
    int i;
    
    wassertr(table->delegate && table->delegate->numberOfRows);
    i = (*table->delegate->numberOfRows)(table->delegate, table);

    if (i != table->rows) {
	table->rows = i;
	W_ResizeView(table->tableView, table->tableWidth, 
		     table->rows * table->rowHeight + 1);   
    }
    

    if (x >= table->tableWidth-1)
	return;
    
    rect.pos = wmkpoint(x,y);
    rect.size = wmksize(width, height);
    
    if (table->drawsGrid) {
	drawGrid(table, rect);
    }
    
    rows = rowsInRect(table, rect);
    for (i = rows.position;
	 i < WMIN(rows.position+rows.count + 1, table->rows);
	 i++) {
	drawRow(table, i, rect);
    }
}


static void stopRowEdit(WMTableView *table, int row)
{
    int i;
    WMTableColumn *column;

    table->editingRow = -1;
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	column = WMGetFromArray(table->columns, i);

	if (column->delegate && column->delegate->endCellEdit)	
	    (*column->delegate->endCellEdit)(column->delegate, column, row);
    }
}



void WMEditTableViewRow(WMTableView *table, int row)
{
    int i;
    WMTableColumn *column;

    if (table->editingRow >= 0) {
	stopRowEdit(table, table->editingRow);
    }

    table->editingRow = row;
    
    if (row < 0)
	return;
    
    for (i = 0; i < WMGetArrayItemCount(table->columns); i++) {
	column = WMGetFromArray(table->columns, i);

	if (column->delegate && column->delegate->beginCellEdit)
	    (*column->delegate->beginCellEdit)(column->delegate, column, row);
    }
}


void WMSelectTableViewRow(WMTableView *table, int row)
{
    if (table->clickedRow >= 0)
	setRowSelected(table, table->clickedRow, False);
    
    if (row >= table->rows) {
	return;
    }
    
    setRowSelected(table, row, True);
    table->clickedRow = row;

    if (table->action)
	(*table->action)(table, table->clientData);
    WMPostNotificationName(WMTableViewSelectionDidChangeNotification,
			   table, NULL);
}


void WMReloadTableView(WMTableView *table)
{
    WMRect rect = WMGetScrollViewVisibleRect(table->scrollView);

    if (table->editingRow >= 0)
	stopRowEdit(table, table->editingRow);
    
    /* when this is called, nothing in the table can be assumed to be
     * like the last time we accessed it (ie, rows might have disappeared) */

    WMEmptyArray(table->selectedRows);
    
    if (table->clickedRow >= 0) {
	table->clickedRow = -1;

	if (table->action)
	    (*table->action)(table, table->clientData);
	WMPostNotificationName(WMTableViewSelectionDidChangeNotification,
			       table, NULL);
    }

    repaintTable(table, 0, 0, 
		 W_VIEW_WIDTH(table->tableView), rect.size.height);
}


void WMNoteTableViewNumberOfRowsChanged(WMTableView *table)
{
    WMReloadTableView(table);
}


static void handleTableEvents(XEvent *event, void *data)
{
    WMTableView *table = (WMTableView*)data;
    int row;
    
    switch (event->type) {
     case ButtonPress:
	if (event->xbutton.button == Button1) {
	    row = event->xbutton.y/table->rowHeight;
	    if (row != table->clickedRow) {
		setRowSelected(table, table->clickedRow, False);	    
		setRowSelected(table, row, True);
		table->clickedRow = row;	 
		table->dragging = 1;
	    }
	}
	break;
	
     case MotionNotify:
	if (table->dragging && event->xmotion.y >= 0) {
	    row = event->xmotion.y/table->rowHeight;
	    if (table->clickedRow != row && row >= 0) {
		setRowSelected(table, table->clickedRow, False);
		setRowSelected(table, row, True);
		table->clickedRow = row;
	    }
	}
	break;
	
     case ButtonRelease:
	if (event->xbutton.button == Button1) {
	    if (table->action)
		(*table->action)(table, table->clientData);
	    WMPostNotificationName(WMTableViewSelectionDidChangeNotification,
				   table, NULL);
	    table->dragging = 0;
	}
	break;

     case Expose:
	repaintTable(table, event->xexpose.x, event->xexpose.y,
		     event->xexpose.width, event->xexpose.height);
	break;
    }
}


static void handleEvents(XEvent *event, void *data)
{
    WMTableView *table = (WMTableView*)data;
    WMScreen *scr = WMWidgetScreen(table);
    
    switch (event->type) {
     case Expose:
	W_DrawRelief(scr, W_VIEW_DRAWABLE(table->view), 0, 0, 
		     W_VIEW_WIDTH(table->view), W_VIEW_HEIGHT(table->view),
		     WRSunken);
	
	if (event->xexpose.serial == 0) {
	    WMRect rect = WMGetScrollViewVisibleRect(table->scrollView);

	    repaintTable(table, rect.pos.x, rect.pos.y,
			 rect.size.width, rect.size.height);
	}
	break;
    }
}


static void handleResize(W_ViewDelegate *self, WMView *view)
{
    int width;
    int height;
    WMTableView *table = view->self;

    width = W_VIEW_WIDTH(view) - 2;
    height = W_VIEW_HEIGHT(view) - 3;
    
    height -= table->headerHeight; /* table header */

    if (table->corner)
	WMResizeWidget(table->corner, 20, table->headerHeight);
    
    if (table->scrollView) {
	WMMoveWidget(table->scrollView, 1, table->headerHeight + 2);
	WMResizeWidget(table->scrollView, width, height);
    }
    if (table->header)
	WMResizeWidget(table->header, width - 21, table->headerHeight);
}


static void rearrangeHeader(WMTableView *table)
{
    int width;
    int count;
    int i;    
    WMRect rect = WMGetScrollViewVisibleRect(table->scrollView);    
    
    width = 0;
    
    count = WMGetArrayItemCount(table->columns);
    for (i = 0; i < count; i++) {
	WMTableColumn *column = WMGetFromArray(table->columns, i);
	WMView *splitter = WMGetFromArray(table->splitters, i);

	WMMoveWidget(column->titleW, width, 0);
	WMResizeWidget(column->titleW, column->width-1, table->headerHeight);

	width += column->width;
	W_MoveView(splitter, width-1, 0);	
    }
    
    wassertr(table->delegate && table->delegate->numberOfRows);
    
    table->rows = table->delegate->numberOfRows(table->delegate, table);

    W_ResizeView(table->tableView, width+1,
		 table->rows * table->rowHeight + 1);

    table->tableWidth = width + 1;    
}
