
#include <WINGs.h>
#include <WINGsP.h>
#include <WUtil.h>

//items in this file go into WINGs.h
#define WC_Ruler 16
typedef struct W_Ruler WMRuler;

/* All indentation and tab markers are _relative_ to the left margin marker */

/* a tabstop is a linked list of tabstops, 
 * each containing the position of the tabstop */

typedef struct _tabstops {
	struct _tabstops *next;
	int value;
} WMTabStops;

typedef struct W_RulerMargins {
	int left;				/* left margin marker */
	int right;				/* right margin marker */
	int first;				/* indentation marker for first line only */
	int body;				/* body indentation marker */
	WMTabStops *tabs;
} WMRulerMargins;

		


WMRuler *WMCreateRuler (WMWidget *parent);
void WMShowRulerTabs(WMRuler *rPtr, Bool Show);
void WMSetRulerMoveAction(WMRuler *rPtr, 
					WMAction *action, void *clientData);
void WMSetRulerReleaseAction(WMRuler *rPtr, 
					WMAction *action, void *clientData);

int WMGetRulerOffset(WMRuler *rPtr);
void WMSetRulerOffset(WMRuler *rPtr, int pixels);

WMRulerMargins WMGetRulerMargins(WMRuler *rPtr);
void WMSetRulerMargins(WMRuler *rPtr, WMRulerMargins margins);

int WMGetReleasedRulerMargin(WMRuler *rPtr);
int WMGetGrabbedRulerMargin(WMRuler *rPtr);


