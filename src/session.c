/* session.c - session state handling and R6 style session management
 *
 *  Copyright (c) 1998 Dan Pascu
 *  Copyright (c) 1998 Alfredo Kojima
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

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef R6SM
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
#include "session.h"
#include "wcore.h"
#include "framewin.h"
#include "workspace.h"
#include "funcs.h"
#include "properties.h"
#include "application.h"
#include "appicon.h"


#include "dock.h"

#include "list.h"

#include <proplist.h>


#ifdef R6SM
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



static proplist_t
makeWindowState(WWindow *wwin, WApplication *wapp)
{
    WScreen *scr = wwin->screen_ptr;
    Window win;
    int argc;
    char **argv;
    char *class, *instance, *command=NULL, buffer[256];
    proplist_t win_state, cmd, name, workspace;
    proplist_t shaded, miniaturized, hidden, geometry;
    proplist_t dock;

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

        win_state = PLMakeDictionaryFromEntries(sName, name,
                                                sCommand, cmd,
                                                sWorkspace, workspace,
                                                sShaded, shaded,
                                                sMiniaturized, miniaturized,
                                                sHidden, hidden,
                                                sGeometry, geometry,
                                                NULL);

        PLRelease(name);
        PLRelease(cmd);
        PLRelease(workspace);
        PLRelease(geometry);
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
    LinkedList *wapp_list=NULL;


    make_keys();

    if (!scr->session_state) {
        scr->session_state = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
        if (!scr->session_state)
            return;
    }

    list = PLMakeArrayFromElements(NULL);

    while (wwin) {
        WApplication *wapp=wApplicationOf(wwin->main_window);

        if (wwin->transient_for==None && list_find(wapp_list, wapp)==NULL
	    && !wwin->window_flags.dont_save_session) {
            /* A entry for this application was not yet saved. Save one. */
            if ((win_info = makeWindowState(wwin, wapp))!=NULL) {
                list = PLAppendArrayElement(list, win_info);
                PLRelease(win_info);
                /* If we were succesful in saving the info for this window
                 * add the application the window belongs to, to the
                 * application list, so no multiple entries for the same
                 * application are saved.
                 */
                wapp_list = list_cons(wapp, wapp_list);
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

    list_free(wapp_list);
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

	CloseDescriptors();

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

    value = PLGetDictionaryEntry(win_state, sGeometry);
    if (value && PLIsString(value)) {
        if (sscanf(PLGetString(value), "%ix%i+%i+%i",
                   &state->w, &state->h, &state->x, &state->y)==4 &&
            (state->w>0 && state->h>0)) {
            state->use_geometry = 1;
        } else if (sscanf(PLGetString(value), "%i,%i,%i,%i",
                   &state->x, &state->y, &state->w, &state->h)==4 &&
            (state->w>0 && state->h>0)) { 
	    /* TODO: remove redundant sscanf() in version 0.20.x */
            state->use_geometry = 1;
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
                    SAME(command, btn->command)) {
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



#ifdef R6SM
/*
 * With full session management support, the part of WMState 
 * that store client window state will become obsolete,
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
 * the current code for when R6SM is not compiled in. 
 * 
 * This will be confusing to old users (well get lots of "SAVE_SESSION broke!"
 * messages), but itll be better.
 * 
 * -readme
 */


/*
 * Windows are identified as:
 * 	WM_CLASS(instance.class).WM_WINDOW_ROLE
 * 
 * 
 */
static void
saveClientState(WWindow *wwin, proplist_t dict)
{
    proplist_t key;
    

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
    FILE *file;

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
	if (gsPrefix)
	    free(prefix);
	wwarning(_("end of memory while saving session state"));
	goto fail;
    }

    t = time();
    i = 0;
    do {
	if (gsPrefix)
	    sprintf(statefile, "%s/.AppInfo/WindowMaker/%l%i.state",
		    prefix, t, i);
	else
	    sprintf(statefile, "%s/wmaker.%l%i.state", prefix, t, i);
	i++;
    } while (access(F_OK, statefile)!=-1);

    if (gsPrefix)
	free(prefix);

    /* save the states of all windows we're managing */
    
    file = fopen(statefile, "w");
    if (!file) {
	wsyserror(_("could not create state file %s"), statefile);
	goto fail;
    }
    
    
    
    fclose(file);
    

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

    RestoreDesktop(NULL);

    ExecExitScript();
    Exit(0);
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
_wSessionCloseDescriptors(void)
{
    if (sSMCConn)
	close(IceConnectionNumber(SmcGetIceConnection(smcConn)));
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

#endif /* !R6SM */
