/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2004 Alfredo K. Kojima
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#include "wconfig.h"

#include <stdlib.h>
#include <string.h>

#include "WindowMaker.h"
#include "screen.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "defaults.h"
#include "switchpanel.h"
#include "funcs.h"
#include "xinerama.h"


static char * tile_xpm[] = {
"64 64 2 1",
" 	c None",
".	c #FFFFFF",
"       .................................................        ",
"     ......................................................     ",
"   .........................................................    ",
"  ...........................................................   ",
"  ............................................................  ",
" .............................................................. ",
" .............................................................. ",
"............................................................... ",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
"................................................................",
" .............................................................. ",
" .............................................................. ",
" .............................................................. ",
"  ............................................................  ",
"   ..........................................................   ",
"    ........................................................    ",
"     ......................................................     ",
"        ................................................        "};


struct SwitchPanel {
  WScreen *scr;
  WMWindow *win;
  WMBox *hbox;
  WMLabel *label;
  WMArray *icons;
  WMArray *images;
  WMArray *windows;
  int current;
    
  RImage *defIcon;
  
  RImage *tile;
};





extern WPreferences wPreferences;

#define ICON_IDEAL_SIZE 48
#define ICON_EXTRASPACE 16

static void changeImage(WSwitchPanel *panel, int index, int selected)
{
  WMPixmap *pixmap= NULL;
  WMLabel *label = WMGetFromArray(panel->icons, index);
  RImage *image= WMGetFromArray(panel->images, index);
  WMScreen *wscr = WMWidgetScreen(label);

  if (image) {
    RColor bgColor;
    RImage *back;

    if (selected) {
      back= RCloneImage(panel->tile);
      RCombineArea(back, image, 0, 0, image->width, image->height,
                   (back->width - image->width)/2, (back->height - image->height)/2);
      
      pixmap= WMCreatePixmapFromRImage(wscr, back, -1);
      RReleaseImage(back);
    } else {
      bgColor.alpha= 100;
      bgColor.red = WMRedComponentOfColor(WMGrayColor(wscr))>>8;
      bgColor.green = WMGreenComponentOfColor(WMGrayColor(wscr))>>8;
      bgColor.blue = WMBlueComponentOfColor(WMGrayColor(wscr))>>8;

      image= RCloneImage(image);
      RCombineImageWithColor(image, &bgColor);
      
      pixmap= WMCreatePixmapFromRImage(wscr, image, -1);
      RReleaseImage(image);
    }
  }

  if (pixmap) {
    WMSetLabelImage(label, pixmap);
    WMSetLabelImagePosition(label, WIPImageOnly);
    WMReleasePixmap(pixmap);
  }
}


static void addIconForWindow(WSwitchPanel *panel, WWindow *wwin, int iconWidth)
{
  WMLabel *label= WMCreateLabel(panel->hbox);
  WMAddBoxSubviewAtEnd(panel->hbox, WMWidgetView(label), False, True, iconWidth + ICON_EXTRASPACE, 0, 0);
  RImage *image = NULL;

  if (!WFLAGP(wwin, always_user_icon) && wwin->net_icon_image)
      image = RRetainImage(wwin->net_icon_image);
  if (!image)
      image = wDefaultGetImage(panel->scr, wwin->wm_instance, wwin->wm_class);

  if (!image && !panel->defIcon)
  {
    char *file = wDefaultGetIconFile(panel->scr, NULL, NULL, False);
    if (file) {
      char *path = FindImage(wPreferences.icon_path, file);
      if (path) {
        image = RLoadImage(panel->scr->rcontext, path, 0);
        wfree(path);
      }
    }
  }
  if (!image && panel->defIcon)
    image= RRetainImage(panel->defIcon);

  if (image && ((image->width - iconWidth) > 2 || (image->height - iconWidth) > 2)) {
    RImage *nimage;
    nimage= RScaleImage(image, iconWidth, (image->height * iconWidth / image->width));
    RReleaseImage(image);
    image= nimage;
  }

  WMAddToArray(panel->images, image);
  WMAddToArray(panel->icons, label);
}


WSwitchPanel *wInitSwitchPanel(WScreen *scr, WWindow *curwin, int workspace)
{
    WWindow *wwin;
    WSwitchPanel *panel= wmalloc(sizeof(WSwitchPanel));
    int i;
    int width;
    int height;
    int iconWidth = ICON_IDEAL_SIZE;
    WMBox *vbox;
  
    memset(panel, 0, sizeof(WSwitchPanel));

    panel->scr= scr;
    panel->windows= WMCreateArray(10);
    
    for (wwin= curwin; wwin; wwin= wwin->prev) {
        if (wwin->frame->workspace == workspace &&  wWindowCanReceiveFocus(wwin) &&
            (!WFLAGP(wwin, skip_window_list) || wwin->flags.internal_window)) {
            WMInsertInArray(panel->windows, 0, wwin);
        }
    }
    wwin = curwin;
    /* start over from the beginning of the list */
    while (wwin->next)
      wwin = wwin->next;

    for (wwin= curwin; wwin && wwin != curwin; wwin= wwin->prev) {
        if (wwin->frame->workspace == workspace &&  wWindowCanReceiveFocus(wwin) &&
            (!WFLAGP(wwin, skip_window_list) || wwin->flags.internal_window)) {
            WMInsertInArray(panel->windows, 0, wwin);
        }
    }

    width= (iconWidth + ICON_EXTRASPACE)*WMGetArrayItemCount(panel->windows);
    
    if (width > WMScreenWidth(scr->wmscreen))
    {
        width= WMScreenWidth(scr->wmscreen) - 100;
        iconWidth = width / WMGetArrayItemCount(panel->windows) - ICON_EXTRASPACE;
    }
    
    if (WMGetArrayItemCount(panel->windows) == 0) {
        WMFreeArray(panel->windows);
        wfree(panel);
        return NULL;
    }
    
    if (iconWidth < 16) {
        /* if there are too many windows, don't bother trying to show the panel */

        return panel; 
    }
    
    height= iconWidth + 20 + 10 + ICON_EXTRASPACE;
    
    panel->icons= WMCreateArray(WMGetArrayItemCount(panel->windows));
    panel->images= WMCreateArray(WMGetArrayItemCount(panel->windows));
    
    panel->win = WMCreateWindow(scr->wmscreen, "");
    WMResizeWidget(panel->win, width + 10, height);
    
    {
        WMFrame *frame = WMCreateFrame(panel->win);
        WMSetFrameRelief(frame, WRSimple);
        WMSetViewExpandsToParent(WMWidgetView(frame), 0, 0, 0, 0);
        
        vbox = WMCreateBox(panel->win);
    }
    WMSetViewExpandsToParent(WMWidgetView(vbox), 5, 5, 5, 5);
    WMSetBoxHorizontal(vbox, False);

    panel->label = WMCreateLabel(vbox);
    WMAddBoxSubviewAtEnd(vbox, WMWidgetView(panel->label), False, True, 20, 0, 0);
    if (scr->focused_window && scr->focused_window->frame->title)
      WMSetLabelText(panel->label, scr->focused_window->frame->title);
    else
      WMSetLabelText(panel->label, "");

    {
        WMColor *color;
        WMFont *boldFont= WMBoldSystemFontOfSize(scr->wmscreen, 12);
        
        WMSetLabelRelief(panel->label, WRSimple);
        WMSetLabelFont(panel->label, boldFont);
        color = WMDarkGrayColor(scr->wmscreen);
        WMSetWidgetBackgroundColor(panel->label, color); 
        WMReleaseColor(color);
        color = WMWhiteColor(scr->wmscreen);
        WMSetLabelTextColor(panel->label, color);
        WMReleaseColor(color);
        
        WMReleaseFont(boldFont);
    }
  
    {
        RImage *tmp= NULL;
        RColor bgColor;
        char *path = FindImage(wPreferences.pixmap_path, "swtile.png");
        if (path) {
            tmp = RLoadImage(panel->scr->rcontext, path, 0);
            wfree(path);
        }
        
        if (!tmp)
          tmp= RGetImageFromXPMData(scr->rcontext, tile_xpm);
        
        panel->tile = RScaleImage(tmp, iconWidth+ICON_EXTRASPACE, iconWidth+ICON_EXTRASPACE);
        
        bgColor.alpha = 255;
        bgColor.red = WMRedComponentOfColor(WMGrayColor(scr->wmscreen))>>8;
        bgColor.green = WMGreenComponentOfColor(WMGrayColor(scr->wmscreen))>>8;
        bgColor.blue = WMBlueComponentOfColor(WMGrayColor(scr->wmscreen))>>8;
        
        RCombineImageWithColor(panel->tile, &bgColor);
        
        RReleaseImage(tmp);
    }
  
    panel->hbox = WMCreateBox(vbox);
    WMSetBoxHorizontal(panel->hbox, True);
    WMAddBoxSubviewAtEnd(vbox, WMWidgetView(panel->hbox), True, True, 20, 0, 4);
    
    WM_ITERATE_ARRAY(panel->windows, wwin, i) {
        addIconForWindow(panel, wwin, iconWidth);
        changeImage(panel, i, 0);
    }
    
    WMMapSubwidgets(panel->win);
    WMRealizeWidget(panel->win);
    WMMapWidget(panel->win);
    {
        WMPoint center;
        
        center= wGetPointToCenterRectInHead(scr, wGetHeadForPointerLocation(scr),
                                            width+10, height);
        WMMoveWidget(panel->win, center.x, center.y);
    }

    panel->current= WMGetFirstInArray(panel->windows, curwin);
    changeImage(panel, panel->current, 1);
    
    return panel;
}


void wSwitchPanelDestroy(WSwitchPanel *panel)
{
  int i;
  RImage *image;

  if (panel->images) {
    WM_ITERATE_ARRAY(panel->images, image, i) {
      if (image)
        RReleaseImage(image);
    }
    WMFreeArray(panel->images);
  }
  if (panel->win)
    WMDestroyWidget(panel->win);
  if (panel->icons)
    WMFreeArray(panel->icons);
  WMFreeArray(panel->windows);
  if (panel->defIcon)
    RReleaseImage(panel->defIcon);
  if (panel->tile)
    RReleaseImage(panel->tile);
  wfree(panel);
}


WWindow *wSwitchPanelSelectNext(WSwitchPanel *panel, int back)
{
  WWindow *wwin;
  int count = WMGetArrayItemCount(panel->windows);

  if (panel->win)
    changeImage(panel, panel->current, 0);
  
  if (!back)
    panel->current = (count + panel->current - 1) % count;
  else
    panel->current = (panel->current + 1) % count;

  wwin = WMGetFromArray(panel->windows, panel->current);

  if (panel->win) {
    WMSetLabelText(panel->label, wwin->frame->title);

    changeImage(panel, panel->current, 1);
  }
  return wwin;
}



WWindow *wSwitchPanelHandleEvent(WSwitchPanel *panel, XEvent *event)
{
  WMLabel *label;
  int i;

  printf("%i %i\n", event->xmotion.x, event->xmotion.y);
  
  WM_ITERATE_ARRAY(panel->icons, label, i) {
    if (WMWidgetXID(label) == event->xmotion.subwindow)
      puts("HOORAY");
  }
  
  return NULL;
}


