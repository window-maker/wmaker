/* WINGsP.h should NEVER be included from user applications */
#include <WINGsP.h>
#include <WINGs.h>
#include <X11/Xlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <ctype.h>

void
wAbort()
{
    exit(0);
}


void
Close(WMWidget *self, void *client)
{
    exit(0);
}



/* =============== a rudimentary HTML parser ======================= */

/* due to the WMSetTextParser stuff, it should not
 be too hard to add parsers. like dis :-]  */


/*
 * A hack to speed up caseless_equal.  Thanks to Quincey Koziol for
 * developing it for the "chimera" folks so I could use it  7 years later ;-)
 * Constraint: nothing but '\0' may map to 0
 */
unsigned char map_table[256] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92,
    93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
    164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
    178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
    206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
    234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
    248, 249, 250, 251, 252, 253, 254, 255};

#define TOLOWER(x)  (map_table[(int)x])
static int
mystrcasecmp(const unsigned char *s1, const unsigned char *s2)
{
    if (!*s1 || !*s2) return 0;
    while (*s2 != '\0') {
        if (TOLOWER (*s1) != TOLOWER (*s2)) /* true if *s1 == 0 ! */
            return 0;
        s1++;
        s2++;
    }
    return (*s1=='\0' ||!isalnum(*s1))?1:0;
}


typedef struct _currentFormat {
    void *para;
    WMBag *fonts;
    WMBag *colors;
    WMColor *ccolor;
    WMFont *cfont;
    WMParserActions actions;
    //WMBag *aligns; // for tables...
    /* the following are "nested"
     i.e.:  <b><b><i></b><b></i>
     1  2  1  1   2   0   get it? */
    short i;
    short b;
    short u;
    short fmargin;
    short bmargin;
    WMAlignment align:2;
    short type:1;
    short ul:3; /* how "nested"... up to 8 levels deep */
    short comment:1; /* ignore text till --> */
    short RESERVED:10;
} CFMT;
CFMT cfmt;



#if 0
getArg(char *t, short type, void *arg)
{
    short d=0;
    while(*(++t) && !d) {
        if(type==0) {
            if(*t>='0' && *t<='9') {
                sscanf(t, "%d", arg);
                while(*t&& (*t<'0' || *t>'9'))
                    t++;
                d=1;
            }
        }
    }
}
#endif

void
parseToken(WMText *tPtr, char *token, short tk)
{
    short mode=0; /* 0 starts, 1 closes */
    WMScreen	*scr = (W_VIEW(tPtr))->screen;

    while(*token && isspace(*(token))) token++;
    if(*token == '/') {
        token++;
        mode = 1;
        while(isspace(*(token))) token++;
    }

    if(strlen(token)==1) {
        /* nice and fast for small tokens... no need for too much brain
         power here */
        switch(TOLOWER(*token)) {
        case 'i':
            if(!mode) {
                cfmt.cfont = WMGetFontItalic(scr, cfmt.cfont);
                WMPutInBag(cfmt.fonts, (void *)cfmt.cfont);
            } else { /*dun wanna remove the baseFont eh? */
                int count = WMGetBagItemCount(cfmt.fonts);
                if(count>1)
                    WMDeleteFromBag(cfmt.fonts, count-1);
                cfmt.cfont = (WMFont *)WMGetFromBag(cfmt.fonts,
                                                    WMGetBagItemCount(cfmt.fonts)-1);
            } break;
        case 'b':
            if(!mode) {
                cfmt.cfont = WMGetFontBold(scr, cfmt.cfont);
                WMPutInBag(cfmt.fonts, (void *)cfmt.cfont);
            } else { /*dun wanna remove the baseFont eh? */
                int count = WMGetBagItemCount(cfmt.fonts);
                if(count>1)
                    WMDeleteFromBag(cfmt.fonts, count-1);
                cfmt.cfont = (WMFont *)WMGetFromBag(cfmt.fonts,
                                                    WMGetBagItemCount(cfmt.fonts)-1);
            } break;
        case 'p':
            cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                        WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
            (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
            cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                        WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
            (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
            break;
        case 'u': cfmt.u = !mode; break;
        }
    } else { /* the <HTML> tag is, as far as I'm concerned, useless */
        if(mystrcasecmp(token, "br")) {
            cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                        WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
            (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
        }
        else if(mystrcasecmp(token, "ul")) {
            if(mode) {
                if(cfmt.ul>1) cfmt.ul--;
            } else cfmt.ul++;
            if(cfmt.ul) {
                cfmt.bmargin = cfmt.ul*30;
                cfmt.fmargin = cfmt.bmargin-10;
            } else cfmt.fmargin = cfmt.bmargin = 0;
        } else if(mystrcasecmp(token, "li")) {
            cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                        WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
            (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
        } else if(mystrcasecmp(token, "align"))
            ;//printf("align");
        else if(mystrcasecmp(token, "img"))  {
            if(!mode) {
                char *mark=NULL;
                WMPixmap *pixmap;
                void *chunk;
                token+=3;
                while(isspace(*(token))) token++;
                do {
                    switch(TOLOWER(*token)) {
                    case 's':
                        if(TOLOWER(*(1+token)) == 'r' && TOLOWER(*(2+token)) == 'c') {
                            mark = strchr(token, '=');
                            if(mark) {
                                char img[256], *iptr;
                                token = mark+1;
                                if(!token) return;
                                sscanf(token, "%s", img);
                                iptr = img;
                                if(*img == '\"') { img[strlen(img)-1] = 0; iptr++;}
                                pixmap = WMCreatePixmapFromFile(scr, iptr);
                                chunk = (cfmt.actions.createPChunk)(pixmap, 0, False);
                                (cfmt.actions.insertChunk) (tPtr, chunk, False);
                                printf("[%s]\n", iptr);
                            } } break; } } while(*(token++));
            }
        } else if(mystrcasecmp(token, "font")) {
#if 0
            if(mode) {
                cfmt.cfont = (WMFont *)WMGetFromBag(cfmt.fonts,
                                                    WMGetBagItemCount(cfmt.fonts)-1);
            } else
                (WMColor *)WMGetFromBag(cfmt.colors,
                                        WMGetBagItemCount(cfmt.colors)-1),
#endif
        }
        else if(mystrcasecmp(token, "center")) {
            printf("center\n");
            if(mode) cfmt.align = WALeft;
            else cfmt.align = WACenter;
            cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                        WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
            (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
        }
    }




    //printf("parse token  (%s)[%s]\n", mode?"close":"open", token);
#if 0
    i=0;
    //while(*token && !isspace(*(token))) token++;
    //printf("A:%d a:%d z%d Z%d\n", '1', 'a', 'Z', 'z');
    do {
        if(!mm) {
            if(c>=65 &&  c<=122) { major[i++] = c;
            } else if(c==' ' || c=='='){ major[i] = 0; i=0; mm=1;
                printf("\nmajor: [%s]", major);}
        } else {
            if(c!=' ') {
                minor[i++] = c;
            } else { minor[i] = 0; i=0; printf("  minor: [%s]  ", minor);}
        }
    }while((c = *(++token)));
#endif


    //printf("parse token  (%s)[%s]\n", mode?"close":"open", token);
}

void
HTMLParser(WMWidget *w, void *clientData, short type)
{
    static short init=1;  /* have we been here at least once before? */
    char *stream = (char *) clientData;
    WMText *tPtr = (WMText *)w;
    WMScreen	*scr;
    void *chunk;
    char c;
#define MAX_TOKEN_SIZE 255
    char token[MAX_TOKEN_SIZE+1];
#define MAX_TEXT_SIZE 1023
    char text[MAX_TEXT_SIZE+1];
    short mode=0;
    short tk=0, txt=0;
    short wasspace=0;

    if(!tPtr || !stream)
        return;

    scr = (W_VIEW(tPtr))->screen;
    cfmt.type = type;
    if(init) {
        cfmt.actions = WMGetTextParserActions(tPtr);
        cfmt.fonts = WMCreateBag(4); /* there sould always be at least 1 font... */
        cfmt.cfont = WMSystemFontOfSize(scr, 12);
        WMPutInBag(cfmt.fonts, (void *)cfmt.cfont);
        cfmt.colors = WMCreateBag(4);
        cfmt.ccolor = WMBlackColor(scr);
        WMPutInBag(cfmt.colors, (void *)cfmt.ccolor);
        cfmt.i = cfmt.b = cfmt.u = cfmt.ul = 0;
        cfmt.align = WALeft;
        cfmt.fmargin = cfmt.bmargin = 0;
        cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                    WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
        (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
        init = 0;
    }

#if 1
    if(strlen(stream) == 1 && stream[0] == '\n') {
        /* sometimes if the text entered is a single char AND is a newline,
         the user prolly typed it */
        cfmt.para = (cfmt.actions.createParagraph) (cfmt.fmargin, cfmt.bmargin,
                                                    WMWidgetWidth(tPtr)-30, NULL, 0, cfmt.align);
        (cfmt.actions.insertParagraph) (tPtr, cfmt.para, cfmt.type);
        return;
    }
#endif


    /*
     */

    while( (c=*(stream++))) {
        //printf("%c", c);
        if(c == '\n' || c =='\t')
            //c = ' '; //continue;
            continue;
        if(c == ' ') {
            if(wasspace)
                continue;
            wasspace = 1;
        }else wasspace = 0;

        if(c == '<'  && !mode) {
            mode=1;
            if(txt>0) {
                text[txt] = 0;
                chunk = (cfmt.actions.createTChunk)(text, txt, cfmt.cfont,
                                                    cfmt.ccolor, 0, (cfmt.u?1:0));
                (cfmt.actions.insertChunk) (tPtr, chunk, cfmt.type);
                //printf("%s\n", text);
            }
            txt = 0;
        } else if(c == '>' && mode) {
            token[tk] = 0;
            if(tk>0) parseToken(tPtr, token, tk);
            mode=0;
            tk=0;
        } else {
            if(mode) {
                if(tk < MAX_TOKEN_SIZE) token[tk++] = c;
            } else if(txt < MAX_TEXT_SIZE) text[txt++] = c;
        }
    }

    if(tk>0) { token[tk] = 0; parseToken(tPtr, token, tk);}
    if(txt>0) {
        text[txt] = 0;
        //printf("%s\n", text);
        chunk = (cfmt.actions.createTChunk)(text, txt,
                                            (WMFont *)WMGetFromBag(cfmt.fonts,
                                                                   WMGetBagItemCount(cfmt.fonts)-1),
                                            (WMColor *)WMGetFromBag(cfmt.colors,
                                                                    WMGetBagItemCount(cfmt.colors)-1),
                                            0, (cfmt.u?1:0));
        (cfmt.actions.insertChunk) (tPtr, chunk, cfmt.type);
    }

}


/* ================= the driver ================== */

WMWidget *win;
WMText *text;

void
NotificationObserver(void *self, WMNotification *notif)
{
    void *object = WMGetNotificationObject(notif);
    unsigned short w = WMWidgetWidth(win);
    unsigned short h = WMWidgetHeight(win);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) {
        if (object == WMWidgetView(win)) {
            WMResizeWidget(text, w-20, h-20);
        }
    }

    {static int i=0;
        switch(i++) {
        case 0: WMSetTextHasHorizontalScroller(text, False); break;
        case 1: WMSetTextHasVerticalScroller(text, False); break;
        case 2: WMSetTextHasVerticalScroller(text, True); break;
        case 3: WMSetTextHasHorizontalScroller(text, True); break;
        default: i=0;
        }
    }
}

int
main(int argc, char **argv)
{
    Display		*dpy;
    WMScreen	*scr;


    WMInitializeApplication("WMText", &argc, argv);
    dpy = XOpenDisplay(NULL);
    if(!dpy) exit(1);

    scr = WMCreateSimpleApplicationScreen(dpy);

    win = WMCreateWindow(scr, "WMText Test");
    WMRealizeWidget(win);
    WMResizeWidget(win, 500, 300);
    //WMResizeWidget(win, 900, 600);
    WMSetWindowTitle(win,"WMText Test");
    WMSetWindowCloseAction(win, Close, NULL);

    text = WMCreateText(win);
    WMRealizeWidget(text);
    WMResizeWidget(text, 480, 280);
    WMMoveWidget(text, 10, 10);
    WMSetTextUseFixedPitchFont(text, False);
    WMSetTextMonoFont(text, False);
    WMSetTextParser(text, HTMLParser);
    WMSetTextEditable(text, True);
    WMFreezeText(text);
    if(1){
        FILE *f = fopen("./wm.html", "r");
        char data[1024];
        if(f) {
            while(fgets(data, 1022,f))
                WMAppendTextStream(text, data);
            fclose(f);
        } else {
            WMAppendTextStream(text, "<i>can't</i> open the <u>wm.html</u> file, but here's a <B>text <I>stream <img src=foo.bar></I><BR>that</B> <U>needs</U> parsing");
        }
    }


    //WMPrependTextStream(text, "this is prepended\n");
    //WMAppendTextStream(text, "blueplanet.rtf");


    WMThawText(text);
    WMRefreshText(text, 0, 0);

    /*
     WMAddNotificationObserver(NotificationObserver, win,
     WMViewSizeDidChangeNotification, WMWidgetView(win));
     */
    WMSetViewNotifySizeChanges(WMWidgetView(win), True);
    WMMapSubwidgets(win);
    WMMapWidget(win);

    WMScreenMainLoop(scr);
}

