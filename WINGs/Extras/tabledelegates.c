

#include <WINGs.h>

#include "wtableview.h"


typedef struct {
    WMTableView *table;
    WMFont *font;
    GC gc;
    GC selGc;
    GC selTextGc;
} StringData;


typedef struct {
    WMTextField *widget;
    WMTableView *table;
    WMFont *font;
    GC gc;
    GC selGc;
    GC selTextGc;
} StringEditorData;


typedef struct {
    WMPopUpButton *widget;
    WMTableView *table;
    WMFont *font;
    char **options;
    int count;    
    GC gc;
    GC selGc;
    GC selTextGc;
} EnumSelectorData;



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






static void SECellPainter(WMTableColumnDelegate *self,
			WMTableColumn *column, int row)
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


static void selectedSECellPainter(WMTableColumnDelegate *self,
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


static void beginSECellEdit(WMTableColumnDelegate *self,
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


static void endSECellEdit(WMTableColumnDelegate *self,
			WMTableColumn *column, int row)
{
    StringEditorData *strdata = (StringEditorData*)self->data;    
    WMRect rect = WMTableViewRectForCell(strdata->table, column, row);
    char *text;
    
    text = WMGetTextFieldText(strdata->widget);
    WMSetTableViewDataForCell(strdata->table, column, row, (void*)text);
}


WMTableColumnDelegate *WTCreateStringEditorDelegate(WMTableView *parent)
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
    delegate->drawCell = SECellPainter;
    delegate->drawSelectedCell = selectedSECellPainter;
    delegate->beginCellEdit = beginSECellEdit;
    delegate->endCellEdit = endSECellEdit;

    return delegate;
}



/* ---------------------------------------------------------------------- */


static void ESCellPainter(WMTableColumnDelegate *self,
			WMTableColumn *column, int row)
{
    EnumSelectorData *strdata = (EnumSelectorData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    int i = WMTableViewDataForCell(table, column, row);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       strdata->options[i],
	       WMTableViewRectForCell(table, column, row), 
	       False);
}


static void selectedESCellPainter(WMTableColumnDelegate *self,
				WMTableColumn *column, int row)
{
    EnumSelectorData *strdata = (EnumSelectorData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    int i = WMTableViewDataForCell(table, column, row);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       strdata->options[i], 
	       WMTableViewRectForCell(table, column, row), 
	       True);
}


static void beginESCellEdit(WMTableColumnDelegate *self,
			  WMTableColumn *column, int row)
{
    EnumSelectorData *strdata = (EnumSelectorData*)self->data;    
    WMRect rect = WMTableViewRectForCell(strdata->table, column, row);
    int data = (int)WMTableViewDataForCell(strdata->table, column, row);

    wassertr(data < strdata->count);
    
    WMSetPopUpButtonSelectedItem(strdata->widget, data);
    
    WMMoveWidget(strdata->widget, rect.pos.x, rect.pos.y-1);
    WMResizeWidget(strdata->widget, rect.size.width, rect.size.height+2);

    WMMapWidget(strdata->widget);
}


static void endESCellEdit(WMTableColumnDelegate *self,
			WMTableColumn *column, int row)
{
    EnumSelectorData *strdata = (EnumSelectorData*)self->data;    
    WMRect rect = WMTableViewRectForCell(strdata->table, column, row);
    int option;
    
    option = WMGetPopUpButtonSelectedItem(strdata->widget);
    WMSetTableViewDataForCell(strdata->table, column, row, (void*)option);
}


WMTableColumnDelegate *WTCreateEnumSelectorDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate = wmalloc(sizeof(WMTableColumnDelegate));
    WMScreen *scr = WMWidgetScreen(parent);
    EnumSelectorData *data = wmalloc(sizeof(EnumSelectorData));
    
    data->widget = WMCreatePopUpButton(parent);
    W_ReparentView(WMWidgetView(data->widget), 
		   WMGetTableViewDocumentView(parent), 
		   0, 0);
    data->table = parent;
    data->font = WMSystemFontOfSize(scr, 12);
    data->selGc = WMColorGC(WMDarkGrayColor(scr));
    data->selTextGc = WMColorGC(WMWhiteColor(scr));
    data->gc = WMColorGC(WMBlackColor(scr));
    data->count = 0;
    data->options = NULL;
    
    delegate->data = data;
    delegate->drawCell = ESCellPainter;
    delegate->drawSelectedCell = selectedESCellPainter;
    delegate->beginCellEdit = beginESCellEdit;
    delegate->endCellEdit = endESCellEdit;

    return delegate;
}


void WTSetEnumSelectorOptions(WMTableColumnDelegate *delegate,
			      char **options, int count)
{
    EnumSelectorData *data = (EnumSelectorData*)delegate->data;
    int i;
    
    for (i = 0;
	 i < WMGetPopUpButtonNumberOfItems(data->widget);
	 i++) {
	WMRemovePopUpButtonItem(data->widget, 0);
    }
    
    data->options = options;
    data->count = count;
    
    for (i = 0; i < count; i++) {
	WMAddPopUpButtonItem(data->widget, options[i]);
    }
}


/* ---------------------------------------------------------------------- */


static void SCellPainter(WMTableColumnDelegate *self,
			WMTableColumn *column, int row)
{
    StringData *strdata = (StringData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       WMTableViewDataForCell(table, column, row),
	       WMTableViewRectForCell(table, column, row),
	       False);
}


static void selectedSCellPainter(WMTableColumnDelegate *self,
				WMTableColumn *column, int row)
{
    StringData *strdata = (StringData*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    stringDraw(WMWidgetScreen(table), 
	       WMViewXID(WMGetTableViewDocumentView(table)),
	       strdata->gc, strdata->selGc, strdata->selTextGc, strdata->font,
	       WMTableViewDataForCell(table, column, row),
	       WMTableViewRectForCell(table, column, row),
	       True);
}


WMTableColumnDelegate *WTCreateStringDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate = wmalloc(sizeof(WMTableColumnDelegate));
    WMScreen *scr = WMWidgetScreen(parent);
    StringData *data = wmalloc(sizeof(StringData));
    
    data->table = parent;
    data->font = WMSystemFontOfSize(scr, 12);
    data->selGc = WMColorGC(WMDarkGrayColor(scr));
    data->selTextGc = WMColorGC(WMWhiteColor(scr));
    data->gc = WMColorGC(WMBlackColor(scr));
    
    delegate->data = data;
    delegate->drawCell = SCellPainter;
    delegate->drawSelectedCell = selectedSCellPainter;
    delegate->beginCellEdit = NULL;
    delegate->endCellEdit = NULL;

    return delegate;
}
