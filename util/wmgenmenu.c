/* Copyright (C) 2010 Carlos R. Mafra */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "../src/wconfig.h"

#include <WINGs/WUtil.h>

#define MAX_NR_APPS 50  /* Maximum number of entries in each apps list */
#define MAX_WMS 10      /* Maximum number of other window managers to check */
#include "wmgenmenu.h"

static void find_and_write(char **list);
static inline void workspaces(void);
static inline void lock_screen(void);
static inline void wmaker_config(void);
static inline void run_command(void);
static void other_window_managers(char **other_wm);
static inline void wm_visual(void);
static inline void write_first_line(int count);

char *path;

int main(int argc, char *argv[])
{
	char *locale;
	extern char *path;

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

	printf("     \(\"");
	printf(_("Applications"));
	printf("\",\n");

	/* This "first" printf is different from the others! */
	printf("     \(\"");
	printf(_("Terminals"));
	printf("\"");
	find_and_write(terminals);

	printf("\n     ),\n     \(\"");
	printf(_("Internet"));
	printf("\"");
	find_and_write(internet);

	printf("\n     ),\n     \(\"");
	printf(_("Email"));
	printf("\"");
	find_and_write(email);

	printf("\n     ),\n     \(\"");
	printf(_("Mathematics"));
	printf("\"");
	find_and_write(Mathematiks);

	printf("\n     ),\n     \(\"");
	printf(_("File Managers"));
	printf("\"");
	find_and_write(file_managers);

	printf("\n     ),\n     \(\"");
	printf(_("Graphics"));
	printf("\"");
	find_and_write(Graphics);

	printf("\n     ),\n     \(\"");
	printf(_("Multimedia"));
	printf("\"");
	find_and_write(Multimedia);

	printf("\n     ),\n     \(\"");
	printf(_("Editors"));
	printf("\"");
	find_and_write(Editors);

	printf("\n     ),\n     \(\"");
	printf(_("Development"));
	printf("\"");
	find_and_write(development);

	printf("\n     ),\n     \(\"");
	printf(_("Window Maker"));
	printf("\"");
	find_and_write(WindowMaker);

	printf("\n     ),\n     \(\"");
	printf(_("Office"));
	printf("\"");
	find_and_write(Office);

	printf("\n     ),\n     \(\"");
	printf(_("Astronomy"));
	printf("\"");
	find_and_write(Astronomie);

	printf("\n     ),\n     \(\"");
	printf(_("Sound"));
	printf("\"");
	find_and_write(Sound);

	printf("\n     ),\n     \(\"");
	printf(_("Comics"));
	printf("\"");
	find_and_write(Comics);

	printf("\n     ),\n     \(\"");
	printf(_("Viewers"));
	printf("\"");
	find_and_write(Viewers);

	printf("\n     ),\n     \(\"");
	printf(_("Utilities"));
	printf("\"");
	find_and_write(Utilities);

	printf("\n     ),\n     \(\"");
	printf(_("System"));
	printf("\"");
	find_and_write(System);

	printf("\n     ),\n     \(\"");
	printf(_("Video"));
	printf("\"");
	find_and_write(Video);

	printf("\n     ),\n     \(\"");
	printf(_("Chat and Talk"));
	printf("\"");
	find_and_write(Chat);

	printf("\n     ),\n     \(\"");
	printf(_("P2P-Network"));
	printf("\"");
	find_and_write(P2P);

	printf("\n     ),\n     \(\"");
	printf(_("Games"));
	printf("\"");
	find_and_write(Games);

	printf("\n     ),\n     \(\"");
	printf(_("OpenSUSE"));
	printf("\"");
	find_and_write(OpenSUSE);

	printf("\n     ),\n     \(\"");
	printf(_("Mandriva"));
	printf("\"");
	find_and_write(Mandriva);

	/* This must be after the last entry */
	printf("\n     )\n");
	printf("     ),\n");

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

static void find_and_write(char **list)
{
	int i, argc;
	char *location, **argv;
	extern char *path;

	for (i = 0; i <= MAX_NR_APPS; i++) {
		if (list[i]) {
			/* Before checking if app exists, split its options */
			wtokensplit(list[i], &argv, &argc);
			location = wfindfile(path, argv[0]);
			if (location) {
				/* check whether it is to be executed in a terminal */
				if (strcmp("!", argv[argc - 1]) < 0)
					printf(",\n       \(\"%s\", EXEC, \"%s\")", argv[0], list[i]);
				else {
					char comm[50], *ptr[1];

					strcpy(comm, list[i]);
					/* ugly hack to delete character ! from list[i] */
					ptr[0] = strchr(comm,'!');
					*ptr[0] = ' ';
					printf(",\n       \(\"%s\", EXEC, \"xterm -e %s\")", argv[0], comm);
				}
			}
		}
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
	/*
	 * %A below requires Voinov's "Add dialog history" (which
	 * is included in wmaker-crm), otherwise it should be %a
	 */
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
