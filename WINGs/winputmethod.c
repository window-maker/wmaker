

#include <X11/Xlib.h>

#include "WINGsP.h"



typedef struct W_IMContext {
    XIM xim;

    struct W_ICContext *icList;
} WMIMContext;


typedef struct W_ICContext {
    struct W_ICContext *next;
    struct W_ICContext *prev;

    XIC xic;
} WMICContext;



Bool
W_InitIMStuff(WMScreen *scr)
{
    WMIMContext *ctx;

    ctx = scr->imctx = wmalloc(sizeof(WMIMContext));

    ctx->xim = XOpenIM(scr->display, NULL, NULL, NULL);
    if (ctx->xim == NULL) {
        wwarning("could not open IM");
        return False;
    }

}


void
W_CloseIMStuff(WMScreen *scr)
{
    if (!scr->imctx)
        return;

    if (scr->imctx->xim)
        XCloseIM(scr->imctx->xim);
    wfree(scr->imctx);
    scr->imctx = NULL;
}



WMICContext*
W_CreateIC(WMView *view)
{
    WMScreen *scr = W_VIEW_SCREEN(view);
    WMICContext *ctx;

    ctx->prev = NULL;
    ctx->next = scr->imctx->icList;
    if (scr->imctx->icList)
        scr->imctx->icList->prev = ctx;


}


void
W_DestroyIC(WMICContext *ctx)
{
    XDestroyIC(ctx->xic);


}


int
W_LookupString(WMView *view, XKeyEvent *event,
               char buffer, int bufsize, KeySym ksym)
{

}



