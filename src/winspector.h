/* winspector.h - window attribute inspector header file
 * 
 *  Window Maker window manager
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

#ifndef WINSPECTOR_H_
#define WINSPECTOR_H_

#include "window.h"

typedef struct InspectorPanel {
    struct InspectorPanel *nextPtr;

    WWindow *frame;

    WWindow *inspected;       /* the window that's being inspected */

    WMWindow *win;
    
    Window parent;

    /* common stuff */
    WMButton *revertBtn;
    WMButton *applyBtn;
    WMButton *saveBtn;

    WMPopUpButton *pagePopUp;

    /* first page. general stuff */
    
    WMFrame *specFrm;
    WMButton *instRb;
    WMButton *clsRb;
    WMButton *bothRb;
    WMButton *defaultRb;

    WMLabel *specLbl;
    
    /* second page. attributes */

    WMFrame *attrFrm;
    WMButton *attrChk[10];

    /* 3rd page. more attributes */
    WMFrame *moreFrm;
    WMButton *moreChk[8];
    
    WMLabel *moreLbl;

    /* 4th page. icon and workspace */
    WMFrame *iconFrm;
    WMLabel *iconLbl;
    WMLabel *fileLbl;
    WMTextField *fileText;
    WMButton *alwChk;
    /*
    WMButton *updateIconBtn;
     */
    WMButton *browseIconBtn;
    
    WMFrame *wsFrm;
    WMButton *curRb;
    WMButton *setRb;
    WMTextField *wsText;
    
    /* 5th page. application wide attributes */
    WMFrame *appFrm;
    WMButton *appChk[2];

    unsigned int done:1;
    unsigned int destroyed:1;
    unsigned int choosingIcon:1;
} InspectorPanel;



void wShowInspectorForWindow(WWindow *wwin);
void wDestroyInspectorPanels();

#endif
