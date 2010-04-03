/* Copyright (C) 2010 Carlos R. Mafra */

#ifdef __GLIBC__
#define _GNU_SOURCE		/* getopt_long */
#endif

#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WINGs/WUtil.h>

#include "../src/wconfig.h"

#define MAX_NR_APPS 50  /* Maximum number of entries in each apps list */
#define MAX_WMS 10      /* Maximum number of other window managers to check */

#include "wmgenmenu.h"

static void find_and_write(char *group, char **list, int this_is_terminals);
static void other_window_managers(void);
static void print_help(int print_usage, int exitval);

extern char *__progname;

char *path, *terminal = NULL;

WMPropList *RMenu, *L1Menu, *L2Menu, *L3Menu;

int main(int argc, char *argv[])
{
	char *t;
	int ch;

	struct option longopts[] = {
		{ "version",		no_argument,	NULL,	'v' },
		{ "help",		no_argument,	NULL,	'h' },
		{ NULL,			0,		NULL,	0 }
	};

	while ((ch = getopt_long(argc, argv, "hv", longopts, NULL)) != -1)
		switch (ch) {
		case 'v':
			printf("%s (Window Maker %s)\n", __progname, VERSION);
			return 0;
			/* NOTREACHED */
		case 'h':
			print_help(1, 0);
			/* NOTREACHED */
		default:
			print_help(0, 1);
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (argc != 0)
		print_help(0, 1);

	path = getenv("PATH");
	setlocale(LC_ALL, "");

#if HAVE_LIBINTL_H && I18N
	if (getenv("NLSPATH"))
		bindtextdomain("wmgenmenu", getenv("NLSPATH"));
	else
		bindtextdomain("wmgenmenu", LOCALEDIR);

	bind_textdomain_codeset("wmgenmenu", "UTF-8");
	textdomain("wmgenmenu");
#endif

	/*
	 * The menu generated is a four-level hierarchy, of which the
	 * top level (RMenu) is only used to hold the others (a single
	 * PLString, which will be the title of the root menu)
	 *
	 * RMenu		Window Maker
	 *   L1Menu		  Applications
	 *     L2Menu		    Terminals
	 *       L3Menu		      XTerm
	 *       L3Menu		      RXVT
	 *     L2Menu		    Internet
	 *       L3Menu		      Firefox
	 *     L2Menu		    E-mail
	 *   L1Menu		  Appearance
	 *     L2Menu		    Themes
	 *   L1Menu		  Configure Window Maker
	 *
	 */

	RMenu = WMCreatePLArray(WMCreatePLString(_("Window Maker")), NULL);

	L1Menu = WMCreatePLArray(WMCreatePLString(_("Applications")), NULL);

	/* Submenus in Applications */
	find_and_write(_("Terminals"), terminals, 1);	/* always keep terminals the top item */
	find_and_write(_("Internet"), internet, 0);
	find_and_write(_("Email"), email, 0);
	find_and_write(_("Mathematics"), Mathematiks, 0);
	find_and_write(_("File Managers"), file_managers, 0);
	find_and_write(_("Graphics"), Graphics, 0);
	find_and_write(_("Multimedia"), Multimedia, 0);
	find_and_write(_("Editors"), Editors, 0);
	find_and_write(_("Development"), development, 0);
	find_and_write(_("Window Maker"), WindowMaker, 0);
	find_and_write(_("Office"), Office, 0);
	find_and_write(_("Astronomy"), Astronomie, 0);
	find_and_write(_("Sound"), Sound, 0);
	find_and_write(_("Comics"), Comics, 0);
	find_and_write(_("Viewers"), Viewers, 0);
	find_and_write(_("Utilities"), Utilities, 0);
	find_and_write(_("System"), System, 0);
	find_and_write(_("Video"), Video, 0);
	find_and_write(_("Chat and Talk"), Chat, 0);
	find_and_write(_("P2P-Network"), P2P, 0);
	find_and_write(_("Games"), Games, 0);
	find_and_write(_("OpenSUSE"), OpenSUSE, 0);
	find_and_write(_("Mandriva"), Mandriva, 0);

	WMAddToPLArray(RMenu, L1Menu);

	/* `Run' dialog */
	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Run...")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString(_("%A(Run, Type command:)")),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	/* Appearance-related items */
	L1Menu = WMCreatePLArray(WMCreatePLString(_("Appearance")), NULL);
	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Themes")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/Themes WITH setstyle"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Icons")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/IconSets WITH seticons"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Background")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Save Theme")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString("getstyle -t $HOME/GNUstep/Library/WindowMaker/Themes/"
			"\"%a(Theme name, Name to save theme as)\""),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Save Icons")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString("geticonset $HOME/GNUstep/Library/WindowMaker/IconSets/"
			"\"%a(IconSet name,Name to save icon set as)\""),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);
	WMAddToPLArray(RMenu, L1Menu);

	/* Workspace-related items */
	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Workspaces")),
		WMCreatePLString("WORKSPACE_MENU"),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	L1Menu = WMCreatePLArray(WMCreatePLString(_("Workspace")), NULL);
	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Hide Others")),
		WMCreatePLString("HIDE_OTHERS"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Show All")),
		WMCreatePLString("SHOW_ALL"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Arrange Icons")),
		WMCreatePLString("ARRANGE_ICONS"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Refresh")),
		WMCreatePLString("REFRESH"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Save Session")),
		WMCreatePLString("SAVE_SESSION"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);

	L2Menu = WMCreatePLArray(
		WMCreatePLString(_("Clear Session")),
		WMCreatePLString("CLEAR_SESSION"),
		NULL
	);
	WMAddToPLArray(L1Menu, L2Menu);
	WMAddToPLArray(RMenu, L1Menu);

	/* Configuration-related items */
	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Configure Window Maker")),
		WMCreatePLString("EXEC"),
		WMCreatePLString("WPrefs"),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Info Panel")),
		WMCreatePLString("INFO_PANEL"),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Restart")),
		WMCreatePLString("RESTART"),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	/* Other window managers */
	other_window_managers();

	/* XLock */
	t = wfindfile(path, "xlock");
	if (t) {
		L1Menu = WMCreatePLArray(
			WMCreatePLString(_("Lock Screen")),
			WMCreatePLString("EXEC"),
			WMCreatePLString("xlock -allowroot -usefirst -mode matrix"),
			NULL
		);
		WMAddToPLArray(RMenu, L1Menu);
		wfree(t);
	}

	/* Exit */
	L1Menu = WMCreatePLArray(
		WMCreatePLString(_("Exit Window Maker")),
		WMCreatePLString("EXIT"),
		NULL
	);
	WMAddToPLArray(RMenu, L1Menu);

	printf("%s", WMGetPropListDescription(RMenu, True));
	puts("");

	return 0;
}

/*
 * Creates an L2Menu made of L3Menu items
 * Attaches to L1Menu
 * - make sure previous menus of these levels are
 *   attached to their parent before calling
 */
static void find_and_write(char *group, char **list, int this_is_terminals)
{
	int i, argc;
	char *t, **argv, buf[PATH_MAX];

	/* or else pre-existing menus of these levels
	 * will badly disturb empty group detection */
	L2Menu = NULL;
	L3Menu = NULL;

	i = 0;
	while (list[i]) {
		/* Before checking if app exists, split its options */
		wtokensplit(list[i], &argv, &argc);
		t = wfindfile(path, argv[0]);
		if (t) {
			/* find a terminal to be used for cmnds that need a terminal */
			if (this_is_terminals && !terminal)
				terminal = wstrdup(list[i]);
			if (*(argv[argc-1]) != '!') {
				L3Menu = WMCreatePLArray(
					WMCreatePLString(argv[0]),
					WMCreatePLString("EXEC"),
					WMCreatePLString(list[i]),
					NULL
				);
			} else {
				char comm[PATH_MAX], *ptr;

				strcpy(comm, list[i]);
				/* delete character " !" from the command */
				ptr = strchr(comm, '!');
				while (ptr >= comm && (*ptr == '!' || isspace(*ptr)))
					*ptr-- = '\0';
				snprintf(buf, sizeof(buf), "%s -e %s", terminal ? terminal : "xterm" , comm);
				L3Menu = WMCreatePLArray(
					WMCreatePLString(argv[0]),
					WMCreatePLString("EXEC"),
					WMCreatePLString(buf),
					NULL
				);
			}
			if (!L2Menu)
				L2Menu = WMCreatePLArray(
					WMCreatePLString(group),
					NULL
				);
			WMAddToPLArray(L2Menu, L3Menu);
			wfree(t);
		}
		i++;
	}
	if (L2Menu)
		WMAddToPLArray(L1Menu, L2Menu);
}

/*
 * Creates an L1Menu made of L2Menu items
 * - make sure previous menus of these levels are
 *   attached to their parent before calling
 * Attaches to RMenu
 */
static void other_window_managers(void)
{
	int i;
	char *t, buf[PATH_MAX];

	/* or else pre-existing menus of these levels
	 * will badly disturb empty group detection */
	L1Menu = NULL;
	L2Menu = NULL;

	i = 0;
	while (other_wm[i]) {
		t = wfindfile(path, other_wm[i]);
		if (t) {
			snprintf(buf, sizeof(buf), _("Start %s"), other_wm[i]);
			L2Menu = WMCreatePLArray(
				WMCreatePLString(buf),
				WMCreatePLString("RESTART"),
				WMCreatePLString(other_wm[i]),
				NULL
			);
			if (!L1Menu)
				L1Menu = WMCreatePLArray(
					WMCreatePLString(_("Other Window Managers")),
					NULL
				);
			WMAddToPLArray(L1Menu, L2Menu);
			wfree(t);
		}
		i++;
	}
	if (L1Menu)
		WMAddToPLArray(RMenu, L1Menu);
}

void print_help(int print_usage, int exitval)
{
	printf("Usage: %s [-h] [-v]\n", __progname);
	if (print_usage) {
		puts("Writes a menu structure usable as ~/GNUstep/Defaults/WMRootMenu to stdout");
		puts("");
		puts("  -h, --help           display this help and exit");
		puts("  -v, --version        output version information and exit");
	}
	exit(exitval);
}
