



#include "WINGsP.h"
#include "WUtil.h"

#include <strings.h>


typedef struct W_FontPanel {
    WMWindow *win;

    WMFrame *upperF;
    WMLabel *sampleL;
    
    WMSplitView *split;

    WMFrame *lowerF;
    WMLabel *famL;
    WMList *famLs;
    WMLabel *typL;
    WMList *typLs;
    WMLabel *sizL;
    WMTextField *sizT;
    WMList *sizLs;
    
    WMFrame *xlfdF;
    WMTextField *xlfdT;

    WMFrame *encF;
    WMPopUpButton *encP;

    WMButton *revertB;
    WMButton *previewB;
    WMButton *setB;
    
    proplist_t fdb;
} FontPanel;


#define MIN_UPPER_HEIGHT	20
#define MIN_LOWER_HEIGHT	140


#define MAX_FONTS_TO_RETRIEVE	2000


#define MIN_WIDTH	250
#define MIN_HEIGHT 	MIN_UPPER_HEIGHT+MIN_LOWER_HEIGHT+70

/*
static char *scalableFontSizes[] = {
    "8", 
	"10",
	"11",
	"12",
	"14",
	"16",
	"18",
	"20",
	"24",
	"36",
	"48",
	"64"
};
*/	
	



static void arrangeLowerFrame(FontPanel *panel);

static void familyClick(WMWidget *, void *);
static void typefaceClick(WMWidget *, void *);
static void sizeClick(WMWidget *, void *);


static void listFamilies(WMScreen *scr, WMFontPanel *panel);

static void
splitViewConstrainCallback(WMSplitView *sPtr, int divIndex, int *min, int *max)
{    
    *min = MIN_UPPER_HEIGHT;
    *max = WMWidgetHeight(sPtr)-WMGetSplitViewDividerThickness(sPtr)-MIN_LOWER_HEIGHT;
}


static void
notificationObserver(void *self, WMNotification *notif)
{
    WMFontPanel *panel = (WMFontPanel*)self;
    void *object = WMGetNotificationObject(notif);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) {
	if (object == WMWidgetView(panel->win)) {
	    int h = WMWidgetHeight(panel->win);
	    int w = WMWidgetWidth(panel->win);
	    
	    WMResizeWidget(panel->split, w, h-40);

	    WMMoveWidget(panel->setB, w-80, h-35);
	    WMMoveWidget(panel->previewB, w-160, h-35);
	    WMMoveWidget(panel->revertB, w-240, h-35);
	} else if (object == WMWidgetView(panel->upperF)) {
	    WMResizeWidget(panel->sampleL, WMWidgetWidth(panel->upperF)-20,
			   WMWidgetHeight(panel->upperF)-10);
	} else if (object == WMWidgetView(panel->lowerF)) {
	    
	    if (WMWidgetHeight(panel->lowerF) < MIN_LOWER_HEIGHT) {
		int h;
		
		h =  WMWidgetHeight(panel->split)-MIN_LOWER_HEIGHT
		    - WMGetSplitViewDividerThickness(panel->split);

		WMResizeWidget(panel->upperF, WMWidgetWidth(panel->upperF), h);

		WMMoveWidget(panel->lowerF, 0, WMWidgetHeight(panel->upperF)
			     + WMGetSplitViewDividerThickness(panel->split));
		WMResizeWidget(panel->lowerF, WMWidgetWidth(panel->lowerF),
			       MIN_LOWER_HEIGHT);
	    } else {
		arrangeLowerFrame(panel);
	    }
	}
    }
}


/*
static void
familyClick(WMWidget *w, void *data)
{
    WMList *lPtr = (WMList*)w;
    FontPanel *panel = (FontPanel*)data;

    
    
}
*/

WMFontPanel*
WMGetFontPanel(WMScreen *scr)
{
    FontPanel *panel;
    WMColor *dark, *white;
    WMFont *font;
    int divThickness;

    if (scr->sharedFontPanel)
	return scr->sharedFontPanel;


    panel = wmalloc(sizeof(FontPanel));
    memset(panel, 0, sizeof(FontPanel));
    
    panel->win = WMCreateWindow(scr, "fontPanel");
    WMResizeWidget(panel->win, 320, 370);
    WMSetWindowMinSize(panel->win, MIN_WIDTH, MIN_HEIGHT);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->win), True);

    panel->split = WMCreateSplitView(panel->win);
    WMResizeWidget(panel->split, 320, 330);
    WMSetSplitViewConstrainProc(panel->split, splitViewConstrainCallback);

    divThickness = WMGetSplitViewDividerThickness(panel->split);

    panel->upperF = WMCreateFrame(panel->win);
    WMSetFrameRelief(panel->upperF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->upperF), True);
    panel->lowerF = WMCreateFrame(panel->win);
    WMSetFrameRelief(panel->lowerF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->lowerF), True);

    WMAddSplitViewSubview(panel->split, W_VIEW(panel->upperF));
    WMAddSplitViewSubview(panel->split, W_VIEW(panel->lowerF));

    WMResizeWidget(panel->upperF, 320, 60);
    WMResizeWidget(panel->lowerF, 320, 330-60-divThickness);
    WMMoveWidget(panel->lowerF, 0, 60+divThickness);

    white = WMWhiteColor(scr);
    dark = WMDarkGrayColor(scr);

    panel->sampleL = WMCreateLabel(panel->upperF);
    WMResizeWidget(panel->sampleL, 300, 50);
    WMMoveWidget(panel->sampleL, 10, 10);
    WMSetWidgetBackgroundColor(panel->sampleL, white);
    WMSetLabelRelief(panel->sampleL, WRSunken);
    WMSetLabelText(panel->sampleL, "It is not yet completed!!!");

    font = WMBoldSystemFontOfSize(scr, 12);

    panel->famL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->famL, dark);
    WMSetLabelText(panel->famL, "Family");
    WMSetLabelFont(panel->famL, font);
    WMSetLabelTextColor(panel->famL, white);
    WMSetLabelRelief(panel->famL, WRSunken);
    WMSetLabelTextAlignment(panel->famL, WACenter);
    
    panel->famLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->famLs, familyClick, panel);

    panel->typL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->typL, dark);
    WMSetLabelText(panel->typL, "Typeface");
    WMSetLabelFont(panel->typL, font);
    WMSetLabelTextColor(panel->typL, white);
    WMSetLabelRelief(panel->typL, WRSunken);
    WMSetLabelTextAlignment(panel->typL, WACenter);    

    panel->typLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->typLs, typefaceClick, panel);

    panel->sizL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->sizL, dark);
    WMSetLabelText(panel->sizL, "Size");
    WMSetLabelFont(panel->sizL, font);
    WMSetLabelTextColor(panel->sizL, white);
    WMSetLabelRelief(panel->sizL, WRSunken);
    WMSetLabelTextAlignment(panel->sizL, WACenter);

    panel->sizT = WMCreateTextField(panel->lowerF);

    panel->sizLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->sizLs, sizeClick, panel);

    WMReleaseFont(font);
    WMReleaseColor(white);
    WMReleaseColor(dark);
    
    panel->encF = WMCreateFrame(panel->lowerF);
    WMSetFrameTitle(panel->encF, "Encoding");
    WMResizeWidget(panel->encF, 130, 50);
    
    panel->encP = WMCreatePopUpButton(panel->encF);
    WMMoveWidget(panel->encP, 10, 20);
    WMResizeWidget(panel->encP, 112, 20);
    
    WMMapSubwidgets(panel->encF);
    
    panel->xlfdF = WMCreateFrame(panel->lowerF);
    WMSetFrameTitle(panel->xlfdF, "Xtra Long Font Description");
    
    panel->xlfdT = WMCreateTextField(panel->xlfdF);
    WMMoveWidget(panel->xlfdT, 10, 20);

    WMMapSubwidgets(panel->xlfdF);

    arrangeLowerFrame(panel);
    
    panel->setB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->setB, 70, 24);
    WMMoveWidget(panel->setB, 240, 335);
    WMSetButtonText(panel->setB, "Set");

    panel->previewB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->previewB, 70, 24);
    WMMoveWidget(panel->previewB, 160, 335);
    WMSetButtonText(panel->previewB, "Preview");

    panel->revertB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->revertB, 70, 24);
    WMMoveWidget(panel->revertB, 80, 335);
    WMSetButtonText(panel->revertB, "Revert");

    WMRealizeWidget(panel->win);
    
    WMMapSubwidgets(panel->upperF);
    WMMapSubwidgets(panel->lowerF);
    WMMapSubwidgets(panel->split);
    WMMapSubwidgets(panel->win);
    
    scr->sharedFontPanel = panel;

    
    /* register notification observers */
    WMAddNotificationObserver(notificationObserver, panel, 
			      WMViewSizeDidChangeNotification, 
			      WMWidgetView(panel->win));
    WMAddNotificationObserver(notificationObserver, panel, 
			      WMViewSizeDidChangeNotification, 
			      WMWidgetView(panel->upperF));
    WMAddNotificationObserver(notificationObserver, panel, 
			      WMViewSizeDidChangeNotification, 
			      WMWidgetView(panel->lowerF));


    listFamilies(scr, panel);
 
    
    return panel;
}


void
WMFreeFontPanel(WMFontPanel *panel)
{
    if (panel == WMWidgetScreen(panel->win)->sharedFontPanel) {
	WMWidgetScreen(panel->win)->sharedFontPanel = NULL;
    }
    WMRemoveNotificationObserver(panel);
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    free(panel);
}


void
WMShowFontPanel(WMFontPanel *panel)
{
    WMMapWidget(panel->win);
}


void
WMHideFontPanel(WMFontPanel *panel)
{
    WMUnmapWidget(panel->win);
}


void
WMSetFontPanelFont(WMFontPanel *panel, WMFont *font)
{
}


WMFont*
WMGetFontPanelFont(WMFontPanel *panel)
{
    return NULL;
}


char*
WMGetFontPanelFontName(WMFontPanel *panel)
{
    return NULL;
}




static void
arrangeLowerFrame(FontPanel *panel)
{
    int width = WMWidgetWidth(panel->lowerF) - 55 - 30;
    int height = WMWidgetHeight(panel->lowerF);
    int oheight = 330-60-WMGetSplitViewDividerThickness(panel->split);
    int fw, tw, sw;
    int h;

    height -= 20 + 3 + 10 + 50;
    oheight -= 20 + 3 + 10 + 50;
    
    fw = (125*width) / 235;
    tw = (110*width) / 235;
    sw = 55;
    
    h = (174*height) / oheight;

    WMMoveWidget(panel->famL, 10, 0);
    WMResizeWidget(panel->famL, fw, 20);

    WMMoveWidget(panel->famLs, 10, 23);
    WMResizeWidget(panel->famLs, fw, h);

    WMMoveWidget(panel->typL, 10+fw+3, 0);
    WMResizeWidget(panel->typL, tw, 20);

    WMMoveWidget(panel->typLs, 10+fw+3, 23);
    WMResizeWidget(panel->typLs, tw, h);

    WMMoveWidget(panel->sizL, 10+fw+3+tw+3, 0);
    WMResizeWidget(panel->sizL, sw+4, 20);

    WMMoveWidget(panel->sizT, 10+fw+3+tw+3, 23);
    WMResizeWidget(panel->sizT, sw+4, 20);

    WMMoveWidget(panel->sizLs, 10+fw+3+tw+3, 46);
    WMResizeWidget(panel->sizLs, sw+4, h-22);

    WMMoveWidget(panel->encF, 10, WMWidgetHeight(panel->lowerF)-55);
    
    WMMoveWidget(panel->xlfdF, 145, WMWidgetHeight(panel->lowerF)-55);
    WMResizeWidget(panel->xlfdF, WMWidgetWidth(panel->lowerF)-155, 50);

    WMResizeWidget(panel->xlfdT, WMWidgetWidth(panel->lowerF)-155-20, 20);
}




#define ALL_FONTS_MASK  "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"

#define FOUNDRY	0
#define FAMILY	1
#define WEIGHT	2
#define SLANT	3
#define SETWIDTH 4
#define ADD_STYLE 5
#define PIXEL_SIZE 6
#define POINT_SIZE 7
#define RES_X	8
#define RES_Y	9
#define SPACING	10
#define AV_WIDTH 11
#define REGISTRY 12
#define ENCODING 13

#define NUM_FIELDS 14


#if 1

static Bool
parseFont(char *font, char values[NUM_FIELDS][256])
{
    char *ptr;
    int part;
    char buffer[256], *bptr;

    part = FOUNDRY;
    ptr = font;
    ptr++; /* skip first - */
    bptr = buffer;
    while (*ptr) {
	if (*ptr == '-') {
	    *bptr = 0;
	    strcpy(values[part], buffer);
	    bptr = buffer;
	    part++;
	} else {
	    *bptr++ = *ptr;
	}
	ptr++;
    }
    *bptr = 0;
    strcpy(values[part], buffer);
    
    return True;
}



static int
isXLFD(char *font, int *length_ret)
{
    int c = 0;
    
    *length_ret = 0;
    while (*font) {
	(*length_ret)++;
	if (*font++ == '-')
	    c++;
    }

    return c==NUM_FIELDS;
}




typedef struct {
    char *weight;
    char *slant;

    char *setWidth;
    char *addStyle;
    
    char showSetWidth; /* when duplicated */
    char showAddStyle; /* when duplicated */
    
    WMBag *sizes;
} Typeface;


typedef struct {
    char *name;

    char *foundry;
    char *registry, *encoding;

    char showFoundry; /* when duplicated */
    char showRegistry; /* when duplicated */

    WMBag *typefaces;
} Family;



static void
addTypefaceToFamily(Family *family, char fontFields[NUM_FIELDS][256])
{
    Typeface *face;
    int i;

    if (family->typefaces) {
	for (i = 0; i < WMGetBagItemCount(family->typefaces); i++) {
	    int size;
	    
	    face = WMGetFromBag(family->typefaces, i);
	    
	    if (strcmp(face->weight, fontFields[WEIGHT]) != 0) {
		continue;
	    }
	    if (strcmp(face->slant, fontFields[SLANT]) != 0) {
		continue;
	    }

	    size = atoi(fontFields[POINT_SIZE]);
	    WMPutInBag(face->sizes, (void*)size);
	
	    return;
	}
    } else {
	family->typefaces = WMCreateBag(4);
    }

    face = wmalloc(sizeof(Typeface));
    memset(face, 0, sizeof(Typeface));

    face->weight = wstrdup(fontFields[WEIGHT]);
    face->slant = wstrdup(fontFields[SLANT]);
    face->setWidth = wstrdup(fontFields[SETWIDTH]);
    face->addStyle = wstrdup(fontFields[ADD_STYLE]);

    face->sizes = WMCreateBag(4);
    WMPutInBag(face->sizes, (void*)atoi(fontFields[POINT_SIZE]));
    
    WMPutInBag(family->typefaces, face);
}



/*
 * families (same family name) (Hashtable of family -> bag)
 * 	registries (same family but different registries)
 * 
 */

static void
addFontToFamily(WMHashTable *families, char fontFields[NUM_FIELDS][256])
{
    int i;
    Family *fam;
    WMBag *family;

    family = WMHashGet(families, fontFields[FAMILY]);
    
    if (family) {
	/* look for same encoding/registry and foundry */
	for (i = 0; i < WMGetBagItemCount(family); i++) {
	    int enc, reg, found;
	    
	    fam = WMGetFromBag(family, i);
	    
	    enc = (strcmp(fam->encoding, fontFields[ENCODING]) == 0);
	    reg = (strcmp(fam->registry, fontFields[REGISTRY]) == 0);
	    found = (strcmp(fam->foundry, fontFields[FOUNDRY]) == 0);
	    
	    if (enc && reg && found) {
		addTypefaceToFamily(fam, fontFields);
		return;
	    }
	}
	/* look for same encoding/registry */
	for (i = 0; i < WMGetBagItemCount(family); i++) {
	    int enc, reg;
	    
	    fam = WMGetFromBag(family, i);
	    
	    enc = (strcmp(fam->encoding, fontFields[ENCODING]) == 0);
	    reg = (strcmp(fam->registry, fontFields[REGISTRY]) == 0);

	    if (enc && reg) {
		/* has the same encoding, but the foundry is different */
		fam->showRegistry = 1;

		fam = wmalloc(sizeof(Family));
		memset(fam, 0, sizeof(Family));

		fam->name = wstrdup(fontFields[FAMILY]);
		fam->foundry = wstrdup(fontFields[FOUNDRY]);
		fam->registry = wstrdup(fontFields[REGISTRY]);
		fam->encoding = wstrdup(fontFields[ENCODING]);
		fam->showRegistry = 1;

		addTypefaceToFamily(fam, fontFields);

		WMPutInBag(family, fam);

		return;
	    }
	}
	/* look for same foundry */
	for (i = 0; i < WMGetBagItemCount(family); i++) {
	    int found;
	    
	    fam = WMGetFromBag(family, i);
	    
	    found = (strcmp(fam->foundry, fontFields[FOUNDRY]) == 0);

	    if (found) {
		/* has the same foundry, but encoding is different */
		fam->showFoundry = 1;

		fam = wmalloc(sizeof(Family));
		memset(fam, 0, sizeof(Family));

		fam->name = wstrdup(fontFields[FAMILY]);
		fam->foundry = wstrdup(fontFields[FOUNDRY]);
		fam->registry = wstrdup(fontFields[REGISTRY]);
		fam->encoding = wstrdup(fontFields[ENCODING]);
		fam->showFoundry = 1;

		addTypefaceToFamily(fam, fontFields);

		WMPutInBag(family, fam);

		return;
	    }
	}
	/* foundry and encoding do not match anything known */
	fam = wmalloc(sizeof(Family));
	memset(fam, 0, sizeof(Family));

	fam->name = wstrdup(fontFields[FAMILY]);
	fam->foundry = wstrdup(fontFields[FOUNDRY]);
	fam->registry = wstrdup(fontFields[REGISTRY]);
	fam->encoding = wstrdup(fontFields[ENCODING]);
	fam->showFoundry = 1;
	fam->showRegistry = 1;

	addTypefaceToFamily(fam, fontFields);

	WMPutInBag(family, fam);
	
	return;
    }

    family = WMCreateBag(8);

    fam = wmalloc(sizeof(Family));
    memset(fam, 0, sizeof(Family));

    fam->name = wstrdup(fontFields[FAMILY]);
    fam->foundry = wstrdup(fontFields[FOUNDRY]);
    fam->registry = wstrdup(fontFields[REGISTRY]);
    fam->encoding = wstrdup(fontFields[ENCODING]);
	
    addTypefaceToFamily(fam, fontFields);
    
    WMPutInBag(family, fam);

    WMHashInsert(families, fam->name, family);
}



static void
listFamilies(WMScreen *scr, WMFontPanel *panel)
{
    char **fontList;
    int count;
    int i;
    WMHashTable *families = WMCreateHashTable(WMStringHashCallbacks);
    char fields[NUM_FIELDS][256];
    WMHashEnumerator enumer;
    WMBag *bag;
    
    fontList = XListFonts(scr->display, ALL_FONTS_MASK, MAX_FONTS_TO_RETRIEVE, 
                          &count);
    if (!fontList) {
	WMRunAlertPanel(scr, panel->win, "Error", 
			"Could not retrieve font list", "OK", NULL, NULL);
	return;
    }

    for (i = 0; i < count; i++) {
	int fname_len;

	if (!isXLFD(fontList[i], &fname_len)) {
	    *fontList[i] = '\0';
	    continue;
	}
	if (fname_len > 255) {
	    wwarning("font name %s is longer than 256, which is invalid.",
		     fontList[i]);
	    *fontList[i] = '\0';
	    continue;
	}
	if (!parseFont(fontList[i], fields)) {
	    *fontList[i] = '\0';
	    continue;
	}
	addFontToFamily(families, fields);
    }

    enumer = WMEnumerateHashTable(families);
    
    while ((bag = WMNextHashEnumeratorItem(&enumer))) {
	int i;
	Family *fam;
	char buffer[256];
	WMListItem *item;

	for (i = 0; i < WMGetBagItemCount(bag); i++) {
	    fam = WMGetFromBag(bag, i);
	    
	    strcpy(buffer, fam->name);

	    if (fam->showFoundry) {
		strcat(buffer, " ");
		strcat(buffer, fam->foundry);
		strcat(buffer, " ");
	    }
	    if (fam->showRegistry) {
		strcat(buffer, " (");
		strcat(buffer, fam->registry);
		strcat(buffer, "-");
		strcat(buffer, fam->encoding);
		strcat(buffer, ")");
	    }
	    item = WMAddSortedListItem(panel->famLs, buffer);
	    
	    item->clientData = fam;
	}
	WMFreeBag(bag);
    }
    
    WMFreeHashTable(families);
}



static void 
familyClick(WMWidget *w, void *data)
{
    WMList *lPtr = (WMList*)w;
    WMListItem *item;
    Family *family;
    FontPanel *panel = (FontPanel*)data;
    Typeface *face;
    int i;

    item = WMGetListSelectedItem(lPtr);
    family = (Family*)item->clientData;

    WMClearList(panel->typLs);
    

    for (i = 0; i < WMGetBagItemCount(family->typefaces); i++) {
	char buffer[256];
	
	face = WMGetFromBag(family->typefaces, i);
	
	strcpy(buffer, face->weight);
	strcat(buffer, " ");
	strcat(buffer, face->slant);
	
	WMAddListItem(panel->typLs, buffer);
    }
    
}


static void 
typefaceClick(WMWidget *w, void *data)
{
    WMList *lPtr = (WMList*)w;
    FontPanel *panel = (FontPanel*)data;
}


static void 
sizeClick(WMWidget *w, void *data)
{
    WMList *lPtr = (WMList*)w;
    FontPanel *panel = (FontPanel*)data;
}


#endif
