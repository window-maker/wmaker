

#include <WINGs.h>

#include "wtableview.h"

#include "fieldeditor.h"


WFStringEditor *WFCreateStringEditor(WMScreen *scr)
{
    WFStringEditor *editor = wmalloc(sizeof(WFStringEditor));
    
    editor->scr = scr;
    editor->field = NULL;
    editor->font = WMSystemFontOfSize(scr, 12);
    editor->gc = WMColorGC(WMBlackColor(scr));
    
    return editor;
}



void WFStringEditorTableDraw(WFStringEditor *self, Drawable d, 
			     void *data, WMRect rect)
{
    int x, y;
    XRectangle rects[1];

    x = rect.pos.x + 5;
    y = rect.pos.y + (rect.size.height - WMFontHeight(self->font))/2;

    rects[0].x = rect.pos.x+1;
    rects[0].y = rect.pos.y+1;
    rects[0].width = rect.size.width-2;
    rects[0].height = rect.size.height-2;
    XSetClipRectangles(WMScreenDisplay(self->scr), self->gc, 0, 0, 
		       rects, 1, YXSorted);
    WMDrawString(self->scr, d, self->gc, self->font, x, y, data, strlen(data));
    XSetClipMask(WMScreenDisplay(self->scr), self->gc, None);
}
