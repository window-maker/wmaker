



#include "WINGsP.h"
#include "WUtil.h"

#include <ctype.h>
#include <strings.h>


typedef struct W_FontPanel {
    WMWindow *win;

    WMFrame *upperF;
    WMTextField *sampleT;
    
    WMSplitView *split;

    WMFrame *lowerF;
    WMLabel *famL;
    WMList *famLs;
    WMLabel *typL;
    WMList *typLs;
    WMLabel *sizL;
    WMTextField *sizT;
    WMList *sizLs;

    WMButton *revertB;
    WMButton *setB;
    
    proplist_t fdb;
} FontPanel;


#define MIN_UPPER_HEIGHT	20
#define MIN_LOWER_HEIGHT	140


#define MAX_FONTS_TO_RETRIEVE	2000

#define BUTTON_SPACE_HEIGHT 40

#define MIN_WIDTH	250
#define MIN_HEIGHT 	(MIN_UPPER_HEIGHT+MIN_LOWER_HEIGHT+BUTTON_SPACE_HEIGHT)

#define DEF_UPPER_HEIGHT 60
#define DEF_LOWER_HEIGHT 310

#define DEF_WIDTH	320
#define DEF_HEIGHT	(DEF_UPPER_HEIGHT+DEF_LOWER_HEIGHT)




static int scalableFontSizes[] = {
    8, 
	10,
	11,
	12,
	14,
	16,
	18,
	20,
	24,
	36,
	48,
	64
};

	

static void getSelectedFont(FontPanel *panel, char buffer[]);


static void arrangeLowerFrame(FontPanel *panel);

static void familyClick(WMWidget *, void *);
static void typefaceClick(WMWidget *, void *);
static void sizeClick(WMWidget *, void *);


static void listFamilies(WMScreen *scr, WMFontPanel *panel);

static void
splitViewConstrainCallback(WMSplitView *sPtr, int divIndex, int *min, int *max)
{    
    *min = MIN_UPPER_HEIGHT;
    *max = WMWidgetHeight(sPtr)-BUTTON_SPACE_HEIGHT-MIN_LOWER_HEIGHT;
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

	    WMResizeWidget(panel->split, w, h-BUTTON_SPACE_HEIGHT);

	    WMMoveWidget(panel->setB, w-80, h-(BUTTON_SPACE_HEIGHT-5));

	    WMMoveWidget(panel->revertB, w-240, h-(BUTTON_SPACE_HEIGHT-5));

	} else if (object == WMWidgetView(panel->upperF)) {

	    if (WMWidgetHeight(panel->upperF) < MIN_UPPER_HEIGHT) {
		WMResizeWidget(panel->upperF, WMWidgetWidth(panel->upperF),
			       MIN_UPPER_HEIGHT);
	    } else {
		WMResizeWidget(panel->sampleT, WMWidgetWidth(panel->upperF)-20,
			       WMWidgetHeight(panel->upperF)-10);
	    }

	} else if (object == WMWidgetView(panel->lowerF)) {
	    
	    if (WMWidgetHeight(panel->lowerF) < MIN_LOWER_HEIGHT) {
		WMResizeWidget(panel->upperF, WMWidgetWidth(panel->upperF), 
			       MIN_UPPER_HEIGHT);

		WMMoveWidget(panel->lowerF, 0, WMWidgetHeight(panel->upperF)
			     + WMGetSplitViewDividerThickness(panel->split));

		WMResizeWidget(panel->lowerF, WMWidgetWidth(panel->lowerF),
			       WMWidgetWidth(panel->split) - MIN_UPPER_HEIGHT
			       - WMGetSplitViewDividerThickness(panel->split));
	    } else {
		arrangeLowerFrame(panel);
	    }
	}
    }
}


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
/*    WMSetWidgetBackgroundColor(panel->win, WMWhiteColor(scr));*/
    WMResizeWidget(panel->win, DEF_WIDTH, DEF_HEIGHT);
    WMSetWindowMinSize(panel->win, MIN_WIDTH, MIN_HEIGHT);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->win), True);

    panel->split = WMCreateSplitView(panel->win);
    WMResizeWidget(panel->split, DEF_WIDTH, DEF_HEIGHT - BUTTON_SPACE_HEIGHT);
    WMSetSplitViewConstrainProc(panel->split, splitViewConstrainCallback);

    divThickness = WMGetSplitViewDividerThickness(panel->split);

    panel->upperF = WMCreateFrame(panel->win);
    WMSetFrameRelief(panel->upperF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->upperF), True);
    panel->lowerF = WMCreateFrame(panel->win);
/*    WMSetWidgetBackgroundColor(panel->lowerF, WMBlackColor(scr));*/
    WMSetFrameRelief(panel->lowerF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->lowerF), True);

    WMAddSplitViewSubview(panel->split, W_VIEW(panel->upperF));
    WMAddSplitViewSubview(panel->split, W_VIEW(panel->lowerF));

    WMResizeWidget(panel->upperF, DEF_WIDTH, DEF_UPPER_HEIGHT);
    
    WMResizeWidget(panel->lowerF, DEF_WIDTH, DEF_LOWER_HEIGHT);

    WMMoveWidget(panel->lowerF, 0, 60+divThickness);

    white = WMWhiteColor(scr);
    dark = WMDarkGrayColor(scr);

    panel->sampleT = WMCreateTextField(panel->upperF);
    WMResizeWidget(panel->sampleT, DEF_WIDTH - 20, 50);
    WMMoveWidget(panel->sampleT, 10, 10);
    WMSetTextFieldText(panel->sampleT, "Test!!!");

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
/*    WMSetTextFieldAlignment(panel->sizT, WARight);*/

    panel->sizLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->sizLs, sizeClick, panel);

    WMReleaseFont(font);
    WMReleaseColor(white);
    WMReleaseColor(dark);
    
    panel->setB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->setB, 70, 24);
    WMMoveWidget(panel->setB, 240, DEF_HEIGHT - (BUTTON_SPACE_HEIGHT-5));
    WMSetButtonText(panel->setB, "Set");

    panel->revertB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->revertB, 70, 24);
    WMMoveWidget(panel->revertB, 80, DEF_HEIGHT - (BUTTON_SPACE_HEIGHT-5));
    WMSetButtonText(panel->revertB, "Revert");

    WMRealizeWidget(panel->win);

    WMMapSubwidgets(panel->upperF);
    WMMapSubwidgets(panel->lowerF);
    WMMapSubwidgets(panel->split);
    WMMapSubwidgets(panel->win);

    WMUnmapWidget(panel->revertB);

    arrangeLowerFrame(panel);

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


Bool
WMSetFontPanelFontName(WMFontPanel *panel, char *fontName)
{
 
    return True;
}


WMFont*
WMGetFontPanelFont(WMFontPanel *panel)
{
    return WMGetTextFieldFont(panel->sampleT);
}


char*
WMGetFontPanelFontName(WMFontPanel *panel)
{
    char name[256];

    getSelectedFont(panel, name);

    return wstrdup(name);
}



static void
arrangeLowerFrame(FontPanel *panel)
{
    int width = WMWidgetWidth(panel->lowerF) - 55 - 30;
    int height = WMWidgetHeight(panel->split) - WMWidgetHeight(panel->upperF);
    int fw, tw, sw;

#define LABEL_HEIGHT 20

    height -= WMGetSplitViewDividerThickness(panel->split);
    

    height -= LABEL_HEIGHT + 8;

    fw = (125*width) / 235;
    tw = (110*width) / 235;
    sw = 55;

    WMMoveWidget(panel->famL, 10, 0);
    WMResizeWidget(panel->famL, fw, LABEL_HEIGHT);

    WMMoveWidget(panel->famLs, 10, 23);
    WMResizeWidget(panel->famLs, fw, height);

    WMMoveWidget(panel->typL, 10+fw+3, 0);
    WMResizeWidget(panel->typL, tw, LABEL_HEIGHT);

    WMMoveWidget(panel->typLs, 10+fw+3, 23);
    WMResizeWidget(panel->typLs, tw, height);

    WMMoveWidget(panel->sizL, 10+fw+3+tw+3, 0);
    WMResizeWidget(panel->sizL, sw+4, LABEL_HEIGHT);

    WMMoveWidget(panel->sizT, 10+fw+3+tw+3, 23);
    WMResizeWidget(panel->sizT, sw+4, 20);

    WMMoveWidget(panel->sizLs, 10+fw+3+tw+3, 46);
    WMResizeWidget(panel->sizLs, sw+4, height-23);
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




static int
compare_int(const void *a, const void *b)
{
    int i1 = *(int*)a;
    int i2 = *(int*)b;

    if (i1 < i2)
	return -1;
    else if (i1 > i2)
	return 1;
    else
	return 0;
}



static void
addSizeToTypeface(Typeface *face, int size)
{
    if (size == 0) {
	int j;
		
	for (j = 0; j < sizeof(scalableFontSizes)/sizeof(int); j++) {
	    size = scalableFontSizes[j];

	    if (!WMCountInBag(face->sizes, (void*)size)) {
		WMPutInBag(face->sizes, (void*)size);
	    }
	}
	WMSortBag(face->sizes, compare_int);
    } else {
	if (!WMCountInBag(face->sizes, (void*)size)) {
	    WMPutInBag(face->sizes, (void*)size);
	    WMSortBag(face->sizes, compare_int);
	}
    }
}



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

	    size = atoi(fontFields[PIXEL_SIZE]);

	    addSizeToTypeface(face, size);

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
    addSizeToTypeface(face, atoi(fontFields[PIXEL_SIZE]));

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
	/* look for same foundry */
	for (i = 0; i < WMGetBagItemCount(family); i++) {
	    int found;
	    
	    fam = WMGetFromBag(family, i);
	    
	    found = (strcmp(fam->foundry, fontFields[FOUNDRY]) == 0);

	    if (found) {
		/* has the same foundry, but encoding is different */
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
    WMHashTable *families = WMCreateHashTable(WMStringPointerHashCallbacks);
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
getSelectedFont(FontPanel *panel, char buffer[])
{
    WMListItem *item;
    Family *family;
    Typeface *face;
    char *size;

    
    item = WMGetListSelectedItem(panel->famLs);
    if (!item)
	return;
    family = (Family*)item->clientData;

    item = WMGetListSelectedItem(panel->typLs);
    if (!item)
	return;
    face = (Typeface*)item->clientData;

    size = WMGetTextFieldText(panel->sizT);

    sprintf(buffer, "-%s-%s-%s-%s-%s-%s-%s-*-*-*-*-*-%s-%s",
	    family->foundry,
	    family->name,
	    face->weight,
	    face->slant,
	    face->setWidth,
	    face->addStyle,
	    size,
	    family->registry,
	    family->encoding);
}



static void
preview(FontPanel *panel)
{
    char buffer[256];
    WMFont *font;
    
    getSelectedFont(panel, buffer);
    
    font = WMCreateFont(WMWidgetScreen(panel->win), buffer);
    if (font) {
	WMSetTextFieldFont(panel->sampleT, font);
	WMReleaseFont(font);
    }
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
    /* current typeface and size */
    char *oface = NULL;
    char *osize = NULL;
    int facei = -1;
    int sizei = -1;

    /* must try to keep the same typeface and size for the new family */
    item = WMGetListSelectedItem(panel->typLs);
    if (item)
	oface = wstrdup(item->text);

    osize = WMGetTextFieldText(panel->sizT);


    item = WMGetListSelectedItem(lPtr);
    family = (Family*)item->clientData;

    WMClearList(panel->typLs);

    for (i = 0; i < WMGetBagItemCount(family->typefaces); i++) {
	char buffer[256];
	int top;
	WMListItem *fitem;

	face = WMGetFromBag(family->typefaces, i);
	
	if (strcmp(face->weight, "medium") == 0) {
	    buffer[0] = 0;
	} else {
	    if (*face->weight) {
		strcpy(buffer, face->weight);
		buffer[0] = toupper(buffer[0]);
		strcat(buffer, " ");
	    } else {
		buffer[0] = 0;
	    }
	}

	if (strcmp(face->slant, "r") == 0) {
	    strcat(buffer, "Roman");
	    top = 1;
	} else if (strcmp(face->slant, "i") == 0) {
	    strcat(buffer, "Italic");
	} else if (strcmp(face->slant, "o") == 0) {
	    strcat(buffer, "Oblique");
	} else if (strcmp(face->slant, "ri") == 0) {
	    strcat(buffer, "Rev Italic");
	} else if (strcmp(face->slant, "ro") == 0) {
	    strcat(buffer, "Rev Oblique");
	} else {
	    strcat(buffer, face->slant);
	}

	if (buffer[0] == 0) {
	    strcpy(buffer, "Normal");
	}

	if (top)
	    fitem = WMInsertListItem(panel->typLs, 0, buffer);
	else
	    fitem = WMAddListItem(panel->typLs, buffer);
	fitem->clientData = face;
    }

    if (oface) {
	facei = WMFindRowOfListItemWithTitle(panel->typLs, oface);
	free(oface);
    }
    if (facei < 0) {
	facei = 0;
    }
    WMSelectListItem(panel->typLs, facei);
    typefaceClick(panel->typLs, panel);
    
    if (osize) {
	sizei = WMFindRowOfListItemWithTitle(panel->sizLs, osize);
    }
    if (sizei >= 0) {
	WMSelectListItem(panel->sizLs, sizei);
	sizeClick(panel->sizLs, panel);
    }

    if (osize)
	free(osize);


    preview(panel);
}


static void 
typefaceClick(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;
    WMListItem *item;
    Typeface *face;
    int i;
    char buffer[32];

    char *osize = NULL;
    int sizei = -1;

    osize = WMGetTextFieldText(panel->sizT);


    item = WMGetListSelectedItem(panel->typLs);
    face = (Typeface*)item->clientData;
    
    WMClearList(panel->sizLs);

    for (i = 0; i < WMGetBagItemCount(face->sizes); i++) {
	int size;

	size = (int)WMGetFromBag(face->sizes, i);
	
	if (size != 0) {
	    sprintf(buffer, "%i", size);

	    WMAddListItem(panel->sizLs, buffer);
	}
    }

    if (osize) {
	sizei = WMFindRowOfListItemWithTitle(panel->sizLs, osize);
    }
    if (sizei < 0) {
	sizei = WMFindRowOfListItemWithTitle(panel->sizLs, "12");
    }
    if (sizei < 0) {
	sizei = 0;
    }
    WMSelectListItem(panel->sizLs, sizei);
    WMSetListPosition(panel->sizLs, sizei);
    sizeClick(panel->sizLs, panel);

    if (osize)
	free(osize);

    preview(panel);
}


static void 
sizeClick(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;
    WMListItem *item;
    
    item = WMGetListSelectedItem(panel->sizLs);

    WMSetTextFieldText(panel->sizT, item->text);
    
    WMSelectTextFieldRange(panel->sizT, wmkrange(0, strlen(item->text)));

    preview(panel);
}

