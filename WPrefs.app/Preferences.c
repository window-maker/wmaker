/* Preferences.c- misc ergonomic preferences
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


static const struct {
	const char *db_key;
	const char *label;
} balloon_choices[] = {
	{ "WindowTitleBalloons",       N_("incomplete window titles"), },
	{ "MiniwindowTitleBalloons",   N_("miniwindow titles"), },
	{ "MiniwindowApercuBalloons",  N_("miniwindow apercus"), },
	{ "AppIconBalloons",           N_("application/dock icons"), },
	{ "HelpBalloons",              N_("internal help"), }
};

static const struct {
	const char *db_key;
	int default_value;
	const char *label;
} appicon_bouncing[] = {
	{ "DoNotMakeAppIconsBounce",   False, N_("Disable AppIcon bounce.") },
	{ "BounceAppIconsWhenUrgent",  True,  N_("Bounce AppIcon when the application wants attention.") },
	{ "RaiseAppIconsWhenBouncing", False, N_("Raise AppIcons when bouncing.") }
};

typedef struct _Panel {
	WMBox *box;

	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;

	WMFrame *sizeF;
	WMPopUpButton *sizeP;

	WMFrame *posiF;
	WMPopUpButton *posiP;

	WMFrame *ballF;
	WMButton *ballB[wlengthof_nocheck(balloon_choices)];

	WMFrame *optF;
	WMButton *bounceB[wlengthof_nocheck(appicon_bouncing)];

	WMFrame *borderF;
	WMSlider *borderS;
	WMLabel *borderL;
	WMButton *lrB;
	WMButton *tbB;

} _Panel;

#define ICON_FILE	"ergonomic"

static void borderCallback(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	char buffer[64];
	int i;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) w;

	i = WMGetSliderValue(panel->borderS);

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
	WMSetLabelText(panel->borderL, buffer);
}

static void showData(_Panel * panel)
{
	char *str;
	int x;

	str = GetStringForKey("ResizeDisplay");
	if (!str)
		str = "corner";
	if (strcasecmp(str, "corner") == 0)
		WMSetPopUpButtonSelectedItem(panel->sizeP, 0);
	else if (strcasecmp(str, "center") == 0)
		WMSetPopUpButtonSelectedItem(panel->sizeP, 1);
	else if (strcasecmp(str, "floating") == 0)
		WMSetPopUpButtonSelectedItem(panel->sizeP, 2);
	else if (strcasecmp(str, "line") == 0)
		WMSetPopUpButtonSelectedItem(panel->sizeP, 3);
	else if (strcasecmp(str, "none") == 0)
		WMSetPopUpButtonSelectedItem(panel->sizeP, 4);

	str = GetStringForKey("MoveDisplay");
	if (!str)
		str = "corner";
	if (strcasecmp(str, "corner") == 0)
		WMSetPopUpButtonSelectedItem(panel->posiP, 0);
	else if (strcasecmp(str, "center") == 0)
		WMSetPopUpButtonSelectedItem(panel->posiP, 1);
	else if (strcasecmp(str, "floating") == 0)
		WMSetPopUpButtonSelectedItem(panel->posiP, 2);
	else if (strcasecmp(str, "none") == 0)
		WMSetPopUpButtonSelectedItem(panel->posiP, 3);

	x = GetIntegerForKey("WorkspaceBorderSize");
	x = x < 0 ? 0 : x;
	x = x > 5 ? 5 : x;
	WMSetSliderValue(panel->borderS, x);
	borderCallback(NULL, panel);

	str = GetStringForKey("WorkspaceBorder");
	if (!str)
		str = "none";
	if (strcasecmp(str, "LeftRight") == 0) {
		WMSetButtonSelected(panel->lrB, True);
	} else if (strcasecmp(str, "TopBottom") == 0) {
		WMSetButtonSelected(panel->tbB, True);
	} else if (strcasecmp(str, "AllDirections") == 0) {
		WMSetButtonSelected(panel->tbB, True);
		WMSetButtonSelected(panel->lrB, True);
	}

	for (x = 0; x < wlengthof(appicon_bouncing); x++) {
		if (GetStringForKey(appicon_bouncing[x].db_key))
			WMSetButtonSelected(panel->bounceB[x], GetBoolForKey(appicon_bouncing[x].db_key));
	}

	for (x = 0; x < wlengthof(balloon_choices); x++)
		WMSetButtonSelected(panel->ballB[x], GetBoolForKey(balloon_choices[x].db_key));
}

static void storeData(_Panel * panel)
{
	char *str;
	Bool lr, tb;
	int i;

	switch (WMGetPopUpButtonSelectedItem(panel->sizeP)) {
	case 0:
		str = "corner";
		break;
	case 1:
		str = "center";
		break;
	case 2:
		str = "floating";
		break;
	case 4:
		str = "none";
		break;
	default:
		str = "line";
		break;
	}
	SetStringForKey(str, "ResizeDisplay");

	switch (WMGetPopUpButtonSelectedItem(panel->posiP)) {
	case 0:
		str = "corner";
		break;
	case 1:
		str = "center";
		break;
	case 3:
		str = "none";
		break;
	default:
		str = "floating";
		break;
	}
	SetStringForKey(str, "MoveDisplay");

	lr = WMGetButtonSelected(panel->lrB);
	tb = WMGetButtonSelected(panel->tbB);
	if (lr && tb)
		str = "AllDirections";
	else if (lr)
		str = "LeftRight";
	else if (tb)
		str = "TopBottom";
	else
		str = "None";
	SetStringForKey(str, "WorkspaceBorder");
	SetIntegerForKey(WMGetSliderValue(panel->borderS), "WorkspaceBorderSize");

	for (i = 0; i < wlengthof(appicon_bouncing); i++)
		SetBoolForKey(WMGetButtonSelected(panel->bounceB[i]), appicon_bouncing[i].db_key);

	for (i = 0; i < wlengthof(balloon_choices); i++)
		SetBoolForKey(WMGetButtonSelected(panel->ballB[i]), balloon_choices[i].db_key);
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	int i;

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    /***************** Size Display ****************/
	panel->sizeF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->sizeF, 240, 60);
	WMMoveWidget(panel->sizeF, 15, 10);
	WMSetFrameTitle(panel->sizeF, _("Size Display"));

	WMSetBalloonTextForView(_("The position or style of the window size\n"
				  "display that's shown when a window is resized."), WMWidgetView(panel->sizeF));

	panel->sizeP = WMCreatePopUpButton(panel->sizeF);
	WMResizeWidget(panel->sizeP, 200, 20);
	WMMoveWidget(panel->sizeP, 20, 24);
	WMAddPopUpButtonItem(panel->sizeP, _("Corner of screen"));
	WMAddPopUpButtonItem(panel->sizeP, _("Center of screen"));
	WMAddPopUpButtonItem(panel->sizeP, _("Center of resized window"));
	WMAddPopUpButtonItem(panel->sizeP, _("Technical drawing-like"));
	WMAddPopUpButtonItem(panel->sizeP, _("Disabled"));

	WMMapSubwidgets(panel->sizeF);

    /***************** Position Display ****************/
	panel->posiF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->posiF, 240, 60);
	WMMoveWidget(panel->posiF, 15, 75);
	WMSetFrameTitle(panel->posiF, _("Position Display"));

	WMSetBalloonTextForView(_("The position or style of the window position\n"
				  "display that's shown when a window is moved."), WMWidgetView(panel->posiF));

	panel->posiP = WMCreatePopUpButton(panel->posiF);
	WMResizeWidget(panel->posiP, 200, 20);
	WMMoveWidget(panel->posiP, 20, 24);
	WMAddPopUpButtonItem(panel->posiP, _("Corner of screen"));
	WMAddPopUpButtonItem(panel->posiP, _("Center of screen"));
	WMAddPopUpButtonItem(panel->posiP, _("Center of resized window"));
	WMAddPopUpButtonItem(panel->posiP, _("Disabled"));

	WMMapSubwidgets(panel->posiF);

    /***************** Balloon Text ****************/
	panel->ballF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->ballF, 240, 126);
	WMMoveWidget(panel->ballF, 265, 10);
	WMSetFrameTitle(panel->ballF, _("Show balloon for..."));

	for (i = 0; i < wlengthof(balloon_choices); i++) {
		panel->ballB[i] = WMCreateSwitchButton(panel->ballF);
		WMResizeWidget(panel->ballB[i], 210, 20);
		WMMoveWidget(panel->ballB[i], 15, 16 + i * 22);
		WMSetButtonText(panel->ballB[i], _(balloon_choices[i].label));
	}

	WMMapSubwidgets(panel->ballF);

    /***************** Options ****************/
	panel->optF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->optF, 240, 91);
	WMMoveWidget(panel->optF, 265, 136);
	WMSetFrameTitle(panel->optF, _("AppIcon bouncing"));

	for (i = 0; i < wlengthof(appicon_bouncing); i++) {
		panel->bounceB[i] = WMCreateSwitchButton(panel->optF);
		WMResizeWidget(panel->bounceB[i], 210, 26);
		WMMoveWidget(panel->bounceB[i], 15, 12 + i * 25);
		WMSetButtonText(panel->bounceB[i], _(appicon_bouncing[i].label));

		if (appicon_bouncing[i].default_value)
			WMSetButtonSelected(panel->bounceB[i], True);
	}

	WMMapSubwidgets(panel->optF);

    /***************** Workspace border ****************/
	panel->borderF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->borderF, 240, 82);
	WMMoveWidget(panel->borderF, 15, 145);
	WMSetFrameTitle(panel->borderF, _("Workspace border"));

	panel->borderS = WMCreateSlider(panel->borderF);
	WMResizeWidget(panel->borderS, 80, 15);
	WMMoveWidget(panel->borderS, 20, 20);
	WMSetSliderMinValue(panel->borderS, 0);
	WMSetSliderMaxValue(panel->borderS, 5);
	WMSetSliderAction(panel->borderS, borderCallback, panel);

	panel->borderL = WMCreateLabel(panel->borderF);
	WMResizeWidget(panel->borderL, 100, 15);
	WMMoveWidget(panel->borderL, 105, 20);

	panel->lrB = WMCreateSwitchButton(panel->borderF);
	WMMoveWidget(panel->lrB, 20, 40);
	WMResizeWidget(panel->lrB, 100, 30);
	WMSetButtonText(panel->lrB, _("Left/Right"));

	panel->tbB = WMCreateSwitchButton(panel->borderF);
	WMMoveWidget(panel->tbB, 120, 40);
	WMResizeWidget(panel->tbB, 100, 30);
	WMSetButtonText(panel->tbB, _("Top/Bottom"));

	WMMapSubwidgets(panel->borderF);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	showData(panel);
}

Panel *InitPreferences(WMWidget *parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Miscellaneous Ergonomic Preferences");
	panel->description = _("Various settings like balloon text, geometry\n" "displays etc.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;

	AddSection(panel, ICON_FILE);

	return panel;
}
