/* Menu.c- menu definition
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

#include <X11/keysym.h>

typedef struct _Panel {
    WMFrame *frame;
    char *sectionName;

    CallbackRec callbacks;
    WMWindow *win;
    
    WMPopUpButton *cmd1P;
    WMPopUpButton *cmd2P;
    
    WMTextField *tit1T;
    WMTextField *tit2T;
    
    WMBrowser *browser;
    
    WMFrame *labF;
    WMTextField *labT;
    
    WMFrame *cmdF;
    WMPopUpButton *cmdP;

    WMButton *noconfirmB;

    WMFrame *proF;
    WMTextField *proT;
    WMLabel *infoL;

    WMFrame *pathF;
    WMTextField *pathT;
    WMLabel *pathL;

    WMFrame *shoF;
    WMTextField *shoT;
    WMButton *shoB;
    
    WMButton *guruB;

    /**/
    proplist_t menu;
    proplist_t editedItem;
    
    proplist_t itemClipboard;	       /* for copy/paste */

    char capturing;		       /* shortcut capture */
    char unsaved;		       /* if there are unsaved changes */
    char dontSave;
} _Panel;



#define ICON_FILE	"menus"



extern char *OpenMenuGuru(WMWindow *mainWindow);

extern Bool AskMenuCopy(WMWindow *wwin);


/* must match the indexes of the commands popup */
enum {
    CAddCommand = 0,
	CAddSubmenu = 1,
	CAddExternal = 2,
	CAddWorkspace = 3,
	CRemove = 4,
	CCut = 5,
	CCopy = 6,
	CPaste = 7
};


enum {
    CpExec = 0,
	CpArrange = 1,
	CpHide = 2,
	CpShow = 3,
	CpExit = 4,
	CpShutdown = 5,
	CpRestart = 6,
	CpRestartWM = 7,
	CpSaveSession = 8,
	CpClearSession = 9,
	CpRefresh = 10,
	CpInfo = 11,
	CpLegal = 12
};

enum {
    TNothing,
	TExec,
	TSimpleCommand,
	TRestart,
	TRestartWM,
	TExit,
	TExternalMenu,
	TWSMenu
};



static void showData(_Panel *panel);



static Bool
isMenu(proplist_t item)
{
    if (PLGetNumberOfElements(item)==1)
	return True;
    
    return PLIsArray(PLGetArrayElement(item, 1));
}


static void
splitOpenMenuParameter(char *str, char **dirs, char **prog)
{
    char *p;
    
    if (!(p = strstr(str, " WITH "))) {
	*dirs = wstrdup(str);
	*prog = NULL;
    } else {
	int i, j;
       
	i = strlen(str);
	j = strlen(p);
	*dirs = wmalloc(i-j+1);
	strncpy(*dirs, str, i-j+1);
	(*dirs)[i-j] = 0;

	p += 6;
	while (isspace(*p)) p++;
	if (*p!=0) {
	    *prog = wmalloc(j);
	    strcpy(*prog, p);
	} else {
	    *prog = NULL;
	}
    }
}


static void
changeItemTitle(proplist_t item, char *title)
{
    proplist_t tmp;
    
    tmp = PLGetArrayElement(item, 0);
    PLRelease(tmp);
    PLRemoveArrayElement(item, 0);
    PLInsertArrayElement(item, title?PLMakeString(title):PLMakeString(""), 0);
}


static void
removeParameter(proplist_t item)
{
    proplist_t tmp;
    int index;
    
    if (strcmp(PLGetString(PLGetArrayElement(item, 1)), "SHORTCUT")==0) {
	index = 4;
    } else {
	index = 2;
    }
    tmp = PLGetArrayElement(item, index);
    PLRemoveArrayElement(item, index);
    if (tmp)
	PLRelease(tmp);
}


static void
changeItemParameter(proplist_t item, char *param)
{
    proplist_t tmp;
    int index;
    
    if (strcmp(PLGetString(PLGetArrayElement(item, 1)), "SHORTCUT")==0) {
	index = 4;
    } else {
	index = 2;
    }
    tmp = PLGetArrayElement(item, index);
    PLRemoveArrayElement(item, index);
    PLRelease(tmp);
    tmp = param?PLMakeString(param):PLMakeString("");
    PLInsertArrayElement(item, tmp, index);
}


static void
changeItemShortcut(proplist_t item, char *shortcut)
{
    proplist_t tmp;

    if (strcmp(PLGetString(PLGetArrayElement(item, 1)), "SHORTCUT")==0) {
	if (shortcut) {
	    tmp = PLGetArrayElement(item, 2);
	    PLRemoveArrayElement(item, 2);
	    PLRelease(tmp);
	    PLInsertArrayElement(item, PLMakeString(shortcut), 2);
	} else {
	    /* remove SHORTCUT keyword */
	    tmp = PLGetArrayElement(item, 1);
	    PLRemoveArrayElement(item, 1);
	    PLRelease(tmp);
	    /* remove the shortcut */
	    tmp = PLGetArrayElement(item, 1);
	    PLRemoveArrayElement(item, 1);
	    PLRelease(tmp);
	}
    } else {
	if (shortcut) {
	    PLInsertArrayElement(item, PLMakeString("SHORTCUT"), 1);
	    PLInsertArrayElement(item, PLMakeString(shortcut), 2);	    
	} else {
	    /* do nothing */
	}
    }
}


static void
changeItemCommand(proplist_t item, char *command)
{
    proplist_t tmp;

    tmp = PLGetArrayElement(item, 1);
    if (strcmp(PLGetString(tmp), "SHORTCUT")==0) {
	PLRelease(tmp);
	PLRemoveArrayElement(item, 3);
	PLInsertArrayElement(item, PLMakeString(command), 3);
    } else {
	PLRelease(tmp);
	PLRemoveArrayElement(item, 1);
	PLInsertArrayElement(item, PLMakeString(command), 1);
    }
}


static char*
getItemTitle(proplist_t item)
{
    return PLGetString(PLGetArrayElement(item, 0));
}


static char*
getItemParameter(proplist_t item)
{
    proplist_t tmp;

    tmp = PLGetArrayElement(item, 1);
    if (strcmp(PLGetString(tmp), "SHORTCUT")==0) {
	tmp = PLGetArrayElement(item, 4);
	return tmp ? PLGetString(tmp) : NULL;
    } else {
	tmp = PLGetArrayElement(item, 2);
	return tmp ? PLGetString(tmp) : NULL;
    }
}



static char*
getItemShortcut(proplist_t item)
{
    proplist_t tmp;

    tmp = PLGetArrayElement(item, 1);
    if (strcmp(PLGetString(tmp), "SHORTCUT")==0) {
	return PLGetString(PLGetArrayElement(item, 2));
    } else {
	return NULL;
    }
}



static char*
getItemCommand(proplist_t item)
{
    proplist_t tmp;
    char *str;

    tmp = PLGetArrayElement(item, 1);
    if (!tmp)
	return "";
    if (strcmp(PLGetString(tmp), "SHORTCUT")==0) {
	str = PLGetString(PLGetArrayElement(item,3));
    } else {
	str = PLGetString(tmp);
    }
    return str;
}



static proplist_t
getSubmenuInColumn(_Panel *panel, int column)
{
    proplist_t parent;
    proplist_t submenu;
    WMList *list;
    int r;

    if (column == 0) {
	return panel->menu;
    }
    if (column >= WMGetBrowserNumberOfColumns(panel->browser))
	return NULL;

    list = WMGetBrowserListInColumn(panel->browser, column - 1);
    assert(list != NULL);

    r = WMGetListSelectedItemRow(list);

    parent = getSubmenuInColumn(panel, column - 1);

    assert(parent != NULL);

    submenu = PLGetArrayElement(parent, r + 1);

    return submenu;
}


static void
updateForItemType(_Panel *panel, int type)
{
    if (type==TNothing) {
	WMUnmapWidget(panel->labF);
    } else {
	WMMapWidget(panel->labF);
    }
    if (type==TExternalMenu || type==TNothing) {
	WMUnmapWidget(panel->cmdF);
    } else {
	WMMapWidget(panel->cmdF);
    }
    if (type==TNothing || type==TWSMenu || type==TExternalMenu) {
	WMUnmapWidget(panel->shoF);
    } else {
	WMMapWidget(panel->shoF);
    }
    if (type==TExec || type==TRestart || type==TExternalMenu) {
	WMMapWidget(panel->proF);
    } else {
	WMUnmapWidget(panel->proF);
    }
    if (type==TExternalMenu) {
	WMMapWidget(panel->pathF);
    } else {
	WMUnmapWidget(panel->pathF);
    }
    if (type==TExit) {
	WMMapWidget(panel->noconfirmB);
    } else {
	WMUnmapWidget(panel->noconfirmB);
    }
    if (type==TWSMenu) {
	WMMapWidget(panel->infoL);
    } else {
	WMUnmapWidget(panel->infoL);
    }
    if (type==TExternalMenu) {
	WMMapWidget(panel->guruB);
    } else {
	WMUnmapWidget(panel->guruB);
    }
    if (type == TRestart) {
	WMSetFrameTitle(panel->proF, _("Window Manager"));
    } else if (type == TExternalMenu) {
	WMSetFrameTitle(panel->proF, _("Program to open files"));
    } else {
	WMSetFrameTitle(panel->proF, _("Program to Run"));
    }
}


proplist_t
getItemOfSelectedEntry(WMBrowser *bPtr)
{
    proplist_t item;
    proplist_t menu;
    int i;

    i = WMGetBrowserSelectedColumn(bPtr);
    menu = getSubmenuInColumn((_Panel*)WMGetHangedData(bPtr), i);

    i = WMGetBrowserSelectedRowInColumn(bPtr, i);
    item = PLGetArrayElement(menu, i+1);

    return item;
}


static void
performCommand(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMPopUpButton *pop = (WMPopUpButton*)w;
    proplist_t menuItem = NULL;
    proplist_t menu;
    int column;
    int row;
    static int cmdIndex=0;
    char *title = NULL;

    column = WMGetBrowserFirstVisibleColumn(panel->browser);
    if (pop == panel->cmd2P) {
	column++;
    }

    if (column >= WMGetBrowserNumberOfColumns(panel->browser))
	return;

    menu = getSubmenuInColumn(panel, column);
    
    row = WMGetBrowserSelectedRowInColumn(panel->browser, column);

    switch (WMGetPopUpButtonSelectedItem(pop)) {
     case CAddCommand:
	title = wmalloc(strlen(_("New Command %i"))+6);
	sprintf(title, _("New Command %i"), cmdIndex++);
	menuItem = PLMakeArrayFromElements(PLMakeString(title),
					   PLMakeString("EXEC"),
					   PLMakeString(""),
					   NULL);
	break;
     case CAddSubmenu:
	title = wstrdup(_("New Submenu"));
	menuItem = PLMakeArrayFromElements(PLMakeString(title),
					   NULL);
	break;
     case CAddExternal:
	title = wstrdup(_("External Menu"));
	menuItem = PLMakeArrayFromElements(PLMakeString(title),
					   PLMakeString("OPEN_MENU"),
					   PLMakeString(""),
					   NULL);
	break;
     case CAddWorkspace:
	title = wstrdup(_("Workspaces"));
	menuItem = PLMakeArrayFromElements(PLMakeString(title),
					   PLMakeString("WORKSPACE_MENU"),
					   NULL);
	WMSetPopUpButtonItemEnabled(panel->cmd1P, CAddWorkspace, False);
	WMSetPopUpButtonItemEnabled(panel->cmd2P, CAddWorkspace, False);
	break;
     case CRemove:
	if (row < 0)
	    return;
	WMRemoveBrowserItem(panel->browser, column, row);
	menuItem = PLGetArrayElement(menu, row+1);
	if (strcmp(getItemCommand(menuItem), "WORKSPACE_MENU")==0) {
	    WMSetPopUpButtonItemEnabled(panel->cmd1P, CAddWorkspace, True);
	    WMSetPopUpButtonItemEnabled(panel->cmd2P, CAddWorkspace, True);
	}
	PLRemoveArrayElement(menu, row+1);
	PLRelease(menuItem);
	updateForItemType(panel, TNothing);
	panel->editedItem = NULL;
	panel->unsaved = 1;
	return;
     case CCut:
	if (row < 0)
	    return;
	if (panel->itemClipboard 
	    && strcmp(getItemCommand(panel->itemClipboard), "WORKSPACE_MENU")==0){
	    WMSetPopUpButtonItemEnabled(panel->cmd1P, CAddWorkspace, True);
	    WMSetPopUpButtonItemEnabled(panel->cmd2P, CAddWorkspace, True);
	}
	if (panel->itemClipboard)
	    PLRelease(panel->itemClipboard);

	WMRemoveBrowserItem(panel->browser, column, row);
	menuItem = PLGetArrayElement(menu, row+1);
	PLRemoveArrayElement(menu, row+1);
	updateForItemType(panel, TNothing);
	
	panel->itemClipboard = menuItem;

	WMSetPopUpButtonItemEnabled(panel->cmd1P, CPaste, True);
	WMSetPopUpButtonItemEnabled(panel->cmd2P, CPaste, True);
	panel->unsaved = 1;
	return;	
     case CCopy:
	if (row < 0)
	    return;
	if (panel->itemClipboard 
	    && strcmp(getItemCommand(panel->itemClipboard), "WORKSPACE_MENU")==0){
	    WMSetPopUpButtonItemEnabled(panel->cmd1P, CAddWorkspace, True);
	    WMSetPopUpButtonItemEnabled(panel->cmd2P, CAddWorkspace, True);
	}
	if (panel->itemClipboard)
	    PLRelease(panel->itemClipboard);
	panel->itemClipboard = NULL;
	menuItem = PLGetArrayElement(menu, row+1);
	if (strcmp(getItemCommand(menuItem), "WORKSPACE_MENU")==0)
	    return;
	panel->itemClipboard = PLDeepCopy(menuItem);
	
	WMSetPopUpButtonItemEnabled(panel->cmd1P, CPaste, True);
	WMSetPopUpButtonItemEnabled(panel->cmd2P, CPaste, True);
	return;
     case CPaste:
	menuItem = panel->itemClipboard;
	title = wstrdup(getItemTitle(menuItem));
	panel->itemClipboard = NULL;
	WMSetPopUpButtonItemEnabled(panel->cmd1P, CPaste, False);
	WMSetPopUpButtonItemEnabled(panel->cmd2P, CPaste, False);	
	break;
    }

    if (row>=0) row++;
    WMInsertBrowserItem(panel->browser, column, row, title, isMenu(menuItem));
    if (row<0)
	PLAppendArrayElement(menu, menuItem);
    else
	PLInsertArrayElement(menu, menuItem, row+1);
    free(title);
    panel->unsaved = 1;
}


static void
browserClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    proplist_t item;
    char *command;
    
    /* stop shortcut capture */
    panel->capturing = 0;

    item = getItemOfSelectedEntry(panel->browser);

    panel->editedItem = item;

    /* set title */
    WMSetTextFieldText(panel->labT, getItemTitle(item));

    if (isMenu(item)) {
	updateForItemType(panel, TNothing);

	WMSetPopUpButtonEnabled(panel->cmd2P, True);
	return;
    } else {
	int column = WMGetBrowserSelectedColumn(panel->browser);

	if (column == WMGetBrowserNumberOfColumns(panel->browser)-1
	    && column > 0)
	    WMSetPopUpButtonEnabled(panel->cmd2P, True);
	else
	    WMSetPopUpButtonEnabled(panel->cmd2P, False);

	if (column==WMGetBrowserFirstVisibleColumn(panel->browser)) {
	    /* second column is empty, because selected item is not a submenu */
	    WMSetTextFieldText(panel->tit2T, NULL);
	}
    }

    command = getItemCommand(item);

    WMSetTextFieldText(panel->shoT, getItemShortcut(item));
    
    if (strcmp(command, "OPEN_MENU")==0) {
	char *p, *c;
	
	splitOpenMenuParameter(getItemParameter(item), &p, &c);
	WMSetTextFieldText(panel->pathT, p);
	WMSetTextFieldText(panel->proT, c);
	if (p)
	    free(p);
	if (c)
	    free(c);
	updateForItemType(panel, TExternalMenu);
    } else if (strcmp(command, "EXEC")==0) {
	WMSetTextFieldText(panel->proT, getItemParameter(item));
	WMSetPopUpButtonSelectedItem(panel->cmdP, CpExec);
	updateForItemType(panel, TExec);
    } else if (strcmp(command, "WORKSPACE_MENU")==0) {
	updateForItemType(panel, TWSMenu);
    } else if (strcmp(command, "EXIT")==0) {
	WMSetPopUpButtonSelectedItem(panel->cmdP, CpExit);
	updateForItemType(panel, TExit);
    } else if (strcmp(command, "SHUTDOWN")==0) {
	WMSetPopUpButtonSelectedItem(panel->cmdP, CpShutdown);
	updateForItemType(panel, TExit);
    } else if (strcmp(command, "RESTARTW")==0) {
	WMSetPopUpButtonSelectedItem(panel->cmdP, CpRestartWM);
	updateForItemType(panel, TRestartWM);
    } else if (strcmp(command, "RESTART")==0) {
	WMSetPopUpButtonSelectedItem(panel->cmdP, CpRestart);
	WMSetTextFieldText(panel->proT, getItemParameter(item));
	updateForItemType(panel, TRestart);
    } else {
	/* simple commands */
	if (strcmp(command, "ARRANGE_ICONS")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpArrange);
	else if (strcmp(command, "HIDE_OTHERS")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpHide);
	else if (strcmp(command, "SHOW_ALL")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpShow);
	else if (strcmp(command, "SAVE_SESSION")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpSaveSession);
	else if (strcmp(command, "CLEAR_SESSION")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpClearSession);
	else if (strcmp(command, "REFRESH")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpRefresh);
	else if (strcmp(command, "INFO_PANEL")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpInfo);
	else if (strcmp(command, "LEGAL_PANEL")==0)
	    WMSetPopUpButtonSelectedItem(panel->cmdP, CpLegal);
	updateForItemType(panel, TSimpleCommand);
    }
}



static void
fillBrowserColumn(WMBrowser *bPtr, int column)
{
    _Panel *panel = (_Panel*)WMGetHangedData(bPtr);
    proplist_t menuItem;
    proplist_t menuList = NULL;
    int i;


    menuList = getSubmenuInColumn(panel, column);
    assert(menuList != NULL);

    if (column > WMGetBrowserFirstVisibleColumn(bPtr)) {
	WMSetTextFieldText(panel->tit2T, getItemTitle(menuList));
    } else {
	WMSetTextFieldText(panel->tit1T, getItemTitle(menuList));
    }

    for (i=1; i<PLGetNumberOfElements(menuList); i++) {
	menuItem = PLGetArrayElement(menuList, i);
	WMInsertBrowserItem(bPtr, column, -1, getItemTitle(menuItem),
			    isMenu(menuItem));
    }
}




static void
changedItem(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;
    WMTextField *t = (WMTextField*)WMGetNotificationObject(notification);
    proplist_t item = panel->editedItem;
    WMList *list;
    WMListItem *litem;
    char *command;
    char *str;


    if (!item)
	return;

    panel->unsaved = 1;
    if (!isMenu(item)) {
	command = getItemCommand(item);
	
	if (t == panel->shoT) {
	    str = WMGetTextFieldText(t);
	    if (strlen(str)==0) {
		free(str);
		str = NULL;
	    }
	    changeItemShortcut(item, str);
	    if (str)
		free(str);		
	} else if (t == panel->labT) {
	    int column;

	    str = WMGetTextFieldText(t);
	    if (!str)
		str = wstrdup("");
	    changeItemTitle(item, str);
	    column = WMGetBrowserSelectedColumn(panel->browser);
	    list = WMGetBrowserListInColumn(panel->browser, column);
	    litem = WMGetListSelectedItem(list);
	    
	    free(litem->text);
	    litem->text = str;
	
	    WMRedisplayWidget(list);
	} else if (strcmp(command, "EXEC")==0 
		   || strcmp(command, "RESTART")==0) {
	    if (t == panel->proT) {
		str = WMGetTextFieldText(t);

		changeItemParameter(item, str);

		free(str);
	    }
	} else if (strcmp(command, "OPEN_MENU")==0) {
	    char *text;
	    char *str2;
	    
	    str = WMGetTextFieldText(panel->pathT);
	    str2 = WMGetTextFieldText(panel->proT);
	    text = wmalloc(strlen(str)+strlen(str2)+16);
	    strcpy(text, str);
	    free(str);
	    if (strlen(str2)>0) {
		strcat(text, " WITH ");
		strcat(text, str2);
	    }
	    free(str2);
	    changeItemParameter(item, text);
	    free(text);
	}
    }
}


static void
changedTitle(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;
    WMTextField *t = (WMTextField*)WMGetNotificationObject(notification);
    proplist_t menu;
    WMList *list;
    int column;
    char *txt;

    column = WMGetBrowserFirstVisibleColumn(panel->browser);
    if (panel->tit2T == t)
	column++;
    
    menu = getSubmenuInColumn(panel, column);
    if (!menu)
	return;

    txt = WMGetTextFieldText(t);
    changeItemTitle(menu, txt);
    
    if (column > 0) {
	WMListItem *litem;
	
	list = WMGetBrowserListInColumn(panel->browser, column-1);
	litem = WMGetListSelectedItem(list);
	
	free(litem->text);
	litem->text = txt;
	
	WMRedisplayWidget(list);
    } else {
	free(txt);
    }
    panel->unsaved = 1;
}


static void
changedCommand(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i;
    char *tmp;

    panel->unsaved = 1;
    i = WMGetPopUpButtonSelectedItem(panel->cmdP);
    changeItemParameter(panel->editedItem, "");
    switch (i) {
     case CpExec:
	if (strcmp(getItemCommand(panel->editedItem), "EXEC")!=0) {
	    changeItemCommand(panel->editedItem, "EXEC");
	    tmp = WMGetTextFieldText(panel->proT);
	    changeItemParameter(panel->editedItem, tmp);
	    free(tmp);
	    updateForItemType(panel, TExec);
	}
	break;
     case CpArrange:
	if (strcmp(getItemCommand(panel->editedItem), "ARRANGE_ICONS")!=0) {
	    changeItemCommand(panel->editedItem, "ARRANGE_ICONS");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpHide:
	if (strcmp(getItemCommand(panel->editedItem), "HIDE_OTHERS")!=0) {
	    changeItemCommand(panel->editedItem, "HIDE_OTHERS");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpShow:
	if (strcmp(getItemCommand(panel->editedItem), "SHOW_ALL")!=0) {
	    changeItemCommand(panel->editedItem, "SHOW_ALL");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpExit:
	if (strcmp(getItemCommand(panel->editedItem), "EXIT")!=0) {
	    changeItemCommand(panel->editedItem, "EXIT");
	    updateForItemType(panel, TExit);
	}
	if (WMGetButtonSelected(panel->noconfirmB))
	    changeItemParameter(panel->editedItem, "QUICK");
	else
	    changeItemParameter(panel->editedItem, "");
	break;
     case CpShutdown:
	if (strcmp(getItemCommand(panel->editedItem), "SHUTDOWN")!=0) {
	    changeItemCommand(panel->editedItem, "SHUTDOWN");
	    updateForItemType(panel, TExit);
	}
	if (WMGetButtonSelected(panel->noconfirmB))
	    changeItemParameter(panel->editedItem, "QUICK");
	else
	    changeItemParameter(panel->editedItem, "");
	break;
     case CpRestartWM:
	changeItemCommand(panel->editedItem, "RESTARTW");
	updateForItemType(panel, TRestartWM);
	break;
     case CpRestart:
	changeItemCommand(panel->editedItem, "RESTART");
	updateForItemType(panel, TRestart);
	tmp = WMGetTextFieldText(panel->proT);
	changeItemParameter(panel->editedItem, tmp);
	free(tmp);
	break;
     case CpSaveSession:
	if (strcmp(getItemCommand(panel->editedItem), "SAVE_SESSION")!=0) {
	    changeItemCommand(panel->editedItem, "SAVE_SESSION");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpClearSession:
	if (strcmp(getItemCommand(panel->editedItem), "CLEAR_SESSION")!=0) {
	    changeItemCommand(panel->editedItem, "CLEAR_SESSION");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpRefresh:
	if (strcmp(getItemCommand(panel->editedItem), "REFRESH")!=0) {
	    changeItemCommand(panel->editedItem, "REFRESH");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpInfo:
	if (strcmp(getItemCommand(panel->editedItem), "INFO_PANEL")!=0) {
	    changeItemCommand(panel->editedItem, "INFO_PANEL");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
     case CpLegal:
	if (strcmp(getItemCommand(panel->editedItem), "LEGAL_PANEL")!=0) {
	    changeItemCommand(panel->editedItem, "LEGAL_PANEL");
	    updateForItemType(panel, TSimpleCommand);
	}
	break;
    }
}





static char*
captureShortcut(Display *dpy, _Panel *panel)
{
    XEvent ev;
    KeySym ksym;
    char buffer[64];
    char *key = NULL;

    while (panel->capturing) {
    	XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
	WMNextEvent(dpy, &ev);
	if (ev.type==KeyPress && ev.xkey.keycode!=0) {
	    ksym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);
	    if (!IsModifierKey(ksym)) {
		key=XKeysymToString(ksym);
		panel->capturing = 0;
		break;
	    }
	}
	WMHandleEvent(&ev);
    }
    
    if (!key)
	return NULL;
    
    buffer[0] = 0;
    
    if (ev.xkey.state & ControlMask) {
	strcat(buffer, "Control+");
    } 
    if (ev.xkey.state & ShiftMask) {
	strcat(buffer, "Shift+");
    } 
    if (ev.xkey.state & Mod1Mask) {
	strcat(buffer, "Mod1+");
    } 
    if (ev.xkey.state & Mod2Mask) {
	strcat(buffer, "Mod2+");
    } 
    if (ev.xkey.state & Mod3Mask) {
	strcat(buffer, "Mod3+");
    } 
    if (ev.xkey.state & Mod4Mask) {
	strcat(buffer, "Mod4+");
    } 
    if (ev.xkey.state & Mod5Mask) {
	strcat(buffer, "Mod5+");
    }
    strcat(buffer, key);
    
    return wstrdup(buffer);
}


static void
captureClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->win));
    char *shortcut;
    
    if (!panel->capturing) {
	panel->capturing = 1;
	WMSetButtonText(w, _("Cancel"));
	XGrabKeyboard(dpy, WMWidgetXID(panel->win), True, GrabModeAsync,
		      GrabModeAsync, CurrentTime);
	shortcut = captureShortcut(dpy, panel);
	if (shortcut) {
	    WMSetTextFieldText(panel->shoT, shortcut);
	    changeItemShortcut(panel->editedItem, shortcut);
	    panel->unsaved = 1;
	}
	free(shortcut);
    }
    panel->capturing = 0;
    WMSetButtonText(w, _("Capture"));
    XUngrabKeyboard(dpy, CurrentTime);
}



static void
scrolledBrowser(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;
    int column;
    WMList *list;
    proplist_t item;

    column = WMGetBrowserFirstVisibleColumn(panel->browser);

    item = getSubmenuInColumn(panel, column);
    WMSetTextFieldText(panel->tit1T, getItemTitle(item));

    item = getSubmenuInColumn(panel, column + 1);
    if (item)
	WMSetTextFieldText(panel->tit2T, getItemTitle(item));
}


static void
confirmClicked(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    
    if (WMGetButtonSelected(panel->noconfirmB)) {
	changeItemParameter(panel->editedItem, "QUICK");
    } else {
	changeItemParameter(panel->editedItem, "");
    }
    panel->unsaved = 1;
}



static void
openGuru(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char *def;
    char *path, *cmd;
    
    def = OpenMenuGuru(GetWindow(panel));
    if (def) {
	changeItemParameter(panel->editedItem, def);
	splitOpenMenuParameter(def, &path, &cmd);
	free(def);
	WMSetTextFieldText(panel->pathT, path);
	if (path) 
	    free(path);

	WMSetTextFieldText(panel->proT, cmd);
	if (cmd)
	    free(cmd);
	panel->unsaved = 1;
    }
}


static void
createPanel(_Panel *p)
{
    _Panel *panel = (_Panel*)p;
    
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    panel->cmd1P = WMCreatePopUpButton(panel->frame);
    WMSetPopUpButtonAction(panel->cmd1P, performCommand, panel);
    WMResizeWidget(panel->cmd1P, 144, 20);
    WMMoveWidget(panel->cmd1P, 15, 15);
    WMSetPopUpButtonPullsDown(panel->cmd1P, True);
    WMSetPopUpButtonText(panel->cmd1P, _("Commands"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Add Command"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Add Submenu"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Add External Menu"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Add Workspace Menu"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Remove Item"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Cut Item"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Copy Item"));
    WMAddPopUpButtonItem(panel->cmd1P, _("Paste Item"));
    
    panel->cmd2P = WMCreatePopUpButton(panel->frame);
    WMSetPopUpButtonAction(panel->cmd2P, performCommand, panel);
    WMResizeWidget(panel->cmd2P, 144, 20);
    WMMoveWidget(panel->cmd2P, 164, 15);
    WMSetPopUpButtonPullsDown(panel->cmd2P, True);
    WMSetPopUpButtonText(panel->cmd2P, _("Commands"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Add Command"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Add Submenu"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Add External Menu"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Add Workspace Menu"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Remove Item"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Cut Item"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Copy Item"));
    WMAddPopUpButtonItem(panel->cmd2P, _("Paste Item"));
    
    panel->tit1T = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->tit1T, 144, 20);
    WMMoveWidget(panel->tit1T, 15, 40);
    WMAddNotificationObserver(changedTitle, panel, WMTextDidChangeNotification,
			      panel->tit1T);
    
    panel->tit2T = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->tit2T, 144, 20);
    WMMoveWidget(panel->tit2T, 164, 40);
    WMAddNotificationObserver(changedTitle, panel, WMTextDidChangeNotification,
			      panel->tit2T);

    panel->browser = WMCreateBrowser(panel->frame);
    WMSetBrowserTitled(panel->browser, False);
    WMResizeWidget(panel->browser, 295, 160);
    WMMoveWidget(panel->browser, 15, 65);
    WMSetBrowserFillColumnProc(panel->browser, fillBrowserColumn);
    WMHangData(panel->browser, panel);
    WMSetBrowserPathSeparator(panel->browser, "\r");
    WMSetBrowserAction(panel->browser, browserClick, panel);
    WMAddNotificationObserver(scrolledBrowser, panel, 
			      WMBrowserDidScrollNotification, panel->browser);
    /**/

    panel->labF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->labF, 190, 50);
    WMMoveWidget(panel->labF, 320, 10);
    WMSetFrameTitle(panel->labF, _("Label"));

    panel->labT = WMCreateTextField(panel->labF);
    WMResizeWidget(panel->labT, 170, 20);
    WMMoveWidget(panel->labT, 10, 20);
    WMAddNotificationObserver(changedItem, panel, WMTextDidChangeNotification,
			      panel->labT);
    
    WMMapSubwidgets(panel->labF);
	
    panel->cmdF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->cmdF, 190, 50);
    WMMoveWidget(panel->cmdF, 320, 65);
    WMSetFrameTitle(panel->cmdF, _("Command"));

    panel->cmdP = WMCreatePopUpButton(panel->cmdF);
    WMResizeWidget(panel->cmdP, 170, 20);
    WMMoveWidget(panel->cmdP, 10, 20);
    WMAddPopUpButtonItem(panel->cmdP, _("Run Program"));
    WMAddPopUpButtonItem(panel->cmdP, _("Arrange Icons"));
    WMAddPopUpButtonItem(panel->cmdP, _("Hide Others"));
    WMAddPopUpButtonItem(panel->cmdP, _("Show All Windows"));
    WMAddPopUpButtonItem(panel->cmdP, _("Exit WindowMaker"));
    WMAddPopUpButtonItem(panel->cmdP, _("Exit X Session"));
    WMAddPopUpButtonItem(panel->cmdP, _("Start window manager"));
    WMAddPopUpButtonItem(panel->cmdP, _("Restart WindowMaker"));
    WMAddPopUpButtonItem(panel->cmdP, _("Save Session"));
    WMAddPopUpButtonItem(panel->cmdP, _("Clear Session"));
    WMAddPopUpButtonItem(panel->cmdP, _("Refresh Screen"));
    WMAddPopUpButtonItem(panel->cmdP, _("Info Panel"));
    WMAddPopUpButtonItem(panel->cmdP, _("Legal Panel"));
    WMSetPopUpButtonAction(panel->cmdP, changedCommand, panel);
    
    WMMapSubwidgets(panel->cmdF);
    
    panel->infoL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->infoL, 190, 50);
    WMMoveWidget(panel->infoL, 320, 65);
    WMSetLabelText(panel->infoL, _("Open workspace menu"));
    WMSetLabelRelief(panel->infoL, WRGroove);
    WMSetLabelTextAlignment(panel->infoL, WACenter);

    panel->noconfirmB = WMCreateSwitchButton(panel->frame);
    WMResizeWidget(panel->noconfirmB, 190, 50);
    WMMoveWidget(panel->noconfirmB, 320, 120);
    WMSetButtonText(panel->noconfirmB, _("No confirmation panel"));
    WMSetButtonAction(panel->noconfirmB, confirmClicked, panel);

    panel->pathF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->pathF, 190, 50);
    WMMoveWidget(panel->pathF, 320, 65);
    WMSetFrameTitle(panel->pathF, _("Menu Path/Directory List"));
    
    panel->pathT = WMCreateTextField(panel->pathF);
    WMResizeWidget(panel->pathT, 170, 20);
    WMMoveWidget(panel->pathT, 10, 20);
    WMAddNotificationObserver(changedItem, panel, WMTextDidChangeNotification,
			      panel->pathT);

    WMMapSubwidgets(panel->pathF);

    panel->proF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->proF, 190, 50);
    WMMoveWidget(panel->proF, 320, 120);
    WMSetFrameTitle(panel->proF, _("Program to Run"));
    
    panel->proT = WMCreateTextField(panel->proF);
    WMResizeWidget(panel->proT, 170, 20);
    WMMoveWidget(panel->proT, 10, 20);
    WMAddNotificationObserver(changedItem, panel, WMTextDidChangeNotification,
			      panel->proT);

    WMMapSubwidgets(panel->proF);

    panel->shoF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->shoF, 190, 50);
    WMMoveWidget(panel->shoF, 320, 175);
    WMSetFrameTitle(panel->shoF, _("Shortcut"));
    
    panel->shoT = WMCreateTextField(panel->shoF);
    WMResizeWidget(panel->shoT, 95, 20);
    WMMoveWidget(panel->shoT, 10, 20);
    WMAddNotificationObserver(changedItem, panel, WMTextDidChangeNotification,
			      panel->shoT);
    
    panel->shoB = WMCreateCommandButton(panel->shoF);
    WMResizeWidget(panel->shoB, 70, 24);
    WMMoveWidget(panel->shoB, 110, 18);
    WMSetButtonText(panel->shoB, _("Capture"));
    WMSetButtonAction(panel->shoB, captureClick, panel);

    WMMapSubwidgets(panel->shoF);
    
    panel->guruB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->guruB, 180, 24);
    WMMoveWidget(panel->guruB, 325, 190);
    WMSetButtonText(panel->guruB, _("Ask help to the Guru"));
    WMSetButtonAction(panel->guruB, openGuru, panel);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    showData(panel);
}




static proplist_t
preProcessMenu(proplist_t menu, int *hasWSMenu)
{
    proplist_t pmenu;
    proplist_t item;
    int i;

    pmenu = PLDeepCopy(menu);
    if (PLGetNumberOfElements(pmenu)==1) {
	return pmenu;
    }
    for (i=1; i<PLGetNumberOfElements(pmenu); i++) {
	item = PLGetArrayElement(pmenu, i);
	if (isMenu(item)) {
	    PLInsertArrayElement(pmenu, preProcessMenu(item, hasWSMenu), i);
	    PLRemoveArrayElement(pmenu, i+1);
	    PLRelease(item);
	} else if (strcmp(getItemCommand(item), "RESTART")==0) {
	    if (getItemShortcut(item)) {
		if (PLGetNumberOfElements(item) == 4) {
		    changeItemCommand(item, "RESTARTW");
		    PLAppendArrayElement(item, PLMakeString(""));
		}
	    } else {
		if (PLGetNumberOfElements(item) == 2) {
		    changeItemCommand(item, "RESTARTW");
		    PLAppendArrayElement(item, PLMakeString(""));
		}
	    }
	} else {
	    if (strcmp(getItemCommand(item),"WORKSPACE_MENU")==0)
		*hasWSMenu = 1;
	    if (getItemShortcut(item)) {
		if (PLGetNumberOfElements(item) == 4)
		    PLAppendArrayElement(item, PLMakeString(""));
	    } else {
		if (PLGetNumberOfElements(item) == 2)
		    PLAppendArrayElement(item, PLMakeString(""));
	    }
	}
    }

    return pmenu;
}



static proplist_t
postProcessMenu(proplist_t menu)
{
    proplist_t pmenu;
    proplist_t item;
    int i;
    int count;

    pmenu = PLDeepCopy(menu);
    if (PLGetNumberOfElements(pmenu)==1) {
	return pmenu;
    }
    count = PLGetNumberOfElements(pmenu);
    for (i=1; i<count; i++) {
	char *cmd;
	item = PLGetArrayElement(pmenu, i);
	if (isMenu(item)) {
	    PLInsertArrayElement(pmenu, postProcessMenu(item), i);
	    PLRemoveArrayElement(pmenu, i+1);
	    PLRelease(item);
	} else {
	    cmd = getItemCommand(item);
	    if (strcmp(cmd, "RESTARTW")==0) {
		changeItemCommand(item, "RESTART");
		removeParameter(item);
	    } else if (strcmp(cmd, "EXEC")==0 || strcmp(cmd, "OPEN_MENU")==0) {
		/* do nothing */
	    } else if (strcmp(cmd, "RESTART")==0 || strcmp(cmd, "SHUTDOWN")==0
		       || strcmp(cmd, "EXIT")==0) {
		char *tmp = getItemParameter(item);
		if (tmp && strlen(tmp)==0)
		    removeParameter(item);
	    } else {
		removeParameter(item);
	    }
	}
    }

    return pmenu;
}


static proplist_t
getDefaultMenu(_Panel *panel, int *hasWSMenu)
{
    proplist_t menu, pmenu;
    char *menuPath, *gspath;

    gspath = wusergnusteppath();

    menuPath = wmalloc(strlen(gspath)+128);
    /* if there is a localized plmenu for the tongue put it's filename here */
    sprintf(menuPath, _("%s/Library/WindowMaker/plmenu"), gspath);

    menu = PLGetProplistWithPath(menuPath);
    if (!menu) {
	wwarning("%s:could not read property list menu", menuPath);

	if (strcmp("%s/Library/WindowMaker/plmenu",
		   _("%s/Library/WindowMaker/plmenu"))!=0) {

	    sprintf(menuPath, "%s/Library/WindowMaker/plmenu", gspath);
	    menu = PLGetProplistWithPath(menuPath);
	    wwarning("%s:could not read property list menu", menuPath);
	}
	if (!menu) {
	    char buffer[512];

	    sprintf(buffer, _("Could not open default menu from '%s'"),
		    menuPath);
	    WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win,
			    _("Error"), buffer,	_("OK"), NULL, NULL);
	}
    }

    free(gspath);
    free(menuPath);

    if (menu) {
	pmenu = preProcessMenu(menu, hasWSMenu);
	PLRelease(menu);
    } else {
	pmenu = NULL;
    }

    return pmenu;
}


static void
showData(_Panel *panel)
{
    char *gspath;
    char *menuPath;
    proplist_t menu, pmenu, plPath;
    int hasWSMenu = 0;

    gspath = wusergnusteppath();

    menuPath = wmalloc(strlen(gspath)+32);
    strcpy(menuPath, gspath);
    free(gspath);
    strcat(menuPath, "/Defaults/WMRootMenu");

    menu = PLGetProplistWithPath(menuPath);
    pmenu = NULL;

    if (!menu || !PLIsArray(menu)) {
	if (AskMenuCopy(panel->win)) {
	    panel->dontSave = 0;
	    panel->unsaved = 1;

	    pmenu = getDefaultMenu(panel, &hasWSMenu);
	} else {
	    WMSetPopUpButtonEnabled(panel->cmd1P, False);
	    WMSetPopUpButtonEnabled(panel->cmd2P, False);
	    panel->dontSave = 1;
	}
	if (!pmenu) {
	    pmenu = PLMakeArrayFromElements(PLMakeString("Applications"),
					    NULL);
	}
    } else {
	pmenu = preProcessMenu(menu, &hasWSMenu);
    }
    plPath = PLMakeString(menuPath);
    free(menuPath);
    PLSetFilename(pmenu, plPath);
    PLRelease(plPath);

    if (menu)
        PLRelease(menu);

    if (panel->itemClipboard) {
	PLRelease(panel->itemClipboard);
	panel->itemClipboard = NULL;
    }
    panel->menu = pmenu;
    panel->editedItem = NULL;
    panel->capturing = 0;

    WMSetPopUpButtonItemEnabled(panel->cmd1P, CPaste, False);
    WMSetPopUpButtonItemEnabled(panel->cmd2P, CPaste, False);
    if (hasWSMenu) {
	WMSetPopUpButtonItemEnabled(panel->cmd1P, CAddWorkspace, False);
	WMSetPopUpButtonItemEnabled(panel->cmd2P, CAddWorkspace, False);
    }
    WMLoadBrowserColumnZero(panel->browser);

    updateForItemType(panel, TNothing);
}


static void
storeData(_Panel *panel)
{
    proplist_t menu;

    if (!panel->unsaved || panel->dontSave)
	return;
    panel->unsaved = 0;

    menu = postProcessMenu(panel->menu);

    PLSetFilename(menu, PLGetFilename(panel->menu));

    PLSave(menu, YES);
    
    PLRelease(menu);
}


Panel*
InitMenu(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;
    
    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Applications Menu Definition");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    
    AddSection(panel, ICON_FILE);

    return panel;
}
