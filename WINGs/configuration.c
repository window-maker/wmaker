

#include "WINGsP.h"

#include <proplist.h>


_WINGsConfiguration WINGsConfiguration;



#define SYSTEM_FONT "-*-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*,-*-*-medium-r-*-*-%d-*-*-*-*-*-*-*"
 
#define BOLD_SYSTEM_FONT "-*-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*,-*-*-bold-r-*-*-%d-*-*-*-*-*-*-*"
 




void
W_ReadConfigurations(void)
{
    WMUserDefaults *defaults;

    memset(&WINGsConfiguration, 0, sizeof(_WINGsConfiguration));

    defaults = WMGetStandardUserDefaults();

    if (defaults) {
	WINGsConfiguration.systemFont = 
	    WMGetUDStringForKey(defaults, "SystemFont");

	WINGsConfiguration.boldSystemFont = 
	    WMGetUDStringForKey(defaults, "BoldSystemFont");

	WINGsConfiguration.useMultiByte =
	    WMGetUDBoolForKey(defaults, "MultiByteText");

	WINGsConfiguration.doubleClickDelay = 
	    WMGetUDIntegerForKey(defaults, "DoubleClickTime");	
	
	WINGsConfiguration.defaultFontSize = 
	  	WMGetUDIntegerForKey(defaults, "DefaultFontSize");
    }
	  

    if (!WINGsConfiguration.systemFont) {
	WINGsConfiguration.systemFont = SYSTEM_FONT;
    }
    if (!WINGsConfiguration.boldSystemFont) {
	WINGsConfiguration.boldSystemFont = BOLD_SYSTEM_FONT;
    }
    if (WINGsConfiguration.doubleClickDelay == 0) {
	WINGsConfiguration.doubleClickDelay = 250;
    }
    if (WINGsConfiguration.defaultFontSize == 0) {
	WINGsConfiguration.defaultFontSize = 12;
    }

}

