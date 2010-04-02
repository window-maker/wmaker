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

static void find_and_write(char *group, char **list);
static void other_window_managers(void);
static void print_help(int print_usage, int exitval);

extern char *__progname;

char *path;

WMPropList *MenuRoot, *MenuGroup, *MenuItem;

int main(int argc, char *argv[])
{
	char *t, *locale;
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
	locale = getenv("LANG");
	setlocale(LC_ALL, "");

#if HAVE_LIBINTL_H && I18N
	if (getenv("NLSPATH"))
		bindtextdomain("wmgenmenu", getenv("NLSPATH"));
	else
		bindtextdomain("wmgenmenu", LOCALEDIR);

	bind_textdomain_codeset("wmgenmenu", "UTF-8");
	textdomain("wmgenmenu");
#endif

	MenuRoot = WMCreatePLArray(WMCreatePLString(_("Window Maker")), NULL);
	MenuGroup = WMCreatePLArray(WMCreatePLString(_("Applications")), NULL);

	/* Submenus in Applications */
	find_and_write(_("Terminals"), terminals);
	find_and_write(_("Internet"), internet);
	find_and_write(_("Email"), email);
	find_and_write(_("Mathematics"), Mathematiks);
	find_and_write(_("File Managers"), file_managers);
	find_and_write(_("Graphics"), Graphics);
	find_and_write(_("Multimedia"), Multimedia);
	find_and_write(_("Editors"), Editors);
	find_and_write(_("Development"), development);
	find_and_write(_("Window Maker"), WindowMaker);
	find_and_write(_("Office"), Office);
	find_and_write(_("Astronomy"), Astronomie);
	find_and_write(_("Sound"), Sound);
	find_and_write(_("Comics"), Comics);
	find_and_write(_("Viewers"), Viewers);
	find_and_write(_("Utilities"), Utilities);
	find_and_write(_("System"), System);
	find_and_write(_("Video"), Video);
	find_and_write(_("Chat and Talk"), Chat);
	find_and_write(_("P2P-Network"), P2P);
	find_and_write(_("Games"), Games);
	find_and_write(_("OpenSUSE"), OpenSUSE);
	find_and_write(_("Mandriva"), Mandriva);

	WMAddToPLArray(MenuRoot, MenuGroup);

	/* `Run' dialog */
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Run...")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString(_("%A(Run, Type command:)")),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuItem);

	/* Appearance-related items */
	MenuGroup = WMCreatePLArray(WMCreatePLString(_("Appearance")), NULL);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Themes")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/Themes WITH setstyle"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Icons")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/IconSets WITH seticons"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Background")),
		WMCreatePLString("OPEN_MENU"),
		WMCreatePLString("-noext $HOME/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Save Theme")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString("getstyle -t $HOME/GNUstep/Library/WindowMaker/Themes/\"%%a(Theme name)\""),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Save Icons")),
		WMCreatePLString("SHEXEC"),
		WMCreatePLString("geticonset $HOME/GNUstep/Library/WindowMaker/IconSets/\"%%a(IconSet name)\""),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);
	WMAddToPLArray(MenuRoot, MenuGroup);

	/* Workspace-related items */
	MenuGroup = WMCreatePLArray(
		WMCreatePLString(_("Workspaces")),
		WMCreatePLString("WORKSPACE_MENU"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuGroup);

	MenuGroup = WMCreatePLArray(WMCreatePLString(_("Workspace")), NULL);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Hide Others")),
		WMCreatePLString("HIDE_OTHERS"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Show All")),
		WMCreatePLString("SHOW_ALL"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Arrange Icons")),
		WMCreatePLString("ARRANGE_ICONS"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Refresh")),
		WMCreatePLString("REFRESH"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Save Session")),
		WMCreatePLString("SAVE_SESSION"),
		NULL
	);
	WMAddToPLArray(MenuGroup, MenuItem);
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Clear Session")),
		WMCreatePLString("CLEAR_SESSION"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuGroup);

	/* Configuration-related items */
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Configure Window Maker")),
		WMCreatePLString("EXEC"),
		WMCreatePLString("WPrefs"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Info Panel")),
		WMCreatePLString("INFO_PANEL"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuItem);

	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Restart")),
		WMCreatePLString("RESTART"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuItem);

	/* Other window managers */
	other_window_managers();

	/* XLock */
	t = wfindfile(path, "xlock");
	if (t) {
		MenuItem = WMCreatePLArray(
			WMCreatePLString(_("Lock Screen")),
			WMCreatePLString("EXEC"),
			WMCreatePLString("xlock -allowroot -usefirst -mode matrix"),
			NULL
		);
		WMAddToPLArray(MenuRoot, MenuItem);
		wfree(t);
	}

	/* Exit */
	MenuItem = WMCreatePLArray(
		WMCreatePLString(_("Exit Window Maker")),
		WMCreatePLString("EXIT"),
		NULL
	);
	WMAddToPLArray(MenuRoot, MenuItem);

	printf("%s", WMGetPropListDescription(MenuRoot, True));

	return 0;
}

static void find_and_write(char *group, char **list)
{
	int i, argc;
	char *t, **argv, buf[PATH_MAX];
	extern char *path;
	WMPropList *SubGroup = NULL, *SubGroupItem = NULL;

	i = 0;
	while (list[i]) {
		/* Before checking if app exists, split its options */
		wtokensplit(list[i], &argv, &argc);
		t = wfindfile(path, argv[0]);
		if (t) {
			/* check whether it is to be executed in a terminal */
			if (strcmp("!", argv[argc - 1]) < 0)
				SubGroupItem = WMCreatePLArray(
					WMCreatePLString(argv[0]),
					WMCreatePLString("EXEC"),
					WMCreatePLString(list[i]),
					NULL
				);
			else {
				char comm[50], *ptr;

				strcpy(comm, list[i]);
				/* ugly hack to delete character ! from list[i] */
				ptr = strchr(comm, '!');
				while (ptr >= comm && (*ptr == '!' || isspace(*ptr)))
					*ptr-- = '\0';
				snprintf(buf, sizeof(buf), "xterm -e %s", comm);
				SubGroupItem = WMCreatePLArray(
					WMCreatePLString(argv[0]),
					WMCreatePLString("EXEC"),
					WMCreatePLString(comm),
					NULL
				);
			}
			if (!SubGroup)
				SubGroup = WMCreatePLArray(WMCreatePLString(group), NULL);
			WMAddToPLArray(SubGroup, SubGroupItem);
			wfree(t);
		}
		i++;
	}
	if (SubGroup)
		WMAddToPLArray(MenuGroup, SubGroup);
}

static void other_window_managers(void)
{
	int i;
	char *t, buf[PATH_MAX];
	WMPropList *SubGroup = NULL, *SubGroupItem = NULL;

	i = 0;
	while (other_wm[i]) {
		t = wfindfile(path, other_wm[i]);
		if (t) {
			snprintf(buf, sizeof(buf), _("Start %s"), other_wm[i]);
			SubGroupItem = WMCreatePLArray(
				WMCreatePLString(buf),
				WMCreatePLString("RESTART"),
				WMCreatePLString(other_wm[i]),
				NULL
			);
			if (!SubGroup)
				SubGroup = WMCreatePLArray(WMCreatePLString(_("Other Window Managers")), NULL);
			WMAddToPLArray(SubGroup, SubGroupItem);
			wfree(t);
		}
		i++;
	}
	if (SubGroup)
		WMAddToPLArray(MenuRoot, SubGroup);
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
