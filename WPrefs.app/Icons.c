/* Icons.c- icon preferences
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
	const char *db_value;
	const char *label;
} icon_animation[] = {
	{ "zoom",  N_("Shrinking/Zooming") },
	{ "twist", N_("Spinning/Twisting") },
	{ "flip",  N_("3D-flipping") },
	{ "none",  N_("None") }
};

typedef struct _Panel {
	WMBox *box;

	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;

	WMFrame *posF;
	WMFrame *posVF;
	WMFrame *posV;

	WMButton *posB[8];

	WMFrame *animF;
	WMButton *animB[wlengthof_nocheck(icon_animation)];

	WMFrame *optF;
	WMButton *arrB;
	WMButton *omnB;
	WMButton *sclB;

	WMFrame *sizeF;
	WMPopUpButton *sizeP;

	int iconPos;
} _Panel;

#define ICON_FILE	"iconprefs"

static void showIconLayout(WMWidget * widget, void *data)
{
	_Panel *panel = (_Panel *) data;
	int w, h;
	int i;

	for (i = 0; i < 8; i++) {
		if (panel->posB[i] == widget) {
			panel->iconPos = i;
			break;
		}
	}

	if (panel->iconPos & 1) {
		w = 32;
		h = 8;
	} else {
		w = 8;
		h = 32;
	}
	WMResizeWidget(panel->posV, w, h);

	switch (panel->iconPos & ~1) {
	case 0:
		WMMoveWidget(panel->posV, 2, 2);
		break;
	case 2:
		WMMoveWidget(panel->posV, 95 - 2 - w, 2);
		break;
	case 4:
		WMMoveWidget(panel->posV, 2, 70 - 2 - h);
		break;
	default:
		WMMoveWidget(panel->posV, 95 - 2 - w, 70 - 2 - h);
		break;
	}
}

static void showData(_Panel * panel)
{
	int i;
	char *str;
	char *def = "blh";

	WMSetButtonSelected(panel->arrB, GetBoolForKey("AutoArrangeIcons"));
	WMSetButtonSelected(panel->omnB, GetBoolForKey("StickyIcons"));
	WMSetButtonSelected(panel->sclB, GetBoolForKey("SingleClickLaunch"));

	str = GetStringForKey("IconPosition");
	if (!str)
		str = def;
	if (strlen(str) != 3) {
		wwarning("bad value %s for option IconPosition. Using default blh", str);
		str = def;
	}

	if (str[0] == 't' || str[0] == 'T') {
		i = 0;
	} else {
		i = 4;
	}
	if (str[1] == 'r' || str[1] == 'R') {
		i += 2;
	}
	if (str[2] == 'v' || str[2] == 'V') {
		i += 0;
	} else {
		i += 1;
	}
	panel->iconPos = i;
	WMPerformButtonClick(panel->posB[i]);

	i = GetIntegerForKey("IconSize");
	i = (i - 24) / 8;

	if (i < 0)
		i = 0;
	else if (i > 9)
		i = 9;
	WMSetPopUpButtonSelectedItem(panel->sizeP, i);

	str = GetStringForKey("IconificationStyle");
	if (str != NULL) {
		for (i = 0; i < wlengthof(icon_animation); i++) {
			if (strcasecmp(str, icon_animation[i].db_value) == 0) {
				WMPerformButtonClick(panel->animB[i]);
				goto found_animation_value;
			}
		}
		wwarning(_("animation style \"%s\" is unknow, resetting to \"%s\""),
		         str, icon_animation[0].db_value);
	}
	/* If we're here, no valid value have been found so we fall-back to the default */
	WMPerformButtonClick(panel->animB[0]);
 found_animation_value:
	;
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	WMColor *color;
	int i;
	char buf[16];

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    /***************** Positioning of Icons *****************/
	panel->posF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->posF, 210, 140);
	WMMoveWidget(panel->posF, 20, 10);
	WMSetFrameTitle(panel->posF, _("Icon Positioning"));

	for (i = 0; i < 8; i++) {
		panel->posB[i] = WMCreateButton(panel->posF, WBTOnOff);
		WMSetButtonAction(panel->posB[i], showIconLayout, panel);

		if (i > 0)
			WMGroupButtons(panel->posB[0], panel->posB[i]);
	}
	WMMoveWidget(panel->posB[1], 58, 25);
	WMResizeWidget(panel->posB[1], 47, 15);
	WMMoveWidget(panel->posB[3], 58 + 47, 25);
	WMResizeWidget(panel->posB[3], 47, 15);

	WMMoveWidget(panel->posB[0], 43, 40);
	WMResizeWidget(panel->posB[0], 15, 35);
	WMMoveWidget(panel->posB[4], 43, 40 + 35);
	WMResizeWidget(panel->posB[4], 15, 35);

	WMMoveWidget(panel->posB[5], 58, 40 + 70);
	WMResizeWidget(panel->posB[5], 47, 15);
	WMMoveWidget(panel->posB[7], 58 + 47, 40 + 70);
	WMResizeWidget(panel->posB[7], 47, 15);

	WMMoveWidget(panel->posB[2], 58 + 95, 40);
	WMResizeWidget(panel->posB[2], 15, 35);
	WMMoveWidget(panel->posB[6], 58 + 95, 40 + 35);
	WMResizeWidget(panel->posB[6], 15, 35);

	color = WMCreateRGBColor(WMWidgetScreen(panel->parent), 0x5100, 0x5100, 0x7100, True);
	panel->posVF = WMCreateFrame(panel->posF);
	WMResizeWidget(panel->posVF, 95, 70);
	WMMoveWidget(panel->posVF, 58, 40);
	WMSetFrameRelief(panel->posVF, WRSunken);
	WMSetWidgetBackgroundColor(panel->posVF, color);
	WMReleaseColor(color);

	panel->posV = WMCreateFrame(panel->posVF);
	WMSetFrameRelief(panel->posV, WRSimple);

	WMMapSubwidgets(panel->posF);

    /***************** Icon Size ****************/
	panel->sizeF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->sizeF, 210, 70);
	WMMoveWidget(panel->sizeF, 20, 155);
	WMSetFrameTitle(panel->sizeF, _("Icon Size"));

	WMSetBalloonTextForView(_("The size of the dock/application icon and miniwindows"),
				WMWidgetView(panel->sizeF));

	panel->sizeP = WMCreatePopUpButton(panel->sizeF);
	WMResizeWidget(panel->sizeP, 161, 20);
	WMMoveWidget(panel->sizeP, 25, 30);
	for (i = 24; i <= 96; i += 8) {
		sprintf(buf, "%ix%i", i, i);
		WMAddPopUpButtonItem(panel->sizeP, buf);
	}

	WMMapSubwidgets(panel->sizeF);

    /***************** Animation ****************/
	panel->animF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->animF, 260, 110);
	WMMoveWidget(panel->animF, 240, 10);
	WMSetFrameTitle(panel->animF, _("Iconification Animation"));

	for (i = 0; i < wlengthof(icon_animation); i++) {
		panel->animB[i] = WMCreateRadioButton(panel->animF);
		WMResizeWidget(panel->animB[i], 145, 20);
		WMMoveWidget(panel->animB[i], 15, 18 + i * 22);

		if (i > 0)
			WMGroupButtons(panel->animB[0], panel->animB[i]);

		WMSetButtonText(panel->animB[i], _(icon_animation[i].label));
	}

	WMMapSubwidgets(panel->animF);

    /***************** Options ****************/
	panel->optF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->optF, 260, 95);
	WMMoveWidget(panel->optF, 240, 130);
	/*    WMSetFrameTitle(panel->optF, _("Icon Display")); */

	panel->arrB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->arrB, 235, 20);
	WMMoveWidget(panel->arrB, 15, 10);
	WMSetButtonText(panel->arrB, _("Auto-arrange icons"));

	WMSetBalloonTextForView(_("Keep icons and miniwindows arranged all the time."), WMWidgetView(panel->arrB));

	panel->omnB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->omnB, 235, 20);
	WMMoveWidget(panel->omnB, 15, 37);
	WMSetButtonText(panel->omnB, _("Omnipresent miniwindows"));

	WMSetBalloonTextForView(_("Make miniwindows be present in all workspaces."), WMWidgetView(panel->omnB));

	panel->sclB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->sclB, 235, 28);
	WMMoveWidget(panel->sclB, 15, 60);
	WMSetButtonText(panel->sclB, _("Single click activation"));

	WMSetBalloonTextForView(_("Launch applications and restore windows with a single click."), WMWidgetView(panel->sclB));

	WMMapSubwidgets(panel->optF);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	showData(panel);
}

static void storeData(_Panel * panel)
{
	char buf[8];
	int i;

	SetBoolForKey(WMGetButtonSelected(panel->arrB), "AutoArrangeIcons");
	SetBoolForKey(WMGetButtonSelected(panel->omnB), "StickyIcons");
	SetBoolForKey(WMGetButtonSelected(panel->sclB), "SingleClickLaunch");

	SetIntegerForKey(WMGetPopUpButtonSelectedItem(panel->sizeP) * 8 + 24, "IconSize");

	buf[3] = 0;

	if (panel->iconPos < 4) {
		buf[0] = 't';
	} else {
		buf[0] = 'b';
	}
	if (panel->iconPos & 2) {
		buf[1] = 'r';
	} else {
		buf[1] = 'l';
	}
	if (panel->iconPos & 1) {
		buf[2] = 'h';
	} else {
		buf[2] = 'v';
	}
	SetStringForKey(buf, "IconPosition");

	for (i = 0; i < wlengthof(icon_animation); i++) {
		if (WMGetButtonSelected(panel->animB[i])) {
			SetStringForKey(icon_animation[i].db_value, "IconificationStyle");
			break;
		}
	}
}

Panel *InitIcons(WMWidget *parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Icon Preferences");

	panel->description = _("Icon/Miniwindow handling options. Icon positioning\n"
			       "area, sizes of icons, miniaturization animation style.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;

	AddSection(panel, ICON_FILE);

	return panel;
}
