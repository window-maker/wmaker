/*
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

#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


/* Xlocale.h and locale.h are the same if X_LOCALE is undefind in wconfig.h,
 * and if X_LOCALE is defined, X's locale emulating functions will be used.
 * See Xlocale.h for more information.
 */
#include <X11/Xlocale.h>

#define MAINFILE

#include "WindowMaker.h"
#include "window.h"
#include "funcs.h"
#include "menu.h"
#include "keybind.h"
#include "xmodifier.h"
#include "defaults.h"
#include "session.h"

#include <proplist.h>

/****** Global Variables ******/

/* general info */

Display	*dpy;

char *ProgName;

unsigned int ValidModMask = 0xff;

/* locale to use. NULL==POSIX or C */
char *Locale=NULL;

int wScreenCount=0;

WPreferences wPreferences;


proplist_t wDomainName;
proplist_t wAttributeDomainName;

WShortKey wKeyBindings[WKBD_LAST];

/* defaults domains */
WDDomain *WDWindowMaker = NULL;
WDDomain *WDRootMenu = NULL;
WDDomain *WDWindowAttributes = NULL;


/* XContexts */
XContext wWinContext;
XContext wAppWinContext;
XContext wStackContext;

/* Atoms */
Atom _XA_WM_STATE;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_SAVE_YOURSELF;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_COLORMAP_NOTIFY;

Atom _XA_GNUSTEP_WM_ATTR;
Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;
Atom _XA_GNUSTEP_WM_RESIZEBAR;

Atom _XA_WINDOWMAKER_MENU;
Atom _XA_WINDOWMAKER_WM_PROTOCOLS;
Atom _XA_WINDOWMAKER_STATE;

Atom _XA_WINDOWMAKER_WM_FUNCTION;
Atom _XA_WINDOWMAKER_NOTICEBOARD;
Atom _XA_WINDOWMAKER_COMMAND;

#ifdef OFFIX_DND
Atom _XA_DND_PROTOCOL;
Atom _XA_DND_SELECTION;
#endif
#ifdef XDE_DND
Atom _XA_XDE_REQUEST;
Atom _XA_XDE_ENTER;
Atom _XA_XDE_LEAVE;
Atom _XA_XDE_DATA_AVAILABLE;
Atom _XDE_FILETYPE;
Atom _XDE_URLTYPE;
#endif


/* cursors */
Cursor wCursor[WCUR_LAST];

/* last event timestamp for XSetInputFocus */
Time LastTimestamp;
/* timestamp on the last time we did XSetInputFocus() */
Time LastFocusChange;

#ifdef SHAPE
Bool wShapeSupported;
int wShapeEventBase;
#endif


/* special flags */
char WProgramState = WSTATE_NORMAL;
char WDelayedActionSet = 0;

/* temporary stuff */
int wVisualID = -1;


/******** End Global Variables *****/

static char *DisplayName = NULL;
static char **Arguments;
static int ArgCount;

extern void EventLoop();
extern void StartUp();


void
Exit(int status)
{
#ifdef XSMP_ENABLED
    wSessionDisconnectManager();
#endif
    if (dpy)
	XCloseDisplay(dpy);

    exit(status);
}

void
Restart(char *manager)
{
    char *prog=NULL;
    char *argv[MAX_RESTART_ARGS];
    int i;

    if (manager && manager[0]!=0) {
	prog = argv[0] = strtok(manager, " ");
	for (i=1; i<MAX_RESTART_ARGS; i++) {
	    argv[i]=strtok(NULL, " ");
	    if (argv[i]==NULL) {
		break;
	    }
	}
    }
#ifdef XSMP_ENABLED
    wSessionDisconnectManager();
#endif
    XCloseDisplay(dpy);
    if (!prog)
      execvp(Arguments[0], Arguments);
    else {
	execvp(prog, argv);
	/* fallback */
	execv(Arguments[0], Arguments);
    }
    wsyserror(_("could not exec window manager"));
    wfatal(_("Restart failed!!!"));
    exit(-1);
}



void
SetupEnvironment(WScreen *scr)
{
    char *tmp, *ptr;
    char buf[16];

    if (wScreenCount > 1) {
    	tmp = wmalloc(strlen(DisplayName)+64);
    	sprintf(tmp, "DISPLAY=%s", XDisplayName(DisplayName));
    	ptr = strchr(strchr(tmp, ':'), '.');
    	if (ptr)
	    *ptr = 0;
    	sprintf(buf, ".%i", scr->screen);
    	strcat(tmp, buf);
    	putenv(tmp);
    }
    tmp = wmalloc(60);
    sprintf(tmp, "WRASTER_COLOR_RESOLUTION%i=%i", scr->screen, 
		scr->rcontext->attribs->colors_per_channel);
    putenv(tmp);
}


/*
 *---------------------------------------------------------------------
 * wAbort--
 * 	Do a major cleanup and exit the program
 * 
 *---------------------------------------------------------------------- 
 */
void
wAbort(Bool dumpCore)
{
    int i;
    WScreen *scr;

    for (i=0; i<wScreenCount; i++) {
	scr = wScreenWithNumber(i);
	if (scr)
	    RestoreDesktop(scr);
    }
    printf(_("%s aborted.\n"), ProgName);
    if (dumpCore)
	abort();
    else
	exit(1);
}


void
print_help()
{
    printf(_("Usage: %s [options]\n"), ProgName);
    puts(_("The Window Maker window manager for the X window system"));
    puts("");
    puts(_(" -display host:dpy	display to use"));
#ifdef USECPP
    puts(_(" --no-cpp 		disable preprocessing of configuration files"));
#endif
    puts(_(" --no-dock		do not open the application Dock"));
    puts(_(" --no-clip		do not open the workspace Clip"));
    /*
    puts(_(" --locale locale		locale to use"));
    */
    puts(_(" --visual-id visualid	visual id of visual to use"));
    puts(_(" --static		do not update or save configurations"));
    puts(_(" --version		print version and exit"));
    puts(_(" --help			show this message"));
}



void
check_defaults()
{
    char *path;

    path = wdefaultspathfordomain("");
    if (access(path, R_OK)!=0) {
	wfatal(_("could not find user GNUstep directory (%s).\n"
		 "Make sure you have installed Window Maker correctly and run wmaker.inst"),
	       path);
	exit(1);
    }
    
    free(path);
}


static void
execInitScript()
{
    char *file;
    
    file = wfindfile(DEF_CONFIG_PATHS, DEF_INIT_SCRIPT);
    if (file) {
	if (system(file) != 0) {
	    wsyserror(_("%s:could not execute initialization script"), file);
	}
#if 0
	if (fork()==0) {
	    execl("/bin/sh", "/bin/sh", "-c",file, NULL);
	    wsyserror(_("%s:could not execute initialization script"), file);
	    exit(1);
	}
#endif
	free(file);
    }
}


void
ExecExitScript()
{
    char *file;
    
    file = wfindfile(DEF_CONFIG_PATHS, DEF_EXIT_SCRIPT);
    if (file) {
	if (system(file) != 0) {
	    wsyserror(_("%s:could not execute exit script"), file);
	}
#if 0
	if (fork()==0) {
	    execl("/bin/sh", "/bin/sh", "-c", file, NULL);
	    wsyserror(_("%s:could not execute exit script"), file);
	    exit(1);
	}
#endif
	free(file);
    }
}



int
main(int argc, char **argv)
{
    int i, restart=0;
    Bool multiHead = True;
    char *str;
    int d, s;

    wsetabort(wAbort);
    
    ArgCount = argc;
    Arguments = argv;

    WMInitializeApplication("WindowMaker", &argc, argv);


    ProgName = strrchr(argv[0],'/');
    if (!ProgName)
      ProgName = argv[0];
    else
      ProgName++;


    restart = 0;
    
    memset(&wPreferences, 0, sizeof(WPreferences));
    
    if (argc>1) {
	for (i=1; i<argc; i++) {
#ifdef USECPP
	    if (strcmp(argv[i], "-nocpp")==0
		|| strcmp(argv[i], "--no-cpp")==0) {
		wPreferences.flags.nocpp=1;
	    } else
#endif
	    if (strcmp(argv[i], "-nodock")==0
		|| strcmp(argv[i], "--no-dock")==0) {
		wPreferences.flags.nodock=1;
	    } else if (strcmp(argv[i], "-noclip")==0
		       || strcmp(argv[i], "--no-clip")==0) {
		wPreferences.flags.noclip=1;
	    } else if (strcmp(argv[i], "-version")==0
		       || strcmp(argv[i], "--version")==0) {
		printf("Window Maker %s\n", VERSION);
		exit(0);
	    } else if (strcmp(argv[i], "--global_defaults_path")==0) {
		puts(SYSCONFDIR);
		exit(0);
	    } else if (strcmp(argv[i], "-locale")==0
		       || strcmp(argv[i], "--locale")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		Locale = argv[i];
	    } else if (strcmp(argv[i], "-display")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		DisplayName = argv[i];
	    } else if (strcmp(argv[i], "-visualid")==0
		       || strcmp(argv[i], "--visual-id")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		if (sscanf(argv[i], "%i", &wVisualID)!=1) {
		    wwarning(_("bad value for visualid: \"%s\""), argv[i]);
		    exit(0);
		}
	    } else if (strcmp(argv[i], "-static")==0
		       || strcmp(argv[i], "--static")==0) {

		wPreferences.flags.noupdates = 1;
#ifdef XSMP_ENABLED
	    } else if (strcmp(argv[i], "-clientid")==0
		       || strcmp(argv[i], "-restore")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
#endif
	    } else if (strcmp(argv[i], "--help")==0) {
		print_help();
		exit(0);
	    } else {
		printf(_("%s: invalid argument '%s'\n"), argv[0], argv[i]);
		printf(_("Try '%s --help' for more information\n"), argv[0]);
		exit(1);
	    }
	}
    }
    
    if (!wPreferences.flags.noupdates) {
	/* check existence of Defaults DB directory */
	check_defaults();
    }

#if 0
    tmp = getenv("LANG");
    if (tmp) {
    	if (setlocale(LC_ALL,"") == NULL) {
	    wwarning("cannot set locale %s", tmp);
	    wwarning("falling back to C locale");
	    setlocale(LC_ALL,"C");
	    Locale = NULL;
	} else {
	    if (strcmp(tmp, "C")==0 || strcmp(tmp, "POSIX")==0)
	      Locale = NULL;
	    else
	      Locale = tmp;
	}
    } else {
	Locale = NULL;
    }
#endif
    if (!Locale) {
    	Locale = getenv("LANG");
    }
    setlocale(LC_ALL, Locale);
    if (!Locale || strcmp(Locale, "C")==0 || strcmp(Locale, "POSIX")==0) 
      Locale = NULL;
#ifdef I18N
    if (getenv("NLSPATH"))
      bindtextdomain("WindowMaker", getenv("NLSPATH"));
    else
      bindtextdomain("WindowMaker", LOCALEDIR);
    textdomain("WindowMaker");

    if (!XSupportsLocale()) {
	wwarning(_("X server does not support locale"));
    }
    if (XSetLocaleModifiers("") == NULL) {
 	wwarning(_("cannot set locale modifiers"));
    }
#endif

    if (Locale) {
	char *ptr;

	Locale = wstrdup(Locale);
	ptr = strchr(Locale, '.');
	if (ptr)
	    *ptr = 0;
    }


    /* open display */
    dpy = XOpenDisplay(DisplayName);
    if (dpy == NULL) {
	wfatal(_("could not open display \"%s\""), XDisplayName(DisplayName));
	exit(1);
    }

    if (fcntl(ConnectionNumber(dpy), F_SETFD, FD_CLOEXEC) < 0) {
	wsyserror("error setting close-on-exec flag for X connection");
	exit(1);
    }

    /* check if the user specified a complete display name (with screen).
     * If so, only manage the specified screen */
    if (DisplayName)
	str = strchr(DisplayName, ':');
    else
	str = NULL;

    if (str && sscanf(str, "%i.%i", &d, &s)==2)
	multiHead = False;

    DisplayName = XDisplayName(DisplayName);
    str = wmalloc(strlen(DisplayName)+64);
    sprintf(str, "DISPLAY=%s", DisplayName);
    putenv(str);

#ifdef DEBUG
    XSynchronize(dpy, True);
#endif

    wXModifierInitialize();

#ifdef XSMP_ENABLED
    wSessionConnectManager(argv, argc);
#endif
    
    StartUp(!multiHead);

    execInitScript();

    EventLoop();
    return -1;
}
