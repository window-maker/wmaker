

#include <WINGs.h>
#include "wtableview.h"
#include "tabledelegates.h"



int numberOfRows(WMTableViewDelegate *self, WMTableView *table)
{
    return 20;
}


void *valueForCell(WMTableViewDelegate *self, WMTableColumn *column, int row)
{
    WMTableView *table = (WMTableView*)WMGetTableColumnTableView(column);
    
    return "TESTE";
}


static WMTableViewDelegate delegate = {
    NULL,
	numberOfRows,
	valueForCell,
	NULL
};



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
    WMSetTableViewGridColor(table, WMGrayColor(scr));
    WMSetTableViewHeaderHeight(table, 20);
    WMSetTableViewDelegate(table, &delegate);
    
    colDeleg = WTCreateStringDelegate(table);
    
    col = WMCreateTableColumn("Group");
    WMSetTableColumnWidth(col, 180);
    WMAddTableViewColumn(table, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)1);

    colDeleg = WTCreateStringDelegate(table);
    
    col = WMCreateTableColumn("Package");
    WMSetTableColumnWidth(col, 240);
    WMAddTableViewColumn(table, col);	
    WMSetTableColumnDelegate(col, colDeleg);
    WMSetTableColumnId(col, (void*)2);	


    WMMapWidget(table);
    WMRealizeWidget(win);
    WMScreenMainLoop(scr);
    
}
