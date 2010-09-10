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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#include "wconfig.h"

#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#endif

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
#include "session.h"
#include "dialog.h"

#include <WINGs/WUtil.h>

#ifndef GLOBAL_DEFAULTS_SUBDIR
#define GLOBAL_DEFAULTS_SUBDIR "WindowMaker"
#endif

/****** Global Variables ******/

/* general info */

Display *dpy;

char *ProgName;

unsigned int ValidModMask = 0xff;

#ifdef HAVE_INOTIFY
int inotifyFD;
int inotifyWD;
#endif
/* locale to use. NULL==POSIX or C */
char *Locale = NULL;

int wScreenCount = 0;

WPreferences wPreferences;

WMPropList *wDomainName;
WMPropList *wAttributeDomainName;

WShortKey wKeyBindings[WKBD_LAST];

/* defaults domains */
WDDomain *WDWindowMaker = NULL;
WDDomain *WDWindowAttributes = NULL;
WDDomain *WDRootMenu = NULL;

/* XContexts */
XContext wWinContext;
XContext wAppWinContext;
XContext wStackContext;
XContext wVEdgeContext;

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
Atom _XA_GNUSTEP_TITLEBAR_STATE;

Atom _XA_WINDOWMAKER_MENU;
Atom _XA_WINDOWMAKER_WM_PROTOCOLS;
Atom _XA_WINDOWMAKER_STATE;

Atom _XA_WINDOWMAKER_WM_FUNCTION;
Atom _XA_WINDOWMAKER_NOTICEBOARD;
Atom _XA_WINDOWMAKER_COMMAND;

Atom _XA_WINDOWMAKER_ICON_SIZE;
Atom _XA_WINDOWMAKER_ICON_TILE;

Atom _XA_WM_IGNORE_FOCUS_EVENTS;

/* cursors */
Cursor wCursor[WCUR_LAST];

/* last event timestamp for XSetInputFocus */
Time LastTimestamp = CurrentTime;
/* timestamp on the last time we did XSetInputFocus() */
Time LastFocusChange = CurrentTime;

#ifdef SHAPE
Bool wShapeSupported;
int wShapeEventBase;
#endif

#ifdef KEEP_XKB_LOCK_STATUS
Bool wXkbSupported;
int wXkbEventBase;
#endif

#ifdef HAVE_XRANDR
Bool has_randr;
int randr_event_base;
#endif

/* special flags */
char WProgramSigState = 0;
char WProgramState = WSTATE_NORMAL;
char WDelayedActionSet = 0;

/* notifications */
const char *WMNManaged = "WMNManaged";
const char *WMNUnmanaged = "WMNUnmanaged";
const char *WMNChangedWorkspace = "WMNChangedWorkspace";
const char *WMNChangedState = "WMNChangedState";
const char *WMNChangedFocus = "WMNChangedFocus";
const char *WMNChangedStacking = "WMNChangedStacking";
const char *WMNChangedName = "WMNChangedName";

const char *WMNWorkspaceCreated = "WMNWorkspaceCreated";
const char *WMNWorkspaceDestroyed = "WMNWorkspaceDestroyed";
const char *WMNWorkspaceChanged = "WMNWorkspaceChanged";
const char *WMNWorkspaceNameChanged = "WMNWorkspaceNameChanged";

const char *WMNResetStacking = "WMNResetStacking";

/******** End Global Variables *****/

static char *DisplayName = NULL;

static char **Arguments;

static int ArgCount;

extern void EventLoop(void);
extern void StartUp(Bool defaultScreenOnly);
extern int MonitorLoop(int argc, char **argv);

static Bool multiHead = True;

static int *wVisualID = NULL;
static int wVisualID_len = 0;

static int real_main(int argc, char **argv);

int getWVisualID(int screen)
{
        if (wVisualID == NULL)
		return -1;
        if (screen < 0 || screen >= wVisualID_len)
		return -1;

        return wVisualID[screen];
}

static void setWVisualID(int screen, int val)
{
        int i;

        if (screen < 0)
		return;

        if (wVisualID == NULL) {
		/* no array at all, alloc space for screen + 1 entries
		 * and init with default value */
		wVisualID_len = screen + 1;
		wVisualID = (int *)malloc(wVisualID_len * sizeof(int));
		for (i = 0; i < wVisualID_len; i++) {
			wVisualID[i] = -1;
		}
	} else if (screen >= wVisualID_len) {
		/* larger screen number than previously allocated
		 so enlarge array */
		int oldlen = wVisualID_len;

		wVisualID_len = screen + 1;
		wVisualID = (int *)realloc(wVisualID, wVisualID_len * sizeof(int));
		for (i = oldlen; i < wVisualID_len; i++) {
			wVisualID[i] = -1;
		}
	}

	wVisualID[screen] = val;
}

/*
 * this function splits a given string at the comma into tokens
 * and set the wVisualID variable to each parsed number
 */
static int initWVisualID(const char *user_str)
{
	char *mystr = strdup(user_str);
	int cur_in_pos = 0;
	int cur_out_pos = 0;
	int cur_screen = 0;
	int error_found = 0;

	for (;;) {
		/* check for delimiter */
		if (user_str[cur_in_pos] == '\0' || user_str[cur_in_pos] == ',') {
			int v;

			mystr[cur_out_pos] = '\0';

			if (sscanf(mystr, "%i", &v) != 1) {
				error_found = 1;
				break;
			}

			setWVisualID(cur_screen, v);

			cur_screen++;
			cur_out_pos = 0;
		}

		/* break in case last char has been consumed */
		if (user_str[cur_in_pos] == '\0')
			break;

                /* if the current char is no delimiter put it into mystr */
		if (user_str[cur_in_pos] != ',') {
			mystr[cur_out_pos++] = user_str[cur_in_pos];
		}
		cur_in_pos++;
	}

	free(mystr);

	if (cur_screen == 0||error_found != 0)
		return 1;

	return 0;
}

void Exit(int status)
{
	if (dpy)
		XCloseDisplay(dpy);

	exit(status);
}

void Restart(char *manager, Bool abortOnFailure)
{
	char *prog = NULL;
	char *argv[MAX_RESTART_ARGS];
	int i;

	if (manager && manager[0] != 0) {
		prog = argv[0] = strtok(manager, " ");
		for (i = 1; i < MAX_RESTART_ARGS; i++) {
			argv[i] = strtok(NULL, " ");
			if (argv[i] == NULL) {
				break;
			}
		}
	}
	if (dpy) {
		XCloseDisplay(dpy);
		dpy = NULL;
	}
	if (!prog) {
		execvp(Arguments[0], Arguments);
		wfatal(_("failed to restart Window Maker."));
	} else {
		execvp(prog, argv);
		wsyserror(_("could not exec %s"), prog);
	}
	if (abortOnFailure)
		exit(7);
}

void SetupEnvironment(WScreen * scr)
{
	char *tmp, *ptr;
	char buf[16];

	if (multiHead) {
		int len = strlen(DisplayName) + 64;
		tmp = wmalloc(len);
		snprintf(tmp, len, "DISPLAY=%s", XDisplayName(DisplayName));
		ptr = strchr(strchr(tmp, ':'), '.');
		if (ptr)
			*ptr = 0;
		snprintf(buf, sizeof(buf), ".%i", scr->screen);
		strcat(tmp, buf);
		putenv(tmp);
	}
	tmp = wmalloc(60);
	snprintf(tmp, 60, "WRASTER_COLOR_RESOLUTION%i=%i", scr->screen,
		 scr->rcontext->attribs->colors_per_channel);
	putenv(tmp);
}

typedef struct {
	WScreen *scr;
	char *command;
} _tuple;

static void shellCommandHandler(pid_t pid, unsigned char status, _tuple * data)
{
	if (status == 127) {
		char *buffer;

		buffer = wstrconcat(_("Could not execute command: "), data->command);

		wMessageDialog(data->scr, _("Error"), buffer, _("OK"), NULL, NULL);
		wfree(buffer);
	} else if (status != 127) {
		/*
		   printf("%s: %i\n", data->command, status);
		 */
	}

	wfree(data->command);
	wfree(data);
}

void ExecuteShellCommand(WScreen * scr, char *command)
{
	static char *shell = NULL;
	pid_t pid;

	/*
	 * This have a problem: if the shell is tcsh (not sure about others)
	 * and ~/.tcshrc have /bin/stty erase ^H somewhere on it, the shell
	 * will block and the command will not be executed.
	 if (!shell) {
	 shell = getenv("SHELL");
	 if (!shell)
	 shell = "/bin/sh";
	 }
	 */
	shell = "/bin/sh";

	pid = fork();

	if (pid == 0) {

		SetupEnvironment(scr);

#ifdef HAVE_SETSID
		setsid();
#endif
		execl(shell, shell, "-c", command, NULL);
		wsyserror("could not execute %s -c %s", shell, command);
		Exit(-1);
	} else if (pid < 0) {
		wsyserror("cannot fork a new process");
	} else {
		_tuple *data = wmalloc(sizeof(_tuple));

		data->scr = scr;
		data->command = wstrdup(command);

		wAddDeathHandler(pid, (WDeathHandler *) shellCommandHandler, data);
	}
}

/*
 *---------------------------------------------------------------------
 * wAbort--
 * 	Do a major cleanup and exit the program
 *
 *----------------------------------------------------------------------
 */
void wAbort(Bool dumpCore)
{
	int i;
	WScreen *scr;

	for (i = 0; i < wScreenCount; i++) {
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

void print_help()
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
	puts(_(" --no-autolaunch	do not autolaunch applications"));
	puts(_(" --dont-restore		do not restore saved session"));

	puts(_(" --locale locale	locale to use"));

	puts(_(" --create-stdcmap	create the standard colormap hint in PseudoColor visuals"));
	puts(_(" --visual-id visualid	visual id of visual to use"));
	puts(_(" --static		do not update or save configurations"));
#ifndef HAVE_INOTIFY
	puts(_(" --no-polling		do not periodically check for configuration updates"));
#endif
	puts(_(" --version		print version and exit"));
	puts(_(" --help			show this message"));
}

void check_defaults()
{
	char *path;

	path = wdefaultspathfordomain("WindowMaker");

	if (access(path, R_OK) != 0) {
		wwarning(_("could not find user GNUstep directory (%s)."), path);

		if (system("wmaker.inst --batch") != 0) {
			wwarning(_("There was an error while creating GNUstep directory, please "
				   "make sure you have installed Window Maker correctly and run wmaker.inst"));
		} else {
			wwarning(_("%s directory created with default configuration."), path);
		}
	}

	wfree(path);
}

#ifdef HAVE_INOTIFY
/*
 * Add watch here, used to notify if configuration
 * files have changed, using linux kernel inotify mechanism
 */

static void inotifyWatchConfig()
{
	char *watchPath = NULL;
	inotifyFD = inotify_init();	/* Initialise an inotify instance */
	if (inotifyFD < 0) {
		wwarning(_("could not initialise an inotify instance."
			   " Changes to the defaults database will require"
			   " a restart to take effect. Check your kernel!"));
	} else {
		watchPath = wstrconcat(wusergnusteppath(), "/Defaults");
		/* Add the watch; really we are only looking for modify events
		 * but we might want more in the future so check all events for now.
		 * The individual events are checked for in event.c.
		 */
		inotifyWD = inotify_add_watch(inotifyFD, watchPath, IN_ALL_EVENTS);
		if (inotifyWD < 0) {
			wwarning(_("could not add an inotify watch on path\n."
				   "%s\n"
				   "Changes to the defaults database will require"
				   " a restart to take effect."), watchPath);
			close(inotifyFD);
		}
	}
	wfree(watchPath);
}
#endif /* HAVE_INOTIFY */

static void execInitScript()
{
	char *file, *paths;

	paths = wstrconcat(wusergnusteppath(), "/Library/WindowMaker");
	paths = wstrappend(paths, ":" DEF_CONFIG_PATHS);

	file = wfindfile(paths, DEF_INIT_SCRIPT);
	wfree(paths);

	if (file) {
		if (system(file) != 0) {
			wsyserror(_("%s:could not execute initialization script"), file);
		}
		wfree(file);
	}
}

void ExecExitScript()
{
	char *file, *paths;

	paths = wstrconcat(wusergnusteppath(), "/Library/WindowMaker");
	paths = wstrappend(paths, ":" DEF_CONFIG_PATHS);

	file = wfindfile(paths, DEF_EXIT_SCRIPT);
	wfree(paths);

	if (file) {
		if (system(file) != 0) {
			wsyserror(_("%s:could not execute exit script"), file);
		}
		wfree(file);
	}
}

int main(int argc, char **argv)
{
	int i_am_the_monitor, i, len;
	char *str, *alt;

	/* setup common stuff for the monitor and wmaker itself */
	WMInitializeApplication("WindowMaker", &argc, argv);

	memset(&wPreferences, 0, sizeof(WPreferences));

	wPreferences.fallbackWMs = WMCreateArray(8);
	alt = getenv("WINDOWMAKER_ALT_WM");
	if (alt != NULL)
		WMAddToArray(wPreferences.fallbackWMs, wstrdup(alt));

	WMAddToArray(wPreferences.fallbackWMs, wstrdup("blackbox"));
	WMAddToArray(wPreferences.fallbackWMs, wstrdup("metacity"));
	WMAddToArray(wPreferences.fallbackWMs, wstrdup("fvwm"));
	WMAddToArray(wPreferences.fallbackWMs, wstrdup("twm"));
	WMAddToArray(wPreferences.fallbackWMs, NULL);
	WMAddToArray(wPreferences.fallbackWMs, wstrdup("rxvt"));
	WMAddToArray(wPreferences.fallbackWMs, wstrdup("xterm"));

	i_am_the_monitor = 1;

	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--for-real", strlen("--for-real")) == 0) {
			i_am_the_monitor = 0;
			break;
		} else if (strcmp(argv[i], "-display") == 0 || strcmp(argv[i], "--display") == 0) {
			i++;
			if (i >= argc) {
				wwarning(_("too few arguments for %s"), argv[i - 1]);
				exit(0);
			}
			DisplayName = argv[i];
		}
	}

	DisplayName = XDisplayName(DisplayName);
	len = strlen(DisplayName) + 64;
	str = wmalloc(len);
	snprintf(str, len, "DISPLAY=%s", DisplayName);
	putenv(str);

	if (i_am_the_monitor)
		return MonitorLoop(argc, argv);
	else
		return real_main(argc, argv);
}

static int real_main(int argc, char **argv)
{
	int i, restart = 0;
	char *pos;
	int d, s;
	int flag;

	setlocale(LC_ALL, "");
	wsetabort(wAbort);

	/* for telling WPrefs what's the name of the wmaker binary being ran */
	setenv("WMAKER_BIN_NAME", argv[0], 1);

	flag = 0;
	ArgCount = argc;
	Arguments = wmalloc(sizeof(char *) * (ArgCount + 1));
	for (i = 0; i < argc; i++) {
		Arguments[i] = argv[i];
	}
	/* add the extra option to signal that we're just restarting wmaker */
	Arguments[argc - 1] = "--for-real=";
	Arguments[argc] = NULL;

	ProgName = strrchr(argv[0], '/');
	if (!ProgName)
		ProgName = argv[0];
	else
		ProgName++;

	restart = 0;

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
#ifdef USECPP
			if (strcmp(argv[i], "-nocpp") == 0 || strcmp(argv[i], "--no-cpp") == 0) {
				wPreferences.flags.nocpp = 1;
			} else
#endif
			if (strcmp(argv[i], "--for-real") == 0) {
				wPreferences.flags.restarting = 0;
			} else if (strcmp(argv[i], "--for-real=") == 0) {
				wPreferences.flags.restarting = 1;
			} else if (strcmp(argv[i], "--for-real-") == 0) {
				wPreferences.flags.restarting = 2;
			} else if (strcmp(argv[i], "-no-autolaunch") == 0
				   || strcmp(argv[i], "--no-autolaunch") == 0) {
				wPreferences.flags.noautolaunch = 1;
			} else if (strcmp(argv[i], "-dont-restore") == 0 || strcmp(argv[i], "--dont-restore") == 0) {
				wPreferences.flags.norestore = 1;
			} else if (strcmp(argv[i], "-nodock") == 0 || strcmp(argv[i], "--no-dock") == 0) {
				wPreferences.flags.nodock = 1;
			} else if (strcmp(argv[i], "-noclip") == 0 || strcmp(argv[i], "--no-clip") == 0) {
				wPreferences.flags.noclip = 1;
			} else if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "--version") == 0) {
				printf("Window Maker %s\n", VERSION);
				exit(0);
			} else if (strcmp(argv[i], "--global_defaults_path") == 0) {
			  printf("%s/%s\n", SYSCONFDIR, GLOBAL_DEFAULTS_SUBDIR);
				exit(0);
			} else if (strcmp(argv[i], "-locale") == 0 || strcmp(argv[i], "--locale") == 0) {
				i++;
				if (i >= argc) {
					wwarning(_("too few arguments for %s"), argv[i - 1]);
					exit(0);
				}
				Locale = argv[i];
			} else if (strcmp(argv[i], "-display") == 0 || strcmp(argv[i], "--display") == 0) {
				i++;
				if (i >= argc) {
					wwarning(_("too few arguments for %s"), argv[i - 1]);
					exit(0);
				}
				DisplayName = argv[i];
			} else if (strcmp(argv[i], "-visualid") == 0 || strcmp(argv[i], "--visual-id") == 0) {
				i++;
				if (i >= argc) {
					wwarning(_("too few arguments for %s"), argv[i - 1]);
					exit(0);
				}
				if (initWVisualID(argv[i]) != 0) {
					wwarning(_("bad value for visualid: \"%s\""), argv[i]);
					exit(0);
				}
			} else if (strcmp(argv[i], "-static") == 0 || strcmp(argv[i], "--static") == 0
#ifndef HAVE_INOTIFY
				    || strcmp(argv[i], "--no-polling") == 0
#endif
				    ) {
				wPreferences.flags.noupdates = 1;
			} else if (strcmp(argv[i], "--help") == 0) {
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

	if (Locale) {
		setenv("LANG", Locale, 1);
	} else {
		Locale = getenv("LC_ALL");
		if (!Locale) {
			Locale = getenv("LANG");
		}
	}

	setlocale(LC_ALL, "");

	if (!Locale || strcmp(Locale, "C") == 0 || strcmp(Locale, "POSIX") == 0)
		Locale = NULL;
#ifdef I18N
	if (getenv("NLSPATH")) {
		bindtextdomain("WindowMaker", getenv("NLSPATH"));
#if defined(MENU_TEXTDOMAIN)
		bindtextdomain(MENU_TEXTDOMAIN, getenv("NLSPATH"));
#endif
	} else {
		bindtextdomain("WindowMaker", LOCALEDIR);
#if defined(MENU_TEXTDOMAIN)
		bindtextdomain(MENU_TEXTDOMAIN, LOCALEDIR);
#endif
	}
	bind_textdomain_codeset("WindowMaker", "UTF-8");
#if defined(MENU_TEXTDOMAIN)
	bind_textdomain_codeset(MENU_TEXTDOMAIN, "UTF-8");
#endif
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


	if (getWVisualID(0) < 0) {
		/*
		 *   If unspecified, use default visual instead of waiting
		 * for wrlib/context.c:bestContext() that may end up choosing
		 * the "fake" 24 bits added by the Composite extension.
		 *   This is required to avoid all sort of corruptions when
		 * composite is enabled, and at a depth other than 24.
		 */
		setWVisualID(0, (int)DefaultVisual(dpy, DefaultScreen(dpy))->visualid);
        }

	/* check if the user specified a complete display name (with screen).
	 * If so, only manage the specified screen */
	if (DisplayName)
		pos = strchr(DisplayName, ':');
	else
		pos = NULL;

	if (pos && sscanf(pos, ":%i.%i", &d, &s) == 2)
		multiHead = False;

	DisplayName = XDisplayName(DisplayName);
	setenv("DISPLAY", DisplayName, 1);

	wXModifierInitialize();
	StartUp(!multiHead);

	if (wScreenCount == 1)
		multiHead = False;

	execInitScript();
#ifdef HAVE_INOTIFY
	inotifyWatchConfig();
#endif
	EventLoop();
	return -1;
}
