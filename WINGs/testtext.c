#include "WINGs.h"
#include <stdio.h>


WMScreen	*scr;

void
wAbort()
{
	exit(0);
}


void Close(WMWidget *self, void *client)
{
	exit(0);
}

extern WMFont * WMGetFontPlain(WMScreen *scrPtr, WMFont *font);

extern WMFont * WMGetFontBold(WMScreen *scrPtr, WMFont *font);

extern WMFont * WMGetFontItalic(WMScreen *scrPtr, WMFont *font);

extern WMFont * WMGetFontOfSize(WMScreen *scrPtr, WMFont *font, int size);




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
	WMBag *fonts;
	WMBag *colors;
	WMColor *ccolor;
	WMFont *cfont;
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
	
void parseToken(WMText *tPtr, char *token, short tk)
{
	short mode=0; /* 0 starts, 1 closes */
	int first = True;
	void *tb= NULL;
	int prepend = WMGetTextInsertType(tPtr);

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
				first = True;
				tb = WMCreateTextBlockWithText(NULL, cfmt.cfont, 	
					cfmt.ccolor, first, 0);
				WMSetTextBlockProperties(tb, first, False, (cfmt.u?1:0), 0, 0);
				WMAppendTextBlock(tPtr, tb);
				break;
			case 'u': cfmt.u = !mode; break;
		}
	} else { /* the <HTML> tag is, as far as I'm concerned, useless */
		if(mystrcasecmp(token, "br")) {
				first = True;
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
				first = True;
//change margins... create a new margin....
			//(cfmt.fmargin, cfmt.bmargin, 
		} else if(mystrcasecmp(token, "align"))
			;//printf("align");
		else if(mystrcasecmp(token, "img"))  {
			if(!mode) {
				char *mark=NULL;
				WMPixmap *pixmap; 
				WMLabel *l;
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
				if(pixmap) {
					l = WMCreateLabel(tPtr);
					WMResizeWidget(l, 48, 48);
					WMSetLabelRelief(l, WRFlat);
					WMSetLabelImagePosition(l, WIPImageOnly);
					WMSetLabelImage(l, pixmap); 
					tb = WMCreateTextBlockWithObject(l, 
						iptr, cfmt.ccolor, first, 0);
					WMSetTextBlockProperties(tb, first, 
						False, (cfmt.u?1:0), 0, 0);
					WMAppendTextBlock(tPtr, tb);
				}
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
				first = True;
//change margins...
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
	
void HTMLParser(WMWidget *w, void *clientData)
{
	static short init=1;  /* have we been here at least once before? */
    char *stream = (char *) clientData;
	WMText *tPtr = (WMText *)w;
	void *tb = NULL;
	char c;
	#define MAX_TOKEN_SIZE 255
	char token[MAX_TOKEN_SIZE+1];
	#define MAX_TEXT_SIZE 1023
	char text[MAX_TEXT_SIZE+1];
	short mode=0;
	short tk=0, textlen=0;
	short wasspace=0;
	int first = False;

    if(!tPtr || !stream)
        return;

	cfmt.type = WMGetTextInsertType(tPtr);
	if(1||init) {
		cfmt.fonts = WMCreateBag(4); /* there sould always be at least 1 font... */
		cfmt.cfont = WMGetTextDefaultFont(tPtr);
		WMPutInBag(cfmt.fonts, (void *)cfmt.cfont);
		cfmt.colors = WMCreateBag(4);
		cfmt.ccolor = WMBlackColor(scr);
		WMPutInBag(cfmt.colors, (void *)cfmt.ccolor);
		cfmt.i = cfmt.b = cfmt.u = cfmt.ul = 0;
		cfmt.align = WALeft;
		cfmt.fmargin = cfmt.bmargin = 0;
		init = 0;
	}

#if 0
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
			if(textlen>0) { 
				text[textlen] = 0;
				tb = WMCreateTextBlockWithText(text, cfmt.cfont,
					 cfmt.ccolor, first, textlen);
				WMSetTextBlockProperties(tb, first, False, (cfmt.u?1:0), 0, 0);
				WMAppendTextBlock(tPtr, tb);
//printf("%s\n", text);
			}
			textlen = 0;
		} else if(c == '>' && mode) {
			token[tk] = 0;
			if(tk>0) parseToken(tPtr, token, tk);
			mode=0; 
			tk=0;
		} else {
			if(mode) {
				if(tk < MAX_TOKEN_SIZE) token[tk++] = c;
			} else if(textlen < MAX_TEXT_SIZE) text[textlen++] = c;
		}
	}

	if(tk>0) { token[tk] = 0; parseToken(tPtr, token, tk);}
	if(textlen>0) {
		text[textlen] = 0;
		//printf("%s\n", text);
		tb = WMCreateTextBlockWithText(text, 
			(WMFont *)WMGetFromBag(cfmt.fonts, 
				WMGetBagItemCount(cfmt.fonts)-1), 
			(WMColor *)WMGetFromBag(cfmt.colors,
				WMGetBagItemCount(cfmt.colors)-1), 
			first, textlen);
		WMSetTextBlockProperties(tb, first, False, (cfmt.u?1:0), 0, 0);
		WMAppendTextBlock(tPtr, tb);
	}
		
}


/* ================= the driver ================== */

static void
buttonPressCB(WMWidget *w, void *client)
{
	WMText *tPtr = (WMText *)client;
	WMAppendTextStream(tPtr, NULL);
	WMAppendTextStream(tPtr, 
		"<p><b>You</b> just <i>had</i> to press that button, didn't you? "
		"Well, this sort of thing is bound to happen when you go about "
		" pressing buttons :-)"
		"<p><p>Cheers, <p><p><u>Nwanua</u>"); 
	WMRefreshText(tPtr, 0, 0);
}

int
main(int argc, char **argv)
{
	Display		*dpy;
	WMWidget		*win;
	WMText 		*text;
	WMRulerMargins margins;
	void *tb = NULL;

	printf("copy and paste this string: \n
		here is some <b> bold <i>italic <u>underlined</u> </i> </b> text :-) \n");

	WMInitializeApplication("WMText", &argc, argv);
	dpy = XOpenDisplay(NULL);
	if(!dpy) { printf("no display? *sniff*  :~( "); exit(1);}

	scr = WMCreateSimpleApplicationScreen(dpy);

	win = WMCreateWindow(scr, "WMText Test");
	WMRealizeWidget(win);
	WMResizeWidget(win, 500, 300);
	//WMResizeWidget(win, 900, 600);
	WMSetWindowTitle(win,"WMText Test");
	WMSetWindowCloseAction(win, Close, NULL);

	text = WMCreateText(win);
	WMRealizeWidget(text);
	//WMSetTextHasHorizontalScroller(text, True);
	WMResizeWidget(text, 480, 280);
	WMMoveWidget(text, 10, 10);
	WMSetTextRelief(text, WRSunken);
	WMSetTextHasVerticalScroller(text, True);
	WMSetTextUseMonoFont(text, !True);
	WMSetTextParser(text, HTMLParser);
#if 0
	WMSetTextUseFixedPitchFont(text, False);
	WMSetTextHasRuler(text, True);
	WMShowTextRuler(text, True);
	WMSetTextHasVerticalScroller(text, True);
	//WMSetTextHasHorizontalScroller(text, True);
	WMSetTextEditable(text, True);
	WMFreezeText(text);
#endif

	//margins = WMGetTextRulerMargins(text);


#if 1


	WMAppendTextStream(text, 
"<p><img src=logo.xpm></i> \
<b><i>WINGs</b></i> is a <i>small</i> widget set with \
a <u>very</u> nice look and feel. Its API is <i>inspired</i> \
and its implementation borrows some ideas from interesting places.\
<p><p>
The library is limited and its design is a little sloppy, \
so it's not intended to build large or complex applications, just 
<i>small</i> and complex ones. <b>:-)</b>\
<p><p> Apart from the usual buttons and labels, it has some \
interesting widgets like this list widget: ");

	{
		WMList *list = WMCreateList(text);
		char t[20];
		int i;
		
		WMResizeWidget(list, 60, 60);
		for (i=0; i<7; i++) {
		    sprintf(t, "Item %i", i);
		    WMAddListItem(list, t);
	    }

		tb = WMCreateTextBlockWithObject(list, "{a list object or somesuch}", 
			WMBlackColor(scr), False, 0);
		WMAppendTextBlock(text, tb);
    
	}


	WMAppendTextStream(text, "  a colour well such as this one: ");
	{
		WMColorWell *well;
		well = WMCreateColorWell(text);
		WMResizeWidget(well, 60, 40);
		WMSetColorWellColor(well, 
			WMCreateRGBColor(scr, 0, 0, 0x8888, True));
		tb = WMCreateTextBlockWithObject(well, "{a button object}", 
			WMBlackColor(scr), False, 0);
		WMAppendTextBlock(text, tb);
	}

	WMAppendTextStream(text, 
		",  as well as a text widget: ");

	{
		WMText *te = WMCreateText(text);
		WMRealizeWidget(te);
		WMResizeWidget(te, 120, 90);
		WMSetTextParser(te, HTMLParser);
		WMSetTextRelief(te, WRFlat);
	//	WMSetTextBackgroundColor(te, WMCreateNamedColor(scr, "Red", False)); 
		WMSetTextDefaultFont(te, WMSystemFontOfSize(scr, 10));
		WMAppendTextStream(te, "into which you can easily embed other "
			"<i><b>WINGs</b></i> objects (such as this one), "
			" without breaking a sweat (type in here)");
		tb = WMCreateTextBlockWithObject(te, "{a text object}", 
			WMBlackColor(scr), False, 0);
		WMAppendTextBlock(text, tb);
		WMSetTextHasVerticalScroller(te, False);
    	WMRefreshText(te, 0, 0);
	}

 {
        WMText *te = WMCreateText(text);
        WMRealizeWidget(te);
        WMResizeWidget(te, 120, 90);
        WMSetTextParser(te, HTMLParser);
        WMSetTextRelief(te, WRFlat);
    //  WMSetTextBackgroundColor(te, WMCreateNamedColor(scr, "Red", False)); 
        WMSetTextDefaultFont(te, WMSystemFontOfSize(scr, 10));
        WMAppendTextStream(te, "I always wanted to be able to have "
			"a <i>multi-column</i> text editor in Unix for my reports, "
			" now all I have to do is wait for <u>someone</u> to code it :-P");
        tb = WMCreateTextBlockWithObject(te, "{a text object}", 
            WMBlackColor(scr), False, 0);
        WMAppendTextBlock(text, tb);
        WMSetTextHasVerticalScroller(te, False);
        WMRefreshText(te, 0, 0);
    }



		
		
	WMAppendTextStream(text, 
" .<p><p> Not bad eh? \
<p><p> Since <i><b>WINGs</b></i> is \
written in C and is sort of \
low-level, it is \
small and faster than say, Motif or even Athena (just you try 
embedding and clicking on buttons");

	{
		WMButton *b = WMCreateCommandButton(text);
		WMSetButtonText(b, "like me");
		WMSetButtonAction(b, buttonPressCB, text);
		WMResizeWidget(b, 60, 15);
		tb = WMCreateTextBlockWithObject(b, "{a button object}", 
			WMBlackColor(scr), False, 0);
		WMAppendTextBlock(text, tb);
	}

	WMAppendTextStream(text, 
"  with those other tool-kits :-). <p><p>Knowing Xlib will help you to \
workaround some of its limitations, although you'll probably be able to \
write something like a trivial tic-tac-toe game ");

	{
		WMTextField *t = WMCreateTextField(text);
		WMInsertTextFieldText(t, "or an interesting text editor", 0);
		WMResizeWidget(t, 160, 20);
		WMSetTextFieldBordered(t, True);
		WMSetTextFieldBeveled(t, True);
		tb = WMCreateTextBlockWithObject(t, "{a textfield object}", 
			WMGrayColor(scr), False, 0);
		WMAppendTextBlock(text, tb);
	}


	WMAppendTextStream(text, " without knowing much Xlib. "
		"<p><p>(BTW, don't  <i>press</i> that button that is <u>screeming</u>"
		" to be pressed!");
#endif



//	WMThawText(text);
    WMRefreshText(text, 0, 0);

/*
	WMAddNotificationObserver(NotificationObserver, win,
        WMViewSizeDidChangeNotification, WMWidgetView(win));
*/
	WMSetViewNotifySizeChanges(WMWidgetView(win), True);
	WMMapSubwidgets(win);
	WMMapWidget(win);

	WMScreenMainLoop(scr);
	return 0;
}

