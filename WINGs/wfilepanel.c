
#include "WINGsP.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef struct W_FilePanel {
    WMWindow *win;
    
    WMLabel *iconLabel;
    WMLabel *titleLabel;
    
    WMFrame *line;

    WMLabel *nameLabel;
    WMBrowser *browser;
    
    WMButton *okButton;
    WMButton *cancelButton;
    
    WMButton *homeButton;

    WMView *accessoryView;

    WMTextField *fileField;
    
    char **fileTypes;

    struct {
	unsigned int canExit:1;
	unsigned int canceled:1;       /* clicked on cancel */
	unsigned int done:1;
	unsigned int filtered:1;
	unsigned int canChooseFiles:1;
	unsigned int canChooseDirectories:1;
	unsigned int autoCompletion:1;
	unsigned int showAllFiles:1;
	unsigned int canFreeFileTypes:1;
	unsigned int fileMustExist:1;
	unsigned int panelType:1;

	/**/
	unsigned int ignoreTextChangeNotification:1;
    } flags;
} W_FilePanel;


/* Type of panel */
#define WP_OPEN         0
#define WP_SAVE         1

#define PWIDTH		320
#define PHEIGHT 	360

static void listDirectoryOnColumn(WMFilePanel *panel, int column, char *path);
static void browserClick();
static void browserDClick();

static void fillColumn(WMBrowser *bPtr, int column);

static void goHome();

static void buttonClick();

static char *getCurrentFileName(WMFilePanel *panel);

static void handleEvents(XEvent *event, void *data);


static int
closestListItem(WMList *list, char *text, Bool exact)
{
    WMListItem *item = WMGetListItem(list, 0);
    int i = 0;
    int len = strlen(text);

    if (len==0)
	return -1;

    while (item) {
        if (strlen(item->text) >= len &&
            ((exact && strcmp(item->text, text)==0) ||
             (!exact && strncmp(item->text, text, len)==0))) {
	    return i;
	}
	item = item->nextPtr;
	i++;
    }
    return -1;
}


static void
textChangedObserver(void *observerData, WMNotification *notification)
{
    W_FilePanel *panel = (W_FilePanel*)observerData;
    char *text;
    WMList *list;
    int col = WMGetBrowserNumberOfColumns(panel->browser) - 1;
    int i, textEvent = (int)WMGetNotificationClientData(notification);
    static int running = 0;

    if (running)
        return;

    running = 1;

    if (panel->flags.ignoreTextChangeNotification)
	return;

    list = WMGetBrowserListInColumn(panel->browser, col);
    if (!list)
	return;

    text = WMGetTextFieldText(panel->fileField);

    if (panel->flags.autoCompletion && textEvent!=WMDeleteTextEvent)
        i = closestListItem(list, text, False);
    else
        i = closestListItem(list, text, True);

    WMSelectListItem(list, i);
    if (i>=0 && panel->flags.autoCompletion) {
        WMListItem *item = WMGetListItem(list, i);
        int textLen = strlen(text), itemTextLen = strlen(item->text);
        int visibleItems = WMWidgetHeight(list)/WMGetListItemHeight(list);
	
        if (textEvent!=WMSetTextEvent || textLen<itemTextLen)
            WMSetListPosition(list, i - visibleItems/2);
	
        if (textEvent!=WMDeleteTextEvent && textLen<itemTextLen) {
            WMRange range;
	    
	    WMInsertTextFieldText(panel->fileField, &item->text[textLen],
				  textLen);
	    WMSetTextFieldCursorPosition(panel->fileField, itemTextLen);
	    range.position = textLen;
	    range.count = itemTextLen - textLen;
	    WMSelectTextFieldRange(panel->fileField, range);
	}
    }

    free(text);
    running = 0;
}


static void
textEditedObserver(void *observerData, WMNotification *notification)
{
    W_FilePanel *panel = (W_FilePanel*)observerData;

    if ((int)WMGetNotificationClientData(notification)==WMReturnTextMovement) {
	WMPerformButtonClick(panel->okButton);
    }
}



static WMFilePanel*
makeFilePanel(WMScreen *scrPtr, char *name, char *title)
{
    WMFilePanel *fPtr;
    WMFont *largeFont;
    
    fPtr = wmalloc(sizeof(WMFilePanel));
    memset(fPtr, 0, sizeof(WMFilePanel));

    fPtr->win = WMCreateWindowWithStyle(scrPtr, name, WMTitledWindowMask
					|WMResizableWindowMask);
    WMResizeWidget(fPtr->win, PWIDTH, PHEIGHT);
    WMSetWindowTitle(fPtr->win, "");

    WMCreateEventHandler(WMWidgetView(fPtr->win), StructureNotifyMask,
                         handleEvents, fPtr);
    WMSetWindowMinSize(fPtr->win, PWIDTH, PHEIGHT);


    fPtr->iconLabel = WMCreateLabel(fPtr->win);
    WMResizeWidget(fPtr->iconLabel, 64, 64);
    WMMoveWidget(fPtr->iconLabel, 0, 0);
    WMSetLabelImagePosition(fPtr->iconLabel, WIPImageOnly);
    WMSetLabelImage(fPtr->iconLabel, scrPtr->applicationIcon);
    
    fPtr->titleLabel = WMCreateLabel(fPtr->win);
    WMResizeWidget(fPtr->titleLabel, PWIDTH-64, 64);
    WMMoveWidget(fPtr->titleLabel, 64, 0);
    largeFont = WMBoldSystemFontOfSize(scrPtr, 24);
    WMSetLabelFont(fPtr->titleLabel, largeFont);
    WMReleaseFont(largeFont);
    WMSetLabelText(fPtr->titleLabel, title);
    
    fPtr->line = WMCreateFrame(fPtr->win);
    WMMoveWidget(fPtr->line, 0, 64);
    WMResizeWidget(fPtr->line, PWIDTH, 2);
    WMSetFrameRelief(fPtr->line, WRGroove);

    fPtr->browser = WMCreateBrowser(fPtr->win);
    WMSetBrowserFillColumnProc(fPtr->browser, fillColumn);
    WMSetBrowserAction(fPtr->browser, browserClick, fPtr);
    WMSetBrowserDoubleAction(fPtr->browser, browserDClick, fPtr);
    WMMoveWidget(fPtr->browser, 7, 72);
    WMHangData(fPtr->browser, fPtr);

    fPtr->nameLabel = WMCreateLabel(fPtr->win);
    WMMoveWidget(fPtr->nameLabel, 7, 282);
    WMResizeWidget(fPtr->nameLabel, 55, 14);
    WMSetLabelText(fPtr->nameLabel, "Name:");
    
    fPtr->fileField = WMCreateTextField(fPtr->win);
    WMMoveWidget(fPtr->fileField, 60, 278);
    WMResizeWidget(fPtr->fileField, PWIDTH-60-10, 24);
    WMAddNotificationObserver(textEditedObserver, fPtr,
			      WMTextDidEndEditingNotification,
			      fPtr->fileField);
    WMAddNotificationObserver(textChangedObserver, fPtr,
			      WMTextDidChangeNotification,
			      fPtr->fileField);
    
    fPtr->okButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->okButton, 230, 325);
    WMResizeWidget(fPtr->okButton, 80, 28);
    WMSetButtonText(fPtr->okButton, "OK");
    WMSetButtonImage(fPtr->okButton, scrPtr->buttonArrow);
    WMSetButtonAltImage(fPtr->okButton, scrPtr->pushedButtonArrow);
    WMSetButtonImagePosition(fPtr->okButton, WIPRight);
    WMSetButtonAction(fPtr->okButton, buttonClick, fPtr);
    
    fPtr->cancelButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->cancelButton, 140, 325);
    WMResizeWidget(fPtr->cancelButton, 80, 28);
    WMSetButtonText(fPtr->cancelButton, "Cancel");
    WMSetButtonAction(fPtr->cancelButton, buttonClick, fPtr);

    fPtr->homeButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->homeButton, 55, 325);
    WMResizeWidget(fPtr->homeButton, 28, 28);
    WMSetButtonImagePosition(fPtr->homeButton, WIPImageOnly);
    WMSetButtonImage(fPtr->homeButton, scrPtr->homeIcon);
    WMSetButtonAltImage(fPtr->homeButton, scrPtr->altHomeIcon);
    WMSetButtonAction(fPtr->homeButton, goHome, fPtr);

    WMRealizeWidget(fPtr->win);
    WMMapSubwidgets(fPtr->win);

    WMSetFocusToWidget(fPtr->fileField);
    WMSetTextFieldCursorPosition(fPtr->fileField, 0);

    WMLoadBrowserColumnZero(fPtr->browser);
    
    fPtr->flags.canChooseFiles = 1;
    fPtr->flags.canChooseDirectories = 1;
    fPtr->flags.autoCompletion = 1;

    return fPtr;
}


WMOpenPanel*
WMGetOpenPanel(WMScreen *scrPtr)
{
    WMFilePanel *panel;
    
    if (scrPtr->sharedOpenPanel)
	return scrPtr->sharedOpenPanel;

    panel = makeFilePanel(scrPtr, "openFilePanel", "Open");
    panel->flags.fileMustExist = 1;
    panel->flags.panelType = WP_OPEN;

    scrPtr->sharedOpenPanel = panel;

    return panel;
}


WMSavePanel*
WMGetSavePanel(WMScreen *scrPtr)
{
    WMFilePanel *panel;

    if (scrPtr->sharedSavePanel)
	return scrPtr->sharedSavePanel;

    panel = makeFilePanel(scrPtr, "saveFilePanel", "Save");
    panel->flags.fileMustExist = 0;
    panel->flags.panelType = WP_SAVE;

    scrPtr->sharedSavePanel = panel;

    return panel;
}


void
WMFreeFilePanel(WMFilePanel *panel)
{
    if (panel == WMWidgetScreen(panel->win)->sharedSavePanel) {
	WMWidgetScreen(panel->win)->sharedSavePanel = NULL;
    }
    if (panel == WMWidgetScreen(panel->win)->sharedOpenPanel) {
	WMWidgetScreen(panel->win)->sharedOpenPanel = NULL;
    }
    WMRemoveNotificationObserver(panel);
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    free(panel);
}


int
WMRunModalFilePanelForDirectory(WMFilePanel *panel, WMWindow *owner,
                                char *path, char *name, char **fileTypes)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    XEvent event;

    if (name && !owner) {
        WMSetWindowTitle(panel->win, name);
    }

    WMChangePanelOwner(panel->win, owner);

    WMSetFilePanelDirectory(panel, path);

    panel->flags.done = 0;
    switch(panel->flags.panelType) {
    case WP_OPEN:
        if (fileTypes)
            panel->flags.filtered = 1;
        panel->fileTypes = fileTypes;
        if (name == NULL)
            name = "Open";
        break;
    case WP_SAVE:
        panel->fileTypes = NULL;
        panel->flags.filtered = 0;
        if (name == NULL)
            name = "Save";
        break;
    default:
        break;
    }

    WMSetLabelText(panel->titleLabel, name);

    scr->modalView = W_VIEW(panel->win);
    WMMapWidget(panel->win);

    WMMoveWidget(panel->win, 
		 (scr->rootView->size.width - WMWidgetWidth(panel->win))/2,
		 (scr->rootView->size.height - WMWidgetHeight(panel->win))/2);

    scr->modal = 1;
    while (!panel->flags.done) {	
	WMNextEvent(scr->display, &event);
	WMHandleEvent(&event);
    }
    scr->modal = 0;

    /* Must withdraw window because the next time we map
     * it, it might have a different transient owner. 
     */
    WMCloseWindow(panel->win);

    return (panel->flags.canceled ? False : True);
}




void
WMSetFilePanelDirectory(WMFilePanel *panel, char *path)
{
    WMList *list;
    WMListItem *item;
    int col;
    char *rest;

    rest = WMSetBrowserPath(panel->browser, path);
    if (strcmp(path, "/")==0)
        rest = NULL;

    col = WMGetBrowserSelectedColumn(panel->browser);
    list = WMGetBrowserListInColumn(panel->browser, col);
    if (list && (item = WMGetListSelectedItem(list))) {
        if (item->isBranch) {
            WMSetTextFieldText(panel->fileField, rest);
        } else {
            WMSetTextFieldText(panel->fileField, item->text);
        }
    } else {
        WMSetTextFieldText(panel->fileField, rest);
    }
}

			
void
WMSetFilePanelCanChooseDirectories(WMFilePanel *panel, Bool flag)
{
    panel->flags.canChooseDirectories = flag;
}

void
WMSetFilePanelCanChooseFiles(WMFilePanel *panel, Bool flag)
{
    panel->flags.canChooseFiles = flag;
}


void
WMSetFilePanelAutoCompletion(WMFilePanel *panel, Bool flag)
{
    panel->flags.autoCompletion = flag;
}


char*
WMGetFilePanelFileName(WMFilePanel *panel)
{
    return getCurrentFileName(panel);
}


void
WMSetFilePanelAccessoryView(WMFilePanel *panel, WMView *view)
{
    WMView *v;

    panel->accessoryView = view;

    v = WMWidgetView(panel->win);

    W_ReparentView(view, v, 0, 0);

    W_MoveView(view, (v->size.width - v->size.width)/2, 300);
}


WMView*
WMGetFilePanelAccessoryView(WMFilePanel *panel)
{
    return panel->accessoryView;
}


static char*
get_name_from_path(char *path)
{
    int size;
    
    assert(path!=NULL);
        
    size = strlen(path);

    /* remove trailing / */
    while (size > 0 && path[size-1]=='/')
	size--;
    /* directory was root */
    if (size == 0)
	return wstrdup("/");

    while (size > 0 && path[size-1] != '/')
	size--;
    
    return wstrdup(&(path[size]));
}


static int
filterFileName(WMFilePanel *panel, char *file, Bool isDirectory)
{
    return True;
}


static void
listDirectoryOnColumn(WMFilePanel *panel, int column, char *path)
{
    WMBrowser *bPtr = panel->browser;
    struct dirent *dentry;
    DIR *dir;
    struct stat stat_buf;
    char pbuf[PATH_MAX+16];

    assert(column >= 0);
    assert(path != NULL);

    /* put directory name in the title */
    WMSetBrowserColumnTitle(bPtr, column, get_name_from_path(path));
    
    dir = opendir(path);
    
    if (!dir) {
#ifdef VERBOSE
	printf("WINGs: could not open directory %s\n", path);
#endif
	return;
    }

    /* list contents in the column */
    while ((dentry = readdir(dir))) {	       
	if (strcmp(dentry->d_name, ".")==0 ||
	    strcmp(dentry->d_name, "..")==0)
	    continue;

	strcpy(pbuf, path);
	if (strcmp(path, "/")!=0)
	    strcat(pbuf, "/");
	strcat(pbuf, dentry->d_name);

	if (stat(pbuf, &stat_buf)!=0) {
#ifdef VERBOSE
	    printf("WINGs: could not stat %s\n", pbuf);
#endif
	    continue;
	} else {
	    int isDirectory;

	    isDirectory = S_ISDIR(stat_buf.st_mode);
	    
	    if (filterFileName(panel, dentry->d_name, isDirectory))
		WMAddSortedBrowserItem(bPtr, column, dentry->d_name, 
					  isDirectory);
	}
    }

    closedir(dir);
}


static void
fillColumn(WMBrowser *bPtr, int column)
{
    char *path;
    WMFilePanel *panel;
    
    if (column > 0) {
	path = WMGetBrowserPathToColumn(bPtr, column-1);
    } else {
	path = wstrdup("/");
    }

    panel = WMGetHangedData(bPtr);
    listDirectoryOnColumn(panel, column, path);
    free(path);
}


static void 
browserDClick(WMBrowser *bPtr, WMFilePanel *panel)
{
    WMPerformButtonClick(panel->okButton);
}

static void 
browserClick(WMBrowser *bPtr, WMFilePanel *panel)
{
    int col = WMGetBrowserSelectedColumn(bPtr);
    WMListItem *item = WMGetBrowserSelectedItemInColumn(bPtr, col);

    panel->flags.ignoreTextChangeNotification = 1;

    if (!item || item->isBranch)
	WMSetTextFieldText(panel->fileField, NULL);
    else {
	WMSetTextFieldText(panel->fileField, item->text);
    }

    panel->flags.ignoreTextChangeNotification = 0;
}


static void
goHome(WMButton *bPtr, WMFilePanel *panel)
{
    char *home;

    /* home is statically allocated. Don't free it! */
    home = wgethomedir();
    if (!home)
	return;
    
    WMSetFilePanelDirectory(panel, home);
}


static void 
handleEvents(XEvent *event, void *data)
{
    W_FilePanel *pPtr = (W_FilePanel*)data;
    W_View *view = WMWidgetView(pPtr->win);

    if (event->type == ConfigureNotify) {
        if (event->xconfigure.width != view->size.width
            || event->xconfigure.height != view->size.height) {
            unsigned int newWidth = event->xconfigure.width;
            unsigned int newHeight = event->xconfigure.height;
	    int newColumnCount;

            W_ResizeView(view, newWidth, newHeight);
            WMResizeWidget(pPtr->line, newWidth, 2);
            WMResizeWidget(pPtr->browser, newWidth-14,
                           newHeight-(PHEIGHT-200));
            WMResizeWidget(pPtr->fileField, newWidth-60-10, 24);
            WMMoveWidget(pPtr->nameLabel, 7, newHeight-(PHEIGHT-282));
            WMMoveWidget(pPtr->fileField, 60, newHeight-(PHEIGHT-278));
            WMMoveWidget(pPtr->okButton, newWidth-(PWIDTH-230),
                         newHeight-(PHEIGHT-325));
            WMMoveWidget(pPtr->cancelButton, newWidth-(PWIDTH-140),
                         newHeight-(PHEIGHT-325));
            WMMoveWidget(pPtr->homeButton, 55, newHeight-(PHEIGHT-325));

	    newColumnCount = (newWidth - 14) / 140;
	    WMSetBrowserMaxVisibleColumns(pPtr->browser, newColumnCount);
        }
    }
}


static char*
getCurrentFileName(WMFilePanel *panel)
{
    char *path;
    char *file;
    char *tmp;
    int len;
    
    path = WMGetBrowserPath(panel->browser);
    
    len = strlen(path);
    if (path[len-1]=='/') {
	file = WMGetTextFieldText(panel->fileField);
	tmp = wmalloc(strlen(path)+strlen(file)+8);
        if (file[0]!='/') {
           strcpy(tmp, path);
           strcat(tmp, file);
        } else
           strcpy(tmp, file);

	free(file);
	free(path);
	return tmp;
    } else {
	return path;
    }
}



static Bool
validOpenFile(WMFilePanel *panel)
{
    WMListItem *item;
    int col, haveFile = 0;
    char *file = WMGetTextFieldText(panel->fileField);

    if (file[0] != '\0')
        haveFile = 1;
    free(file);

    col = WMGetBrowserSelectedColumn(panel->browser);
    item = WMGetBrowserSelectedItemInColumn(panel->browser, col);
    if (item) {
	if (item->isBranch && !panel->flags.canChooseDirectories && !haveFile)
	    return False;
	else if (!item->isBranch && !panel->flags.canChooseFiles)
            return False;
        else
            return True;
    } else {
        /* we compute for / here */
	if (!panel->flags.canChooseDirectories && !haveFile)
	    return False;
        else
            return True;
    }
    return True;
}



static void
buttonClick(WMButton *bPtr, WMFilePanel *panel)
{
    WMRange range;

    if (bPtr == panel->okButton) {
        if (!validOpenFile(panel))
            return;
	if (panel->flags.fileMustExist) {
	    char *file;
	    
	    file = getCurrentFileName(panel);
	    if (access(file, F_OK)!=0) {
		WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win,
				"Error", "File does not exist.",
				"Ok", NULL, NULL);
		free(file);
		return;
	    }
	    free(file);
	}
	panel->flags.canceled = 0;
    } else
	panel->flags.canceled = 1;

    range.count = range.position = 0;
    WMSelectTextFieldRange(panel->fileField, range);
    panel->flags.done = 1;
}


