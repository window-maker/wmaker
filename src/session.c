/* session.c - session state handling and R6 style session management
 *
 *  Copyright (c) 1998 Dan Pascu
 *  Copyright (c) 1998, 1999 Alfredo Kojima
 *
 *  Window Maker window manager
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


/*
 * 
 * If defined(XSMP_ENABLED) and session manager is running then
 * 	do normal stuff
 * else
 * 	do pre-R6 session management stuff (save window state and relaunch)
 *
 * When doing a checkpoint:
 * 
 * = Without XSMP
 * Open "Stop"/status Dialog
 * Send SAVE_YOURSELF to clients and wait for reply
 * Save restart info
 * Save state of clients
 * 
 * = With XSMP
 * Send checkpoint request to sm
 *
 * When exiting:
 * -------------
 * 
 * = Without XSMP
 * 
 * Open "Exit Now"/status Dialog
 * Send SAVE_YOURSELF to clients and wait for reply
 * Save restart info
 * Save state of clients
 * Send DELETE to all clients
 * When no more clients are left or user hit "Exit Now", exit
 * 
 * = With XSMP
 *
 * Send Shutdown request to session manager
 * if SaveYourself message received, save state of clients 
 * if the Die message is received, exit.
 */

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef XSMP_ENABLED
#include <X11/SM/SMlib.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#include "WindowMaker.h"
#include "screen.h"
#include "window.h"
#include "client.h"
#include "session.h"
#include "wcore.h"
#include "framewin.h"
#include "workspace.h"
#include "funcs.h"
#include "properties.h"
#include "application.h"
#include "appicon.h"


#include "dock.h"


#include <proplist.h>

/** Global **/

extern Atom _XA_WM_SAVE_YOURSELF;

extern Time LastTimestamp;

#ifdef XSMP_ENABLED

extern int wScreenCount;

/* requested for SaveYourselfPhase2 */
static Bool sWaitingPhase2 = False;

static SmcConn sSMCConn = NULL;

static WMHandlerID sSMInputHandler = NULL;

/* our SM client ID */
static char *sClientID = NULL;
#endif


static proplist_t sApplications = NULL;
static proplist_t sCommand;
static proplist_t sName;
static proplist_t sHost;
static proplist_t sWorkspace;
static proplist_t sShaded;
static proplist_t sMiniaturized;
static proplist_t sHidden;
static proplist_t sGeometry;
static proplist_t sShortcutMask;

static proplist_t sDock;

static proplist_t sYes, sNo;


static void
make_keys()
{
    if (sApplications!=NULL)
        return;

    sApplications = PLMakeString("Applications");
    sCommand = PLMakeString("Command");
    sName = PLMakeString("Name");
    sHost = PLMakeString("Host");
    sWorkspace = PLMakeString("Workspace");
    sShaded = PLMakeString("Shaded");
    sMiniaturized = PLMakeString("Miniaturized");
    sHidden = PLMakeString("Hidden");
    sGeometry = PLMakeString("Geometry");
    sDock = PLMakeString("Dock");
    sShortcutMask = PLMakeString("ShortcutMask");

    sYes = PLMakeString("Yes");
    sNo = PLMakeString("No");
}



static int 
getBool(proplist_t value)
{
    char *val;

    if (!PLIsString(value)) {
        return 0;
    }
    if (!(val = PLGetString(value))) {
        return 0;
    }

    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y'))
	|| strcasecmp(val, "YES")==0) {

	return 1;
    } else if ((val[1]=='\0' && (val[0]=='n' || val[0]=='N'))
	     || strcasecmp(val, "NO")==0) {
	return 0;
    } else {
	int i;
	if (sscanf(val, "%i", &i)==1) {
	    return (i!=0);
	} else {
	    wwarning(_("can't convert \"%s\" to boolean"), val);
	    return 0;
	}
    }
}


static unsigned
getInt(proplist_t value)
{
    char *val;
    unsigned n;
    
    if (!PLIsString(value))
	return 0;
    val = PLGetString(value);
    if (!val)
	return 0;
    if (sscanf(val, "%u", &n) != 1)
	return 0;

    return n;
}



static proplist_t
makeWindowState(WWindow *wwin, WApplication *wapp)
{
    WScreen *scr = wwin->screen_ptr;
    Window win;
    int argc;
    char **argv;
    int i;
    unsigned mask;
    char *class, *instance, *command=NULL, buffer[256];
    proplist_t win_state, cmd, name, workspace;
    proplist_t shaded, miniaturized, hidden, geometry;
    proplist_t dock, shortcut;

    if (wwin->main_window!=None && wwin->main_window!=wwin->client_win)
        win = wwin->main_window;
    else
        win = wwin->client_win;

    if (XGetCommand(dpy, win, &argv, &argc) && argc>0) {
        command = FlattenStringList(argv, argc);
        XFreeStringList(argv);
    }
    if (!command)
        return NULL;

    if (PropGetWMClass(win, &class, &instance)) {
        if (class && instance)
            sprintf(buffer, "%s.%s", instance, class);
        else if (instance)
            sprintf(buffer, "%s", instance);
        else if (class)
            sprintf(buffer, ".%s", class);
        else
            sprintf(buffer, ".");

        name = PLMakeString(buffer);
        cmd = PLMakeString(command);
        /*sprintf(buffer, "%d", wwin->frame->workspace+1);
        workspace = PLMakeString(buffer);*/
        workspace = PLMakeString(scr->workspaces[wwin->frame->workspace]->name);
        shaded = wwin->flags.shaded ? sYes : sNo;
        miniaturized = wwin->flags.miniaturized ? sYes : sNo;
        hidden = wwin->flags.hidden ? sYes : sNo;
        sprintf(buffer, "%ix%i+%i+%i", wwin->client.width, wwin->client.height,
		wwin->frame_x, wwin->frame_y);
        geometry = PLMakeString(buffer);
	
	for (mask = 0, i = 0; i < MAX_WINDOW_SHORTCUTS; i++) {
	    if (scr->shortcutWindows[i] != NULL &&
		WMGetFirstInBag(scr->shortcutWindows[i], wwin) != WBNotFound) {
		mask |= 1<<i;
	    }
	}

	sprintf(buffer, "%u", mask);
	shortcut = PLMakeString(buffer);

        win_state = PLMakeDictionaryFromEntries(sName, name,
                                                sCommand, cmd,
                                                sWorkspace, workspace,
                                                sShaded, shaded,
                                                sMiniaturized, miniaturized,
                                                sHidden, hidden,
						sShortcutMask, shortcut,
                                                sGeometry, geometry,
                                                NULL);

        PLRelease(name);
        PLRelease(cmd);
        PLRelease(workspace);
        PLRelease(geometry);
	PLRelease(shortcut);
        if (wapp && wapp->app_icon && wapp->app_icon->dock) {
            int i;
            char *name;
            if (wapp->app_icon->dock == scr->dock) {
                name="Dock";
            } else {
                for(i=0; i<scr->workspace_count; i++)
                    if(scr->workspaces[i]->clip == wapp->app_icon->dock)
                        break;
                assert( i < scr->workspace_count);
                /*n = i+1;*/
                name = scr->workspaces[i]->name;
            }
            dock = PLMakeString(name);
            PLInsertDictionaryEntry(win_state, sDock, dock);
            PLRelease(dock);
        }
    } else {
        win_state = NULL;
    }

    if (instance) XFree(instance);
    if (class) XFree(class);
    if (command) free(command);

    return win_state;
}


void
wSessionSaveState(WScreen *scr)
{
    WWindow *wwin = scr->focused_window;
    proplist_t win_info, wks;
    proplist_t list=NULL;
    WMBag *wapp_list=NULL;


    make_keys();

    if (!scr->session_state) {
        scr->session_state = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
        if (!scr->session_state)
            return;
    }

    list = PLMakeArrayFromElements(NULL);

    wapp_list = WMCreateBag(16);

    while (wwin) {
        WApplication *wapp=wApplicationOf(wwin->main_window);

        if (wwin->transient_for==None 
	    && WMGetFirstInBag(wapp_list, wapp)==WBNotFound
	    && !WFLAGP(wwin, dont_save_session)) {
            /* A entry for this application was not yet saved. Save one. */
            if ((win_info = makeWindowState(wwin, wapp))!=NULL) {
                list = PLAppendArrayElement(list, win_info);
                PLRelease(win_info);
                /* If we were succesful in saving the info for this window
                 * add the application the window belongs to, to the
                 * application list, so no multiple entries for the same
                 * application are saved.
                 */
		WMPutInBag(wapp_list, wapp);
            }
        }
        wwin = wwin->prev;
    }
    PLRemoveDictionaryEntry(scr->session_state, sApplications);
    PLInsertDictionaryEntry(scr->session_state, sApplications, list);
    PLRelease(list);

    wks = PLMakeString(scr->workspaces[scr->current_workspace]->name);
    PLInsertDictionaryEntry(scr->session_state, sWorkspace, wks);
    PLRelease(wks);

    WMFreeBag(wapp_list);
}


void
wSessionClearState(WScreen *scr)
{
    make_keys();

    if (!scr->session_state)
        return;

    PLRemoveDictionaryEntry(scr->session_state, sApplications);
    PLRemoveDictionaryEntry(scr->session_state, sWorkspace);
}


static pid_t
execCommand(WScreen *scr, char *command, char *host)
{
    pid_t pid;
    char **argv;
    int argc;

    ParseCommand(command, &argv, &argc);

    if (argv==NULL) {
        return 0;
    }

    if ((pid=fork())==0) {
        char **args;
        int i;

	SetupEnvironment(scr);

        args = malloc(sizeof(char*)*(argc+1));
        if (!args)
            exit(111);
        for (i=0; i<argc; i++) {
            args[i] = argv[i];
        }
        args[argc] = NULL;
        execvp(argv[0], args);
        exit(111);
    }
    while (argc > 0)
        free(argv[--argc]);
    free(argv);
    return pid;
}


static WSavedState*
getWindowState(WScreen *scr, proplist_t win_state)
{
    WSavedState *state = wmalloc(sizeof(WSavedState));
    proplist_t value;
    char *tmp;
    unsigned mask;
    int i;

    memset(state, 0, sizeof(WSavedState));
    state->workspace = -1;
    value = PLGetDictionaryEntry(win_state, sWorkspace);
    if (value && PLIsString(value)) {
        tmp = PLGetString(value);
        if (sscanf(tmp, "%i", &state->workspace)!=1) {
            state->workspace = -1;
            for (i=0; i < scr->workspace_count; i++) {
                if (strcmp(scr->workspaces[i]->name, tmp)==0) {
                    state->workspace = i;
                    break;
                }
            }
        } else {
            state->workspace--;
        }
    }
    if ((value = PLGetDictionaryEntry(win_state, sShaded))!=NULL)
        state->shaded = getBool(value);
    if ((value = PLGetDictionaryEntry(win_state, sMiniaturized))!=NULL)
        state->miniaturized = getBool(value);
    if ((value = PLGetDictionaryEntry(win_state, sHidden))!=NULL)
        state->hidden = getBool(value);
    if ((value = PLGetDictionaryEntry(win_state, sShortcutMask))!=NULL) {
        mask = getInt(value);
	state->window_shortcuts = mask;
    }
    
    value = PLGetDictionaryEntry(win_state, sGeometry);
    if (value && PLIsString(value)) {
        if (!(sscanf(PLGetString(value), "%ix%i+%i+%i",
                   &state->w, &state->h, &state->x, &state->y)==4 &&
	      (state->w>0 && state->h>0))) {
	    state->w = 0;
	    state->h = 0;
	}
    }

    return state;
}


#define SAME(x, y) (((x) && (y) && !strcmp((x), (y))) || (!(x) && !(y)))

void
wSessionRestoreState(WScreen *scr)
{
    WSavedState *state;
    char *instance, *class, *command, *host;
    proplist_t win_info, apps, cmd, value;
    pid_t pid;
    int i, count;
    WDock *dock;
    WAppIcon *btn=NULL;
    int j, n, found;
    char *tmp;

    make_keys();

    if (!scr->session_state)
        return;

    PLSetStringCmpHook(NULL);

    apps = PLGetDictionaryEntry(scr->session_state, sApplications);
    if (!apps)
        return;

    count = PLGetNumberOfElements(apps);
    if (count==0) 
        return;

    for (i=0; i<count; i++) {
        win_info = PLGetArrayElement(apps, i);

        cmd = PLGetDictionaryEntry(win_info, sCommand);
        if (!cmd || !PLIsString(cmd) || !(command = PLGetString(cmd))) {
            continue;
        }

        value = PLGetDictionaryEntry(win_info, sName);
        if (!value)
            continue;

        ParseWindowName(value, &instance, &class, "session");
        if (!instance && !class)
            continue;

        value = PLGetDictionaryEntry(win_info, sHost);
        if (value && PLIsString(value))
            host = PLGetString(value);
        else
            host = NULL;

        state = getWindowState(scr, win_info);

        dock = NULL;
        value = PLGetDictionaryEntry(win_info, sDock);
        if (value && PLIsString(value) && (tmp = PLGetString(value))!=NULL) {
            if (sscanf(tmp, "%i", &n)!=1) {
                if (!strcasecmp(tmp, "DOCK")) {
                    dock = scr->dock;
                } else {
                    for (j=0; j < scr->workspace_count; j++) {
                        if (strcmp(scr->workspaces[j]->name, tmp)==0) {
                            dock = scr->workspaces[j]->clip;
                            break;
                        }
                    }
                }
            } else {
                if (n == 0) {
                    dock = scr->dock;
                } else if (n>0 && n<=scr->workspace_count) {
                    dock = scr->workspaces[n-1]->clip;
                }
            }
        }

        found = 0;
        if (dock!=NULL) {
            for (j=0; j<dock->max_icons; j++) {
                btn = dock->icon_array[j];
                if (btn && SAME(instance, btn->wm_instance) &&
                    SAME(class, btn->wm_class) &&
                    SAME(command, btn->command) &&
		    !btn->launching) {
                    found = 1;
                    break;
                }
            }
        }

        if (found) {
            wDockLaunchWithState(dock, btn, state);
        } else if ((pid = execCommand(scr, command, host)) > 0) {
            wWindowAddSavedState(instance, class, command, pid, state);
        } else {
            free(state);
        }

        if (instance) free(instance);
        if (class) free(class);
    }
    /* clean up */
    PLSetStringCmpHook(StringCompareHook);
}


void
wSessionRestoreLastWorkspace(WScreen *scr)
{
    proplist_t wks;
    int w, i;
    char *tmp;

    make_keys();

    if (!scr->session_state)
        return;

    PLSetStringCmpHook(NULL);

    wks = PLGetDictionaryEntry(scr->session_state, sWorkspace);
    if (!wks || !PLIsString(wks))
        return;

    tmp = PLGetString(wks);

    /* clean up */
    PLSetStringCmpHook(StringCompareHook);

    if (sscanf(tmp, "%i", &w)!=1) {
        w = -1;
        for (i=0; i < scr->workspace_count; i++) {
            if (strcmp(scr->workspaces[i]->name, tmp)==0) {
                w = i;
                break;
            }
        }
    } else {
        w--;
    }

    if (w!=scr->current_workspace && w<scr->workspace_count) {
        wWorkspaceChange(scr, w);
    }
}


static void
clearWaitingAckState(WScreen *scr)
{
    WWindow *wwin;
    WApplication *wapp;

    for (wwin = scr->focused_window; wwin != NULL; wwin = wwin->prev) {
	wwin->flags.waiting_save_ack = 0;
	if (wwin->main_window != None) {
	    wapp = wApplicationOf(wwin->main_window);
	    if (wapp)
		wapp->main_window_desc->flags.waiting_save_ack = 0;
	}
    }
}


void
wSessionSaveClients(WScreen *scr)
{
    
}
		    

/*
 * With XSMP, this job is done by smproxy
 */
void
wSessionSendSaveYourself(WScreen *scr)
{
    WWindow *wwin;
    int count;

    /* freeze client interaction with clients */
    XGrabKeyboard(dpy, scr->root_win, False, GrabModeAsync, GrabModeAsync,
		  CurrentTime);
    XGrabPointer(dpy, scr->root_win, False, ButtonPressMask|ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, scr->root_win, None,
		 CurrentTime);

    clearWaitingAckState(scr);

    count = 0;

    /* first send SAVE_YOURSELF for everybody */
    for (wwin = scr->focused_window; wwin != NULL; wwin = wwin->prev) {
	WWindow *mainWin;

	mainWin = wWindowFor(wwin->main_window);

	if (mainWin) {
	    /* if the client is a multi-window client, only send message
	     * to the main window */
	    wwin = mainWin;
	}

	/* make sure the SAVE_YOURSELF flag is up-to-date */
	PropGetProtocols(wwin->client_win, &wwin->protocols);

	if (wwin->protocols.SAVE_YOURSELF) {
	    if (!wwin->flags.waiting_save_ack) {
		wClientSendProtocol(wwin, _XA_WM_SAVE_YOURSELF, LastTimestamp);

		wwin->flags.waiting_save_ack = 1;
		count++;
	    }
	} else {
	    wwin->flags.waiting_save_ack = 0;
	}
    }

    /* then wait for acknowledge */
    while (count > 0) {
	
    }

    XUngrabPointer(dpy, CurrentTime);
    XUngrabKeyboard(dpy, CurrentTime);
    XFlush(dpy);
}


#ifdef XSMP_ENABLED
/*
 * With full session management support, the part of WMState 
 * that store client window state will become obsolete (maybe we can reuse
 *							the old code too),
 * but we still need to store state info like the dock and workspaces.
 * It is better to keep dock/wspace info in WMState because the user
 * might want to keep the dock configuration while not wanting to
 * resume a previously saved session. 
 * So, wmaker specific state info can be saved in 
 * ~/GNUstep/.AppInfo/WindowMaker/statename.state 
 * Its better to not put it in the defaults directory because:
 * - its not a defaults file (having domain names like wmaker0089504baa
 * in the defaults directory wouldn't be very neat)
 * - this state file is not meant to be edited by users
 * 
 * The old session code will become obsolete. When wmaker is
 * compiled with R6 sm support compiled in, itll be better to
 * use a totally rewritten state saving code, but we can keep
 * the current code for when XSMP_ENABLED is not compiled in. 
 * 
 * This will be confusing to old users (well get lots of "SAVE_SESSION broke!"
 * messages), but itll be better.
 * 
 * -readme
 */


static char*
getWindowRole(Window window)
{
    XTextProperty prop;
    static Atom atom = 0;
    
    if (!atom)
	atom = XInternAtom(dpy, "WM_WINDOW_ROLE", False);

    if (XGetTextProperty(dpy, window, &prop, atom)) {
	if (prop.encoding == XA_STRING && prop.format == 8 && prop.nitems > 0)
	    return prop.value;
    }
    
    return NULL;
}


/*
 *
 * Saved Info:
 *
 * WM_WINDOW_ROLE
 *
 * WM_CLASS.instance
 * WM_CLASS.class
 * WM_NAME
 * WM_COMMAND
 * 
 * geometry
 * state = (miniaturized, shaded, etc)
 * attribute
 * workspace #
 * app state = (which dock, hidden)
 * window shortcut #
 */

static proplist_t
makeAppState(WWindow *wwin)
{
    WApplication *wapp;
    proplist_t state;
    WScreen *scr = wwin->screen_ptr;
        
    state = PLMakeArrayWithElements(NULL, NULL);

    wapp = wApplicationOf(wwin->main_window);
    
    if (wapp) {
        if (wapp->app_icon && wapp->app_icon->dock) {

	    if (wapp->app_icon->dock == scr->dock) {
		PLAppendArrayElement(state, PLMakeString("Dock"));
	    } else {
		int i;

                for(i=0; i<scr->workspace_count; i++)
                    if(scr->workspaces[i]->clip == wapp->app_icon->dock)
                        break;

                assert(i < scr->workspace_count);

                PLAppendArrayElement(state, 
				     PLMakeString(scr->workspaces[i]->name));
	    }
	}
	
	PLAppendArrayElement(state, PLMakeString(wapp->hidden ? "1" : "0"));
    }
    
    return state;
}



Bool
wSessionGetStateFor(WWindow *wwin, WSessionData *state)
{
    char *str;
    proplist_t slist;
    proplist_t elem;
    proplist_t value;
    int index = 0;

    index = 3;

    /* geometry */
    value = PLGetArrayElement(slist, index++);    
    str = PLGetString(value);

    sscanf(str, "%i %i %i %i %i %i", &state->x, &state->y,
	   &state->width, &state->height,
	   &state->user_changed_width, &state->user_changed_height);


    /* state */
    value = PLGetArrayElement(slist, index++);
    str = PLGetString(value);

    sscanf(str, "%i %i %i", &state->miniaturized, &state->shaded,
	   &state->maximized);


    /* attributes */
    value = PLGetArrayElement(slist, index++);
    str = PLGetString(value);

    getAttributeState(str, &state->mflags, &state->flags);


    /* workspace */
    value = PLGetArrayElement(slist, index++);
    str = PLGetString(value);

    sscanf(str, "%i", &state->workspace);


    /* app state (repeated for all windows of the app) */
    value = PLGetArrayElement(slist, index++);
    str = PLGetString(value);

    /* ???? */

    /* shortcuts */
    value = PLGetArrayElement(slist, index++);
    str = PLGetString(value);

    sscanf(str, "%i", &state->shortcuts);
}



static proplist_t
makeAttributeState(WWindow *wwin)
{
    unsigned int data1, data2;
    char buffer[256];

#define W_FLAG(wwin, FLAG)	((wwin)->defined_user_flags.FLAG \
					? (wwin)->user_flags.FLAG : -1)

    sprintf(buffer, 
	    "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
	    W_FLAG(no_titlebar),
	    W_FLAG(no_resizable),
	    W_FLAG(no_closable),
	    W_FLAG(no_miniaturizable),
	    W_FLAG(no_resizebar),
	    W_FLAG(no_close_button),
	    W_FLAG(no_miniaturize_button),
	    /*
	    W_FLAG(broken_close),
	    W_FLAG(kill_close),
	     */
	    W_FLAG(no_shadeable),
	    W_FLAG(omnipresent),
	    W_FLAG(skip_window_list),
	    W_FLAG(floating),
	    W_FLAG(sunken),
	    W_FLAG(no_bind_keys),
	    W_FLAG(no_bind_mouse),
	    W_FLAG(no_hide_others),
	    W_FLAG(no_appicon),
	    W_FLAG(dont_move_off),
	    W_FLAG(no_focusable),
	    W_FLAG(always_user_icon),
	    W_FLAG(start_miniaturized),
	    W_FLAG(start_hidden),
	    W_FLAG(start_maximized),
	    W_FLAG(dont_save_session),
	    W_FLAG(emulate_appicon));

    return PLMakeString(buffer);
}


static void
appendStringInArray(proplist_t array, char *str)
{
    proplist_t val;
    
    val = PLMakeString(str);
    PLAppendArrayElement(array, val);
    PLRelease(val);
}


static proplist_t
makeClientState(WWindow *wwin)
{
    proplist_t state;
    proplist_t tmp;
    char *str;
    char buffer[256];
    int i;
    unsigned shortcuts;

    state = PLMakeArrayWithElements(NULL, NULL);
    
    /* WM_WINDOW_ROLE */
    str = getWindowRole(wwin->client_win);
    if (!str)
	appendStringInArray(state, "");
    else {
	appendStringInArray(state, str);
	XFree(str);
    }

    /* WM_CLASS.instance */
    appendStringInArray(state, wwin->wm_instance);

    /* WM_CLASS.class */
    appendStringInArray(state, wwin->wm_class);

    /* WM_NAME */
    if (wwin->flags.wm_name_changed)
	appendStringInArray(state, "");
    else
	appendStringInArray(state, wwin->frame->name);

    /* geometry */
    sprintf(buffer, "%i %i %i %i %i %i", wwin->frame_x, wwin->frame_y,
	    wwin->client.width, wwin->client.height,
	    wwin->flags.user_changed_width, wwin->flags.user_changed_height);
    appendStringInArray(state, buffer);

    /* state */
    sprintf(buffer, "%i %i %i", wwin->flags.miniaturized,
	    wwin->flags.shaded, wwin->flags.maximized);
    appendStringInArray(state, buffer);

    /* attributes */
    tmp = makeAttributeState(wwin);
    PLAppendArrayElement(state, tmp);
    PLRelease(tmp);

    /* workspace */
    sprintf(buffer, "%i", wwin->frame->workspace);
    appendStringInArray(state, buffer);

    /* app state (repeated for all windows of the app) */
    tmp = makeAppState(wwin);
    PLAppendArrayElement(state, tmp);
    PLRelease(tmp);

    /* shortcuts */
    shortcuts = 0;
    for (i = 0; i < MAX_WINDOW_SHORTCUTS; i++) {
	if (scr->shortcutWindow[i] == wwin) {
	    shortcuts |= 1 << i;
	}
    }
    sprintf(buffer, "%ui", shortcuts);
    appendStringInArray(tmp, buffer);

    return state;
}


static void
smSaveYourselfPhase2Proc(SmcConn smc_conn, SmPointer client_data)
{
    SmProp props[4];
    SmPropValue prop1val, prop2val, prop3val, prop4val;
    char **argv = (char**)client_data;
    int argc;
    int i, j;
    Bool ok = False;
    char *statefile = NULL;
    char *prefix;
    Bool gsPrefix = False;
    char *discardCmd = NULL;
    time_t t;
    proplist_t state;

#ifdef DEBUG1
    puts("received SaveYourselfPhase2 SM message");
#endif

    /* save session state */
    
    /* the file that will contain the state */
    prefix = getenv("SM_SAVE_DIR");
    if (!prefix) {
	prefix = wusergnusteppath();
	if (prefix)
	    gsPrefix = True;
    }
    if (!prefix) {
	prefix = getenv("HOME");
    }
    if (!prefix)
	prefix = ".";

    statefile = malloc(strlen(prefix)+64);
    if (!statefile) {
	wwarning(_("out of memory while saving session state"));
	goto fail;
    }

    t = time();
    i = 0;
    do {
	if (gsPrefix)
	    sprintf(statefile, "%s/.AppInfo/WindowMaker/wmaker.%l%i.state",
		    prefix, t, i);
	else
	    sprintf(statefile, "%s/wmaker.%l%i.state", prefix, t, i);
	i++;
    } while (access(F_OK, statefile)!=-1);

    /* save the states of all windows we're managing */
    state = PLMakeArrayFromElements(NULL, NULL);

    /*
     * Format:
     *
     * state_file ::= dictionary with version_info ; state
     * version_info ::= 'version' = '1';
     * state ::= 'state' = array of screen_info
     * screen_info ::= array of (screen number, window_info, window_info, ...)
     * window_info ::= 
     */
    for (i=0; i<wScreenCount; i++) {
	WScreen *scr;
	WWindow *wwin;
	char buf[32];
	proplist_t pscreen;
	
	scr = wScreenWithNumber(i);

	sprintf(buf, "%i", scr->screen);
	pscreen = PLMakeArrayFromElements(PLMakeString(buf), NULL);

	wwin = scr->focused_window;
	while (wwin) {
	    proplist_t pwindow;

	    pwindow = makeClientState(wwin);
	    PLAppendArrayElement(pscreen, pwindow);

	    wwin = wwin->prev;
	}

	PLAppendArrayElement(state, pscreen);
    }

    {
	proplist_t statefile;

	statefile = PLMakeDictionaryFromEntries(PLMakeString("Version"),
						PLMakeString("1.0"),

						PLMakeString("Screens"),
						state,

						NULL);

	PLSetFilename(statefile, PLMakeString(statefile));
	PLSave(statefile, NO);

	PLRelease(statefile);
    }

    /* set the remaining properties that we didn't set at
     * startup time */

    for (argc=0, i=0; argv[i]!=NULL; i++) {
	if (strcmp(argv[i], "-clientid")==0
	    || strcmp(argv[i], "-restore")==0) {
	    i++;
	} else {
	    argc++;
	}
    }

    prop[0].name = SmRestartCommand;
    prop[0].type = SmLISTofARRAY8;
    prop[0].vals = malloc(sizeof(SmPropValue)*(argc+4));
    prop[0].num_vals = argc+4;

    prop[1].name = SmCloneCommand;
    prop[1].type = SmLISTofARRAY8;
    prop[1].vals = malloc(sizeof(SmPropValue)*(argc));
    prop[1].num_vals = argc;

    if (!prop[0].vals || !prop[1].vals) {
	wwarning(_("end of memory while saving session state"));
	goto fail;
    }

    for (j=0, i=0; i<argc+4; i++) {
	if (strcmp(argv[i], "-clientid")==0
	    || strcmp(argv[i], "-restore")==0) {
	    i++;
	} else {
	    prop[0].vals[j].value = argv[i];
	    prop[0].vals[j].length = strlen(argv[i]);
	    prop[1].vals[j].value = argv[i];
	    prop[1].vals[j].length = strlen(argv[i]);
	    j++;
	}
    }
    prop[0].vals[j].value = "-clientid";
    prop[0].vals[j].length = 9;
    j++;
    prop[0].vals[j].value = sClientID;
    prop[0].vals[j].length = strlen(sClientID);
    j++;
    prop[0].vals[j].value = "-restore";
    prop[0].vals[j].length = 11;
    j++;
    prop[0].vals[j].value = statefile;
    prop[0].vals[j].length = strlen(statefile);

    discardCmd = malloc(strlen(statefile)+8);
    if (!discardCmd)
	goto fail;
    sprintf(discardCmd, "rm %s", statefile);
    prop[2].name = SmDiscardCommand;
    prop[2].type = SmARRAY8;
    prop[2].vals[0] = discardCmd;
    prop[2].num_vals = 1;

    SmcSetProperties(sSMCConn, 3, prop);

    ok = True;
fail:
    SmcSaveYourselfDone(smc_conn, ok);
    
    if (prop[0].vals)
	free(prop[0].vals);
    if (prop[1].vals)
	free(prop[1].vals);
    if (discardCmd)
	free(discardCmd);

    if (!ok) {
	remove(statefile);
    }
    if (statefile)
	free(statefile);
}


static void
smSaveYourselfProc(SmcConn smc_conn, SmPointer client_data, int save_type,
		   Bool shutdown, int interact_style, Bool fast)
{
#ifdef DEBUG1
    puts("received SaveYourself SM message");
#endif

    if (!SmcRequestSaveYourselfPhase2(smc_conn, smSaveYourselfPhase2Proc,
				      client_data)) {

	SmcSaveYourselfDone(smc_conn, False);
	sWaitingPhase2 = False;
    } else {
#ifdef DEBUG1
	puts("successfull request of SYS phase 2");
#endif
	sWaitingPhase2 = True;
    }
}


static void
smDieProc(SmcConn smc_conn, SmPointer client_data)
{
#ifdef DEBUG1
    puts("received Die SM message");
#endif

    wSessionDisconnectManager();

    Shutdown(WSExitMode, True);
}



static void
smSaveCompleteProc(SmcConn smc_conn)
{
    /* it means that we can resume doing things that can change our state */
#ifdef DEBUG1
    puts("received SaveComplete SM message");
#endif
}


static void
smShutdownCancelledProc(SmcConn smc_conn, SmPointer client_data)
{
    if (sWaitingPhase2) {

	sWaitingPhase2 = False;

	SmcSaveYourselfDone(smc_conn, False);
    }
}


static void
iceMessageProc(int fd, int mask, void *clientData)
{
    IceConn iceConn = (IceConn)clientData;

    IceProcessMessages(iceConn, NULL, NULL);
}


static void
iceIOErrorHandler(IceConnection ice_conn)
{
    /* This is not fatal but can mean the session manager exited.
     * If the session manager exited normally we would get a 
     * Die message, so this probably means an abnormal exit.
     * If the sm was the last client of session, then we'll die
     * anyway, otherwise we can continue doing our stuff.
     */
    wwarning(_("connection to the session manager was lost"));
    wSessionDisconnectManager();
}


void
wSessionConnectManager(char **argv, int argc)
{
    IceConn iceConn;
    char *previous_id = NULL;
    char buffer[256];
    SmcCallbacks callbacks;
    unsigned long mask;
    char uid[32];
    char pid[32];
    SmProp props[4];
    SmPropValue prop1val, prop2val, prop3val, prop4val;
    char restartStyle;
    int i;

    mask = SmcSaveYourselfProcMask|SmcDieProcMask|SmcSaveCompleteProcMask
	|SmcShutdownCancelledProcMask;

    callbacks.save_yourself.callback = smSaveYourselfProc;
    callbacks.save_yourself.client_data = argv;

    callbacks.die.callback = smDieProc;
    callbacks.die.client_data = NULL;

    callbacks.save_complete.callback = smSaveCompleteProc;
    callbacks.save_complete.client_data = NULL;

    callbacks.shutdown_cancelled.callback = smShutdownCancelledProc;
    callbacks.shutdown_cancelled.client_data = NULL;

    for (i=0; i<argc; i++) {
	if (strcmp(argv[i], "-clientid")==0) {
	    previous_id = argv[i+1];
	    break;
	}
    }

    /* connect to the session manager */
    sSMCConn = SmcOpenConnection(NULL, NULL, SmProtoMajor, SmProtoMinor,
				 mask, &callbacks, previous_id,
				 &sClientID, 255, buffer);
    if (!sSMCConn) {
	return;
    }
#ifdef DEBUG1
    puts("connected to the session manager");
#endif

/*    IceSetIOErrorHandler(iceIOErrorHandler);*/

    /* check for session manager clients */
    iceConn = SmcGetIceConnection(smcConn);

    if (fcntl(IceConnectionNumber(iceConn), F_SETFD, FD_CLOEXEC) < 0) {
	wsyserror("error setting close-on-exec flag for ICE connection");
    }

    sSMInputHandler = WMAddInputHandler(IceConnectionNumber(iceConn), 
					WIReadMask, iceMessageProc, iceConn);

    /* setup information about ourselves */

    /* program name */
    prop1val.value = argv[0];
    prop1val.length = strlen(argv[0]);
    prop[0].name = SmProgram;
    prop[0].type = SmARRAY8;
    prop[0].num_vals = 1;
    prop[0].vals = &prop1val;

    /* The XSMP doc from X11R6.1 says it contains the user name,
     * but every client implementation I saw places the uid # */
    sprintf(uid, "%i", getuid());
    prop2val.value = uid;
    prop2val.length = strlen(uid);
    prop[1].name = SmUserID;
    prop[1].type = SmARRAY8;
    prop[1].num_vals = 1;
    prop[1].vals = &prop2val;

    /* Restart style. We should restart only if we were running when
     * the previous session finished. */
    restartStyle = SmRestartIfRunning;
    prop3val.value = &restartStyle;
    prop3val.length = 1;
    prop[2].name = SmRestartStyleHint;
    prop[2].type = SmCARD8;
    prop[2].num_vals = 1;
    prop[2].vals = &prop3val;

    /* Our PID. Not required but might be usefull */
    sprintf(pid, "%i", getpid());
    prop4val.value = pid;
    prop4val.length = strlen(pid);
    prop[3].name = SmProcessID;
    prop[3].type = SmARRAY8;
    prop[3].num_vals = 1;
    prop[3].vals = &prop4val;

    /* we'll set the rest of the hints later */

    SmcSetProperties(sSMCConn, 4, props);

}


void
wSessionDisconnectManager(void)
{
    if (sSMCConn) {
	WMDeleteInputHandler(sSMInputHandler);
	sSMInputHandler = NULL;

	SmcCloseConnection(sSMCConn, 0, NULL);
	sSMCConn = NULL;
    }
}

void
wSessionRequestShutdown(void)
{
    /* request a shutdown to the session manager */
    if (sSMCConn)
	SmcRequestSaveYourself(sSMCConn, SmSaveBoth, True, SmInteractStyleAny,
			       False, True);
}


Bool 
wSessionIsManaged(void)
{
    return sSMCConn!=NULL;
}

#endif /* !XSMP_ENABLED */
