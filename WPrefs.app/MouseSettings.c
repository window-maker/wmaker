
/* MouseSettings.c- mouse options (some are equivalent to xset)
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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

#include <X11/Xutil.h>

#include <math.h>

/* double-click tester */
#include "double.h"



#define XSET	"xset"



typedef struct _Panel {
    WMFrame *frame;

    char *sectionName;

    char *description;

    CallbackRec callbacks;
    
    WMWindow *win;
    
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
    WMLabel *listL;
    WMLabel *appL;
    WMLabel *selL;
    WMPopUpButton *listP;
    WMPopUpButton *appP;
    WMPopUpButton *selP;

    WMButton *disaB;

    WMFrame *grabF;
    WMPopUpButton *grabP;

    /**/
    int maxThreshold;
    float acceleration;
} _Panel;


#define ICON_FILE "mousesettings"

#define SPEED_ICON_FILE "mousespeed"

#define DELAY_ICON "timer%i"
#define DELAY_ICON_S "timer%is"


/* need access to the double click variables */
#include "WINGsP.h"



static char *modifierNames[] = {
    "Shift",
	"Lock",
	"Control",
	"Mod1",
	"Mod2",
	"Mod3",
	"Mod4",
	"Mod5"
};


static char *buttonNames[] = {
    "None",
	"Btn1 (left)",
	"Btn2 (middle)",
	"Btn3 (right)",
	"Btn4",
	"Btn5"
};


#define DELAY(i)		((i)*75+170)


int ModifierFromKey(Display *dpy, char *key);


static void
setMouseAccel(WMScreen *scr, float accel, int threshold)
{
    int n, d;
    
    d = 10;
    n = accel*d;

    XChangePointerControl(WMScreenDisplay(scr), True, True, n, d, threshold);
}


static void
speedChange(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i;
    char buffer[64];
    int threshold;
    char *tmp;

    if (w == NULL) {
	float accel;

	tmp = WMGetTextFieldText(panel->acceT);
	if (sscanf(tmp, "%f", &accel)!=1 || accel < 0) {
	    WMRunAlertPanel(WMWidgetScreen(panel->acceT), GetWindow(panel),
			    _("Error"),
			    _("Invalid mouse acceleration value. Must be a positive real value."),
			    _("OK"), NULL, NULL);
	    free(tmp);
	    return;
	}
	panel->acceleration = accel;
	free(tmp);
    } else {
	i = (int)WMGetSliderValue(panel->speedS);
    
	panel->acceleration = 0.25+(i*0.25);

	sprintf(buffer, "%.2f", 0.25+(i*0.25));
	WMSetTextFieldText(panel->acceT, buffer);
    }

    tmp = WMGetTextFieldText(panel->threT);
    if (sscanf(tmp, "%i", &threshold)!=1 || threshold < 0 
	|| threshold > panel->maxThreshold) {
	WMRunAlertPanel(WMWidgetScreen(panel->win), GetWindow(panel), _("Error"),
			_("Invalid mouse acceleration threshold value. Must be the number of pixels to travel before accelerating."),
			_("OK"), NULL, NULL);
    } else {
	setMouseAccel(WMWidgetScreen(panel->win), panel->acceleration, 
		      threshold);
    }
    free(tmp);
}


static void
returnPressed(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;

    speedChange(NULL, panel);
}


static void
doubleClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i;
    extern _WINGsConfiguration WINGsConfiguration;
    char buffer[32];

    for (i=0; i<5; i++) {
	if (panel->ddelaB[i]==w)
	    break;
    }
    WINGsConfiguration.doubleClickDelay = DELAY(i);
    
    sprintf(buffer, "%i", DELAY(i));
    WMSetTextFieldText(panel->ddelaT, buffer);
}



int
getbutton(char *str)
{
    if (!str)
	return -2;

    if (strcasecmp(str, "left")==0)
	return 1;
    else if (strcasecmp(str, "middle")==0)
	return 2;
    else if (strcasecmp(str, "right")==0)
	return 3;
    else if (strcasecmp(str, "button1")==0)
	return 1;
    else if (strcasecmp(str, "button2")==0)
	return 2;
    else if (strcasecmp(str, "button3")==0)
	return 3;
    else if (strcasecmp(str, "button4")==0)
	return 4;
    else if (strcasecmp(str, "button5")==0) {
	return 5;
    } else {
	return -1;
    }
}


static void
getMouseParameters(Display *dpy, float *accel, int *thre)
{
    int n, d;
    
    XGetPointerControl(dpy, &n, &d, thre);
    
    *accel = (float)n/(float)d;
}



static void
showData(_Panel *panel)
{
    char *str;
    int i;
    int a=-1, b=-1, c=-1;
    float accel;
    char buffer[32];
    Display *dpy = WMScreenDisplay(WMWidgetScreen(panel->win));

    str = GetStringForKey("SelectWindowsMouseButton");
    if (str) {
	i = getbutton(str);
	if (i==-1) {
	    a = 1;
	    wwarning(_("bad value %s for option %s"),str, "SelectWindowsMouseButton");
	} else if (i>=0) {
	    a = i;
	} 
    } else {
	a = 0;
    }
    WMSetPopUpButtonSelectedItem(panel->selP, a);
    
    str = GetStringForKey("WindowListMouseButton");
    if (str) {
	i = getbutton(str);
	if (i==-1) {
	    b = 2;
	    wwarning(_("bad value %s for option %s"), str, "WindowListMouseButton");
	} else if (i>=0) {
	    b = i;
	} 
    } else {
	b = 0;
    }
    WMSetPopUpButtonSelectedItem(panel->listP, b);
    
    str = GetStringForKey("ApplicationMenuMouseButton");
    if (str) {
	i = getbutton(str);
	if (i==-1) {
	    c = 3;
	    wwarning(_("bad value %s for option %s"), str, "ApplicationMenuMouseButton");
	} else if (i>=0) {
	    c = i;
	}
    } else {
	c = 0;
    }
    WMSetPopUpButtonSelectedItem(panel->appP, c);

    WMSetButtonSelected(panel->disaB, GetBoolForKey("DisableWSMouseActions"));
    
    /**/
    getMouseParameters(dpy, &accel, &a);
    panel->maxThreshold = WidthOfScreen(DefaultScreenOfDisplay(dpy));
    if (a > panel->maxThreshold) {
	panel->maxThreshold = a;
    }
    sprintf(buffer, "%i", a);
    WMSetTextFieldText(panel->threT, buffer);

    WMSetSliderValue(panel->speedS, (accel - 0.25)/0.25);

    panel->acceleration = accel;
    sprintf(buffer, "%.2f", accel);
    WMSetTextFieldText(panel->acceT, buffer);

    /**/
    b = GetIntegerForKey("DoubleClickTime");
    /* find best match */
    a = -1;
    for (i=0; i<5; i++) {
	if (DELAY(i) == b)
	    a = i;
    }
    if (a >= 0)
	WMPerformButtonClick(panel->ddelaB[a]);
    sprintf(buffer, "%i", b);
    WMSetTextFieldText(panel->ddelaT, buffer);

    /**/
    str = GetStringForKey("ModifierKey");
    if (!str)
	str = "mod1";
    a = ModifierFromKey(dpy, str);

    if (a != -1) {
	str = modifierNames[a];
    
	a = 0;
	for (i=0; i<WMGetPopUpButtonNumberOfItems(panel->grabP); i++) {
	    if (strstr(WMGetPopUpButtonItem(panel->grabP, i), str)) {
		WMSetPopUpButtonSelectedItem(panel->grabP, i);
		a = 1;
		break;
	    }
	}
    }

    if (a < 1) {
	sscanf(WMGetPopUpButtonItem(panel->grabP, 0), "%s", buffer);
	WMSetPopUpButtonSelectedItem(panel->grabP, 0);
	wwarning(_("modifier key %s for option ModifierKey was not recognized. Using %s as default"),
		 str, buffer);
    }
}



static void
fillModifierPopUp(WMPopUpButton *pop)
{
    XModifierKeymap *mapping;
    Display *dpy = WMScreenDisplay(WMWidgetScreen(pop));
    int i, j;
    char *str;
    char buffer[64];

    
    mapping = XGetModifierMapping(dpy);

    if (!mapping || mapping->max_keypermod==0) {
	WMAddPopUpButtonItem(pop, "Mod1");
	WMAddPopUpButtonItem(pop, "Mod2");
	WMAddPopUpButtonItem(pop, "Mod3");
	WMAddPopUpButtonItem(pop, "Mod4");
	WMAddPopUpButtonItem(pop, "Mod5");
	wwarning(_("could not retrieve keyboard modifier mapping"));
	return;
    }
    
    
    for (j=0; j<8; j++) {
	int idx;
	char *array[8];
	int a;
	KeySym ksym;
	int k;
	char *ptr;
	char *tmp;

	a = 0;
	memset(array, 0, sizeof(char*)*8);
	for (i=0; i < mapping->max_keypermod; i++) {
	    idx = i+j*mapping->max_keypermod;
	    if (mapping->modifiermap[idx]!=0) {
		int l;
		for (l=0; l<4; l++) {
		    ksym = XKeycodeToKeysym(dpy, mapping->modifiermap[idx], l);
		    if (ksym!=NoSymbol)
			break;
		}
		if (ksym!=NoSymbol)
		    str = XKeysymToString(ksym);
		else
		    str = NULL;
		if (str && !strstr(str, "_Lock") && !strstr(str, "Shift") 
		    && !strstr(str, "Control")) {
		    array[a++] = wstrdup(str);
		}
	    }
	}

	for (k=0; k<a; k++) {
	    if (array[k]==NULL)
		continue;
	    tmp = wstrdup(array[k]);
	    ptr = strstr(tmp, "_L");
	    if (ptr)
		*ptr = 0;
	    ptr = strstr(tmp, "_R");
	    if (ptr)
		*ptr = 0;
	    sprintf(buffer, "%s (%s)", modifierNames[j], tmp);
	    WMAddPopUpButtonItem(pop, buffer);
	    for (i=k+1; i<a; i++) {
		if (array[i] == NULL)
		    continue;
		if (strstr(array[i], tmp)) {
		    free(array[i]);
		    array[i]=NULL;
		    break;
		}
	    }
	    free(tmp);
	}
	
	while (--a>0) {
	    if (array[a])
		free(array[a]);
	}
    }
    
    if (mapping)
	XFreeModifiermap(mapping);
}



static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMPixmap *icon;
    char *buf1, *buf2;
    int i;
    RColor color;
    char *path;

    color.red = 0xaa;
    color.green = 0xae;
    color.blue = 0xaa;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    /**************** Mouse Speed ****************/
    panel->speedF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->speedF, 245, 100);
    WMMoveWidget(panel->speedF, 15, 15);
    WMSetFrameTitle(panel->speedF, _("Mouse Speed"));
    
    panel->speedL = WMCreateLabel(panel->speedF);
    WMResizeWidget(panel->speedL, 40, 46);
    WMMoveWidget(panel->speedL, 15, 14);
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
	free(path);
    }

    panel->speedS = WMCreateSlider(panel->speedF);
    WMResizeWidget(panel->speedS, 160, 15);
    WMMoveWidget(panel->speedS, 70, 35);
    WMSetSliderMinValue(panel->speedS, 0);
    WMSetSliderMaxValue(panel->speedS, 40);
    WMSetSliderContinuous(panel->speedS, False);
    WMSetSliderAction(panel->speedS, speedChange, panel);

    panel->acceL = WMCreateLabel(panel->speedF);
    WMResizeWidget(panel->acceL, 70, 16);
    WMMoveWidget(panel->acceL, 10, 67);
    WMSetLabelTextAlignment(panel->acceL, WARight);
    WMSetLabelText(panel->acceL, _("Acceler.:"));

    panel->acceT = WMCreateTextField(panel->speedF);
    WMResizeWidget(panel->acceT, 40, 20);
    WMMoveWidget(panel->acceT, 80, 65);
    WMAddNotificationObserver(returnPressed, panel,
			      WMTextDidEndEditingNotification, panel->acceT);


    panel->threL = WMCreateLabel(panel->speedF);
    WMResizeWidget(panel->threL, 80, 16);
    WMMoveWidget(panel->threL, 120, 67);
    WMSetLabelTextAlignment(panel->threL, WARight);
    WMSetLabelText(panel->threL, _("Threshold:"));
    
    panel->threT = WMCreateTextField(panel->speedF);
    WMResizeWidget(panel->threT, 30, 20);
    WMMoveWidget(panel->threT, 200, 65);
    WMAddNotificationObserver(returnPressed, panel,
			      WMTextDidEndEditingNotification, panel->threT);

    WMMapSubwidgets(panel->speedF);
    
    /***************** Doubleclick Delay ****************/

    panel->ddelaF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->ddelaF, 245, 95);
    WMMoveWidget(panel->ddelaF, 15, 125);
    WMSetFrameTitle(panel->ddelaF, _("Double-Click Delay"));
    
    buf1 = wmalloc(strlen(DELAY_ICON)+2);
    buf2 = wmalloc(strlen(DELAY_ICON_S)+2);
    
    for (i = 0; i < 5; i++) {
	panel->ddelaB[i] = WMCreateCustomButton(panel->ddelaF, 
						WBBStateChangeMask);
	WMResizeWidget(panel->ddelaB[i], 25, 25);
	WMMoveWidget(panel->ddelaB[i], 30+(40*i), 20);
	WMSetButtonBordered(panel->ddelaB[i], False);
	WMSetButtonImagePosition(panel->ddelaB[i], WIPImageOnly);
	WMSetButtonAction(panel->ddelaB[i], doubleClick, panel);
	if (i>0) {
	    WMGroupButtons(panel->ddelaB[0], panel->ddelaB[i]);
	}
	sprintf(buf1, DELAY_ICON, i+1);
	sprintf(buf2, DELAY_ICON_S, i+1);
	path = LocateImage(buf1);
	if (path) {
	    icon = WMCreatePixmapFromFile(scr, path);
	    if (icon) {
		WMSetButtonImage(panel->ddelaB[i], icon);
		WMReleasePixmap(icon);
	    } else {
		wwarning(_("could not load icon file %s"), path);
	    }
	    free(path);
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
	    free(path);
	}
    }
    free(buf1);
    free(buf2);

    panel->tester = CreateDoubleTest(panel->ddelaF, _("Test"));
    WMResizeWidget(panel->tester, 84, 29);
    WMMoveWidget(panel->tester, 35, 55);

    panel->ddelaT = WMCreateTextField(panel->ddelaF);
    WMResizeWidget(panel->ddelaT, 40, 20);
    WMMoveWidget(panel->ddelaT, 140, 60);

    panel->ddelaL = WMCreateLabel(panel->ddelaF);
    WMResizeWidget(panel->ddelaL, 40, 16);
    WMMoveWidget(panel->ddelaL, 185, 65);
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
    WMSetLabelText(panel->ddelaL, "msec");

    WMMapSubwidgets(panel->ddelaF);
    
    /* ************** Workspace Action Buttons **************** */
    panel->menuF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->menuF, 240, 145);
    WMMoveWidget(panel->menuF, 270, 15);
    WMSetFrameTitle(panel->menuF, _("Workspace Mouse Actions"));

    panel->disaB = WMCreateSwitchButton(panel->menuF);
    WMResizeWidget(panel->disaB, 205, 18);
    WMMoveWidget(panel->disaB, 10, 20);
    WMSetButtonText(panel->disaB, _("Disable mouse actions"));

    
    panel->appL = WMCreateLabel(panel->menuF);
    WMResizeWidget(panel->appL, 125, 16);
    WMMoveWidget(panel->appL, 5, 45);
    WMSetLabelTextAlignment(panel->appL, WARight);
    WMSetLabelText(panel->appL, _("Applications menu"));

    panel->appP = WMCreatePopUpButton(panel->menuF);
    WMResizeWidget(panel->appP, 95, 20);
    WMMoveWidget(panel->appP, 135, 45);
    
    panel->listL = WMCreateLabel(panel->menuF);
    WMResizeWidget(panel->listL, 125, 16);
    WMMoveWidget(panel->listL, 5, 80);
    WMSetLabelTextAlignment(panel->listL, WARight);
    WMSetLabelText(panel->listL, _("Window list menu"));
    
    panel->listP = WMCreatePopUpButton(panel->menuF);
    WMResizeWidget(panel->listP, 95, 20);
    WMMoveWidget(panel->listP, 135, 80);

    
    panel->selL = WMCreateLabel(panel->menuF);
    WMResizeWidget(panel->selL, 125, 16);
    WMMoveWidget(panel->selL, 5, 115);
    WMSetLabelTextAlignment(panel->selL, WARight);
    WMSetLabelText(panel->selL, _("Select windows"));

    panel->selP = WMCreatePopUpButton(panel->menuF);
    WMResizeWidget(panel->selP, 95, 20);
    WMMoveWidget(panel->selP, 135, 115);

    for (i = 0; i < sizeof(buttonNames)/sizeof(char*); i++) {
	WMAddPopUpButtonItem(panel->appP, buttonNames[i]);
	WMAddPopUpButtonItem(panel->selP, buttonNames[i]);
	WMAddPopUpButtonItem(panel->listP, buttonNames[i]);
    }
    
    WMMapSubwidgets(panel->menuF);
    
    /* ************** Grab Modifier **************** */
    panel->grabF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->grabF, 240, 55);
    WMMoveWidget(panel->grabF, 270, 165);
    WMSetFrameTitle(panel->grabF, _("Mouse Grab Modifier"));

    WMSetBalloonTextForView(_("Keyboard modifier to use for actions that\n"
			      "involve dragging windows with the mouse,\n"
			      "clicking inside the window."),
			    WMWidgetView(panel->grabF));

    panel->grabP = WMCreatePopUpButton(panel->grabF);
    WMResizeWidget(panel->grabP, 160, 20);
    WMMoveWidget(panel->grabP, 40, 25);

    fillModifierPopUp(panel->grabP);
    
    WMMapSubwidgets(panel->grabF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    
    showData(panel);
}


static void
storeCommandInScript(char *cmd, char *line)
{
    char *path;
    FILE *f;
    char buffer[128];

    path = wstrappend(wusergnusteppath(), "/Library/WindowMaker/autostart");
    
    f = fopen(path, "r");
    if (!f) {
	f = fopen(path, "w");
	if (!f) {
	    wsyserror(_("could not create %s"), path);
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

	tmppath = wstrappend(wusergnusteppath(), 
			     "/Library/WindowMaker/autostart.tmp");
	fo = fopen(tmppath, "w");
	if (!fo) {
	    wsyserror(_("could not create temporary file %s"), tmppath);
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
	    if (strncmp(buffer, cmd, len)==0) {
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
	fclose(fo);

	if (rename(tmppath, path)!=0) {
	    wsyserror(_("could not rename file %s to %s\n"), tmppath, path);
	}
	free(tmppath);
    }
    sprintf(buffer, "chmod u+x %s", path);
    system(buffer);

end:
    free(path);
    if (f)
	fclose(f);
}


static void
storeData(_Panel *panel)
{
    char buffer[64];
    int i;
    char *tmp, *p;
    static char *button[6] = {"None", "left", "middle", "right", "Button4", "Button5"};
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    if (!WMGetUDBoolForKey(udb, "NoXSetStuff")) {
	tmp = WMGetTextFieldText(panel->threT);
	if (strlen(tmp)==0) {
	    free(tmp);
	    tmp = wstrdup("4");
	}

	sprintf(buffer, XSET" m %i/%i %s\n", (int)(panel->acceleration*10),
		10, tmp);
	storeCommandInScript(XSET" m", buffer);

	free(tmp);
    }

    tmp = WMGetTextFieldText(panel->ddelaT);
    if (sscanf(tmp, "%i", &i) == 1 && i > 0)
	SetIntegerForKey(i, "DoubleClickTime");

    SetBoolForKey(WMGetButtonSelected(panel->disaB), "DisableWSMouseActions");

    i = WMGetPopUpButtonSelectedItem(panel->appP);
    SetStringForKey(button[i], "ApplicationMenuMouseButton");

    i = WMGetPopUpButtonSelectedItem(panel->listP);
    SetStringForKey(button[i], "WindowListMouseButton");
    
    i = WMGetPopUpButtonSelectedItem(panel->selP);
    SetStringForKey(button[i], "SelectWindowsMouseButton");
    
    tmp = WMGetPopUpButtonItem(panel->grabP,
			       WMGetPopUpButtonSelectedItem(panel->grabP));
    tmp = wstrdup(tmp);
    p = strchr(tmp, ' ');
    *p = 0;

    SetStringForKey(tmp, "ModifierKey");

    free(tmp);
}


Panel*
InitMouseSettings(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;
    
    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Mouse Preferences");

    panel->description = _("Mouse speed/acceleration, double click delay,\n"
			   "mouse button bindings etc.");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
