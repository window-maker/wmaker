/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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

#ifndef WMFUNCS_H_
#define WMFUNCS_H_

#include <sys/types.h>
#include <stdio.h>

#include "window.h"
#include "defaults.h"
#include "keybind.h"

typedef void (WCallBack)(void *cdata);
typedef void (WDeathHandler)(pid_t pid, unsigned int status, void *cdata);


/* ---[ main.c ]---------------------------------------------------------- */

int getWVisualID(int screen);


/* ---[ monitor.c ]------------------------------------------------------- */

int MonitorLoop(int argc, char **argv);


/* ---[ osdep_*.c ]------------------------------------------------------- */

Bool GetCommandForPid(int pid, char ***argv, int *argc);


/* ---[ switchmenu.c ]---------------------------------------------------- */

void UpdateSwitchMenu(WScreen *scr, WWindow *wwin, int action);
void OpenSwitchMenu(WScreen *scr, int x, int y, int keyboard);
void InitializeSwitchMenu(void);


/* ---[ winmenu.c ]------------------------------------------------------- */

void OpenWindowMenu(WWindow *wwin, int x, int y, int keyboard);
void OpenWindowMenu2(WWindow *wwin, int x, int y, int keyboard);
void OpenMiniwindowMenu(WWindow *wwin, int x, int y);
void CloseWindowMenu(WScreen *scr);
void DestroyWindowMenu(WScreen *scr);

#endif
