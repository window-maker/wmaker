/* Focus.c- input and colormap focus stuff
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */


#include "WPrefs.h"

typedef struct _Panel {
    WMBox *box;

    char *sectionName;

    char *description;

    CallbackRec callbacks;

    WMWidget *parent;

    WMFrame *kfocF;
    WMButton *kfocB[2];

    WMFrame *cfocF;
    WMButton *autB;
    WMButton *manB;

    WMFrame *raisF;
    WMButton *raisB[5];
    WMTextField *raisT;
    WMLabel *raisL;

    WMFrame *optF;
    WMButton *ignB;
    WMButton *newB;

    char raiseDelaySelected;
} _Panel;



#define ICON_FILE	"windowfocus"

#define DELAY_ICON "timer%i"
#define DELAY_ICON_S "timer%is"


static void
showData(_Panel *panel)
{
    char *str;
    int i;
    char buffer[32];

    str = GetStringForKey("FocusMode");
    if (!str)
        str = "manual";
    if (strcasecmp(str, "manual")==0 || strcasecmp(str, "clicktofocus")==0)
        WMSetButtonSelected(panel->kfocB[0], 1);
    else if (strcasecmp(str, "auto")==0 || strcasecmp(str, "semiauto")==0
             || strcasecmp(str, "sloppy")==0)
        WMSetButtonSelected(panel->kfocB[1], 1);
    else {
        wwarning(_("bad option value %s for option FocusMode. Using default Manual"),
                 str);
        WMSetButtonSelected(panel->kfocB[0], 1);
    }

    /**/
    str = GetStringForKey("ColormapMode");
    if (!str)
        str = "auto";
    if (strcasecmp(str, "manual")==0 || strcasecmp(str, "clicktofocus")==0) {
        WMPerformButtonClick(panel->manB);
    } else if (strcasecmp(str, "auto")==0 || strcasecmp(str, "focusfollowsmouse")==0) {
        WMPerformButtonClick(panel->autB);
    } else {
        wwarning(_("bad option value %s for option ColormapMode. Using default Auto"),
                 str);
        WMPerformButtonClick(panel->manB);
    }

    /**/
    i = GetIntegerForKey("RaiseDelay");
    sprintf(buffer, "%i", i);
    WMSetTextFieldText(panel->raisT, buffer);

    switch (i) {
    case 0:
        WMPerformButtonClick(panel->raisB[0]);
        break;
    case 10:
        WMPerformButtonClick(panel->raisB[1]);
        break;
    case 100:
        WMPerformButtonClick(panel->raisB[2]);
        break;
    case 350:
        WMPerformButtonClick(panel->raisB[3]);
        break;
    case 800:
        WMPerformButtonClick(panel->raisB[4]);
        break;
    }

    /**/
    WMSetButtonSelected(panel->ignB, GetBoolForKey("IgnoreFocusClick"));

    WMSetButtonSelected(panel->newB, GetBoolForKey("AutoFocus"));
}



static void
storeData(_Panel *panel)
{
    char *str;
    int i;

    if (WMGetButtonSelected(panel->kfocB[1]))
        str = "sloppy";
    else
        str = "manual";

    SetStringForKey(str, "FocusMode");

    if (WMGetButtonSelected(panel->manB)) {
        SetStringForKey("manual", "ColormapMode");
    } else {
        SetStringForKey("auto", "ColormapMode");
    }

    str = WMGetTextFieldText(panel->raisT);
    if (sscanf(str, "%i", &i)!=1)
        i = 0;
    SetIntegerForKey(i, "RaiseDelay");

    SetBoolForKey(WMGetButtonSelected(panel->ignB), "IgnoreFocusClick");
    SetBoolForKey(WMGetButtonSelected(panel->newB), "AutoFocus");
}


static void
pushDelayButton(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;

    panel->raiseDelaySelected = 1;
    if (w == panel->raisB[0]) {
        WMSetTextFieldText(panel->raisT, "OFF");
    } else if (w == panel->raisB[1]) {
        WMSetTextFieldText(panel->raisT, "10");
    } else if (w == panel->raisB[2]) {
        WMSetTextFieldText(panel->raisT, "100");
    } else if (w == panel->raisB[3]) {
        WMSetTextFieldText(panel->raisT, "350");
    } else if (w == panel->raisB[4]) {
        WMSetTextFieldText(panel->raisT, "800");
    }
}


static void
raiseTextChanged(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;
    int i;

    if (panel->raiseDelaySelected) {
        for (i=0; i<5; i++) {
            WMSetButtonSelected(panel->raisB[i], False);
        }
        panel->raiseDelaySelected = 0;
    }
}




static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->parent);
    int i;
    char *buf1, *buf2;
    WMPixmap *icon;
    WMColor *color;
    WMFont *font;

    panel->box = WMCreateBox(panel->parent);
    WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    /***************** Input Focus Mode *****************/
    panel->kfocF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->kfocF, 240, 130);
    WMMoveWidget(panel->kfocF, 15, 15);
    WMSetFrameTitle(panel->kfocF, _("Input Focus Mode"));

    {
        WMBox *box = WMCreateBox(panel->kfocF);
        WMSetViewExpandsToParent(WMWidgetView(box), 10, 15, 10, 10);
        WMSetBoxHorizontal(box, False);

        panel->kfocB[0] = WMCreateRadioButton(box);
        WMSetButtonText(panel->kfocB[0], _("Manual:  Click on the window to set "\
                                           "keyboard input focus"));
        WMAddBoxSubview(box, WMWidgetView(panel->kfocB[0]), True, True,
                        20, 0, 0);

        panel->kfocB[1] = WMCreateRadioButton(box);
        WMGroupButtons(panel->kfocB[0], panel->kfocB[1]);
        WMSetButtonText(panel->kfocB[1], _("Auto:  Set keyboard input focus to "\
                                           "the window under the mouse pointer"));
        WMAddBoxSubview(box, WMWidgetView(panel->kfocB[1]), True, True,
                        20, 0, 0);

        WMMapSubwidgets(box);
        WMMapWidget(box);
    }

    /***************** Colormap Installation Mode ****************/

    panel->cfocF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->cfocF, 240, 70);
    WMMoveWidget(panel->cfocF, 15, 150);
    WMSetFrameTitle(panel->cfocF, _("Install colormap in the window..."));

    panel->manB = WMCreateRadioButton(panel->cfocF);
    WMResizeWidget(panel->manB, 225, 20);
    WMMoveWidget(panel->manB, 10, 18);
    WMSetButtonText(panel->manB, _("...that has the input focus."));

    panel->autB = WMCreateRadioButton(panel->cfocF);
    WMResizeWidget(panel->autB, 225, 20);
    WMMoveWidget(panel->autB, 10, 43);
    WMSetButtonText(panel->autB, _("...that is under the mouse pointer."));
    WMGroupButtons(panel->manB, panel->autB);

    WMMapSubwidgets(panel->cfocF);

    /***************** Automatic window raise delay *****************/
    panel->raisF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->raisF, 245, 70);
    WMMoveWidget(panel->raisF, 265, 15);
    WMSetFrameTitle(panel->raisF, _("Automatic Window Raise Delay"));

    buf1 = wmalloc(strlen(DELAY_ICON)+1);
    buf2 = wmalloc(strlen(DELAY_ICON_S)+1);

    for (i = 0; i < 5; i++) {
        char *path;

        panel->raisB[i] = WMCreateCustomButton(panel->raisF,
                                               WBBStateChangeMask);
        WMResizeWidget(panel->raisB[i], 25, 25);
        WMMoveWidget(panel->raisB[i], 10+(30*i), 25);
        WMSetButtonBordered(panel->raisB[i], False);
        WMSetButtonImagePosition(panel->raisB[i], WIPImageOnly);
        WMSetButtonAction(panel->raisB[i], pushDelayButton, panel);
        if (i>0)
            WMGroupButtons(panel->raisB[0], panel->raisB[i]);
        sprintf(buf1, DELAY_ICON, i);
        sprintf(buf2, DELAY_ICON_S, i);
        path = LocateImage(buf1);
        if (path) {
            icon = WMCreatePixmapFromFile(scr, path);
            if (icon) {
                WMSetButtonImage(panel->raisB[i], icon);
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
                WMSetButtonAltImage(panel->raisB[i], icon);
                WMReleasePixmap(icon);
            } else {
                wwarning(_("could not load icon file %s"), path);
            }
            wfree(path);
        }
    }
    wfree(buf1);
    wfree(buf2);

    panel->raisT = WMCreateTextField(panel->raisF);
    WMResizeWidget(panel->raisT, 36, 20);
    WMMoveWidget(panel->raisT, 165, 30);
    WMAddNotificationObserver(raiseTextChanged, panel,
                              WMTextDidChangeNotification, panel->raisT);

    color = WMDarkGrayColor(scr);
    font = WMSystemFontOfSize(scr, 10);

    panel->raisL = WMCreateLabel(panel->raisF);
    WMResizeWidget(panel->raisL, 36, 16);
    WMMoveWidget(panel->raisL, 205, 35);
    WMSetLabelText(panel->raisL, _("msec"));
    WMSetLabelTextColor(panel->raisL, color);
    WMSetLabelFont(panel->raisL, font);

    WMReleaseColor(color);
    WMReleaseFont(font);

    WMMapSubwidgets(panel->raisF);

    /***************** Options ****************/
    panel->optF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->optF, 245, 125);
    WMMoveWidget(panel->optF, 265, 95);

    panel->ignB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->ignB, 225, 50);
    WMMoveWidget(panel->ignB, 10, 10);
    WMSetButtonText(panel->ignB, _("Do not let applications receive "
                                   "the click used to focus windows."));

    panel->newB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->newB, 225, 35);
    WMMoveWidget(panel->newB, 10, 70);
    WMSetButtonText(panel->newB, _("Automatically focus new windows."));

    WMMapSubwidgets(panel->optF);


    WMRealizeWidget(panel->box);
    WMMapSubwidgets(panel->box);

    showData(panel);
}



Panel*
InitFocus(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Window Focus Preferences");

    panel->description = _("Keyboard focus switching policy, colormap switching\n"
                           "policy for 8bpp displays and other related options.");

    panel->parent = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}

