

#include <WINGs.h>
#include "wtableview.h"
#include "wtabledelegates.h"


static char *col1[20] = {0};
static int col2[20];


static char *options[] = {
    "Option1",
	"Option2",
	"Option3",
	"Option4",
	"Option5"
};


int numberOfRows(WMTableViewDelegate *self, WMTableView *table)
{
    return 20;
}


void *valueForCell(WMTableViewDelegate *self, WMTableColumn *column, int row)
{
    WMTableView *table = (WMTableView*)WMGetTableColumnTableView(column);
    int i;
    if (col1[0] == 0) {
	for (i = 0; i < 20; i++) {
	    col1[i] = "teste";
	    col2[i] = 0;
	}
    }
    if ((int)WMGetTableColumnId(column) == 1)
	return col1[row];
    else
	return (void*)col2[row];
}


void setValueForCell(WMTableViewDelegate *self, WMTableColumn *column, int row,
		     void *data)
{
    if ((int)WMGetTableColumnId(column) == 1)
	col1[row] = data;
    else
	col2[row] = (int)data;
}


static WMTableViewDelegate delegate = {
    NULL,
	numberOfRows,
	valueForCell,
	setValueForCell
};



void clickedTable(WMWidget *w, void *self)
{    
    int row = WMGetTableViewClickedRow((WMTableView*)self);

    WMEditTableViewRow(self, row);
}




main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay("");
    WMScreen *scr;
    WMWindow *win;
    WMTableView *table;
    WMTableColumn *col;
    WMTableColumnDelegate *colDeleg;
    
    WMInitializeApplication("test", &argc, argv);
    
    
    
    dpy = XOpenDisplay("");
    if (!dpy) {
	exit(1);
    }
    
    scr = WMCreateScreen(dpy, DefaultScreen(dpy));

    
    win = WMCreateWindow(scr, "eweq");
    WMResizeWidget(win, 400, 200);
    WMMapWidget(win);
    
    table = WMCreateTableView(win);
    WMResizeWidget(table, 400, 200);
    WMSetTableViewBackgroundColor(table, WMWhiteColor(scr));
//    WMSetTableViewGridColor(table, WMGrayColor(scr));
    WMSetTableViewHeaderHeight(table, 20);
    WMSetTableViewDelegate(table, &delegate);
    WMSetTableViewAction(table, clickedTable, table);
    
    colDeleg = WTCreateStringEditorDelegate(table);
    
    col = WMCreateTableColumn("Group");
    WMSetTableColumnWidth(col, 180);
    WMAddTableViewColumn(table, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)1);

    colDeleg = WTCreateEnumSelectorDelegate(table);
    WTSetEnumSelectorOptions(colDeleg, options, 5);

    col = WMCreateTableColumn("Package");
    WMSetTableColumnWidth(col, 140);
    WMAddTableViewColumn(table, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)2);	

    
    colDeleg = WTCreateBooleanSwitchDelegate(table);

    col = WMCreateTableColumn("Bool");
    WMSetTableColumnWidth(col, 50);
    WMAddTableViewColumn(table, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)2);
    

    WMMapWidget(table);
    WMRealizeWidget(win);
    WMScreenMainLoop(scr);
    
}
