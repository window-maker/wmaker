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

#include "WindowMaker.h"
#include "screen.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "defaults.h"
#include "switchpanel.h"
#include "funcs.h"

struct SwitchPanel {
  WScreen *scr;
  WMWindow *win;
  WMBox *hbox;
  WMLabel *label;
  WMArray *icons;
  WMArray *windows;
  int current;
  
  WMColor *normalColor;
  WMColor *selectColor;
  
  WMPixmap *defIcon;
};


extern WPreferences wPreferences;

#define ICON_IDEAL_SIZE 64
#define ICON_EXTRASPACE 4

static void addIconForWindow(WSwitchPanel *panel, WWindow *wwin, int iconWidth)
{
  WMLabel *label= WMCreateLabel(panel->hbox);
  WMAddBoxSubviewAtEnd(panel->hbox, WMWidgetView(label), False, True, iconWidth + ICON_EXTRASPACE, 0, 0);
  RImage *image = NULL;
  WMPixmap *pixmap;
  WMScreen *wscr = WMWidgetScreen(label);
  
  if (!WFLAGP(wwin, always_user_icon) && wwin->net_icon_image)
      image = RRetainImage(wwin->net_icon_image);
  if (!image)
    image = wDefaultGetImage(panel->scr, wwin->wm_instance, wwin->wm_class);

  if (image && (abs(image->width - iconWidth) > 2 || abs(image->height - iconWidth) > 2)) {
    RImage *nimage;
    nimage= RScaleImage(image, iconWidth, (image->height * iconWidth / image->width));
    RReleaseImage(image);
    image= nimage;
  }
  
  if (image) {
    pixmap= WMCreatePixmapFromRImage(wscr, image, 100);
    RReleaseImage(image);
  } else {
    if (!panel->defIcon)
    {
      char *file = wDefaultGetIconFile(panel->scr, NULL, NULL, False);
      if (file) {
        char *path = FindImage(wPreferences.icon_path, file);
        if (path) {
          image = RLoadImage(panel->scr->rcontext, path, 0);
          wfree(path);
          
          panel->defIcon= WMCreatePixmapFromRImage(wscr, image, 100);
          RReleaseImage(image);
        }
      }
    }

    if (panel->defIcon)
      pixmap= WMRetainPixmap(panel->defIcon);
    else
      pixmap= NULL;
  }

  if (pixmap) {
    WMSetLabelImage(label, pixmap);
    WMSetLabelImagePosition(label, WIPImageOnly);
    WMReleasePixmap(pixmap);
  }

  WMAddToArray(panel->icons, label);
}


WSwitchPanel *wInitSwitchPanel(WScreen *scr, int workspace)
{
  WWindow *wwin;
  WSwitchPanel *panel= wmalloc(sizeof(WSwitchPanel));
  int i;
  int width;
  int height;
  int iconWidth = ICON_IDEAL_SIZE;
  WMBox *vbox;
  
  panel->current= 0;
  panel->defIcon= NULL;

  panel->normalColor = WMGrayColor(scr->wmscreen);
  panel->selectColor = WMWhiteColor(scr->wmscreen);

  panel->scr= scr;
  panel->windows= WMCreateArray(10);

  for (wwin= scr->focused_window; wwin; wwin= wwin->prev) {
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
  
  if (iconWidth < 16 || WMGetArrayItemCount(panel->windows) == 0)
  {
    /* if there are too many windows, don't bother trying to show the panel */
    WMFreeArray(panel->windows);
    wfree(panel);
    return NULL;
  }

  height= iconWidth + 20 + 10 + ICON_EXTRASPACE;
  
  panel->icons= WMCreateArray(WMGetArrayItemCount(panel->windows));
  
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
  
  panel->hbox = WMCreateBox(vbox);
  WMSetBoxHorizontal(panel->hbox, True);
  WMAddBoxSubviewAtEnd(vbox, WMWidgetView(panel->hbox), True, True, 20, 0, 2);

  WM_ITERATE_ARRAY(panel->windows, wwin, i) {
      addIconForWindow(panel, wwin, iconWidth);
  }

  WMMapSubwidgets(panel->win);
  WMRealizeWidget(panel->win);
  WMMapWidget(panel->win);
  WMMoveWidget(panel->win,
               (WMScreenWidth(scr->wmscreen) - (width+10))/2,
               (WMScreenHeight(scr->wmscreen) - height)/2);

  WMSetWidgetBackgroundColor(WMGetFromArray(panel->icons, 0),
                             panel->selectColor);

  return panel;
}


void wSwitchPanelDestroy(WSwitchPanel *panel)
{
  WMDestroyWidget(panel->win);
  WMFreeArray(panel->icons);
  WMFreeArray(panel->windows);
  WMReleaseColor(panel->selectColor);
  WMReleaseColor(panel->normalColor);
  if (panel->defIcon)
    WMReleasePixmap(panel->defIcon);
  wfree(panel);
}


WWindow *wSwitchPanelSelectNext(WSwitchPanel *panel, int back)
{
  WWindow *wwin;
  int count = WMGetArrayItemCount(panel->windows);
  WMLabel *label;
  
  label= WMGetFromArray(panel->icons, panel->current);
  WMSetWidgetBackgroundColor(label, panel->normalColor);
  WMSetLabelRelief(label, WRFlat);

  if (!back)
    panel->current = (count + panel->current - 1) % count;
  else
    panel->current = (panel->current + 1) % count;

  wwin = WMGetFromArray(panel->windows, panel->current);
  
  WMSetLabelText(panel->label, wwin->frame->title);

  label= WMGetFromArray(panel->icons, panel->current);
  WMSetWidgetBackgroundColor(label, panel->selectColor);
  WMSetLabelRelief(label, WRSimple);
  
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


