
#ifndef _TABLEDELEGATES_H_
#define _TABLEDELEGATES_H_

#ifdef __cplusplus
extern "C" {
#endif

WMTableColumnDelegate *WTCreateStringDelegate(WMTableView *table);
WMTableColumnDelegate *WTCreateStringEditorDelegate(WMTableView *table);
WMTableColumnDelegate *WTCreateEnumSelectorDelegate(WMTableView *table);

#ifdef __cplusplus
}
#endif
    
#endif
