


#ifndef _WTABLEVIEW_H_
#define _WTABLEVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct W_TableColumn WMTableColumn;
typedef struct W_TableView WMTableView;


extern const char *WMTableViewRowWasSelectedNotification;
extern const char *WMTableViewRowWasUnselectedNotification;


typedef struct WMTableColumnDelegate {
    void *data;
    void (*drawCell)(struct WMTableColumnDelegate *self, WMTableColumn *column,
		     int row);
    void (*drawSelectedCell)(struct WMTableColumnDelegate *self,
			     WMTableColumn *column, int row);
    void (*beginCellEdit)(struct WMTableColumnDelegate *self, WMTableColumn *column,
			  int row);
    void (*endCellEdit)(struct WMTableColumnDelegate *self, WMTableColumn *column,
			int row);    
} WMTableColumnDelegate;


typedef struct W_TableViewDelegate {
    void *data;
    int (*numberOfRows)(struct W_TableViewDelegate *self, 
			WMTableView *table);
    void *(*valueForCell)(struct W_TableViewDelegate *self, 
			  WMTableColumn *column, int row);
    void (*setValueForCell)(struct W_TableViewDelegate *self, 
			    WMTableColumn *column, int row, void *value);
} WMTableViewDelegate;

    
    
    

WMTableColumn *WMCreateTableColumn(char *title);

void WMSetTableColumnWidth(WMTableColumn *column, unsigned width);
    
void WMSetTableColumnDelegate(WMTableColumn *column, 
			      WMTableColumnDelegate *delegate);    

WMTableView *WMGetTableColumnTableView(WMTableColumn *column);
    
void WMSetTableColumnId(WMTableColumn *column, void *id);

void *WMGetTableColumnId(WMTableColumn *column);
    

WMTableView *WMCreateTableView(WMWidget *parent);

    
void WMSetTableViewDataSource(WMTableView *table, void *source);

void *WMGetTableViewDataSource(WMTableView *table);

void WMSetTableViewHeaderHeight(WMTableView *table, unsigned height);
    
void WMAddTableViewColumn(WMTableView *table, WMTableColumn *column);

void WMSetTableViewDelegate(WMTableView *table, WMTableViewDelegate *delegate);

WMView *WMGetTableViewDocumentView(WMTableView *table);

void WMEditTableViewRow(WMTableView *table, int row);
    
void *WMTableViewDataForCell(WMTableView *table, WMTableColumn *column, 
			     int row);

void WMSetTableViewDataForCell(WMTableView *table, WMTableColumn *column, 
			       int row, void *data);
    
WMRect WMTableViewRectForCell(WMTableView *table, WMTableColumn *column, 
			      int row);

void WMSetTableViewBackgroundColor(WMTableView *table, WMColor *color);

void WMSetTableViewGridColor(WMTableView *table, WMColor *color);

#ifdef __cplusplus
}
#endif

#endif

