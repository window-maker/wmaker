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

#define DEFAULT_FONT "sans-serif:pixelsize=12"

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


static int
countChar(char *str, char c)
{
    int count = 0;

    if (!str)
        return 0;

    for (; *str!=0; str++) {
        if (*str == c) {
            count++;
        }
    }

    return count;
}


typedef struct str {
    char *str;
    int len;
} str;

#define XLFD_TOKENS 14


static str*
getXLFDTokens(char *xlfd)
{
    static str tokens[XLFD_TOKENS];
    int i, len, size;
    char *ptr;

    if (!xlfd || *xlfd!='-' || countChar(xlfd, '-')<XLFD_TOKENS)
        return NULL;

    memset(tokens, 0, sizeof(str)*XLFD_TOKENS);

    len  = strlen(xlfd);

    for (ptr=xlfd, i=0; i<XLFD_TOKENS && len>0; i++) {
        /* skip one '-' */
        ptr++;
        len--;
        if (len <= 0)
            break;
        size = strcspn(ptr, "-");
        tokens[i].str = ptr;
        tokens[i].len = size;
        ptr += size;
        len -= size;
    }

    return tokens;
}


static int
strToInt(str *token)
{
    int res=0, pos, c;

    if (token->len==0 || token->str[0]=='*') {
        return -1;
    } else {
        for (res=0, pos=0; pos<token->len; pos++) {
            c = token->str[pos] - '0';
            if (c<0 || c>9)
                break;
            res = res*10 + c;
        }
    }
    return res;
}


static char*
mapSlantToName(str *slant)
{
    if (slant->len==0 || slant->str[0]=='*')
        return "roman";

    switch(slant->str[0]) {
    case 'i':
        return "italic";
    case 'o':
        return "oblique";
    case 'r':
    default:
        return "roman";
    }
}


char*
xlfdToFc(char *xlfd, char *useFamily, Bool keepXLFD)
{
    str *tokens, *family, *weight, *slant;
    char *name, buf[512];
    int size, pixelsize;

    tokens = getXLFDTokens(xlfd);
    if (!tokens)
        return wstrdup(DEFAULT_FONT);

    family = &(tokens[1]);
    weight = &(tokens[2]);
    slant  = &(tokens[3]);

    if (useFamily) {
        name = wstrdup(useFamily);
    } else {
        if (family->len==0 || family->str[0]=='*')
            return wstrdup(DEFAULT_FONT);

        sprintf(buf, "%.*s", family->len, family->str);
        name = wstrdup(buf);
    }

    pixelsize = strToInt(&tokens[6]);
    size = strToInt(&tokens[7]);

    if (size<=0 && pixelsize<=0) {
        name = wstrappend(name, ":pixelsize=12");
    } else if (pixelsize>0) {
        /* if pixelsize is present size will be ignored so we skip it */
        sprintf(buf, ":pixelsize=%d", pixelsize);
        name = wstrappend(name, buf);
    } else {
        sprintf(buf, "-%d", size/10);
        name = wstrappend(name, buf);
    }

    if (weight->len>0 && weight->str[0]!='*') {
        sprintf(buf, ":weight=%.*s", weight->len, weight->str);
        name = wstrappend(name, buf);
    }

    if (slant->len>0 && slant->str[0]!='*') {
        sprintf(buf, ":slant=%s", mapSlantToName(slant));
        name = wstrappend(name, buf);
    }

    if (keepXLFD) {
        name = wstrappend(name, ":xlfd=");
        name = wstrappend(name, xlfd);
    }

    return name;
}


/* return converted font (if conversion is needed) else the original font */
static char*
convertFont(char *font, Bool keepXLFD)
{
    if (font[0]=='-') {
        if (!strchr(font, ',')) {
            return xlfdToFc(font, NULL, keepXLFD);
        } else {
            return xlfdToFc(font, "sans-serif", keepXLFD);
        }
    } else {
        return font;
    }
}


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
    puts("  --sets-too   try to approximate fontsets by using their first complete xlfd.");
    puts("               This only works for singlebyte languages. The default is to");
    puts("               replace the fontset with the default: 'sans-serif:pixelsize=12'");
    puts("               which should display properly for any language.");
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


