/*
 * WINGs demo: font lister
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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




#include <stdlib.h>
#include <stdio.h>
#include <WINGs/WINGs.h>
#include <WINGs/WUtil.h>

void
wAbort()
{
    exit(0);
}

void show(WMWidget *self, void *data)
{
    char buf[60];
    void *d;
    WMLabel *l = (WMLabel*)data;
    d = WMGetHangedData(self);
    sprintf(buf, "%i -  0x%x - 0%o", (int)d, (int)d, (int)d);
    WMSetLabelText(l, buf);
}

void quit(WMWidget *self, void *data)
{
    exit(0);
}


int
main(int argc, char **argv)
{
    Display *dpy;
    WMWindow *win;
    WMScreen *scr;
    WMButton *lab, *l0=NULL;
    WMLabel *pos;
    int x, y, c;
    char buf[20];

    WMInitializeApplication("FontView", &argc, argv);

    dpy = XOpenDisplay("");
    if (!dpy) {
	wfatal("cant open display");
	exit(0);
    }
    
    scr = WMCreateSimpleApplicationScreen(dpy);

    win = WMCreateWindow(scr, "main");
    WMResizeWidget(win, 20*33, 20+20*9);
    WMSetWindowTitle(win, "Font Chars");
    WMSetWindowCloseAction(win, quit, NULL);
    pos = WMCreateLabel(win);
    WMResizeWidget(pos, 20*33, 20);
    WMMoveWidget(pos, 10, 5);

    c = 0;
    for (y=0; y<8; y++) {
	for (x=0; x<32; x++, c++) {
	    lab = WMCreateCustomButton(win, WBBStateLightMask);
	    WMResizeWidget(lab, 20, 20);
	    WMMoveWidget(lab, 10+x*20, 30+y*20);
	    sprintf(buf, "%c", c);
	    WMSetButtonText(lab, buf);
	    WMSetButtonAction(lab, show, pos);
	    WMHangData(lab, (void*)c);
	    if (c>0) {
		WMGroupButtons(l0, lab);
	    } else {
		l0 = lab;
	    }
	}
    }
    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
    WMScreenMainLoop(scr);
    return 0;   
}

