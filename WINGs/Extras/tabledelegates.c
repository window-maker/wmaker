

#include <WINGs.h>

#include "fieldeditor.h"
#include "wtableview.h"




static void cellPainter(WMTableColumnDelegate *self, WMTableColumn *column,
			int row)
{
//    WTStringDelegate *strdel = (WTStringDelegate*)self->data;
    WMTableView *table = WMGetTableColumnTableView(column);
    
    WFStringEditorTableDraw(self->data,
			    WMViewXID(WMGetTableViewDocumentView(table)),
			    WMTableViewDataForCell(table, column, row),
			    WMTableViewRectForCell(table, column, row));
}



WMTableColumnDelegate *WTCreateStringDelegate(WMTableView *parent)
{
    WMTableColumnDelegate *delegate = wmalloc(sizeof(WMTableColumnDelegate)); 
    
    delegate->data = wmalloc(sizeof(StringEditorData));
    delegate->data->widget = WFCreateStringEditor(WMWidgetScreen(parent));
    delegate->data->table = parent;
    delegate->drawCell = cellPainter;
    delegate->beginCellEdit = NULL;
    delegate->endCellEdit = NULL;
    
    return delegate;
}



