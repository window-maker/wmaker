

#include <WINGs.h>

#include "wtableview.h"


typedef struct {
    WMTextField *widget;
    WMTableView *table;
    WMFont *font;
    GC gc;
    GC selGc;
    GC selTextGc;
} StringEditorData;


static void stringDraw(WMScreen *scr, Drawable d, GC gc,
		       GC sgc, GC stgc, WMFont *font, void *data,
		       WMRect rect, Bool selected)
{
    int x, y;
    XRectangle rects[1];
    Display *dpy = WMScreenDisplay(scr);

    x = rect.pos.x + 5;
    y = rect.pos.y + (rect.size.height - WMFontHeight(font))/2;

    rects[0].x = rect.pos.x+1;
    rects[0].y = rect.pos.y+1;
    rects[0].width = rect.size.width-1;
    rects[0].height = rect.size.height-1;
    XSetClipRectangles(dpy, gc, 0, 0, rects, 1, YXSorted);
    
    if (!selected) {
	XClearArea(dpy, d, rects[0].x, rects[0].y, 
		   rects[0].width, rects[0].height,
		   False);
    
	WMDrawString(scr, d, gc, font, x, y, 
		     data, strlen(data));	
    } else {
	XFillRectangles(dpy, d, sgc, rects, 1);
    
	WMDrawString(scr, d, stgc, font, x, y, 
		     data, strlen(data));
    }
    
    XSetClipMask(dpy, gc, None);
}




static void cellPainter(WMTableColumnDelegate *self, WMTableColumn *column,
			int row)
{
    StringEditorData *strdata = (StringEditorData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       WMTableViewDataForCell(table, column, row),
	       WMTableViewRectForCell(table, column, row),
	       False);
}


static void selectedCellPainter(WMTableColumnDelegate *self,
				WMTableColumn *column, int row)
{
    StringEditorData *strdata = (StringEditorData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       WMTableViewDataForCell(table, column, row),
	       WMTableViewRectForCell(table, column, row),
	       True);
}


static void beginCellEdit(WMTableColumnDelegate *self,
			  WMTableColumn *column, int row)
{
    StringEditorData *strdata = (StringEditorData*)self->data;    
    WMRect rect = WMTableViewRectForCell(strdata->table, column, row);
    void *data = WMTableViewDataForCell(strdata->table, column, row);

    WMSetTextFieldText(strdata->widget, (char*)data);
    WMMoveWidget(strdata->widget, rect.pos.x, rect.pos.y);
    WMResizeWidget(strdata->widget, rect.size.width+1, rect.size.height+1);

    WMMapWidget(strdata->widget);
}



WMTableColumnDelegate *WTCreateStringDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate = wmalloc(sizeof(WMTableColumnDelegate));
    WMScreen *scr = WMWidgetScreen(parent);
    StringEditorData *data = wmalloc(sizeof(StringEditorData));
    
    data->widget = WMCreateTextField(parent);
    W_ReparentView(WMWidgetView(data->widget), 
		   WMGetTableViewDocumentView(parent), 
		   0, 0);
    data->table = parent;
    data->font = WMSystemFontOfSize(scr, 12);
    data->selGc = WMColorGC(WMDarkGrayColor(scr));
    data->selTextGc = WMColorGC(WMWhiteColor(scr));
    data->gc = WMColorGC(WMBlackColor(scr));
    
    delegate->data = data;
    delegate->drawCell = cellPainter;
    delegate->drawSelectedCell = selectedCellPainter;
    delegate->beginCellEdit = beginCellEdit;
    delegate->endCellEdit = NULL;
    
    return delegate;
}



