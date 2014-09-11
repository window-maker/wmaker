
/* MouseSettings.c- mouse options (some are equivalent to xset)
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

#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <unistd.h>
#include <math.h>


#define XSET	"xset"

typedef struct _Panel {
	WMBox *box;

	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;

	WMFrame *speedF;
	WMLabel *speedL;
	WMSlider *speedS;
	WMLabel *acceL;
	WMTextField *acceT;
	WMLabel *threL;
	WMTextField *threT;

	WMFrame *ddelaF;
	WMButton *ddelaB[5];
	WMTextField *ddelaT;
	WMLabel *ddelaL;
	DoubleTest *tester;

	WMFrame *menuF;
	WMLabel *button1L;
	WMLabel *button2L;
	WMLabel *button3L;
	WMLabel *button8L;
	WMLabel *button9L;
	WMLabel *wheelL;
	WMLabel *wheelTiltL;
	WMPopUpButton *button1P;
	WMPopUpButton *button2P;
	WMPopUpButton *button3P;
	WMPopUpButton *button8P;
	WMPopUpButton *button9P;
	WMPopUpButton *wheelP;
	WMPopUpButton *wheelTiltP;

	WMButton *disaB;

	WMFrame *grabF;
	WMPopUpButton *grabP;

	 /**/ int maxThreshold;
	float acceleration;
} _Panel;

#define ICON_FILE "mousesettings"

#define SPEED_ICON_FILE "mousespeed"

#define DELAY_ICON "timer%i"
#define DELAY_ICON_S "timer%is"

/* need access to the double click variables */
#include <WINGs/WINGsP.h>

static char *modifierNames[8];

static char *buttonActions[8];

static char *wheelActions[3];

#define DELAY(i)		((i)*75+170)


static void setMouseAccel(WMScreen * scr, float accel, int threshold)
{
	int n, d;

	d = 10;
	n = accel * d;

	XChangePointerControl(WMScreenDisplay(scr), True, True, n, d, threshold);
}

static void speedChange(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	int i;
	char buffer[64];
	int threshold;
	char *tmp;

	if (w == NULL) {
		float accel;

		tmp = WMGetTextFieldText(panel->acceT);
		if (sscanf(tmp, "%f", &accel) != 1 || accel < 0) {
			WMRunAlertPanel(WMWidgetScreen(panel->acceT), GetWindow(),
					_("Error"),
					_("Invalid mouse acceleration value. Must be a positive real value."),
					_("OK"), NULL, NULL);
			wfree(tmp);
			return;
		}
		panel->acceleration = accel;
		wfree(tmp);
	} else {
		i = (int)WMGetSliderValue(panel->speedS);

		panel->acceleration = 0.25 + (i * 0.25);

		sprintf(buffer, "%.2f", 0.25 + (i * 0.25));
		WMSetTextFieldText(panel->acceT, buffer);
	}

	tmp = WMGetTextFieldText(panel->threT);
	if (sscanf(tmp, "%i", &threshold) != 1 || threshold < 0 || threshold > panel->maxThreshold) {
		WMRunAlertPanel(WMWidgetScreen(panel->parent), GetWindow(), _("Error"),
				_
				("Invalid mouse acceleration threshold value. Must be the number of pixels to travel before accelerating."),
				_("OK"), NULL, NULL);
	} else {
		setMouseAccel(WMWidgetScreen(panel->parent), panel->acceleration, threshold);
	}
	wfree(tmp);
}

static void returnPressed(void *observerData, WMNotification * notification)
{
	_Panel *panel = (_Panel *) observerData;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) notification;

	speedChange(NULL, panel);
}

static void doubleClick(WMWidget * w, void *data)
{
	_Panel *panel = (_Panel *) data;
	int i;
	char buffer[32];

	for (i = 0; i < 5; i++) {
		if (panel->ddelaB[i] == w)
			break;
	}
	WINGsConfiguration.doubleClickDelay = DELAY(i);

	sprintf(buffer, "%i", DELAY(i));
	WMSetTextFieldText(panel->ddelaT, buffer);
}

static int getButtonAction(const char *str)
{
	if (!str)
		return -2;

	if (strcasecmp(str, "None") == 0)
		return 0;
	else if (strcasecmp(str, "OpenApplicationsMenu") == 0)
		return 1;
	else if (strcasecmp(str, "OpenWindowListMenu") == 0)
		return 2;
	else if (strcasecmp(str, "SelectWindows") == 0)
		return 3;
	else if (strcasecmp(str, "MoveToPrevWorkspace") == 0)
		return 4;
	else if (strcasecmp(str, "MoveToNextWorkspace") == 0)
		return 5;
	else if (strcasecmp(str, "MoveToPrevWindow") == 0)
		return 6;
	else if (strcasecmp(str, "MoveToNextWindow") == 0)
		return 7;
	else
		return -1;

}

static int getWheelAction(const char *str)
{
	if (!str)
		return -2;

	if (strcasecmp(str, "None") == 0)
		return 0;
	else if (strcasecmp(str, "SwitchWorkspaces") == 0)
		return 1;
	else if (strcasecmp(str, "SwitchWindows") == 0)
		return 2;
	else
		return -1;

}

static void getMouseParameters(Display * dpy, float *accel, int *thre)
{
	int n, d;

	XGetPointerControl(dpy, &n, &d, thre);

	*accel = (float)n / (float)d;
}

static void showData(_Panel * panel)
{
	char *str;
	int i;
	int a = -1, b = -1, c = -1, w = -1;
	float accel;
	char buffer[32];
	Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->parent));

	str = GetStringForKey("MouseLeftButtonAction");
	i = getButtonAction(str);
	if (i < 0) {
		a = 3;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseLeftButtonAction");
		}
	} else {
		a = i;
	}
	WMSetPopUpButtonSelectedItem(panel->button1P, a);

	str = GetStringForKey("MouseMiddleButtonAction");
	i = getButtonAction(str);
	if (i < 0) {
		b = 2;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseMiddleButtonAction");
		}
	} else {
		b = i;
	}
	WMSetPopUpButtonSelectedItem(panel->button2P, b);

	str = GetStringForKey("MouseRightButtonAction");
	i = getButtonAction(str);
	if (i < 0) {
		c = 1;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseRightButtonAction");
		}
	} else {
		c = i;
	}
	WMSetPopUpButtonSelectedItem(panel->button3P, c);

	str = GetStringForKey("MouseBackwardButtonAction");
	i = getButtonAction(str);
	if (i < 0) {
		b = 0;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseBackwardButtonAction");
		}
	} else {
		b = i;
	}
	WMSetPopUpButtonSelectedItem(panel->button8P, b);

	str = GetStringForKey("MouseForwardButtonAction");
	i = getButtonAction(str);
	if (i < 0) {
		b = 0;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseForwardButtonAction");
		}
	} else {
		b = i;
	}
	WMSetPopUpButtonSelectedItem(panel->button9P, b);

	str = GetStringForKey("MouseWheelAction");
	i = getWheelAction(str);
	if (i < 0) {
		w = 0;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseWheelAction");
		}
	} else {
		w = i;
	}
	WMSetPopUpButtonSelectedItem(panel->wheelP, w);

	str = GetStringForKey("MouseWheelTiltAction");
	i = getWheelAction(str);
	if (i < 0) {
		w = 0;
		if (i == -1) {
			wwarning(_("bad value %s for option %s"), str, "MouseWheelTiltAction");
		}
	} else {
		w = i;
	}
	WMSetPopUpButtonSelectedItem(panel->wheelTiltP, w);

	WMSetButtonSelected(panel->disaB, GetBoolForKey("DisableWSMouseActions"));

	 /**/ getMouseParameters(dpy, &accel, &a);
	panel->maxThreshold = WidthOfScreen(DefaultScreenOfDisplay(dpy));
	if (a > panel->maxThreshold) {
		panel->maxThreshold = a;
	}
	sprintf(buffer, "%i", a);
	WMSetTextFieldText(panel->threT, buffer);

	WMSetSliderValue(panel->speedS, (accel - 0.25) / 0.25);

	panel->acceleration = accel;
	sprintf(buffer, "%.2f", accel);
	WMSetTextFieldText(panel->acceT, buffer);

	 /**/ b = GetIntegerForKey("DoubleClickTime");
	/* find best match */
	a = -1;
	for (i = 0; i < 5; i++) {
		if (DELAY(i) == b)
			a = i;
	}
	if (a >= 0)
		WMPerformButtonClick(panel->ddelaB[a]);
	sprintf(buffer, "%i", b);
	WMSetTextFieldText(panel->ddelaT, buffer);

	 /**/ str = GetStringForKey("ModifierKey");
	if (!str)
		str = "mod1";
	a = ModifierFromKey(dpy, str);

	if (a != -1) {
		str = modifierNames[a];

		a = 0;
		for (i = 0; i < WMGetPopUpButtonNumberOfItems(panel->grabP); i++) {
			if (strstr(WMGetPopUpButtonItem(panel->grabP, i), str)) {
				WMSetPopUpButtonSelectedItem(panel->grabP, i);
				a = 1;
				break;
			}
		}
	}

	if (a < 1) {
		char *previous;

		previous = WMGetPopUpButtonItem(panel->grabP, 0);
		if (previous != NULL)
			WMSetPopUpButtonSelectedItem(panel->grabP, 0);
		wwarning(_("modifier key %s for option ModifierKey was not recognized. Using %s as default"),
					str, previous?previous:"(empty)");
	}
}

static void fillModifierPopUp(WMPopUpButton * pop)
{
	XModifierKeymap *mapping;
	Display *dpy = WMScreenDisplay(WMWidgetScreen(pop));
	int i, j;
	char *str;
	char buffer[64];

	mapping = XGetModifierMapping(dpy);

	if (!mapping || mapping->max_keypermod == 0) {
		WMAddPopUpButtonItem(pop, "Mod1");
		WMAddPopUpButtonItem(pop, "Mod2");
		WMAddPopUpButtonItem(pop, "Mod3");
		WMAddPopUpButtonItem(pop, "Mod4");
		WMAddPopUpButtonItem(pop, "Mod5");
		wwarning(_("could not retrieve keyboard modifier mapping"));
		return;
	}

	for (j = 0; j < 8; j++) {
		int idx;
		char *array[8];
		int a;
		KeySym ksym;
		int k;
		char *ptr;
		char *tmp;

		a = 0;
		memset(array, 0, sizeof(char *) * 8);
		for (i = 0; i < mapping->max_keypermod; i++) {
			idx = i + j * mapping->max_keypermod;
			if (mapping->modifiermap[idx] != 0) {
				int l;
				for (l = 0; l < 4; l++) {
					if (xext_xkb_supported)
						ksym = XkbKeycodeToKeysym(dpy, mapping->modifiermap[idx], 0, l);
					else
						ksym = XKeycodeToKeysym(dpy, mapping->modifiermap[idx], 0);
					if (ksym != NoSymbol)
						break;
				}
				if (ksym != NoSymbol)
					str = XKeysymToString(ksym);
				else
					str = NULL;
				if (str && !strstr(str, "_Lock") && !strstr(str, "Shift")
				    && !strstr(str, "Control")) {
					array[a++] = wstrdup(str);
				}
			}
		}

		for (k = 0; k < a; k++) {
			if (array[k] == NULL)
				continue;
			tmp = wstrdup(array[k]);
			ptr = strstr(tmp, "_L");
			if (ptr)
				*ptr = 0;
			ptr = strstr(tmp, "_R");
			if (ptr)
				*ptr = 0;
			sprintf(buffer, "%s (%s)", modifierNames[j], tmp);
			/*sprintf(buffer, "%s", tmp); */
			WMAddPopUpButtonItem(pop, buffer);
			for (i = k + 1; i < a; i++) {
				if (array[i] == NULL)
					continue;
				if (strstr(array[i], tmp)) {
					wfree(array[i]);
					array[i] = NULL;
					break;
				}
			}
			wfree(tmp);
		}

		while (--a > 0) {
			if (array[a])
				wfree(array[a]);
		}
	}

	if (mapping)
		XFreeModifiermap(mapping);
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	WMScreen *scr = WMWidgetScreen(panel->parent);
	WMPixmap *icon;
	char *buf1, *buf2;
	int i;
	RColor color;
	char *path;

	color.red = 0xae;
	color.green = 0xaa;
	color.blue = 0xae;

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    /**************** Mouse Speed ****************/
	panel->speedF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->speedF, 225, 90);
	WMMoveWidget(panel->speedF, 15, 5);
	WMSetFrameTitle(panel->speedF, _("Mouse Speed"));

	panel->speedL = WMCreateLabel(panel->speedF);
	WMResizeWidget(panel->speedL, 40, 46);
	WMMoveWidget(panel->speedL, 10, 14);
	WMSetLabelImagePosition(panel->speedL, WIPImageOnly);
	path = LocateImage(SPEED_ICON_FILE);
	if (path) {
		icon = WMCreateBlendedPixmapFromFile(scr, path, &color);
		if (icon) {
			WMSetLabelImage(panel->speedL, icon);
			WMReleasePixmap(icon);
		} else {
			wwarning(_("could not load icon %s"), path);
		}
		wfree(path);
	}

	panel->speedS = WMCreateSlider(panel->speedF);
	WMResizeWidget(panel->speedS, 150, 15);
	WMMoveWidget(panel->speedS, 60, 35);
	WMSetSliderMinValue(panel->speedS, 0);
	WMSetSliderMaxValue(panel->speedS, 40);
	WMSetSliderContinuous(panel->speedS, False);
	WMSetSliderAction(panel->speedS, speedChange, panel);

	panel->acceL = WMCreateLabel(panel->speedF);
	WMResizeWidget(panel->acceL, 50, 16);
	WMMoveWidget(panel->acceL, 10, 65);
	WMSetLabelTextAlignment(panel->acceL, WARight);
	WMSetLabelText(panel->acceL, _("Accel.:"));

	panel->acceT = WMCreateTextField(panel->speedF);
	WMResizeWidget(panel->acceT, 40, 20);
	WMMoveWidget(panel->acceT, 60, 63);
	WMAddNotificationObserver(returnPressed, panel, WMTextDidEndEditingNotification, panel->acceT);

	panel->threL = WMCreateLabel(panel->speedF);
	WMResizeWidget(panel->threL, 80, 16);
	WMMoveWidget(panel->threL, 100, 65);
	WMSetLabelTextAlignment(panel->threL, WARight);
	WMSetLabelText(panel->threL, _("Threshold:"));

	panel->threT = WMCreateTextField(panel->speedF);
	WMResizeWidget(panel->threT, 30, 20);
	WMMoveWidget(panel->threT, 180, 63);
	WMAddNotificationObserver(returnPressed, panel, WMTextDidEndEditingNotification, panel->threT);

	WMMapSubwidgets(panel->speedF);

	/* ************** Grab Modifier **************** */
	panel->grabF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->grabF, 225, 45);
	WMMoveWidget(panel->grabF, 15, 95);
	WMSetFrameTitle(panel->grabF, _("Mouse Grab Modifier"));

	WMSetBalloonTextForView(_("Keyboard modifier to use for actions that\n"
				  "involve dragging windows with the mouse,\n"
				  "clicking inside the window."), WMWidgetView(panel->grabF));

	panel->grabP = WMCreatePopUpButton(panel->grabF);
	WMResizeWidget(panel->grabP, 160, 20);
	WMMoveWidget(panel->grabP, 50, 18);

	fillModifierPopUp(panel->grabP);
	WMMapSubwidgets(panel->grabF);

	/***************** Doubleclick Delay ****************/

	panel->ddelaF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->ddelaF, 225, 87);
	WMMoveWidget(panel->ddelaF, 15, 140);
	WMSetFrameTitle(panel->ddelaF, _("Double-Click Delay"));

	buf1 = wmalloc(strlen(DELAY_ICON) + 2);
	buf2 = wmalloc(strlen(DELAY_ICON_S) + 2);

	for (i = 0; i < 5; i++) {
		panel->ddelaB[i] = WMCreateCustomButton(panel->ddelaF, WBBStateChangeMask);
		WMResizeWidget(panel->ddelaB[i], 25, 25);
		WMMoveWidget(panel->ddelaB[i], 20 + (40 * i), 20);
		WMSetButtonBordered(panel->ddelaB[i], False);
		WMSetButtonImagePosition(panel->ddelaB[i], WIPImageOnly);
		WMSetButtonAction(panel->ddelaB[i], doubleClick, panel);
		if (i > 0) {
			WMGroupButtons(panel->ddelaB[0], panel->ddelaB[i]);
		}
		sprintf(buf1, DELAY_ICON, i + 1);
		sprintf(buf2, DELAY_ICON_S, i + 1);
		path = LocateImage(buf1);
		if (path) {
			icon = WMCreatePixmapFromFile(scr, path);
			if (icon) {
				WMSetButtonImage(panel->ddelaB[i], icon);
				WMReleasePixmap(icon);
			} else {
				wwarning(_("could not load icon file %s"), path);
			}
			wfree(path);
		}
		path = LocateImage(buf2);
		if (path) {
			icon = WMCreatePixmapFromFile(scr, path);
			if (icon) {
				WMSetButtonAltImage(panel->ddelaB[i], icon);
				WMReleasePixmap(icon);
			} else {
				wwarning(_("could not load icon file %s"), path);
			}
			wfree(path);
		}
	}
	wfree(buf1);
	wfree(buf2);

	panel->tester = CreateDoubleTest(panel->ddelaF, _("Test"));
	WMResizeWidget(panel->tester, 84, 29);
	WMMoveWidget(panel->tester, 25, 52);

	panel->ddelaT = WMCreateTextField(panel->ddelaF);
	WMResizeWidget(panel->ddelaT, 40, 20);
	WMMoveWidget(panel->ddelaT, 130, 57);

	panel->ddelaL = WMCreateLabel(panel->ddelaF);
	WMResizeWidget(panel->ddelaL, 40, 16);
	WMMoveWidget(panel->ddelaL, 175, 63);
	{
		WMFont *font;
		WMColor *color;

		font = WMSystemFontOfSize(scr, 10);
		color = WMDarkGrayColor(scr);
		WMSetLabelTextColor(panel->ddelaL, color);
		WMSetLabelFont(panel->ddelaL, font);
		WMReleaseFont(font);
		WMReleaseColor(color);
	}
	WMSetLabelText(panel->ddelaL, _("msec"));

	WMMapSubwidgets(panel->ddelaF);

	/* ************** Workspace Action Buttons **************** */

	panel->menuF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->menuF, 260, 222);
	WMMoveWidget(panel->menuF, 250, 5);
	WMSetFrameTitle(panel->menuF, _("Workspace Mouse Actions"));

	panel->disaB = WMCreateSwitchButton(panel->menuF);
	WMResizeWidget(panel->disaB, 205, 18);
	WMMoveWidget(panel->disaB, 10, 15);
	WMSetButtonText(panel->disaB, _("Disable mouse actions"));

	panel->button1L = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->button1L, 107, 20);
	WMMoveWidget(panel->button1L, 5, 40);
	WMSetLabelTextAlignment(panel->button1L, WARight);
	WMSetLabelText(panel->button1L, _("Left Button"));

	panel->button1P = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->button1P, 135, 20);
	WMMoveWidget(panel->button1P, 115, 40);

	panel->button2L = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->button2L, 107, 20);
	WMMoveWidget(panel->button2L, 5, 65);
	WMSetLabelTextAlignment(panel->button2L, WARight);
	WMSetLabelText(panel->button2L, _("Middle Button"));

	panel->button2P = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->button2P, 135, 20);
	WMMoveWidget(panel->button2P, 115, 65);

	panel->button3L = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->button3L, 107, 20);
	WMMoveWidget(panel->button3L, 5, 90);
	WMSetLabelTextAlignment(panel->button3L, WARight);
	WMSetLabelText(panel->button3L, _("Right Button"));

	panel->button3P = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->button3P, 135, 20);
	WMMoveWidget(panel->button3P, 115, 90);

	panel->button8L = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->button8L, 107, 20);
	WMMoveWidget(panel->button8L, 5, 115);
	WMSetLabelTextAlignment(panel->button8L, WARight);
	WMSetLabelText(panel->button8L, _("Back Button"));

	panel->button8P = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->button8P, 135, 20);
	WMMoveWidget(panel->button8P, 115, 115);

	panel->button9L = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->button9L, 107, 20);
	WMMoveWidget(panel->button9L, 5, 140);
	WMSetLabelTextAlignment(panel->button9L, WARight);
	WMSetLabelText(panel->button9L, _("Forward Button"));

	panel->button9P = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->button9P, 135, 20);
	WMMoveWidget(panel->button9P, 115, 140);

	panel->wheelL = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->wheelL, 107, 20);
	WMMoveWidget(panel->wheelL, 5, 165);
	WMSetLabelTextAlignment(panel->wheelL, WARight);
	WMSetLabelText(panel->wheelL, _("Mouse Wheel"));

	panel->wheelP = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->wheelP, 135, 20);
	WMMoveWidget(panel->wheelP, 115, 165);

	panel->wheelTiltL = WMCreateLabel(panel->menuF);
	WMResizeWidget(panel->wheelTiltL, 107, 20);
	WMMoveWidget(panel->wheelTiltL, 5, 190);
	WMSetLabelTextAlignment(panel->wheelTiltL, WARight);
	WMSetLabelText(panel->wheelTiltL, _("Mouse Wheel Tilt"));

	panel->wheelTiltP = WMCreatePopUpButton(panel->menuF);
	WMResizeWidget(panel->wheelTiltP, 135, 20);
	WMMoveWidget(panel->wheelTiltP, 115, 190);

	for (i = 0; i < wlengthof(buttonActions); i++) {
		WMAddPopUpButtonItem(panel->button1P, buttonActions[i]);
		WMAddPopUpButtonItem(panel->button2P, buttonActions[i]);
		WMAddPopUpButtonItem(panel->button3P, buttonActions[i]);
		WMAddPopUpButtonItem(panel->button8P, buttonActions[i]);
		WMAddPopUpButtonItem(panel->button9P, buttonActions[i]);
	}

	for (i = 0; i < wlengthof(wheelActions); i++) {
		WMAddPopUpButtonItem(panel->wheelP, wheelActions[i]);
		WMAddPopUpButtonItem(panel->wheelTiltP, wheelActions[i]);
	}

	WMMapSubwidgets(panel->menuF);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	showData(panel);
}

static void storeCommandInScript(const char *cmd, const char *line)
{
	char *path;
	FILE *f;
	char buffer[128];

	path = wstrconcat(wusergnusteppath(), "/Library/WindowMaker/autostart");

	f = fopen(path, "rb");
	if (!f) {
		f = fopen(path, "wb");
		if (!f) {
			werror(_("could not create %s"), path);
			goto end;
		}
		fprintf(f, "#!/bin/sh\n");
		fputs(line, f);
		fputs("\n", f);
	} else {
		int len = strlen(cmd);
		int ok = 0;
		char *tmppath;
		FILE *fo;

		tmppath = wstrconcat(wusergnusteppath(), "/Library/WindowMaker/autostart.tmp");
		fo = fopen(tmppath, "wb");
		if (!fo) {
			werror(_("could not create temporary file %s"), tmppath);
			wfree(tmppath);
			goto end;
		}

		while (!feof(f)) {
			if (!fgets(buffer, 127, f)) {
				break;
			}
			if (buffer[0] == '\n') {
				/* don't write empty lines, else the file will grow
				 * indefinitely (one '\n' added at end of file on each save).
				 */
				continue;
			}
			if (strncmp(buffer, cmd, len) == 0) {
				if (!ok) {
					fputs(line, fo);
					fputs("\n", fo);
					ok = 1;
				}
			} else {
				fputs(buffer, fo);
			}
		}
		if (!ok) {
			fputs(line, fo);
			fputs("\n", fo);
		}
		fsync(fileno(fo));
		fclose(fo);

		if (rename(tmppath, path) != 0) {
			werror(_("could not rename file %s to %s"), tmppath, path);
		}
		wfree(tmppath);
	}
	sprintf(buffer, "chmod u+x %s", path);
	if (system(buffer) == -1)
		werror(_("could not execute command \"%s\""), buffer);

 end:
	wfree(path);
	if (f) {
		fsync(fileno(f));	/* this may be rw */
		fclose(f);
	}
}

static void storeData(_Panel * panel)
{
	char buffer[64];
	int i;
	char *tmp, *p;
	static char *button[8] = { "None", "OpenApplicationsMenu", "OpenWindowListMenu", "SelectWindows",
						"MoveToPrevWorkspace", "MoveToNextWorkspace", "MoveToPrevWindow", "MoveToNextWindow" };
	static char *wheel[3] = { "None", "SwitchWorkspaces", "SwitchWindows" };
	WMUserDefaults *udb = WMGetStandardUserDefaults();

	if (!WMGetUDBoolForKey(udb, "NoXSetStuff")) {
		tmp = WMGetTextFieldText(panel->threT);
		if (strlen(tmp) == 0) {
			wfree(tmp);
			tmp = wstrdup("4");
		}

		sprintf(buffer, XSET " m %i/%i %s\n", (int)(panel->acceleration * 10), 10, tmp);
		storeCommandInScript(XSET " m", buffer);

		wfree(tmp);
	}

	tmp = WMGetTextFieldText(panel->ddelaT);
	if (sscanf(tmp, "%i", &i) == 1 && i > 0)
		SetIntegerForKey(i, "DoubleClickTime");
	wfree(tmp);

	SetBoolForKey(WMGetButtonSelected(panel->disaB), "DisableWSMouseActions");

	i = WMGetPopUpButtonSelectedItem(panel->button1P);
	SetStringForKey(button[i], "MouseLeftButtonAction");

	i = WMGetPopUpButtonSelectedItem(panel->button2P);
	SetStringForKey(button[i], "MouseMiddleButtonAction");

	i = WMGetPopUpButtonSelectedItem(panel->button3P);
	SetStringForKey(button[i], "MouseRightButtonAction");

	i = WMGetPopUpButtonSelectedItem(panel->button8P);
	SetStringForKey(button[i], "MouseBackwardButtonAction");

	i = WMGetPopUpButtonSelectedItem(panel->button9P);
	SetStringForKey(button[i], "MouseForwardButtonAction");

	i = WMGetPopUpButtonSelectedItem(panel->wheelP);
	SetStringForKey(wheel[i], "MouseWheelAction");

	i = WMGetPopUpButtonSelectedItem(panel->wheelTiltP);
	SetStringForKey(wheel[i], "MouseWheelTiltAction");

	tmp = WMGetPopUpButtonItem(panel->grabP, WMGetPopUpButtonSelectedItem(panel->grabP));
	tmp = wstrdup(tmp);
	p = strchr(tmp, ' ');
	if (p != NULL)
		*p = '\0';

	SetStringForKey(tmp, "ModifierKey");

	wfree(tmp);
}

Panel *InitMouseSettings(WMWidget *parent)
{
	_Panel *panel;

	modifierNames[0] = wstrdup(_("Shift"));
	modifierNames[1] = wstrdup(_("Lock"));
	modifierNames[2] = wstrdup(_("Control"));
	modifierNames[3] = wstrdup(_("Mod1"));
	modifierNames[4] = wstrdup(_("Mod2"));
	modifierNames[5] = wstrdup(_("Mod3"));
	modifierNames[6] = wstrdup(_("Mod4"));
	modifierNames[7] = wstrdup(_("Mod5"));

	buttonActions[0] = wstrdup(_("None"));
	buttonActions[1] = wstrdup(_("Applications Menu"));
	buttonActions[2] = wstrdup(_("Window List Menu"));
	buttonActions[3] = wstrdup(_("Select Windows"));
	buttonActions[4] = wstrdup(_("Previous Workspace"));
	buttonActions[5] = wstrdup(_("Next Workspace"));
	buttonActions[6] = wstrdup(_("Previous Window"));
	buttonActions[7] = wstrdup(_("Next Window"));

	wheelActions[0] = wstrdup(_("None"));
	wheelActions[1] = wstrdup(_("Switch Workspaces"));
	wheelActions[2] = wstrdup(_("Switch Windows"));

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Mouse Preferences");

	panel->description = _("Mouse speed/acceleration, double click delay,\n" "mouse button bindings etc.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;

	AddSection(panel, ICON_FILE);

	return panel;
}
