#ifdef USER_MENU

#ifndef _WUSERMENU_H_
#define _WUSERMENU_H_

WMenu *wUserMenuGet(WScreen *scr, WWindow *wwin);
void wUserMenuRefreshInstances(WMenu *menu, WWindow *wwin);

#endif
#endif /* USER_MENU */
