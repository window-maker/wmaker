/* convertfonts.c - converts fonts in a style file to fontconfig format
 *
 *  WindowMaker window manager
 *
 *  Copyright (c) 2004 Dan Pascu
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


#define PROG_VERSION "convertfonts (Window Maker) 1.0"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <WINGs/WUtil.h>

#include "../src/wconfig.h"


char *FontOptions[] = {
    "IconTitleFont",
    "ClipTitleFont",
    "DisplayFont",
    "LargeDisplayFont",
    "MenuTextFont",
    "MenuTitleFont",
    "WindowTitleFont",
    "SystemFont",
    "BoldSystemFont",
    NULL
};


char *ProgName;

extern char* convertFont(char *font, Bool keepXLFD);


void
print_help()
{
    printf("\nUsage: %s <style_file>\n\n", ProgName);
    puts("Converts fonts in a style file into fontconfig format");
    puts("");
    puts("  --help       display this help and exit");
    puts("  --version    output version information and exit");
    puts("  --keep-xlfd  preserve the original xlfd by appending a ':xlfd=<xlfd>' hint");
    puts("               to the font name. This property is not used by the fontconfig");
    puts("               matching engine to find the font, but it is useful as a hint");
    puts("               about what the original font was to allow hand tuning the");
    puts("               result or restoring the xlfd. The default is to not add it");
    puts("               as it results in long, unreadable and confusing names.");
    puts("");
}

// replace --sets-too with something better
int
main(int argc, char **argv)
{
    WMPropList *style, *key, *val;
    char *file = NULL, *oldfont, *newfont;
    struct stat statbuf;
    Bool keepXLFD = False;
    int i;

    ProgName = argv[0];

    if (argc<2) {
        print_help();
        exit(0);
    }

    for (i=1; i < argc; i++) {
        if (strcmp("--version", argv[i])==0) {
            puts(PROG_VERSION);
            exit(0);
        } else if (strcmp("--help", argv[i])==0) {
            print_help();
            exit(0);
        } else if (strcmp("--keep-xlfd", argv[i])==0) {
            keepXLFD = True;;
        } else if (argv[i][0]=='-') {
            printf("%s: invalid argument '%s'\n", ProgName, argv[i]);
            printf("Try '%s --help' for more information\n", ProgName);
            exit(1);
        } else {
            file = argv[i];
        }
    }

    WMPLSetCaseSensitive(False);

    if (stat(file, &statbuf) < 0) {
        perror(file);
        exit(1);
    }

    style = WMReadPropListFromFile(file);
    if (!style) {
        perror(file);
        printf("%s: could not load style file.\n", ProgName);
        exit(1);
    }

    if (!WMIsPLDictionary(style)) {
        printf("%s: '%s' is not a well formatted style file\n", ProgName, file);
        exit(1);
    }

    for (i=0; FontOptions[i]!=NULL; i++) {
        key = WMCreatePLString(FontOptions[i]);
        val = WMGetFromPLDictionary(style, key);
        if (val) {
            oldfont = WMGetFromPLString(val);
            newfont = convertFont(oldfont, keepXLFD);
            if (oldfont != newfont) {
                val = WMCreatePLString(newfont);
                WMPutInPLDictionary(style, key, val);
                WMReleasePropList(val);
                wfree(newfont);
            }
        }
        WMReleasePropList(key);
    }

    WMWritePropListToFile(style, file, True);

    exit(0);
}


