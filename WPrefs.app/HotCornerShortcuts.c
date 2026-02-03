/* HotCornerShortcuts.c - screen corners actions
 *
 *  WPrefs - Window Maker Preferences Program
 *
 *  Copyright (c) 2023 Window Maker Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "WPrefs.h"

typedef struct _Panel {
	WMBox *box;

	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;
	WMPixmap *icon;
	Pixmap quarter[4];

	WMFrame *hcF;
	WMButton *hcB;
	WMFrame *hceF;
	WMSlider *hceS;
	WMLabel *hceL;
	WMFrame *hcdescF;
	WMLabel *hcdescL;

	WMFrame *hcdelayF;
	WMButton *hcdelayB[5];
	WMTextField *hcdelayT;
	WMLabel *hcdelayL;
	WMLabel *icornerL;

	WMFrame *hcactionsF;
	WMTextField *hcactionsT[4];

	char hotcornerDelaySelected;
} _Panel;

#define ICON_FILE	"hotcorners"

#define DELAY_ICON "timer%i"
#define DELAY_ICON_S "timer%is"

static void edgeCallback(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	char buffer[64];
	int i;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) w;

	i = WMGetSliderValue(panel->hceS);

	if (i == 0)
		sprintf(buffer, _("OFF"));
	else if (i == 1)
		sprintf(buffer, _("1 pixel"));
	else if (i <= 4)
		/* 2-4 */
		sprintf(buffer, _("%i pixels"), i);
	else
		/* >4 */
		sprintf(buffer, _("%i pixels "), i);	/* note space! */
	WMSetLabelText(panel->hceL, buffer);
}

static void showData(_Panel * panel)
{
	int i;
	char buffer[32];
	WMPropList *array;

	if (!GetObjectForKey("HotCornerDelay"))
		i = 250;
	else
		i = GetIntegerForKey("HotCornerDelay");
	sprintf(buffer, "%i", i);
	WMSetTextFieldText(panel->hcdelayT, buffer);

	switch (i) {
	case 0:
		WMPerformButtonClick(panel->hcdelayB[0]);
		break;
	case 250:
		WMPerformButtonClick(panel->hcdelayB[1]);
		break;
	case 500:
		WMPerformButtonClick(panel->hcdelayB[2]);
		break;
	case 750:
		WMPerformButtonClick(panel->hcdelayB[3]);
		break;
	case 1000:
		WMPerformButtonClick(panel->hcdelayB[4]);
		break;
	}

	i = GetIntegerForKey("HotCornerEdge");
	i = i < 0 ? 2 : i;
	i = i > 10 ? 10 : i;
	WMSetSliderValue(panel->hceS, i);
	edgeCallback(NULL, panel);

	WMSetButtonSelected(panel->hcB, GetBoolForKey("HotCorners"));

        array = GetObjectForKey("HotCornerActions");
	if (array && (!WMIsPLArray(array) || WMGetPropListItemCount(array) != sizeof(panel->hcactionsT) / sizeof(WMTextField *))) {
		wwarning(_("invalid data in option HotCornerActions."));
	} else {
		if (array) {
			for (i = 0; i < sizeof(panel->hcactionsT) / sizeof(WMTextField *); i++)
				if (strcasecmp(WMGetFromPLString(WMGetFromPLArray(array, i)), "None") != 0)
					WMSetTextFieldText(panel->hcactionsT[i], WMGetFromPLString(WMGetFromPLArray(array, i)));
		}
	}
}

static void storeData(_Panel * panel)
{
	WMPropList *list;
	WMPropList *tmp;
	char *str;
	int i;

	SetBoolForKey(WMGetButtonSelected(panel->hcB), "HotCorners");

	str = WMGetTextFieldText(panel->hcdelayT);
	if (sscanf(str, "%i", &i) != 1)
		i = 0;
	SetIntegerForKey(i, "HotCornerDelay");
	free(str);

	SetIntegerForKey(WMGetSliderValue(panel->hceS), "HotCornerEdge");	

	list = WMCreatePLArray(NULL, NULL);
	for (i = 0; i < sizeof(panel->hcactionsT) / sizeof(WMTextField *); i++) {
		str = WMGetTextFieldText(panel->hcactionsT[i]);
		if (strlen(str) == 0)
			str = "None";
		tmp = WMCreatePLString(str);
		WMAddToPLArray(list, tmp);
	}
	SetObjectForKey(list, "HotCornerActions");
}

static void pushDelayButton(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;

	panel->hotcornerDelaySelected = 1;
	if (w == panel->hcdelayB[0]) {
		WMSetTextFieldText(panel->hcdelayT, "0");
	} else if (w == panel->hcdelayB[1]) {
		WMSetTextFieldText(panel->hcdelayT, "250");
	} else if (w == panel->hcdelayB[2]) {
		WMSetTextFieldText(panel->hcdelayT, "500");
	} else if (w == panel->hcdelayB[3]) {
		WMSetTextFieldText(panel->hcdelayT, "700");
	} else if (w == panel->hcdelayB[4]) {
		WMSetTextFieldText(panel->hcdelayT, "1000");
	}
}

static void delayTextChanged(void *observerData, WMNotification * notification)
{
	_Panel *panel = (_Panel *) observerData;
	int i;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) notification;

	if (panel->hotcornerDelaySelected) {
		for (i = 0; i < 5; i++) {
			WMSetButtonSelected(panel->hcdelayB[i], False);
		}
		panel->hotcornerDelaySelected = 0;
	}
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	int i;
	char *buf1, *buf2;
	WMColor *color;
	WMFont *font;
	char *path;
	RImage *xis = NULL;
	WMScreen *scr = WMWidgetScreen(panel->parent);
	RContext *rc = WMScreenRContext(scr);
	GC gc = XCreateGC(scr->display, WMWidgetXID(panel->parent), 0, NULL);

	path = LocateImage(ICON_FILE);
	if (path) {
		xis = RLoadImage(rc, path, 0);
		if (!xis) {
			wwarning(_("could not load image file %s"), path);
		}
		wfree(path);
	}

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);


    /***************** Hot Corner lelf side frames *****************/
	panel->hcF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->hcF, 240, 53);
	WMMoveWidget(panel->hcF, 15, 17);

	panel->hcB = WMCreateSwitchButton(panel->hcF);
	WMResizeWidget(panel->hcB, 150, 30);
	WMMoveWidget(panel->hcB, 15, 12);
	WMSetButtonText(panel->hcB, _("Enable Hot Corners"));

	WMMapSubwidgets(panel->hcF);

	panel->hceF = WMCreateFrame(panel->box);
	WMSetFrameTitle(panel->hceF, _("Hot Corner Edge"));
	WMResizeWidget(panel->hceF, 240, 40);
	WMMoveWidget(panel->hceF, 15, 77);

	panel->hceS = WMCreateSlider(panel->hceF);
	WMResizeWidget(panel->hceS, 80, 15);
	WMMoveWidget(panel->hceS, 15, 18);
	WMSetSliderMinValue(panel->hceS, 2);
	WMSetSliderMaxValue(panel->hceS, 10);
	WMSetSliderAction(panel->hceS, edgeCallback, panel);

	panel->hceL = WMCreateLabel(panel->hceF);
	WMResizeWidget(panel->hceL, 100, 15);
	WMMoveWidget(panel->hceL, 105, 18);

	WMMapSubwidgets(panel->hceF);

	panel->hcdescF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->hcdescF, 240, 95);
	WMMoveWidget(panel->hcdescF, 15, 130);

	panel->hcdescL = WMCreateLabel(panel->hcdescF);
	WMResizeWidget(panel->hcdescL, 200, 70);
	WMMoveWidget(panel->hcdescL, 15, 10);
	WMSetLabelText(panel->hcdescL,
		       _("Instructions:\n\n"
			 " - assign command to corner\n"
			 " - or leave it empty\n"));

	WMMapSubwidgets(panel->hcdescF);

    /***************** Hot Corner Action Delay *****************/
	panel->hcdelayF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->hcdelayF, 245, 60);
	WMMoveWidget(panel->hcdelayF, 265, 10);

	WMSetFrameTitle(panel->hcdelayF, _("Hot Corner Delay"));

	buf1 = wmalloc(strlen(DELAY_ICON) + 1);
	buf2 = wmalloc(strlen(DELAY_ICON_S) + 1);

	for (i = 0; i < 5; i++) {
		char *path;

		panel->hcdelayB[i] = WMCreateCustomButton(panel->hcdelayF, WBBStateChangeMask);
		WMResizeWidget(panel->hcdelayB[i], 25, 25);
		WMMoveWidget(panel->hcdelayB[i], 12 + (30 * i), 25);
		WMSetButtonBordered(panel->hcdelayB[i], False);
		WMSetButtonImagePosition(panel->hcdelayB[i], WIPImageOnly);
		WMSetButtonAction(panel->hcdelayB[i], pushDelayButton, panel);
		if (i > 0)
			WMGroupButtons(panel->hcdelayB[0], panel->hcdelayB[i]);
		sprintf(buf1, DELAY_ICON, i);
		sprintf(buf2, DELAY_ICON_S, i);
		path = LocateImage(buf1);
		if (path) {
			panel->icon = WMCreatePixmapFromFile(scr, path);
			if (panel->icon) {
				WMSetButtonImage(panel->hcdelayB[i], panel->icon);
				WMReleasePixmap(panel->icon);
			} else {
				wwarning(_("could not load icon file %s"), path);
			}
			wfree(path);
		}
		path = LocateImage(buf2);
		if (path) {
			panel->icon = WMCreatePixmapFromFile(scr, path);
			if (panel->icon) {
				WMSetButtonAltImage(panel->hcdelayB[i], panel->icon);
				WMReleasePixmap(panel->icon);
			} else {
				wwarning(_("could not load icon file %s"), path);
			}
			wfree(path);
		}
	}
	wfree(buf1);
	wfree(buf2);

	panel->hcdelayT = WMCreateTextField(panel->hcdelayF);

	WMResizeWidget(panel->hcdelayT, 36, 20);
	WMMoveWidget(panel->hcdelayT, 165, 28);
	WMAddNotificationObserver(delayTextChanged, panel, WMTextDidChangeNotification, panel->hcdelayT);

	panel->hcdelayL = WMCreateLabel(panel->hcdelayF);
	WMResizeWidget(panel->hcdelayL, 36, 16);
	WMMoveWidget(panel->hcdelayL, 205, 32);
	WMSetLabelText(panel->hcdelayL, _("ms"));

	color = WMDarkGrayColor(scr);
	font = WMSystemFontOfSize(scr, 10);

	WMSetLabelTextColor(panel->hcdelayL, color);
	WMSetLabelFont(panel->hcdelayL, font);

	WMReleaseColor(color);
	WMReleaseFont(font);

	WMMapSubwidgets(panel->hcdelayF);

    /***************** Set Hot Corner Actions ****************/
	panel->hcactionsF = WMCreateFrame(panel->box);
	WMSetFrameTitle(panel->hcactionsF, _("Hot Corner Actions"));
	WMResizeWidget(panel->hcactionsF, 245, 148);
	WMMoveWidget(panel->hcactionsF, 265, 77);

	panel->icornerL = WMCreateLabel(panel->hcactionsF);
	WMResizeWidget(panel->icornerL, 24, 24);
	WMMoveWidget(panel->icornerL, 10, 18);
	WMSetLabelImagePosition(panel->icornerL, WIPImageOnly);
	CreateImages(scr, rc, xis, ICON_FILE, &panel->icon, NULL);
	if (panel->icon)
	{
		WMPixmap *nicon;

		panel->quarter[0] = XCreatePixmap(scr->display, WMWidgetXID(panel->parent), panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		XCopyArea(scr->display, WMGetPixmapXID(panel->icon), panel->quarter[0], gc, 0, 0, panel->icon->width/2, panel->icon->height/2,  0, 0);
		nicon = WMCreatePixmapFromXPixmaps(scr, panel->quarter[0], WMGetPixmapMaskXID(panel->icon),
                                     panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		WMSetLabelImage(panel->icornerL, nicon);
		WMReleasePixmap(nicon);
	}

	panel->hcactionsT[0] = WMCreateTextField(panel->hcactionsF);
	WMResizeWidget(panel->hcactionsT[0], 180, 20);
	WMMoveWidget(panel->hcactionsT[0], 50, 20);

	panel->icornerL = WMCreateLabel(panel->hcactionsF);
	WMResizeWidget(panel->icornerL, 24, 24);
	WMMoveWidget(panel->icornerL, 10, 48);
	WMSetLabelImagePosition(panel->icornerL, WIPImageOnly);
	if (panel->icon)
	{
		WMPixmap *nicon;

		panel->quarter[1] = XCreatePixmap(scr->display, WMWidgetXID(panel->parent), panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		XCopyArea(scr->display, WMGetPixmapXID(panel->icon), panel->quarter[1], gc, panel->icon->width/2, 0, panel->icon->width/2, panel->icon->height/2,  0, 0);
		nicon = WMCreatePixmapFromXPixmaps(scr, panel->quarter[1], WMGetPixmapMaskXID(panel->icon),
                                     panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		WMSetLabelImage(panel->icornerL, nicon);
		WMReleasePixmap(nicon);
	}

	panel->hcactionsT[1] = WMCreateTextField(panel->hcactionsF);
	WMResizeWidget(panel->hcactionsT[1], 180, 20);
	WMMoveWidget(panel->hcactionsT[1], 50, 50);

	panel->icornerL = WMCreateLabel(panel->hcactionsF);
	WMResizeWidget(panel->icornerL, 24, 24);
	WMMoveWidget(panel->icornerL, 10, 78);
	WMSetLabelImagePosition(panel->icornerL, WIPImageOnly);
	if (panel->icon)
	{
		WMPixmap *nicon;

		panel->quarter[2] = XCreatePixmap(scr->display, WMWidgetXID(panel->parent), panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		XCopyArea(scr->display, WMGetPixmapXID(panel->icon), panel->quarter[2], gc, 0, panel->icon->height/2, panel->icon->width/2, panel->icon->height/2,  0, 0);
		nicon = WMCreatePixmapFromXPixmaps(scr, panel->quarter[2], WMGetPixmapMaskXID(panel->icon),
                                     panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		WMSetLabelImage(panel->icornerL, nicon);
		WMReleasePixmap(nicon);
	}
	panel->hcactionsT[2] = WMCreateTextField(panel->hcactionsF);
	WMResizeWidget(panel->hcactionsT[2], 180, 20);
	WMMoveWidget(panel->hcactionsT[2], 50, 80);

	panel->icornerL = WMCreateLabel(panel->hcactionsF);
	WMResizeWidget(panel->icornerL, 24, 24);
	WMMoveWidget(panel->icornerL, 10, 108);
	WMSetLabelImagePosition(panel->icornerL, WIPImageOnly);
	if (panel->icon)
	{
		WMPixmap *nicon;

		panel->quarter[3] = XCreatePixmap(scr->display, WMWidgetXID(panel->parent), panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		XCopyArea(scr->display, WMGetPixmapXID(panel->icon), panel->quarter[3], gc, panel->icon->width/2, panel->icon->height/2, panel->icon->width/2, panel->icon->height/2,  0, 0);
		nicon = WMCreatePixmapFromXPixmaps(scr, panel->quarter[3], WMGetPixmapMaskXID(panel->icon),
                                     panel->icon->width/2, panel->icon->height/2, WMScreenDepth(scr));
		WMSetLabelImage(panel->icornerL, nicon);
		WMReleasePixmap(nicon);
	}

	panel->hcactionsT[3] = WMCreateTextField(panel->hcactionsF);
	WMResizeWidget(panel->hcactionsT[3], 180, 20);
	WMMoveWidget(panel->hcactionsT[3], 50, 107);

	WMMapSubwidgets(panel->hcactionsF);

	if (xis)
		RReleaseImage(xis);
	XFreeGC(scr->display, gc);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	showData(panel);
}
static void prepareForClose(_Panel *panel)
{
	int i;
	WMScreen *scr = WMWidgetScreen(panel->parent);

	WMReleasePixmap(panel->icon);
	for (i = 0; i < sizeof(panel->quarter) / sizeof(Pixmap); i++)
		XFreePixmap(scr->display, panel->quarter[i]);
}

Panel *InitHotCornerShortcuts(WMWidget *parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Hot Corner Shortcut Preferences");
	panel->description = _("Choose actions to perform when you move the\n"
			       "mouse pointer to the screen corners.");
	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;
	panel->callbacks.prepareForClose = prepareForClose;

	AddSection(panel, ICON_FILE);

	return panel;
}
