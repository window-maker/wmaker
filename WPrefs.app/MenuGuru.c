/* MenuGuru.c- OPEN_MENU definition "guru" assistant
 * 
 *  WPrefs - Window Maker Preferences Program
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


#include "WPrefs.h"
#include <assert.h>
#include <ctype.h>


typedef struct _MenuGuru {
    WMWindow *win;

    WMButton *nextB;
    WMButton *backB;
    WMButton *cancelB;
    
    WMLabel *typetopL;
    WMButton *typefB;
    WMButton *typepB;
    WMButton *typedB;
    
    WMLabel *pathtopL;
    WMTextField *pathT;
    WMButton *pathB;
    WMLabel *pathbotL;
    
    WMLabel *pipetopL;
    WMTextField *pipeT;
    WMLabel *pipebotL;
    
    WMLabel *dirtopL;
    WMTextField *dirT;
    WMLabel *dirbotL;
    
    WMLabel *progtopL;
    WMTextField *progT;
    WMLabel *progbotL;
    

    char ok;
    char end;
    int section;
} MenuGuru;


enum {
    GSelectType,
	GSelectFile,
	GSelectPaths,
	GSelectPipe,
	GSelectProgram,
	GDone
};



static char*
trimstr(char *str)
{
    char *p = str;
    int i;
    
    while (isspace(*p)) p++;
    p = wstrdup(p);
    i = strlen(p);
    while (isspace(p[i]) && i>0) {
	p[i] = 0;
	i--;
    }
    
    return p;
}


static void
showPart(MenuGuru *panel, int part)
{
    WMUnmapSubwidgets(panel->win);
    WMMapWidget(panel->nextB);
    WMMapWidget(panel->backB);
    WMMapWidget(panel->cancelB);

    WMSetButtonEnabled(panel->backB, part!=GSelectType);

    switch (part) {
     case GSelectType:
	WMSetWindowTitle(panel->win, _("Menu Guru - Select Type"));
	WMMapWidget(panel->typetopL);
	WMMapWidget(panel->typedB);
	WMMapWidget(panel->typepB);
	WMMapWidget(panel->typefB);
	WMSetButtonText(panel->nextB, _("Next"));
	break;
     case GSelectFile:
	WMSetWindowTitle(panel->win, _("Menu Guru - Select Menu File"));
	WMMapWidget(panel->pathtopL);
	WMMapWidget(panel->pathT);
/*	WMMapWidget(panel->pathB);*/
	WMMapWidget(panel->pathbotL);
	WMSetButtonText(panel->nextB, _("OK"));
	break;
     case GSelectPipe:
	WMSetWindowTitle(panel->win, _("Menu Guru - Select Pipe Command"));
	WMMapWidget(panel->pipetopL);
	WMMapWidget(panel->pipeT);
	WMMapWidget(panel->pipebotL);
	WMSetButtonText(panel->nextB, _("OK"));
	break;
     case GSelectPaths:
	WMSetWindowTitle(panel->win, _("Menu Guru - Select Directories"));
	WMMapWidget(panel->dirtopL);
	WMMapWidget(panel->dirT);
	WMMapWidget(panel->dirbotL);
	WMSetButtonText(panel->nextB, _("Next"));
	break;
     case GSelectProgram:
	WMSetWindowTitle(panel->win, _("Menu Guru - Select Command"));
	WMMapWidget(panel->progtopL);
	WMMapWidget(panel->progT);
	WMMapWidget(panel->progbotL);
	WMSetButtonText(panel->nextB, _("OK"));
	break;
    }
    panel->section = part;
}


static void
clickNext(WMWidget *w, void *data)
{
    MenuGuru *panel = (MenuGuru*)data;
    char *tmp, *p;

    switch (panel->section) {
     case GSelectType:
	if (WMGetButtonSelected(panel->typefB)) {
	    showPart(panel, GSelectFile);
	} else if (WMGetButtonSelected(panel->typepB)) {
	    showPart(panel, GSelectPipe);
	} else {
	    showPart(panel, GSelectPaths);
	}
	break;
     case GSelectFile:
	tmp = WMGetTextFieldText(panel->pathT);
	p = trimstr(tmp); free(tmp);
	if (strlen(p)==0) {
	    free(p);
	    return;
	}
	free(p);
	panel->ok = 1;
	panel->end = 1;
	break;
     case GSelectPaths:
	tmp = WMGetTextFieldText(panel->dirT);
	p = trimstr(tmp); free(tmp);
	if (strlen(p)==0) {
	    free(p);
	    return;
	}
	free(p);	
	showPart(panel, GSelectProgram);
	break;
     case GSelectPipe:
	tmp = WMGetTextFieldText(panel->pipeT);
	p = trimstr(tmp); free(tmp);
	if (strlen(p)==0) {
	    free(p);
	    return;
	}
	free(p);
	panel->ok = 1;
	panel->end = 1;
	break;
     case GSelectProgram:
	panel->ok = 1;
	panel->end = 1;	
	break;
     default:
	panel->end = 1;
    }
}



static void
clickBack(WMWidget *w, void *data)
{
    MenuGuru *panel = (MenuGuru*)data;
    int newSection;
    
    switch (panel->section) {
     case GSelectFile:
	newSection = GSelectType;
	break;
     case GSelectPipe:
	newSection = GSelectType;
	break;
     case GSelectPaths:
	newSection = GSelectType;
	break;
     case GSelectProgram:
	newSection = GSelectPaths;
	break;
     default:
	newSection = panel->section;
    }
    showPart(panel, newSection);
}



static void
closeWindow(WMWidget *w, void *data)
{
    MenuGuru *panel = (MenuGuru*)data;

    panel->end = 1;
}


static void
createPanel(WMWindow *mainWindow, MenuGuru *panel)
{
    panel->win = WMCreatePanelForWindow(mainWindow, "menuGuru");
    WMResizeWidget(panel->win, 370, 220);
    
    panel->nextB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->nextB, 80, 24);
    WMMoveWidget(panel->nextB, 280, 185);
    WMSetButtonText(panel->nextB, _("Next"));
    WMSetButtonAction(panel->nextB, clickNext, panel);

    panel->backB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->backB, 80, 24);
    WMMoveWidget(panel->backB, 195, 185);
    WMSetButtonText(panel->backB, _("Back"));
    WMSetButtonAction(panel->backB, clickBack, panel);
    
    panel->cancelB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancelB, 80, 24);
    WMMoveWidget(panel->cancelB, 110, 185);
    WMSetButtonText(panel->cancelB, _("Cancel"));
    WMSetButtonAction(panel->cancelB, closeWindow, panel);
    
    /**/
    
    panel->typetopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->typetopL, 350, 45);
    WMMoveWidget(panel->typetopL, 10, 10);
    WMSetLabelText(panel->typetopL, _("This process will help you create a "
		  "submenu which definition is located in another file "
		  "or is created dynamically.\nWhat do you want to use as the "
		  "contents of the submenu?"));
    
    panel->typefB = WMCreateRadioButton(panel->win);
    WMResizeWidget(panel->typefB, 330, 35);
    WMMoveWidget(panel->typefB, 20, 65);
    WMSetButtonText(panel->typefB, _("A file containing the menu definition "
		    "in the plain text (non-property list) menu format."));

    panel->typepB = WMCreateRadioButton(panel->win);
    WMResizeWidget(panel->typepB, 330, 35);
    WMMoveWidget(panel->typepB, 20, 105);
    WMSetButtonText(panel->typepB, _("The menu definition generated by a "
		    "script/program read through a pipe."));

    panel->typedB = WMCreateRadioButton(panel->win);
    WMResizeWidget(panel->typedB, 330, 35);
    WMMoveWidget(panel->typedB, 20, 140);
    WMSetButtonText(panel->typedB, _("The files in one or more directories."));

    WMGroupButtons(panel->typefB, panel->typepB);
    WMGroupButtons(panel->typefB, panel->typedB);
    
    WMPerformButtonClick(panel->typefB);
    
    /**/
    
    panel->pathtopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->pathtopL, 330, 20);
    WMMoveWidget(panel->pathtopL, 20, 25);
    WMSetLabelText(panel->pathtopL, _("Type the path for the menu file:"));
    
    panel->pathT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->pathT, 330, 20);
    WMMoveWidget(panel->pathT, 20, 50);
/*
    panel->pathB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->pathB, 70, 24);
    WMMoveWidget(panel->pathB, 275, 75);
    WMSetButtonText(panel->pathB, _("Browse"));
 */
    
    panel->pathbotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->pathbotL, 330, 80);
    WMMoveWidget(panel->pathbotL, 20, 100);
    WMSetLabelText(panel->pathbotL, _("The menu file must contain a menu "
		   "in the plain text menu file format. This format is "
		   "described in the menu files included with WindowMaker, "
		   "probably at ~/GNUstep/Library/WindowMaker/menu"));
    
    /**/
    
    panel->pipetopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->pipetopL, 330, 32);
    WMMoveWidget(panel->pipetopL, 20, 20);
    WMSetLabelText(panel->pipetopL, _("Type the command that will generate "
		   "the menu definition:"));
    
    panel->pipeT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->pipeT, 330, 20);
    WMMoveWidget(panel->pipeT, 20, 55);

    panel->pipebotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->pipebotL, 330, 80);
    WMMoveWidget(panel->pipebotL, 20, 85);
    WMSetLabelText(panel->pipebotL, _("The command supplied must generate and "
		   "output a valid menu definition to stdout. This definition "
		   "should be in the plain text menu file format, described "
		   "in the menu files included with WindowMaker, usually "
		   "at ~/GNUstep/Library/WindowMaker/menu"));
    
    
    /**/
    
    panel->dirtopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirtopL, 330, 32);
    WMMoveWidget(panel->dirtopL, 20, 20);
    WMSetLabelText(panel->dirtopL, _("Type the path for the directory. You "
		   "can type more than one path by separating them with "
		   "spaces."));

    panel->dirT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->dirT, 330, 20);
    WMMoveWidget(panel->dirT, 20, 55);

    panel->dirbotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirbotL, 330, 80);
    WMMoveWidget(panel->dirbotL, 20, 85);
    WMSetLabelText(panel->dirbotL, _("The menu generated will have an item "
		   "for each file in the directory. The directories can "
		   "contain program executables or data files (such as "
		   "jpeg images)."));
    

    /**/
    
    panel->dirtopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirtopL, 330, 32);
    WMMoveWidget(panel->dirtopL, 20, 20);
    WMSetLabelText(panel->dirtopL, _("Type the path for the directory. You "
		   "can type more than one path by separating them with "
		   "spaces."));

    panel->dirT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->dirT, 330, 20);
    WMMoveWidget(panel->dirT, 20, 55);

    panel->dirbotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirbotL, 330, 80);
    WMMoveWidget(panel->dirbotL, 20, 85);
    WMSetLabelText(panel->dirbotL, _("The menu generated will have an item "
		   "for each file in the directory. The directories can "
		   "contain program executables or data files (such as "
		   "jpeg images)."));
    

    /**/
    
    panel->dirtopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirtopL, 330, 32);
    WMMoveWidget(panel->dirtopL, 20, 20);
    WMSetLabelText(panel->dirtopL, _("Type the path for the directory. You "
		   "can type more than one path by separating them with "
		   "spaces."));

    panel->dirT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->dirT, 330, 20);
    WMMoveWidget(panel->dirT, 20, 60);

    panel->dirbotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirbotL, 330, 80);
    WMMoveWidget(panel->dirbotL, 20, 85);
    WMSetLabelText(panel->dirbotL, _("The menu generated will have an item "
		   "for each file in the directory. The directories can "
		   "contain program executables or data files (such as "
		   "jpeg images)."));
    
    /**/
    
    panel->progtopL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->progtopL, 330, 48);
    WMMoveWidget(panel->progtopL, 20, 10);
    WMSetLabelText(panel->progtopL, _("If the directory contain data files, "
		   "type the command used to open these files. Otherwise, "
		   "leave it in blank."));

    panel->progT = WMCreateTextField(panel->win);
    WMResizeWidget(panel->progT, 330, 20);
    WMMoveWidget(panel->progT, 20, 60);

    panel->progbotL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->progbotL, 330, 72);
    WMMoveWidget(panel->progbotL, 20, 90);
    WMSetLabelText(panel->progbotL, _("Each file in the directory will have "
		   "an item and they will be opened with the supplied command."
		   "For example, if the directory contains image files and "
		   "the command is \"xv -root\", each file in the directory "
		   "will have a menu item like \"xv -root imagefile\"."));
    
    WMRealizeWidget(panel->win);
}



char*
OpenMenuGuru(WMWindow *mainWindow)
{
    WMScreen *scr = WMWidgetScreen(mainWindow);
    MenuGuru panel;
    char *text, *p, *dirs;

    createPanel(mainWindow, &panel);
    WMSetWindowCloseAction(panel.win, closeWindow, &panel);

    showPart(&panel, GSelectType);
        
    WMMapWidget(panel.win);

    panel.ok = 0;
    panel.end = 0;
    while (!panel.end) {
	XEvent ev;
	WMNextEvent(WMScreenDisplay(scr), &ev);
	WMHandleEvent(&ev);
    }
    
    text = NULL;
    if (panel.ok) {
	switch (panel.section) {
	 case GSelectFile:
	    text = WMGetTextFieldText(panel.pathT);
	    break;
	 case GSelectPipe:
	    text = WMGetTextFieldText(panel.pipeT);
	    p = trimstr(text); free(text);
	    if (p[0]!='|') {
		text = wmalloc(strlen(p)+4);
		strcpy(text, "| ");
		strcat(text, p);
		free(p);
	    } else {
		text = p;
	    }
	    break;
	 case GSelectProgram:
	    dirs = WMGetTextFieldText(panel.dirT);
	    text = WMGetTextFieldText(panel.progT);
	    p = trimstr(text); free(text);
	    if (strlen(p)==0) {
		free(p);
		text = dirs;
	    } else {
		text = wmalloc(strlen(dirs)+16+strlen(p));
		sprintf(text, "%s WITH %s", dirs, p);
		free(dirs);
		free(p);
	    }
	    break;
	}
    }
    
    WMDestroyWidget(panel.win);
    
    return text;
}





