
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <WINGs/WUtil.h>

#include "../src/wconfig.h"

#define DEFAULT_FONT "sans serif:pixelsize=12"

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
        size = strcspn(ptr, "-,");
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
mapWeightToName(str *weight)
{
    char *normalNames[] = {"medium", "normal", "regular"};
    static char buf[32];
    int i;

    if (weight->len==0)
        return "";

    for (i=0; i<sizeof(normalNames)/sizeof(char*); i++) {
        if (strlen(normalNames[i])==weight->len &&
            strncmp(normalNames[i], weight->str, weight->len))
        {
            return "";
        }
    }

    snprintf(buf, sizeof(buf), ":%.*s", weight->len, weight->str);

    return buf;
}


static char*
mapSlantToName(str *slant)
{
    if (slant->len==0)
        return "";

    switch(slant->str[0]) {
    case 'i':
        return ":italic";
    case 'o':
        return ":oblique";
    case 'r':
    default:
        return "";
    }
}


char*
xlfdToFc(char *xlfd, char *useFamily, Bool keepXLFD)
{
    str *tokens, *family, *weight, *slant;
    char *name, buf[64], *slt;
    int size, pixelsize;

    tokens = getXLFDTokens(xlfd);
    if (!tokens)
        return wstrdup(DEFAULT_FONT);

    family = &(tokens[1]);
    weight = &(tokens[2]);
    slant  = &(tokens[3]);
    pixelsize = strToInt(&tokens[6]);
    size = strToInt(&tokens[7]);

    if (useFamily) {
        name = wstrdup(useFamily);
    } else {
        if (family->len==0 || family->str[0]=='*')
            return wstrdup(DEFAULT_FONT);

        snprintf(buf, sizeof(buf), "%.*s", family->len, family->str);
        name = wstrdup(buf);
    }

    if (size>0 && pixelsize<=0) {
        snprintf(buf, sizeof(buf), "-%d", size/10);
        name = wstrappend(name, buf);
    }

    name = wstrappend(name, mapWeightToName(weight));
    name = wstrappend(name, mapSlantToName(slant));

    if (size<=0 && pixelsize<=0) {
        name = wstrappend(name, ":pixelsize=12");
    } else if (pixelsize>0) {
        /* if pixelsize is present size will be ignored so we skip it */
        snprintf(buf, sizeof(buf), ":pixelsize=%d", pixelsize);
        name = wstrappend(name, buf);
    }

    if (keepXLFD) {
        name = wstrappend(name, ":xlfd=");
        name = wstrappend(name, xlfd);
    }

    return name;
}


/* return converted font (if conversion is needed) else the original font */
char*
convertFont(char *font, Bool keepXLFD)
{
    if (font[0]=='-') {
        if (MB_CUR_MAX < 2) {
            return xlfdToFc(font, NULL, keepXLFD);
        } else {
            return xlfdToFc(font, "sans serif", keepXLFD);
        }
    } else {
        return font;
    }
}

