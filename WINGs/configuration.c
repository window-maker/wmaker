

#include "WINGsP.h"
#include "wconfig.h"

#include <X11/Xlocale.h>


_WINGsConfiguration WINGsConfiguration;



#define SYSTEM_FONT "-*-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*,-*-*-medium-r-*-*-%d-*-*-*-*-*-*-*"

#define BOLD_SYSTEM_FONT "-*-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*,-*-*-bold-r-*-*-%d-*-*-*-*-*-*-*"

#define XFTSYSTEM_FONT "-*-arial-medium-r-normal-*-%d-*-*-*-*-*-*-*"

#define XFTBOLD_SYSTEM_FONT "-*-arial-bold-r-normal-*-%d-*-*-*-*-*-*-*"

#define FLOPPY_PATH "/floppy"


static unsigned
getButtonWithName(const char *name, unsigned defaultButton)
{
    if (strncmp(name, "Button", 6)==0 && strlen(name)==7) {
        switch (name[6]) {
        case '1':
            return Button1;
        case '2':
            return Button2;
        case '3':
            return Button3;
        case '4':
            return Button4;
        case '5':
            return Button5;
        default:
            break;
        }
    }

    return defaultButton;
}


static Bool
missingOrInvalidXLFD(char *xlfd)
{
    char *ptr = xlfd;
    Bool broken = False;
    int count = 0;

    if (!xlfd)
        return True;

    while (*ptr) {
        if (*ptr=='%') {
            ptr++;
            if ((*ptr=='d' || *ptr=='i') && count==0) {
                count++;
            } else {
                broken = True;
                break;
            }
        } else if (*ptr==',') {
            count = 0;
        }
        ptr++;
    }

    if (broken) {
        if (xlfd == WINGsConfiguration.systemFont) {
            ptr = "system font";
        } else if (xlfd == WINGsConfiguration.boldSystemFont) {
            ptr = "bold system font";
        } else {
            ptr = "Unknown System Font";
        }
        wwarning(_("Invalid %s specification: '%s' (only %%d is allowed and "
                   "at most once for each font in a fontset)."), ptr, xlfd);
    }

    return broken;
}


void
W_ReadConfigurations(void)
{
    WMUserDefaults *defaults;

    memset(&WINGsConfiguration, 0, sizeof(_WINGsConfiguration));

    defaults = WMGetStandardUserDefaults();

    if (defaults) {
        char *buttonName;
        unsigned button;
        char *str;

        WINGsConfiguration.systemFont =
            WMGetUDStringForKey(defaults, "SystemFont");

        WINGsConfiguration.boldSystemFont =
            WMGetUDStringForKey(defaults, "BoldSystemFont");

#ifdef XFT
        WINGsConfiguration.antialiasedText =
            WMGetUDBoolForKey(defaults, "AntialiasedText");
#else
        WINGsConfiguration.antialiasedText = False;
#endif

        WINGsConfiguration.useMultiByte = False;
        str = WMGetUDStringForKey(defaults, "MultiByteText");
        if (str) {
            if (strcasecmp(str, "YES") == 0) {
                WINGsConfiguration.useMultiByte = True;
            } else if (strcasecmp(str, "AUTO") == 0) {
                char *locale;

                /* if it's a multibyte language (japanese, chinese or korean)
                 * then set it to True */
                locale = setlocale(LC_CTYPE, NULL);
                if (locale != NULL
                    && (strncmp(locale, "ja", 2) == 0
                        || strncmp(locale, "zh", 2) == 0
                        || strncmp(locale, "ru", 2) == 0
                        || strncmp(locale, "ko", 2) == 0)) {

                    WINGsConfiguration.useMultiByte = True;
                }
            }
        }

        WINGsConfiguration.doubleClickDelay =
            WMGetUDIntegerForKey(defaults, "DoubleClickTime");

        WINGsConfiguration.floppyPath =
            WMGetUDStringForKey(defaults, "FloppyPath");

        buttonName = WMGetUDStringForKey(defaults, "MouseWheelUp");
        if (buttonName) {
            button = getButtonWithName(buttonName, Button4);
            wfree(buttonName);
        } else {
            button = Button4;
        }
        WINGsConfiguration.mouseWheelUp = button;

        buttonName = WMGetUDStringForKey(defaults, "MouseWheelDown");
        if (buttonName) {
            button = getButtonWithName(buttonName, Button5);
            wfree(buttonName);
        } else {
            button = Button5;
        }
        WINGsConfiguration.mouseWheelDown = button;

        if (WINGsConfiguration.mouseWheelDown==WINGsConfiguration.mouseWheelUp) {
            WINGsConfiguration.mouseWheelUp = Button4;
            WINGsConfiguration.mouseWheelDown = Button5;
        }

        WINGsConfiguration.defaultFontSize =
            WMGetUDIntegerForKey(defaults, "DefaultFontSize");
    }

    if (missingOrInvalidXLFD(WINGsConfiguration.systemFont)) {
        WINGsConfiguration.systemFont = SYSTEM_FONT;
    }
    if (missingOrInvalidXLFD(WINGsConfiguration.boldSystemFont)) {
        WINGsConfiguration.boldSystemFont = BOLD_SYSTEM_FONT;
    }
    if (!WINGsConfiguration.floppyPath) {
        WINGsConfiguration.floppyPath = FLOPPY_PATH;
    }
    if (WINGsConfiguration.doubleClickDelay == 0) {
        WINGsConfiguration.doubleClickDelay = 250;
    }
    if (WINGsConfiguration.mouseWheelUp == 0) {
        WINGsConfiguration.mouseWheelUp = Button4;
    }
    if (WINGsConfiguration.mouseWheelDown == 0) {
        WINGsConfiguration.mouseWheelDown = Button5;
    }
    if (WINGsConfiguration.defaultFontSize == 0) {
        WINGsConfiguration.defaultFontSize = 12;
    }

}

