/* WPrefs.h- general definitions
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1998, 1999 Alfredo K. Kojima
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

#ifndef WPREFS_H_
#define WPREFS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <X11/Xlib.h>

#include <wraster.h>

#include <WINGs/WINGs.h>

/* For EXTENDED_WINDOWSHORTCUT */
#include "../src/wconfig.h"

/****/

#define WVERSION	"0.45"
#define WMVERSION	"0.65.x"

extern char *NOptionValueChanged;

typedef struct _Panel Panel;

typedef struct {
    unsigned flags;		       /* reserved for WPrefs.c Don't access it */
    
    void (*createWidgets)(Panel*);     /* called when showing for first time */
    void (*updateDomain)(Panel*);      /* save the changes to the dictionary */
    Bool (*requiresRestart)(Panel*);   /* return True if some static option was changed */
    void (*undoChanges)(Panel*);       /* reset values to those in the dictionary */
    void (*prepareForClose)(Panel*);   /* called when exiting WPrefs */
    void (*showPanel)(Panel*);	       /* called when entering the panel */
    void (*hidePanel)(Panel*);	       /* called when exiting the panel */
} CallbackRec;


/* all Panels must start with the following layout */
typedef struct PanelRec {
    WMBox *box;

    char *sectionName;		       /* section name to display in titlebar */

    char *description;

    CallbackRec callbacks;
} PanelRec;



void AddSection(Panel *panel, char *iconFile);

char *LocateImage(char *name);

void SetButtonAlphaImage(WMScreen *scr, WMButton *bPtr, char *file,
                         char *title1, char *title2);

WMWindow *GetWindow(Panel *panel);

/* manipulate the dictionary for the WindowMaker domain */

WMPropList *GetObjectForKey(char *defaultName);

void SetObjectForKey(WMPropList *object, char *defaultName);

void RemoveObjectForKey(char *defaultName);

char *GetStringForKey(char *defaultName);

int GetIntegerForKey(char *defaultName);

Bool GetBoolForKey(char *defaultName);

int GetSpeedForKey(char *defaultName);

void SetIntegerForKey(int value, char *defaultName);

void SetStringForKey(char *value, char *defaultName);

void SetBoolForKey(Bool value, char *defaultName);

void SetSpeedForKey(int speed, char *defaultName);



void AddDeadChildHandler(pid_t pid, void (*handler)(void*), void *data);


#define FRAME_TOP	105
#define FRAME_LEFT	-2
#define FRAME_WIDTH	524
#define FRAME_HEIGHT	235



/*
 * Needed for HAVE_LIBINTL_H
 */
#include "../src/config.h"

#if HAVE_LIBINTL_H && I18N
# include <libintl.h>
# define _(text) gettext(text)
# define N_(text) (text)
#else
# define _(text) (text)
# define N_(text) (text)
#endif

#endif




