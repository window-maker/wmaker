/* wdefaults.c - window specific defaults
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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

#include "wconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
 
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <wraster.h>


#include "WindowMaker.h"
#include "window.h"
#include "screen.h"
#include "funcs.h"
#include "workspace.h"
#include "defaults.h"
#include "icon.h"


/* Global stuff */

extern WPreferences wPreferences;

extern proplist_t wAttributeDomainName;

extern WDDomain *WDWindowAttributes;


/* Local stuff */


/* type converters */
static int getBool(proplist_t, proplist_t);

static char* getString(proplist_t, proplist_t);


static proplist_t ANoTitlebar = NULL;
static proplist_t ANoResizebar;
static proplist_t ANoMiniaturizeButton;
static proplist_t ANoCloseButton;
static proplist_t ANoBorder;
static proplist_t ANoHideOthers;
static proplist_t ANoMouseBindings;
static proplist_t ANoKeyBindings;
static proplist_t ANoAppIcon;	       /* app */
static proplist_t AKeepOnTop;
static proplist_t AKeepOnBottom;
static proplist_t AOmnipresent;
static proplist_t ASkipWindowList;
static proplist_t AKeepInsideScreen;
static proplist_t AUnfocusable;
static proplist_t AAlwaysUserIcon;
static proplist_t AStartMiniaturized;
static proplist_t AStartMaximized;
static proplist_t AStartHidden;	       /* app */
static proplist_t ADontSaveSession;    /* app */
static proplist_t AEmulateAppIcon;
static proplist_t AFullMaximize;
#ifdef XKB_BUTTON_HINT
static proplist_t ANoLanguageButton;
#endif

static proplist_t AStartWorkspace;

static proplist_t AIcon;


static proplist_t AnyWindow;
static proplist_t No;


static void
init_wdefaults(WScreen *scr)
{
    AIcon = PLMakeString("Icon");
    
    ANoTitlebar = PLMakeString("NoTitlebar");
    ANoResizebar = PLMakeString("NoResizebar");
    ANoMiniaturizeButton = PLMakeString("NoMiniaturizeButton");
    ANoCloseButton = PLMakeString("NoCloseButton");
    ANoBorder = PLMakeString("NoBorder");
    ANoHideOthers = PLMakeString("NoHideOthers");
    ANoMouseBindings = PLMakeString("NoMouseBindings");
    ANoKeyBindings = PLMakeString("NoKeyBindings");
    ANoAppIcon = PLMakeString("NoAppIcon");
    AKeepOnTop = PLMakeString("KeepOnTop");
    AKeepOnBottom = PLMakeString("KeepOnBottom");
    AOmnipresent = PLMakeString("Omnipresent");
    ASkipWindowList = PLMakeString("SkipWindowList");
    AKeepInsideScreen = PLMakeString("KeepInsideScreen");
    AUnfocusable = PLMakeString("Unfocusable");
    AAlwaysUserIcon = PLMakeString("AlwaysUserIcon");
    AStartMiniaturized = PLMakeString("StartMiniaturized");
    AStartHidden = PLMakeString("StartHidden");
    AStartMaximized = PLMakeString("StartMaximized");
    ADontSaveSession = PLMakeString("DontSaveSession");
    AEmulateAppIcon = PLMakeString("EmulateAppIcon");
    AFullMaximize = PLMakeString("FullMaximize");
#ifdef XKB_BUTTON_HINT
    ANoLanguageButton = PLMakeString("NoLanguageButton");
#endif

    AStartWorkspace = PLMakeString("StartWorkspace");
    
    AnyWindow = PLMakeString("*");
    No = PLMakeString("No");
    /*
    if (!scr->wattribs) {
	scr->wattribs = PLGetDomain(wAttributeDomainName);
    }*/
}



static proplist_t
get_value(proplist_t dict_win, proplist_t dict_class, proplist_t dict_name, 
	  proplist_t dict_any, proplist_t option, proplist_t default_value,
	  Bool useGlobalDefault)
{
    proplist_t value;
    

    if (dict_win) {
	value = PLGetDictionaryEntry(dict_win, option);
	if (value)
	    return value;
    }
    
    if (dict_name) {
	value = PLGetDictionaryEntry(dict_name, option);
	if (value)
	    return value;
    }

    if (dict_class) {
	value = PLGetDictionaryEntry(dict_class, option);
	if (value)
	    return value;
    }
    
    if (!useGlobalDefault)
	return NULL;
        
    if (dict_any) {
	value = PLGetDictionaryEntry(dict_any, option);
	if (value)
	    return value;
    }
    
    return default_value;
}


/*
 *----------------------------------------------------------------------
 * wDefaultFillAttributes--
 * 	Retrieves attributes for the specified instance/class and
 * fills attr with it. Values that are actually defined are also
 * set in mask. If useGlobalDefault is True, the default for
 * all windows ("*") will be used for when no values are found
 * for that instance/class. 
 * 
 *----------------------------------------------------------------------
 */
void
wDefaultFillAttributes(WScreen *scr, char *instance, char *class, 
		       WWindowAttributes *attr,
		       WWindowAttributes *mask,
		       Bool useGlobalDefault)
{
    proplist_t value;
    proplist_t key1, key2, key3;
    proplist_t dw, dc, dn, da;


    if (class && instance) {
      char *buffer = NULL;
	  buffer = wmalloc(strlen(class)+strlen(instance)+4);
      key1 = PLMakeString(strcat(strcat(strcpy(buffer,instance),"."),class));
	  free(buffer);
    } else
      key1 = NULL;

    if (instance)
      key2 = PLMakeString(instance);
    else
      key2 = NULL;

    if (class)
      key3 = PLMakeString(class);
    else
      key3 = NULL;
    
    if (!ANoTitlebar) {
	init_wdefaults(scr);
    }

    PLSetStringCmpHook(NULL);

    if (WDWindowAttributes->dictionary) {
	dw = key1 ? PLGetDictionaryEntry(WDWindowAttributes->dictionary, key1) : NULL;
	dn = key2 ? PLGetDictionaryEntry(WDWindowAttributes->dictionary, key2) : NULL;
	dc = key3 ? PLGetDictionaryEntry(WDWindowAttributes->dictionary, key3) : NULL;
	if (useGlobalDefault)
	    da = PLGetDictionaryEntry(WDWindowAttributes->dictionary, AnyWindow);
	else
	    da = NULL;
    } else {
	dw = NULL;
	dn = NULL;
	dc = NULL;
	da = NULL;
    }
    if (key1)
      PLRelease(key1);
    if (key2)
      PLRelease(key2);
    if (key3)
      PLRelease(key3);

#define APPLY_VAL(value, flag, attrib)	\
	if (value) {attr->flag = getBool(attrib, value); \
			if (mask) mask->flag = 1;}

    /* get the data */        
    value = get_value(dw, dc, dn, da, ANoTitlebar, No, useGlobalDefault);
    APPLY_VAL(value, no_titlebar, ANoTitlebar);
        
    value = get_value(dw, dc, dn, da, ANoResizebar, No, useGlobalDefault);
    APPLY_VAL(value, no_resizebar, ANoResizebar);

    value = get_value(dw, dc, dn, da, ANoMiniaturizeButton, No, useGlobalDefault);
    APPLY_VAL(value, no_miniaturize_button, ANoMiniaturizeButton);
    
    value = get_value(dw, dc, dn, da, ANoCloseButton, No, useGlobalDefault);
    APPLY_VAL(value, no_close_button, ANoCloseButton);

    value = get_value(dw, dc, dn, da, ANoBorder, No, useGlobalDefault);
    APPLY_VAL(value, no_border, ANoBorder);

    value = get_value(dw, dc, dn, da, ANoHideOthers, No, useGlobalDefault);
    APPLY_VAL(value, no_hide_others, ANoHideOthers);
    
    value = get_value(dw, dc, dn, da, ANoMouseBindings, No, useGlobalDefault);
    APPLY_VAL(value, no_bind_mouse, ANoMouseBindings);
    
    value = get_value(dw, dc, dn, da, ANoKeyBindings, No, useGlobalDefault);
    APPLY_VAL(value, no_bind_keys, ANoKeyBindings);
    
    value = get_value(dw, dc, dn, da, ANoAppIcon, No, useGlobalDefault);
    APPLY_VAL(value, no_appicon, ANoAppIcon);

    value = get_value(dw, dc, dn, da, AKeepOnTop, No, useGlobalDefault);
    APPLY_VAL(value, floating, AKeepOnTop);

    value = get_value(dw, dc, dn, da, AKeepOnBottom, No, useGlobalDefault);
    APPLY_VAL(value, sunken, AKeepOnBottom);

    value = get_value(dw, dc, dn, da, AOmnipresent, No, useGlobalDefault);
    APPLY_VAL(value, omnipresent, AOmnipresent);

    value = get_value(dw, dc, dn, da, ASkipWindowList, No, useGlobalDefault);
    APPLY_VAL(value, skip_window_list, ASkipWindowList);
    
    value = get_value(dw, dc, dn, da, AKeepInsideScreen, No, useGlobalDefault);
    APPLY_VAL(value, dont_move_off, AKeepInsideScreen);

    value = get_value(dw, dc, dn, da, AUnfocusable, No, useGlobalDefault);
    APPLY_VAL(value, no_focusable, AUnfocusable);

    value = get_value(dw, dc, dn, da, AAlwaysUserIcon, No, useGlobalDefault);
    APPLY_VAL(value, always_user_icon, AAlwaysUserIcon);

    value = get_value(dw, dc, dn, da, AStartMiniaturized, No, useGlobalDefault);
    APPLY_VAL(value, start_miniaturized, AStartMiniaturized);
    
    value = get_value(dw, dc, dn, da, AStartHidden, No, useGlobalDefault);
    APPLY_VAL(value, start_hidden, AStartHidden);

    value = get_value(dw, dc, dn, da, AStartMaximized, No, useGlobalDefault);
    APPLY_VAL(value, start_maximized, AStartMaximized);

    value = get_value(dw, dc, dn, da, ADontSaveSession, No, useGlobalDefault);
    APPLY_VAL(value, dont_save_session, ADontSaveSession);

    value = get_value(dw, dc, dn, da, AEmulateAppIcon, No, useGlobalDefault);
    APPLY_VAL(value, emulate_appicon, AEmulateAppIcon);

    value = get_value(dw, dc, dn, da, AFullMaximize, No, useGlobalDefault);
    APPLY_VAL(value, full_maximize, AFullMaximize);

#ifdef XKB_BUTTON_HINT
    value = get_value(dw, dc, dn, da, ANoLanguageButton, No, useGlobalDefault);
    APPLY_VAL(value, no_language_button, ANoLanguageButton);
#endif

    /* clean up */
    PLSetStringCmpHook(StringCompareHook);
}



proplist_t
get_generic_value(WScreen *scr, char *instance, char *class, proplist_t option,
		  Bool noDefault)
{
    proplist_t value, key, dict;
    
    value = NULL;

    PLSetStringCmpHook(NULL);

    if (class && instance) {
	char *buffer = NULL;
	buffer = wmalloc(strlen(class)+strlen(instance)+4);
	key = PLMakeString(strcat(strcat(strcpy(buffer,instance),"."),class));

	dict = PLGetDictionaryEntry(WDWindowAttributes->dictionary, key);
	PLRelease(key);
	free(buffer);

	if (dict) {
	    value = PLGetDictionaryEntry(dict, option);
	}
    }

    if (!value && instance) {
	key = PLMakeString(instance);
	
	dict = PLGetDictionaryEntry(WDWindowAttributes->dictionary, key);
	PLRelease(key);
	if (dict) {
	    value = PLGetDictionaryEntry(dict, option);
	}
    }

    if (!value && class) {
	key = PLMakeString(class);
	
	dict = PLGetDictionaryEntry(WDWindowAttributes->dictionary, key);
	PLRelease(key);

	if (dict) {
	    value = PLGetDictionaryEntry(dict, option);
	}
    }

    if (!value && !noDefault) {
	dict = PLGetDictionaryEntry(WDWindowAttributes->dictionary, AnyWindow);
	
	if (dict) {
	    value = PLGetDictionaryEntry(dict, option);
	}
    }
    
    PLSetStringCmpHook(StringCompareHook);

    return value;
}


char*
wDefaultGetIconFile(WScreen *scr, char *instance, char *class, 
		    Bool noDefault)
{
    proplist_t value;
    char *tmp;

    if (!ANoTitlebar) {
	init_wdefaults(scr);
    }

    if (!WDWindowAttributes->dictionary)
	return NULL;

    value = get_generic_value(scr, instance, class, AIcon, noDefault);

    if (!value)
	return NULL;

    tmp = getString(AIcon, value);

    return tmp;
}


RImage*
wDefaultGetImage(WScreen *scr, char *winstance, char *wclass)
{
    char *file_name;
    char *path;
    RImage *image;

    file_name = wDefaultGetIconFile(scr, winstance, wclass, False);
    if (!file_name)
	return NULL;
    
    path = FindImage(wPreferences.icon_path, file_name);
    
    if (!path) {
	wwarning(_("could not find icon file \"%s\""), file_name);
	return NULL;
    }
    
    image = RLoadImage(scr->rcontext, path, 0);
    if (!image) {
	wwarning(_("error loading image file \"%s\""), path, RMessageForError(RErrorCode));
    }
    free(path);

    image = wIconValidateIconSize(scr, image);

    return image;
}


int
wDefaultGetStartWorkspace(WScreen *scr, char *instance, char *class)
{
    proplist_t value;
    int w, i;
    char *tmp;

    if (!ANoTitlebar) {
	init_wdefaults(scr);
    }

    if (!WDWindowAttributes->dictionary)
	return -1;

    value = get_generic_value(scr, instance, class, AStartWorkspace, 
			      False);
    
    if (!value)
	return -1;

    tmp = getString(AStartWorkspace, value);

    if (!tmp || strlen(tmp)==0)
	return -1;

    if (sscanf(tmp, "%i", &w)!=1) {
	w = -1;
	for (i=0; i < scr->workspace_count; i++) {
	    if (strcmp(scr->workspaces[i]->name, tmp)==0) {
		w = i;
		break;
	    }
	}
    } else {
	w--;
    }

    return w;
}


void
wDefaultChangeIcon(WScreen *scr, char *instance, char* class, char *file)
{
    WDDomain *db = WDWindowAttributes;
    proplist_t icon_value=NULL, value, attr, key, def_win, def_icon=NULL;
    proplist_t dict = db->dictionary;
    int same = 0;

    if (!dict) {
        dict = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
        if (dict) {
            db->dictionary = dict;
            value = PLMakeString(db->path);
            PLSetFilename(dict, value);
            PLRelease(value);
        }
        else
            return;
    }

    PLSetStringCmpHook(NULL);

    if (instance && class) {
    	char *buffer;
        buffer = wmalloc(strlen(instance) + strlen(class) + 2);
        strcat(strcat(strcpy(buffer, instance), "."), class);
        key = PLMakeString(buffer);
        free(buffer);
    } else if (instance) {
        key = PLMakeString(instance);
    } else if (class) {
        key = PLMakeString(class);
    } else {
        key = PLRetain(AnyWindow);
    }

    if (file) {
        value = PLMakeString(file);
        icon_value = PLMakeDictionaryFromEntries(AIcon, value, NULL);
        PLRelease(value);

        if ((def_win = PLGetDictionaryEntry(dict, AnyWindow)) != NULL) {
            def_icon = PLGetDictionaryEntry(def_win, AIcon);
        }

        if (def_icon && !strcmp(PLGetString(def_icon), file))
            same = 1;
    }

    if ((attr = PLGetDictionaryEntry(dict, key)) != NULL) {
        if (PLIsDictionary(attr)) {
            if (icon_value!=NULL && !same)
                PLMergeDictionaries(attr, icon_value);
            else
                PLRemoveDictionaryEntry(attr, AIcon);
        }
    } else if (icon_value!=NULL && !same) {
        PLInsertDictionaryEntry(dict, key, icon_value);
    }
    if (!wPreferences.flags.noupdates)
	PLSave(dict, YES);

    PLRelease(key);
    if(icon_value)
        PLRelease(icon_value);

    PLSetStringCmpHook(StringCompareHook);
}



/* --------------------------- Local ----------------------- */

static int 
getBool(proplist_t key, proplist_t value)
{
    char *val;

    if (!PLIsString(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
		 PLGetString(key), "Boolean");
	return 0;
    }
    val = PLGetString(value);
    
    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y' || val[0]=='T' 
			  || val[0]=='t' || val[0]=='1'))
	|| (strcasecmp(val, "YES")==0 || strcasecmp(val, "TRUE")==0)) {
	
	return 1;
    } else if ((val[1]=='\0' 
	      && (val[0]=='n' || val[0]=='N' || val[0]=='F' 
		  || val[0]=='f' || val[0]=='0')) 
	     || (strcasecmp(val, "NO")==0 || strcasecmp(val, "FALSE")==0)) {
	
	return 0;
    } else {
	wwarning(_("can't convert \"%s\" to boolean"), val);
        /* We return False if we can't convert to BOOLEAN.
         * This is because all options defaults to False.
         * -1 is not checked and thus is interpreted as True,
         * which is not good.*/
        return 0;
    }
}



/*
 * WARNING: Do not free value returned by this!!
 */
static char*
getString(proplist_t key, proplist_t value)
{    
    if (!PLIsString(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
		 PLGetString(key), "String");
	return NULL;
    }
        
    return PLGetString(value);
}
