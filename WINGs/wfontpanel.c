



#include "WINGsP.h"

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

static proplist_t createFontDatabase(WMScreen *scr);

static void listFamilies(proplist_t fdb, WMList *list);

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

    panel->typL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->typL, dark);
    WMSetLabelText(panel->typL, "Typeface");
    WMSetLabelFont(panel->typL, font);
    WMSetLabelTextColor(panel->typL, white);
    WMSetLabelRelief(panel->typL, WRSunken);
    WMSetLabelTextAlignment(panel->typL, WACenter);    

    panel->typLs = WMCreateList(panel->lowerF);

    panel->sizL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->sizL, dark);
    WMSetLabelText(panel->sizL, "Size");
    WMSetLabelFont(panel->sizL, font);
    WMSetLabelTextColor(panel->sizL, white);
    WMSetLabelRelief(panel->sizL, WRSunken);
    WMSetLabelTextAlignment(panel->sizL, WACenter);

    panel->sizT = WMCreateTextField(panel->lowerF);

    panel->sizLs = WMCreateList(panel->lowerF);

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

    panel->fdb = createFontDatabase(scr);
    listFamilies(panel->fdb, panel->famLs);
    
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


static Bool
parseFont(char *font, char **values)
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
	    values[part]=wstrdup(buffer);
	    bptr = buffer;
	    part++;
	} else {
	    *bptr++ = *ptr;
	}
	ptr++;
    }
    *bptr = 0;
    values[part]=wstrdup(buffer);
    
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




static proplist_t foundryKey = NULL;
static proplist_t typeKey = NULL;
static proplist_t sizeKey, encKey, xlfdKey;


static void
addSizeToEnc(proplist_t enc, char **fields)
{
    proplist_t sizel;
    
    sizel = PLGetDictionaryEntry(enc, sizeKey);
    if (!sizel) {
	sizel = PLMakeArrayFromElements(PLMakeString(fields[PIXEL_SIZE]),NULL);
	PLInsertDictionaryEntry(enc, sizeKey, sizel);
    } else {
	PLAppendArrayElement(sizel, PLMakeString(fields[PIXEL_SIZE]));
    }
}


static proplist_t
addTypefaceToFont(proplist_t font, char **fields)
{
    proplist_t face, encod, facedic, encodic;
    char buffer[256];
    
    strcpy(buffer, fields[WEIGHT]);
    
    if (strcasecmp(fields[SLANT], "R")==0)
	strcat(buffer, " Roman ");
    else if (strcasecmp(fields[SLANT], "I")==0)
	strcat(buffer, " Italic ");
    else if (strcasecmp(fields[SLANT], "O")==0)
	strcat(buffer, " Oblique ");
    else if (strcasecmp(fields[SLANT], "RI")==0)
	strcat(buffer, " Reverse Italic ");
    else if (strcasecmp(fields[SLANT], "RO")==0)
	strcat(buffer, " Reverse Oblique ");
    else if (strcasecmp(fields[SLANT], "OT")==0)
	strcat(buffer, " ? ");
/*    else
 * polymorphic fonts
 */
    strcat(buffer, fields[SETWIDTH]);
    strcat(buffer, " ");
    strcat(buffer, fields[ADD_STYLE]);

    face = PLMakeString(buffer);

    facedic = PLGetDictionaryEntry(font, face);
    if (!facedic) {
	facedic = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
	PLInsertDictionaryEntry(font, face, facedic);
	PLRelease(facedic);
    }
    PLRelease(face);

    strcpy(buffer, fields[REGISTRY]);
    strcat(buffer, "-");
    strcat(buffer, fields[ENCODING]);
    encod = PLMakeString(buffer);

    encodic = PLGetDictionaryEntry(facedic, encod);
    if (!encodic) {
	proplist_t tmp;
	sprintf(buffer, "-%s-%s-%s-%s-%s-%s-%%d-0-%s-%s-%s-%s-%s-%s",
		fields[FOUNDRY], fields[FAMILY], fields[WEIGHT], 
		fields[SLANT], fields[SETWIDTH], fields[ADD_STYLE],
		fields[RES_X], fields[RES_Y], fields[SPACING],
		fields[AV_WIDTH], fields[REGISTRY], fields[ENCODING]);
	tmp = PLMakeString(buffer);

	encodic = PLMakeDictionaryFromEntries(xlfdKey, tmp, NULL);
	PLRelease(tmp);
	PLInsertDictionaryEntry(facedic, encod, encodic);
	PLRelease(encodic);
    }
    addSizeToEnc(encodic, fields);
    return font;
}


static proplist_t
makeFontEntry(char **fields)
{
    proplist_t font;
    proplist_t value;
    proplist_t tmp;
    value = PLMakeString(fields[FOUNDRY]);
    
    tmp = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
    
    font = PLMakeDictionaryFromEntries(foundryKey, value,
				       typeKey, tmp,
				       NULL);
    PLRelease(value);
    PLRelease(tmp);

    addTypefaceToFont(font, fields);

    return font;
}


static proplist_t
createFontDatabase(WMScreen *scr)
{
    char **fontList;
    int count;
    char *fields[NUM_FIELDS];
    int font_name_length;
    char buffer[256];
    int i;
    proplist_t fdb;
    proplist_t font;
    proplist_t family;
    proplist_t foundry;
    proplist_t tmp;

    if (!foundryKey) {
	foundryKey = PLMakeString("Foundry");
	typeKey = PLMakeString("Typeface");
	encKey = PLMakeString("Encoding");
	sizeKey = PLMakeString("Sizes");
	xlfdKey = PLMakeString("XLFD");
    }

    /* retrieve a complete listing of the available fonts */
    fontList = XListFonts(scr->display, ALL_FONTS_MASK, MAX_FONTS_TO_RETRIEVE, 
			  &count);
    if (!fontList) {
	wwarning("could not retrieve font list");
	return NULL;
    }

    fdb = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    for (i=0; i<count; i++) {
	if (!isXLFD(fontList[i], &font_name_length)) {
	    continue;
	}
	/* the XLFD specs limit the size of a font description in 255 chars */
	assert(font_name_length < 256);

	if (parseFont(fontList[i], fields)) {	    
	    family = PLMakeString(fields[FAMILY]);
	    font = PLGetDictionaryEntry(fdb, family);
	    if (font) {
		foundry = PLGetDictionaryEntry(font, foundryKey);
		if (strcmp(PLGetString(foundry), fields[FOUNDRY])==0) {
		    /* already a font with the same family */
		    addTypefaceToFont(font, fields);
		} else {
		    /* same font family by different foundries */
		    sprintf(buffer, "%s (%s)", fields[FAMILY],
			    fields[FOUNDRY]);
		    PLRelease(family);
		    family = PLMakeString(buffer);
		    
		    font = PLGetDictionaryEntry(fdb, family);
		    if (font) {
			/* already a font with the same family */
			addTypefaceToFont(font, fields);
		    } else {
			tmp = makeFontEntry(fields);
			PLInsertDictionaryEntry(fdb, family, tmp);
			PLRelease(tmp);
		    }
		}
	    } else {
		tmp = makeFontEntry(fields);
		PLInsertDictionaryEntry(fdb, family, tmp);
		PLRelease(tmp);
	    }
	    PLRelease(family);
	}
    }
    XFreeFontNames(fontList);
    
    return fdb;
}






static void
listFamilies(proplist_t fdb, WMList *list)
{
    proplist_t arr;
    proplist_t fam;
    int i;
    
    arr = PLGetAllDictionaryKeys(fdb);
    for (i = 0; i<PLGetNumberOfElements(arr); i++) {
	fam = PLGetArrayElement(arr, i);
	WMAddSortedListItem(list, PLGetString(fam));
    }
}
