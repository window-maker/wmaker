/*
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

#include <X11/Xlocale.h>

#include <sys/wait.h>
#include <unistd.h>

extern void Initialize(WMScreen *scr);

#define MAX_DEATHS	64

struct {
    pid_t pid;
    void *data;
    void (*handler)(void*);
} DeadHandlers[MAX_DEATHS];


static pid_t DeadChildren[MAX_DEATHS];
static int DeadChildrenCount = 0;

void
wAbort(Bool foo)
{
    exit(1);
}


static BOOL
stringCompareHook(proplist_t pl1, proplist_t pl2)
{
    char *str1, *str2;

    str1 = PLGetString(pl1);
    str2 = PLGetString(pl2);

    if (strcasecmp(str1, str2)==0)
      return YES;
    else
      return NO;
}


static void
print_help(char *progname)
{
    printf(_("usage: %s [options]\n"), progname);
    puts(_("options:"));
    puts(_(" -display <display>	display to be used"));
    puts(_(" --version		print version number and exit"));
    puts(_(" --help		print this message and exit"));
}


#if 0
static RETSIGTYPE
handleDeadChild(int sig)
{
    pid_t pid;
    int status;
    
    pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
	DeadChildren[DeadChildrenCount++] = pid;
    }
}
#endif

void
AddDeadChildHandler(pid_t pid, void (*handler)(void*), void *data)
{
    int i;

    for (i = 0; i < MAX_DEATHS; i++) {
	if (DeadHandlers[i].pid == 0) {
	    DeadHandlers[i].pid = pid;
	    DeadHandlers[i].handler = handler;
	    DeadHandlers[i].data = data;
	    break;
	}
    }
    assert(i!=MAX_DEATHS);
}


int 
main(int argc, char **argv)
{
    Display *dpy;
    WMScreen *scr;
    char *locale;
    int i;
    char *display_name="";

    wsetabort(wAbort);
    
    memset(DeadHandlers, 0, sizeof(DeadHandlers));
    
    WMInitializeApplication("WPrefs", &argc, argv);
    
    if (argc>1) {
	for (i=1; i<argc; i++) {
	    if (strcmp(argv[i], "-version")==0
		|| strcmp(argv[i], "--version")==0) {
		printf("WPrefs (Window Maker) %s\n", WVERSION);
		exit(0);
	    } else if (strcmp(argv[i], "-display")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		display_name = argv[i];
	    } else {
		print_help(argv[0]);
		exit(0);
	    }
	}
    }

    locale = getenv("LANG");
    setlocale(LC_ALL, "");

#ifdef I18N
    if (getenv("NLSPATH"))
	bindtextdomain("WPrefs", getenv("NLSPATH"));
    else
	bindtextdomain("WPrefs", LOCALEDIR);
    textdomain("WPrefs");

    if (!XSupportsLocale()) {
	wwarning(_("X server does not support locale"));
    }
    if (XSetLocaleModifiers("") == NULL) {
 	wwarning(_("cannot set locale modifiers"));
    }
#endif

    dpy = XOpenDisplay(display_name);
    if (!dpy) {
	wfatal(_("could not open display %s"), XDisplayName(display_name));
	exit(0);
    }
#if 1
    XSynchronize(dpy, 1);
#endif
    scr = WMCreateScreen(dpy, DefaultScreen(dpy));
    if (!scr) {
	wfatal(_("could not initialize application"));
	exit(0);
    }

    PLSetStringCmpHook(stringCompareHook);

    Initialize(scr);

    while (1) {
	XEvent event;

	WMNextEvent(dpy, &event);

	while (DeadChildrenCount-- > 0) {
	    int i;
	    
	    for (i=0; i<MAX_DEATHS; i++) {
		if (DeadChildren[i] == DeadHandlers[i].pid) {
		    (*DeadHandlers[i].handler)(DeadHandlers[i].data);
		    DeadHandlers[i].pid = 0;
		}
	    }
	}

	WMHandleEvent(&event);
    }
}
