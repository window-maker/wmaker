/* Copyright (C) 2010 Carlos R. Mafra */

#ifdef __GLIBC__
#define _GNU_SOURCE		/* getopt_long */
#endif

#include <getopt.h>
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
static inline void workspaces(void);
static inline void lock_screen(void);
static inline void wmaker_config(void);
static inline void run_command(void);
static void other_window_managers(char **other_wm);
static inline void wm_visual(void);
static inline void write_first_line(int count);
static void print_help(int print_usage, int exitval);

extern char *__progname;

char *path;
int first_group = 1;

int main(int argc, char *argv[])
{
	char *locale;
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

	printf("\(\"Window Maker\",\n");

	printf("  \(\"");
	printf(_("Applications"));
	printf("\",\n");

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


	/* This must be after the last entry */
	printf("\n     )\n");
	printf("  ),\n");

	run_command();
	wm_visual();
	workspaces();
	wmaker_config();

	printf("(\"");
	printf(_("Info Panel"));
	printf("\", INFO_PANEL),\n");

	printf("(\"");
	printf(_("Restart"));
	printf("\", RESTART),\n");

	other_window_managers(other_wm);
	lock_screen();

	printf("(\"");
	printf(_("Exit..."));
	printf("\", EXIT)\n");

	/* Final closing parenthesis */
	printf(")\n");

	exit(EXIT_SUCCESS);
}

static void find_and_write(char *group, char **list)
{
	int i, argc, found;
	char *location, **argv;
	extern char *path;
	static char buf[2048];	/* any single group must fit in this; just increase if not enough */
	static char buf2[512];	/* single items' holding cell */

	i = 0;
	found = 0;
	memset(buf, 0, sizeof(buf));
	memset(buf2, 0, sizeof(buf2));
	while (list[i]) {
		/* Before checking if app exists, split its options */
		wtokensplit(list[i], &argv, &argc);
		location = wfindfile(path, argv[0]);
		if (location) {
			found = 1;
			/* check whether it is to be executed in a terminal */
			if (strcmp("!", argv[argc - 1]) < 0)
				snprintf(buf2, sizeof(buf2), ",\n       \(\"%s\", EXEC, \"%s\")", argv[0], list[i]);
			else {
				char comm[50], *ptr;

				strcpy(comm, list[i]);
				/* ugly hack to delete character ! from list[i] */
				ptr = strchr(comm, '!');
				while (ptr >= comm && (*ptr == '!' || isspace(*ptr)))
					*ptr-- = '\0';
				snprintf(buf2, sizeof(buf2), ",\n       \(\"%s\", EXEC, \"xterm -e %s\")", argv[0], comm);
			}
			strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
		}
	i++;
	}
	if (found) {
		/* This "first" printf is different from the others! */
		if (!first_group) {
			printf("\n     ),\n");
		} else {
			first_group = 0;
		}

		printf("     \(\"%s\"", group);
		printf("%s", buf);
	}
}

static inline void wmaker_config(void)
{
	char *location = wfindfile(path, "WPrefs");

	if (location) {
		printf("(\"");
		printf(_("Config wmaker..."));
		printf("\", EXEC, \"WPrefs\"),\n");
	}
}

static inline void workspaces(void)
{
	printf("(\"");
	printf(_("Workspaces"));
	printf("\", WORKSPACE_MENU),\n");

	printf("  (\"");
	printf(_("Workspace"));
	printf("\",\n");

	printf("     (\"");
	printf(_("Hide others"));
	printf("\", HIDE_OTHERS),\n");

	printf("     (\"");
	printf(_("Show all"));
	printf("\", SHOW_ALL),\n");

	printf("     (\"");
	printf(_("Arrange Icons"));
	printf("\", ARRANGE_ICONS),\n");

	printf("     (\"");
	printf(_("Refresh"));
	printf("\", REFRESH),\n");

	printf("     (\"");
	printf(_("Save Session"));
	printf("\", SAVE_SESSION),\n");

	printf("     (\"");
	printf(_("Clear Session"));
	printf("\", CLEAR_SESSION)\n");

	printf("  ),\n");
}

static inline void lock_screen(void)
{
	char *location;

	location = wfindfile(path, "xlock");
	if (location) {
		printf("(\"");
		printf(_("Lock Screen"));
		printf("\", EXEC, \"xlock -allowroot -usefirst -mode matrix\"),\n");
	}
}

static void other_window_managers(char **other_wm)
{
	int count = 0;
	char *location;
	int i;

	printf("(\"");
	printf(_("Other Window Managers"));
	printf("\",\n");

	for (i = 0; i <= MAX_WMS; i++) {
		if (other_wm[i]) {
			location = wfindfile(path, other_wm[i]);
			if (location) {
				write_first_line(count);
				printf(_("Start %s"), other_wm[i]);
				printf("\", RESTART, \"%s\")", other_wm[i]);
				count++;
			}
		}
	}
	printf("\n),\n");
}

static void wm_visual(void)
{
	/* TODO: add more pre-defined dirs to Themes etc */
	printf("(\"");
	printf(_("Workspace Appearance"));
	printf("\",\n");

	printf("(\"");
	printf(_("Themes"));
	printf("\", OPEN_MENU, \"-noext $HOME/GNUstep/Library/WindowMaker/Themes WITH setstyle\"),\n");

	printf("(\"");
	printf(_("Styles"));
	printf("\", OPEN_MENU, \"-noext $HOME/GNUstep/Library/WindowMaker/Styles WITH setstyle\"),\n");

	printf("(\"");
	printf(_("Icons"));
	printf("\", OPEN_MENU, \"-noext $HOME/GNUstep/Library/WindowMaker/IconSets WITH seticons\"),\n");

	printf("(\"");
	printf(_("Background"));
	printf("\", OPEN_MENU, \"-noext $HOME/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t\"),\n");

	printf("(\"");
	printf(_("Save Theme"));
	printf("\", SHEXEC, \"getstyle -t $HOME/GNUstep/Library/WindowMaker/Themes/\\\"%%a(Theme name)\\\"\"),\n");

	printf("(\"");
	printf(_("Save Icons"));
	printf("\", SHEXEC, \"geticonset $HOME/GNUstep/Library/WindowMaker/IconSets/\\\"%%a(IconSet name)\\\"\")\n");
	printf("),\n");

}

static inline void run_command(void)
{
	printf("\(\"");
	printf(_("Run..."));
	printf("\", SHEXEC, \"%%A(");
	printf(_("Run, Type command:"));
	printf(")\"),\n");
}

static inline void write_first_line(int count)
{
	/* All lines inside a menu must end with a comma, except for
	 * the last one. To cope with this let's check if there is a
	 * previous entry before writing the "start of the line", and
	 * write the trailing comma if there is.
	 */
	if (count == 0)
		printf(" (\"");
	else {
		printf(",\n (\"");
	}
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
