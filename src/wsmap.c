
#include "WindowMaker.h"
#include "window.h"


typedef struct {
  WWindow *wwin;
  WMLabel *mini;
} WSMWindow;


typedef struct {
    WScreen *scr;

    WMWindow *win;

    WSMWindow *windows;
    int windowCount;

    int xcount, ycount;
    int wswidth, wsheight;
} WWorkspaceMap;


#define WSMAP_DEFAULT_WIDTH 150


static WWorkspaceMap *createWorkspaceMap(WMScreen *scr, int xcount, int ycount)
{
    WWorkspaceMap *wsm= wnew0(WWorkspaceMap, 1);
    WMRect rect;
    //
    rect.width= 1024;
    rect.height= 768;

    wsm->win= WMCreateWindow(scr, "wsmap");
    
    /* find out the ideal size of the mini-workspaces */

    wsm->wswidth = WSMAP_DEFAULT_WIDTH;
    wsm->wsheight = (wsm->wswidth*rect.height) / rect.width;

    // check if it fits screen

    wsm->xcount = xcount;
    wsm->ycount = ycount;

    
    
    return wsm;
}



static void handleEvent(WWorkspaceMap *map, XEvent *event)
{
  switch (event->type)
  {
  }
}



void wShowWorkspaceMap(WScreen *scr)
{
  
}



Display *dpy;

int main(int argc, char **argv)
{
    WWorkspaceMap *wsmap;
    WMScreen *scr;
    
    WMInitializeApplication("WSMap", &argc, argv);

    dpy = XOpenDisplay("");
    if (!dpy) {
        wfatal("cant open display");
        exit(0);
    }

    scr = WMCreateSimpleApplicationScreen(dpy);
    
    wsmap= createWorkspaceMap(scr);
    
    WMRealizeWidget(wsmap->win);
    WMMapWidget(wsmap->win);
  
    WMScreenMainLoop(scr);

    return 0;
}
