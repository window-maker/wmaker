

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



WMTableColumnDelegate *WTCreateStringDelegate(WMScreen *scr)
{
    WMTableColumnDelegate *delegate = wmalloc(sizeof(WMTableColumnDelegate)); 
    
    delegate->data = WFCreateStringEditor(scr);
    delegate->drawCell = cellPainter;
    delegate->editCell = NULL;
    
    return delegate;
}
