
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <WINGs/WUtil.h>

#include "../src/wconfig.h"

#define DEFAULT_FONT "sans:pixelsize=12"

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
char*
convertFont(char *font, Bool keepXLFD)
{
    if (font[0]=='-') {
        char *res;
        char *tmp= wstrdup(font);
        
        if (MB_CUR_MAX < 2) {
            char *ptr = strchr(tmp, ',');
            if (ptr) *ptr= 0;
            res = xlfdToFc(tmp, NULL, keepXLFD);
        } else {
            res = xlfdToFc(tmp, "sans", keepXLFD);
        }
        wfree(tmp);
        
        return res;
    } else {
        return font;
    }
}

