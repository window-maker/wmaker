/* editmenu.h - editable menus
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1999 Alfredo K. Kojima
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



typedef struct W_EditMenu WEditMenu;

typedef struct W_EditMenuItem WEditMenuItem;


WEditMenuItem *WCreateEditMenuItem(WMWidget *parent, char *title);


WEditMenu *WCreateEditMenu(WMScreen *scr, char *title);

WEditMenuItem *WInsertMenuItemWithTitle(WEditMenu *mPtr, char *title,
					int index);

void WSetMenuSubmenu(WEditMenu *mPtr, WEditMenu *submenu, WEditMenuItem *item);

void WRemoveMenuItem(WEditMenu *mPtr, WEditMenuItem *item);
