/*
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <unistd.h>

#include <wraster.h>

#include "WindowMaker.h"
#include "screen.h"
#include "superfluous.h"
#include "dock.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"


extern WPreferences wPreferences;


#ifdef SPEAKER_SOUND
static void
play(Display *dpy, int pitch, int delay)
{
    XKeyboardControl kc;
    
    kc.bell_pitch = pitch;
    kc.bell_percent = 50;
    kc.bell_duration = delay;
    XChangeKeyboardControl(dpy, KBBellPitch|KBBellDuration|KBBellPercent,&kc);
    XBell(dpy, 50);
    XFlush(dpy);
    wusleep(delay);
}
#endif


void
DoKaboom(WScreen *scr, Window win, int x, int y)
{
    int i, j, k;
    int sw=scr->scr_width, sh=scr->scr_height;
    short px[PIECES], py[PIECES];
    char pvx[PIECES], pvy[PIECES];
    char ax[PIECES], ay[PIECES];
    Pixmap tmp;
    XImage *img;
    
    img=XGetImage(dpy, win, 0, 0, wPreferences.icon_size, 
		  wPreferences.icon_size, AllPlanes, ZPixmap);
    XUnmapWindow(dpy,win);
    if (!img)
      return;
    XSetClipMask(dpy, scr->copy_gc, None);
    tmp = XCreatePixmap(dpy, scr->root_win, wPreferences.icon_size, 
			wPreferences.icon_size, scr->depth);
    XPutImage(dpy, tmp, scr->copy_gc, img, 0, 0, 0, 0, wPreferences.icon_size,
	      wPreferences.icon_size);
    XDestroyImage(img);
    for (i=0,k=0; i<wPreferences.icon_size/PSIZE; i++) {
	for (j=0; j<wPreferences.icon_size/PSIZE; j++) {
	    if (rand()%3) {
		ax[k]=i;
		ay[k]=j;
		px[k]=x+i*PSIZE;
		py[k]=y+j*PSIZE;
		pvx[k]=rand()%7-3;
#ifdef SPREAD_ICON
                pvy[k]=rand()%7-3;
#else
                pvy[k]=-12-rand()%6;
#endif
		k++;
	    } else {
		ax[k]=-1;
	    }
	}
    }
    j=k;
    while (k>0) {
	for (i=0; i<j; i++) {
	    if (ax[i]>=0) {
		px[i]+=pvx[i];
		py[i]+=pvy[i];
#ifdef SPREAD_ICON
                pvx[i]+=(pvx[i]>0 ? 2 : -2);
                pvy[i]+=(pvy[i]>0 ? 2 : -2);
                /* The following are nice, but too slow.
                 * The animation can have an unknown duration, depending
                 * on what rand() returns. Until the animation ends, the user
                 * cannot do anything, which is not too good. -Dan
                 */
                /*pvx[i]+=rand()%5-2;
                pvy[i]+=rand()%5-2;*/

#else
                pvy[i]++;
#endif
                if (px[i]<-wPreferences.icon_size || px[i]>sw || py[i]>sh
#ifdef SPREAD_ICON
                    || px[i]<0 || py[i]<0
#endif
                   ) {
                    ax[i]=-1;
		    k--;
		} else {
		    XCopyArea(dpy, tmp, scr->root_win, scr->copy_gc, 
			      ax[i]*PSIZE, ay[i]*PSIZE, 
			      PSIZE, PSIZE, px[i], py[i]);
		}
	    }
	}
#ifdef SPEAKER_SOUND
	play(dpy, 100+rand()%250, 12);
#else
# if (MINIATURIZE_ANIMATION_DELAY_Z > 0)
	wusleep(MINIATURIZE_ANIMATION_DELAY_Z*2);
# endif
#endif
	for (i=0; i<j; i++) {
	    if (ax[i]>=0) {
		XClearArea(dpy, scr->root_win, px[i], py[i], PSIZE, PSIZE, 1);
	    }
	}
    }
    XFreePixmap(dpy, tmp);
}



Pixmap
MakeGhostDock(WDock *dock, int sx, int dx, int y)
{
    WScreen *scr = dock->screen_ptr;
    XImage *img;
    RImage *back, *dock_image;
    Pixmap pixmap;
    int i, virtual_tiles, h, j, n;
    unsigned long red_mask, green_mask, blue_mask;

    virtual_tiles = 0;
    for (i=0; i<dock->max_icons; i++) {
        if (dock->icon_array[i]!=NULL &&
            dock->icon_array[i]->yindex > virtual_tiles)
            virtual_tiles = dock->icon_array[i]->yindex;
    }
    virtual_tiles++;
    h = virtual_tiles * wPreferences.icon_size;
    h = (y + h > scr->scr_height) ? scr->scr_height-y : h;
    virtual_tiles = h / wPreferences.icon_size; /* The visible ones */
    if (h % wPreferences.icon_size)
        virtual_tiles++;  /* There is one partially visible tile at end */
    
    img=XGetImage(dpy, scr->root_win, dx, y, wPreferences.icon_size, h,
                  AllPlanes, ZPixmap);
    if (!img)
      return None;

    red_mask = img->red_mask;
    green_mask = img->green_mask;
    blue_mask = img->blue_mask;
    
    back = RCreateImageFromXImage(scr->rcontext, img, NULL);
    XDestroyImage(img);
    if (!back) {
	return None;
    }

    for (i=0;i<dock->max_icons;i++) {
        if (dock->icon_array[i]!=NULL &&
            dock->icon_array[i]->yindex < virtual_tiles) {
            Pixmap which;
            j = dock->icon_array[i]->yindex * wPreferences.icon_size;
            n = (h - j < wPreferences.icon_size) ? h - j :
                                                   wPreferences.icon_size;
            if (dock->icon_array[i]->icon->pixmap)
                which = dock->icon_array[i]->icon->pixmap;
            else
                which = dock->icon_array[i]->icon->core->window;

            img=XGetImage(dpy, which, 0, 0, wPreferences.icon_size, n,
                          AllPlanes, ZPixmap);

            if (!img){
                RDestroyImage(back);
                return None;
            }
            img->red_mask = red_mask;
            img->green_mask = green_mask;
            img->blue_mask = blue_mask;

            dock_image = RCreateImageFromXImage(scr->rcontext, img, NULL);
            XDestroyImage(img);
            if (!dock_image) {
                RDestroyImage(back);
                return None;
            }
            RCombineAreaWithOpaqueness(back, dock_image, 0, 0,
                                       wPreferences.icon_size, n,
                                       0, j, 30 * 256 / 100);
            RDestroyImage(dock_image);
        }
    }


    RConvertImage(scr->rcontext, back, &pixmap);
    
    RDestroyImage(back);
    
    return pixmap;
}


Pixmap
MakeGhostIcon(WScreen *scr, Drawable drawable)
{
    RImage *back;
    RColor color;
    Pixmap pixmap;


    if (!drawable)
	return None;
    
    back = RCreateImageFromDrawable(scr->rcontext, drawable, None);
    if (!back) 
	return None;
    
    color.red = 0xff;
    color.green = 0xff;
    color.blue = 0xff;
    color.alpha = 200;

    RClearImage(back, &color);
    RConvertImage(scr->rcontext, back, &pixmap);
    
    RDestroyImage(back);
    
    return pixmap;
}



#ifdef WINDOW_BIRTH_ZOOM
void
DoWindowBirth(WWindow *wwin)
{
    int width = wwin->frame->core->width;
    int height = wwin->frame->core->height;
    int w = WMIN(width, 20);
    int h = WMIN(height, 20); 
    int x, y;
    int dw, dh;
    int i;

    dw = (width-w)/WINDOW_BIRTH_STEPS;
    dh = (height-h)/WINDOW_BIRTH_STEPS;

    x = wwin->frame_x + (width-w)/2;
    y = wwin->frame_y + (height-h)/2;

    XMoveResizeWindow(dpy, wwin->frame->core->window, x, y, w, h);
    
    XMapWindow(dpy, wwin->frame->core->window);
    
    XFlush(dpy);
    for (i=0; i<WINDOW_BIRTH_STEPS; i++) {
	x -= dw/2 + dw%2;
	y -= dh/2 + dh%2;
	w += dw;
	h += dh;
	XMoveResizeWindow(dpy, wwin->frame->core->window, x, y, w, h);
	XFlush(dpy);
    }
       
    XMoveResizeWindow(dpy, wwin->frame->core->window,
		      wwin->frame_x, wwin->frame_y, width, height);
    XFlush(dpy);
}
#else
void
DoWindowBirth(WWindow *wwin)
{
    /* dummy stub */
}
#endif

