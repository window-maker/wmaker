



typedef struct {
    WMTextField *field;
    WMScreen *scr;
    GC gc;
    WMFont *font;
} WFStringEditor;


typedef struct {
    
} WFStringSelector;


WFStringEditor *WFCreateStringEditor();

void WFStringEditorTableDraw(WFStringEditor *self, Drawable d, void *data, WMRect rect);

