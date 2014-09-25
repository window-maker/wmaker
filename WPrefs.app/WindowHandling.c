/* WindowHandling.c- options for handling windows
 *
 *  WPrefs - Window Maker Preferences Program
 *
 *  Copyright (c) 1998-2003 Alfredo K. Kojima
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

	WMFrame *placF;
	WMPopUpButton *placP;
	WMLabel *porigL;
	WMLabel *porigvL;
	WMFrame *porigF;
	WMLabel *porigW;

	WMSlider *vsli;
	WMSlider *hsli;

	WMFrame *resF;
	WMSlider *resS;
	WMLabel *resL;
	WMButton *resaB;
	WMButton *resrB;

	WMFrame *maxiF;
	WMButton *miconB;
	WMButton *mdockB;

	WMLabel *resizeL;
	WMLabel *resizeTextL;
	WMSlider *resizeS;

	WMFrame *opaqF;
	WMButton *opaqB;
	WMButton *opaqresizeB;
	WMButton *opaqkeybB;

	WMFrame *dragmaxF;
	WMPopUpButton *dragmaxP;
} _Panel;

#define ICON_FILE "whandling"

#define OPAQUE_MOVE_PIXMAP "opaque"

#define NON_OPAQUE_MOVE_PIXMAP "nonopaque"

#define OPAQUE_RESIZE_PIXMAP "opaqueresize"

#define NON_OPAQUE_RESIZE_PIXMAP "noopaqueresize"

#define THUMB_SIZE	16

static const char *const placements[] = {
	"auto",
	"random",
	"manual",
	"cascade",
	"smart",
	"center"
};

static const char *const dragMaximizedWindowOptions[] = {
	"Move",
	"RestoreGeometry",
	"Unmaximize",
	"NoMove"
};

static void sliderCallback(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	int x, y, rx, ry;
	char buffer[64];
	int swidth = WMGetSliderMaxValue(panel->hsli);
	int sheight = WMGetSliderMaxValue(panel->vsli);

	/* Parameter not used, but tell the compiler that it is ok */
	(void) w;

	x = WMGetSliderValue(panel->hsli);
	y = WMGetSliderValue(panel->vsli);

	rx = x * (WMWidgetWidth(panel->porigF) - 3) / swidth + 2;
	ry = y * (WMWidgetHeight(panel->porigF) - 3) / sheight + 2;
	WMMoveWidget(panel->porigW, rx, ry);

	sprintf(buffer, "(%i,%i)", x, y);
	WMSetLabelText(panel->porigvL, buffer);
}

static void resistanceCallback(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	char buffer[64];
	int i;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) w;

	i = WMGetSliderValue(panel->resS);

	if (i == 0)
		WMSetLabelText(panel->resL, _("OFF"));
	else {
		sprintf(buffer, "%i", i);
		WMSetLabelText(panel->resL, buffer);
	}
}

static void resizeCallback(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	char buffer[64];
	int i;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) w;

	i = WMGetSliderValue(panel->resizeS);

	if (i == 0)
		WMSetLabelText(panel->resizeL, _("OFF"));
	else {
		sprintf(buffer, "%i", i);
		WMSetLabelText(panel->resizeL, buffer);
	}
}

static int getPlacement(const char *str)
{
	if (!str)
		return 0;

	if (strcasecmp(str, "auto") == 0)
		return 0;
	else if (strcasecmp(str, "random") == 0)
		return 1;
	else if (strcasecmp(str, "manual") == 0)
		return 2;
	else if (strcasecmp(str, "cascade") == 0)
		return 3;
	else if (strcasecmp(str, "smart") == 0)
		return 4;
	else if (strcasecmp(str, "center") == 0)
		return 5;
	else
		wwarning(_("bad option value %s in WindowPlacement. Using default value"), str);
	return 0;
}

static int getDragMaximizedWindow(const char *str)
{
	if (!str)
		return 0;

	if (strcasecmp(str, "Move") == 0)
		return 0;
	else if (strcasecmp(str, "RestoreGeometry") == 0)
		return 1;
	else if (strcasecmp(str, "Unmaximize") == 0)
		return 2;
	else if (strcasecmp(str, "NoMove") == 0)
		return 3;
	else
		wwarning(_("bad option value %s in WindowPlacement. Using default value"), str);
	return 0;
}


static void showData(_Panel * panel)
{
	char *str;
	WMPropList *arr;
	int x, y;

	str = GetStringForKey("WindowPlacement");

	WMSetPopUpButtonSelectedItem(panel->placP, getPlacement(str));

	arr = GetObjectForKey("WindowPlaceOrigin");

	x = 0;
	y = 0;
	if (arr && (!WMIsPLArray(arr) || WMGetPropListItemCount(arr) != 2)) {
		wwarning(_("invalid data in option WindowPlaceOrigin. Using default (0,0)"));
	} else {
		if (arr) {
			x = atoi(WMGetFromPLString(WMGetFromPLArray(arr, 0)));
			y = atoi(WMGetFromPLString(WMGetFromPLArray(arr, 1)));
		}
	}

	WMSetSliderValue(panel->hsli, x);
	WMSetSliderValue(panel->vsli, y);

	sliderCallback(NULL, panel);

	x = GetIntegerForKey("EdgeResistance");
	WMSetSliderValue(panel->resS, x);
	resistanceCallback(NULL, panel);

	str = GetStringForKey("DragMaximizedWindow");
	WMSetPopUpButtonSelectedItem(panel->dragmaxP, getDragMaximizedWindow(str));

	x = GetIntegerForKey("ResizeIncrement");
	WMSetSliderValue(panel->resizeS, x);
	resizeCallback(NULL, panel);

	WMSetButtonSelected(panel->opaqB, GetBoolForKey("OpaqueMove"));
	WMSetButtonSelected(panel->opaqresizeB, GetBoolForKey("OpaqueResize"));
	WMSetButtonSelected(panel->opaqkeybB, GetBoolForKey("OpaqueMoveResizeKeyboard"));

	WMSetButtonSelected(panel->miconB, GetBoolForKey("NoWindowOverIcons"));
	WMSetButtonSelected(panel->mdockB, GetBoolForKey("NoWindowOverDock"));

	if (GetBoolForKey("Attraction"))
		WMPerformButtonClick(panel->resrB);
	else
		WMPerformButtonClick(panel->resaB);
}

static void storeData(_Panel * panel)
{
	WMPropList *arr;
	WMPropList *x, *y;
	char buf[16];

	SetBoolForKey(WMGetButtonSelected(panel->miconB), "NoWindowOverIcons");
	SetBoolForKey(WMGetButtonSelected(panel->mdockB), "NoWindowOverDock");

	SetBoolForKey(WMGetButtonSelected(panel->opaqB), "OpaqueMove");
	SetBoolForKey(WMGetButtonSelected(panel->opaqresizeB), "OpaqueResize");
	SetBoolForKey(WMGetButtonSelected(panel->opaqkeybB), "OpaqueMoveResizeKeyboard");

	SetStringForKey(placements[WMGetPopUpButtonSelectedItem(panel->placP)], "WindowPlacement");
	sprintf(buf, "%i", WMGetSliderValue(panel->hsli));
	x = WMCreatePLString(buf);
	sprintf(buf, "%i", WMGetSliderValue(panel->vsli));
	y = WMCreatePLString(buf);
	arr = WMCreatePLArray(x, y, NULL);
	WMReleasePropList(x);
	WMReleasePropList(y);
	SetObjectForKey(arr, "WindowPlaceOrigin");

	SetIntegerForKey(WMGetSliderValue(panel->resS), "EdgeResistance");

	SetStringForKey(dragMaximizedWindowOptions[WMGetPopUpButtonSelectedItem(panel->dragmaxP)],
			"DragMaximizedWindow");

	SetIntegerForKey(WMGetSliderValue(panel->resizeS), "ResizeIncrement");
	SetBoolForKey(WMGetButtonSelected(panel->resrB), "Attraction");

	WMReleasePropList(arr);
}

static void createPanel(Panel * p)
{
	_Panel *panel = (Panel *) p;
	WMScreen *scr = WMWidgetScreen(panel->parent);
	WMColor *color;
	WMPixmap *pixmap;
	int width, height;
	int swidth, sheight;
	char *path;
	WMBox *hbox;

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);
	WMSetBoxHorizontal(panel->box, False);
	WMSetBoxBorderWidth(panel->box, 8);

	hbox = WMCreateBox(panel->box);
	WMSetBoxHorizontal(hbox, True);
	WMAddBoxSubview(panel->box, WMWidgetView(hbox), False, True, 110, 0, 10);

    /************** Window Placement ***************/
	panel->placF = WMCreateFrame(hbox);
	WMMapWidget(panel->placF);
	WMAddBoxSubview(hbox, WMWidgetView(panel->placF), True, True, 100, 0, 10);

	WMSetFrameTitle(panel->placF, _("Window Placement"));
	WMSetBalloonTextForView(_("How to place windows when they are first put\n"
				  "on screen."), WMWidgetView(panel->placF));

	panel->placP = WMCreatePopUpButton(panel->placF);
	WMResizeWidget(panel->placP, 105, 20);
	WMMoveWidget(panel->placP, 10, 20);
	WMAddPopUpButtonItem(panel->placP, _("Automatic"));
	WMAddPopUpButtonItem(panel->placP, _("Random"));
	WMAddPopUpButtonItem(panel->placP, _("Manual"));
	WMAddPopUpButtonItem(panel->placP, _("Cascade"));
	WMAddPopUpButtonItem(panel->placP, _("Smart"));
	WMAddPopUpButtonItem(panel->placP, _("Center"));

	panel->porigL = WMCreateLabel(panel->placF);
	WMResizeWidget(panel->porigL, 110, 32);
	WMMoveWidget(panel->porigL, 3, 45);
	WMSetLabelTextAlignment(panel->porigL, WACenter);
	WMSetLabelText(panel->porigL, _("Placement Origin"));

	panel->porigvL = WMCreateLabel(panel->placF);
	WMResizeWidget(panel->porigvL, 80, 20);
	WMMoveWidget(panel->porigvL, 18, 75);
	WMSetLabelTextAlignment(panel->porigvL, WACenter);

	color = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);
	panel->porigF = WMCreateFrame(panel->placF);
	WMSetWidgetBackgroundColor(panel->porigF, color);
	WMReleaseColor(color);
	WMSetFrameRelief(panel->porigF, WRSunken);

	swidth = WidthOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));
	sheight = HeightOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));

	if (sheight > swidth) {
		width = 70 * swidth / sheight;
		if (width > 195)
			width = 195;
		height = 195 * sheight / swidth;
	} else {
		height = 195 * sheight / swidth;
		if (height > 70)
			height = 70;
		width = 70 * swidth / sheight;
	}
	WMResizeWidget(panel->porigF, width, height);
	WMMoveWidget(panel->porigF, 125 + (195 - width) / 2, 20 + (70 - height) / 2);

	panel->porigW = WMCreateLabel(panel->porigF);
	WMResizeWidget(panel->porigW, THUMB_SIZE, THUMB_SIZE);
	WMMoveWidget(panel->porigW, 2, 2);
	WMSetLabelRelief(panel->porigW, WRRaised);

	panel->hsli = WMCreateSlider(panel->placF);
	WMResizeWidget(panel->hsli, width, 12);
	WMMoveWidget(panel->hsli, 125 + (195 - width) / 2, 20 + (70 - height) / 2 + height + 2);
	WMSetSliderAction(panel->hsli, sliderCallback, panel);
	WMSetSliderMinValue(panel->hsli, 0);
	WMSetSliderMaxValue(panel->hsli, swidth);

	panel->vsli = WMCreateSlider(panel->placF);
	WMResizeWidget(panel->vsli, 12, height);
	WMMoveWidget(panel->vsli, 125 + (195 - width) / 2 + width + 2, 20 + (70 - height) / 2);
	WMSetSliderAction(panel->vsli, sliderCallback, panel);
	WMSetSliderMinValue(panel->vsli, 0);
	WMSetSliderMaxValue(panel->vsli, sheight);

	WMMapSubwidgets(panel->porigF);

	WMMapSubwidgets(panel->placF);

    /************** Opaque Move, Resize ***************/
	panel->opaqF = WMCreateFrame(hbox);
	WMMapWidget(panel->opaqF);
	WMAddBoxSubview(hbox, WMWidgetView(panel->opaqF), False, True, 150, 0, 0);

	WMSetFrameTitle(panel->opaqF, _("Opaque Move/Resize"));
	WMSetBalloonTextForView(_("Whether the window contents or only a frame should\n"
				  "be displayed during a move or resize.\n"),
				WMWidgetView(panel->opaqF));

	panel->opaqB = WMCreateButton(panel->opaqF, WBTToggle);
	WMResizeWidget(panel->opaqB, 54, 54);
	WMMoveWidget(panel->opaqB, 14, 20);
	WMSetButtonImagePosition(panel->opaqB, WIPImageOnly);

	path = LocateImage(NON_OPAQUE_MOVE_PIXMAP);
	if (path) {
		pixmap = WMCreatePixmapFromFile(scr, path);
		if (pixmap) {
			WMSetButtonImage(panel->opaqB, pixmap);
			WMReleasePixmap(pixmap);
		} else {
			wwarning(_("could not load icon %s"), path);
		}
		wfree(path);
	}

	path = LocateImage(OPAQUE_MOVE_PIXMAP);
	if (path) {
		pixmap = WMCreatePixmapFromFile(scr, path);
		if (pixmap) {
			WMSetButtonAltImage(panel->opaqB, pixmap);
			WMReleasePixmap(pixmap);
		} else {
			wwarning(_("could not load icon %s"), path);
		}
		wfree(path);
	}



	panel->opaqresizeB = WMCreateButton(panel->opaqF, WBTToggle);
	WMResizeWidget(panel->opaqresizeB, 54, 54);
	WMMoveWidget(panel->opaqresizeB, 82, 20);
	WMSetButtonImagePosition(panel->opaqresizeB, WIPImageOnly);

	path = LocateImage(NON_OPAQUE_RESIZE_PIXMAP);
	if (path) {
		pixmap = WMCreatePixmapFromFile(scr, path);
		if (pixmap) {
			WMSetButtonImage(panel->opaqresizeB, pixmap);
			WMReleasePixmap(pixmap);
		} else {
			wwarning(_("could not load icon %s"), path);
		}
		wfree(path);
	}

	path = LocateImage(OPAQUE_RESIZE_PIXMAP);
	if (path) {
		pixmap = WMCreatePixmapFromFile(scr, path);
		if (pixmap) {
			WMSetButtonAltImage(panel->opaqresizeB, pixmap);
			WMReleasePixmap(pixmap);
		} else {
			wwarning(_("could not load icon %s"), path);
		}
		wfree(path);
	}

	panel->opaqkeybB = WMCreateSwitchButton(panel->opaqF);
	WMResizeWidget(panel->opaqkeybB, 122, 25);
	WMMoveWidget(panel->opaqkeybB, 14, 79);
	WMSetButtonText(panel->opaqkeybB, _("by keyboard"));

	WMMapSubwidgets(panel->opaqF);


    /**************** Account for Icon/Dock ***************/
	panel->maxiF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->maxiF, 205, 100);
	WMMoveWidget(panel->maxiF, 307, 125);
	WMSetFrameTitle(panel->maxiF, _("When maximizing..."));

	panel->miconB = WMCreateSwitchButton(panel->maxiF);
	WMResizeWidget(panel->miconB, 190, 30);
	WMMoveWidget(panel->miconB, 10, 14);
	WMSetButtonText(panel->miconB, _("...do not cover icons"));

	panel->mdockB = WMCreateSwitchButton(panel->maxiF);
	WMResizeWidget(panel->mdockB, 190, 30);
	WMMoveWidget(panel->mdockB, 10, 39);

	WMSetButtonText(panel->mdockB, _("...do not cover dock"));

	panel->resizeS = WMCreateSlider(panel->maxiF);
	WMResizeWidget(panel->resizeS, 50, 15);
	WMMoveWidget(panel->resizeS, 10, 74);
	WMSetSliderMinValue(panel->resizeS, 0);
	WMSetSliderMaxValue(panel->resizeS, 100);
	WMSetSliderAction(panel->resizeS, resizeCallback, panel);

	panel->resizeL = WMCreateLabel(panel->maxiF);
	WMResizeWidget(panel->resizeL, 30, 15);
	WMMoveWidget(panel->resizeL, 60, 74);

	panel->resizeTextL = WMCreateLabel(panel->maxiF);
	WMSetLabelText(panel->resizeTextL, _("Mod+Wheel\nresize increment"));
	WMResizeWidget(panel->resizeTextL, 110, 30);
	WMMoveWidget(panel->resizeTextL, 90, 66);

	WMMapSubwidgets(panel->maxiF);

    /**************** Edge Resistance  ****************/
	panel->resF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->resF, 289, 47);
	WMMoveWidget(panel->resF, 8, 125);
	WMSetFrameTitle(panel->resF, _("Edge Resistance"));

	WMSetBalloonTextForView(_("Edge resistance will make windows `resist'\n"
				  "being moved further for the defined threshold\n"
				  "when moved against other windows or the edges\n"
				  "of the screen."), WMWidgetView(panel->resF));

	panel->resS = WMCreateSlider(panel->resF);
	WMResizeWidget(panel->resS, 80, 15);
	WMMoveWidget(panel->resS, 10, 20);
	WMSetSliderMinValue(panel->resS, 0);
	WMSetSliderMaxValue(panel->resS, 80);
	WMSetSliderAction(panel->resS, resistanceCallback, panel);

	panel->resL = WMCreateLabel(panel->resF);
	WMResizeWidget(panel->resL, 30, 15);
	WMMoveWidget(panel->resL, 95, 22);

	panel->resaB = WMCreateRadioButton(panel->resF);
	WMMoveWidget(panel->resaB, 130, 15);
	WMResizeWidget(panel->resaB, 70, 27);
	WMSetButtonText(panel->resaB, _("Resist"));

	panel->resrB = WMCreateRadioButton(panel->resF);
	WMMoveWidget(panel->resrB, 200, 15);
	WMResizeWidget(panel->resrB, 70, 27);
	WMSetButtonText(panel->resrB, _("Attract"));
	WMGroupButtons(panel->resrB, panel->resaB);

	WMMapSubwidgets(panel->resF);

    /**************** Dragging a Maximized Window ****************/
	panel->dragmaxF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->dragmaxF, 289, 46);
	WMMoveWidget(panel->dragmaxF, 8, 179);
	WMSetFrameTitle(panel->dragmaxF, _("When dragging a maximized window..."));

	panel->dragmaxP = WMCreatePopUpButton(panel->dragmaxF);
	WMResizeWidget(panel->dragmaxP, 269, 20);
	WMMoveWidget(panel->dragmaxP, 10, 20);
	WMAddPopUpButtonItem(panel->dragmaxP, _("...change position (normal behavior)"));
	WMAddPopUpButtonItem(panel->dragmaxP, _("...restore unmaximized geometry"));
	WMAddPopUpButtonItem(panel->dragmaxP, _("...consider the window unmaximized"));
	WMAddPopUpButtonItem(panel->dragmaxP, _("...do not move the window"));

	WMMapSubwidgets(panel->dragmaxF);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	/* show the config data */
	showData(panel);
}

static void undo(_Panel * panel)
{
	showData(panel);
}

Panel *InitWindowHandling(WMWidget *parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Window Handling Preferences");

	panel->description = _("Window handling options. Initial placement style\n"
			       "edge resistance, opaque move etc.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;
	panel->callbacks.undoChanges = undo;

	AddSection(panel, ICON_FILE);

	return panel;
}
