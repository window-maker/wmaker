/* rootmenu.h- user defined menu
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 2000-2003 Alfredo K. Kojima
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef WMROOTMENU_H
#define WMROOTMENU_H

#include "WindowMaker.h"
#include "screen.h"
#include "menu.h"

typedef void *WRootMenuData;

typedef struct _WRootMenuReader {
    Bool (*checkMenuChange)(char *path, time_t lastAccessTime);

    WRootMenuData (*openMenuFile)(char *path);
    Bool (*hasMoreData)(WRootMenuData *data);
    Bool (*nextCommand)(WRootMenuData *data,
                        char **title,
                        char **command,
                        char **parameter,
                        char **shortcut);
    void (*closeMenuFile)(WRootMenuData *data);
} WRootMenuReader;

void rebind_key_grabs(WScreen *scr);
WMenu *configureMenu(WScreen *scr, WMPropList *definition, Bool includeGlobals);

#endif /* WMROOTMENU_H */
