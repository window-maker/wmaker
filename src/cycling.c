/* cycling.c- window cycling
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 2000-2003 Alfredo K. Kojima
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define MOX_CYCLING

#include "WindowMaker.h"
#include "GNUstep.h"
#include "screen.h"
#include "wcore.h"
#include "window.h"
#include "framewin.h"
#include "keybind.h"
#include "actions.h"
#include "stacking.h"
#include "funcs.h"
#include "xinerama.h"
#include "switchpanel.h"

/* Globals */
extern WPreferences wPreferences;

extern WShortKey wKeyBindings[WKBD_LAST];





#ifndef MOX_CYCLING
static WWindow*
nextToFocusAfter(WWindow *wwin)
{
    WWindow *tmp = wwin->prev;

    while (tmp) {
        if (wWindowCanReceiveFocus(tmp) && !WFLAGP(tmp, skip_window_list)) {
            return tmp;
        }
        tmp = tmp->prev;
    }

    tmp = wwin;
    /* start over from the beginning of the list */
    while (tmp->next)
        tmp = tmp->next;

    while (tmp && tmp != wwin) {
        if (wWindowCanReceiveFocus(tmp) && !WFLAGP(tmp, skip_window_list)) {
            return tmp;
        }
        tmp = tmp->prev;
    }

    return wwin;
}


static WWindow*
nextToFocusBefore(WWindow *wwin)
{
    WWindow *tmp = wwin->next;

    while (tmp) {
        if (wWindowCanReceiveFocus(tmp) && !WFLAGP(tmp, skip_window_list)) {
            return tmp;
        }
        tmp = tmp->next;
    }

    /* start over from the beginning of the list */
    tmp = wwin;
    while (tmp->prev)
        tmp = tmp->prev;

    while (tmp && tmp != wwin) {
        if (wWindowCanReceiveFocus(tmp) && !WFLAGP(tmp, skip_window_list)) {

            return tmp;
        }
        tmp = tmp->next;
    }

    return wwin;
}




static WWindow*
nextFocusWindow(WWindow *wwin)
{
    WWindow *tmp, *closest, *min;
    Window d;

    if (!wwin)
        return NULL;
    tmp = wwin->prev;
    closest = NULL;
    min = wwin;
    d = 0xffffffff;
    while (tmp) {
        if (wWindowCanReceiveFocus(tmp)
            && (!WFLAGP(tmp, skip_window_list)|| tmp->flags.internal_window)) {
            if (min->client_win > tmp->client_win)
                min = tmp;
            if (tmp->client_win > wwin->client_win
                && (!closest
                    || (tmp->client_win - wwin->client_win) < d)) {
                closest = tmp;
                d = tmp->client_win - wwin->client_win;
            }
        }
        tmp = tmp->prev;
    }
    if (!closest||closest==wwin)
        return min;
    return closest;
}


static WWindow*
prevFocusWindow(WWindow *wwin)
{
    WWindow *tmp, *closest, *max;
    Window d;

    if (!wwin)
        return NULL;
    tmp = wwin->prev;
    closest = NULL;
    max = wwin;
    d = 0xffffffff;
    while (tmp) {
        if (wWindowCanReceiveFocus(tmp) &&
            (!WFLAGP(tmp, skip_window_list) || tmp->flags.internal_window)) {
            if (max->client_win < tmp->client_win)
                max = tmp;
            if (tmp->client_win < wwin->client_win
                && (!closest
                    || (wwin->client_win - tmp->client_win) < d)) {
                closest = tmp;
                d = wwin->client_win - tmp->client_win;
            }
        }
        tmp = tmp->prev;
    }
    if (!closest||closest==wwin)
        return max;
    return closest;
}
#endif /* !MOX_CYCLING */


void
StartWindozeCycle(WWindow *wwin, XEvent *event, Bool next)
{
    WScreen *scr = wScreenForRootWindow(event->xkey.root);
    Bool done = False;
    WWindow *newFocused;
    WWindow *oldFocused;
    int modifiers;
    XModifierKeymap *keymap = NULL;
    Bool hasModifier;
    Bool somethingElse = False;
    XEvent ev;
#ifdef MOX_CYCLING
    WSwitchPanel *swpanel = NULL;
#endif
    KeyCode leftKey, rightKey;

    if (!wwin)
        return;
  
    leftKey = XKeysymToKeycode(dpy, XK_Left);
    rightKey = XKeysymToKeycode(dpy, XK_Right);

    if (next)
        hasModifier = (wKeyBindings[WKBD_FOCUSNEXT].modifier != 0);
    else
        hasModifier = (wKeyBindings[WKBD_FOCUSPREV].modifier != 0);

    if (hasModifier) {
        keymap = XGetModifierMapping(dpy);

#ifdef DEBUG
        printf("Grabbing keyboard\n");
#endif
        XGrabKeyboard(dpy, scr->root_win, False, GrabModeAsync, GrabModeAsync,
                      CurrentTime);
    }

    scr->flags.doing_alt_tab = 1;

#ifdef MOX_CYCLING
    swpanel =  wInitSwitchPanel(scr, 0);
    oldFocused = wwin;
  
    if (swpanel) {
        newFocused = wSwitchPanelSelectNext(swpanel, next);
        wWindowFocus(newFocused, oldFocused);
        oldFocused = newFocused;
    }
#else /* !MOX_CYCLING */
    if (next) {
        if (wPreferences.windows_cycling)
            newFocused = nextToFocusAfter(wwin);
        else
            newFocused = nextFocusWindow(wwin);
    } else {
        if (wPreferences.windows_cycling)
            newFocused = nextToFocusBefore(wwin);
        else
            newFocused = prevFocusWindow(wwin);
    }

    if (wPreferences.circ_raise)
        XRaiseWindow(dpy, newFocused->frame->core->window);
    wWindowFocus(newFocused, scr->focused_window);
    oldFocused = newFocused;
#endif /* !MOX_CYCLING */

    while (hasModifier && !done) {
        WMMaskEvent(dpy, KeyPressMask|KeyReleaseMask|ExposureMask|PointerMotionMask, &ev);

        if (ev.type != KeyRelease && ev.type != KeyPress) {
            WMHandleEvent(&ev);
            continue;
        }
        /* ignore CapsLock */
        modifiers = ev.xkey.state & ValidModMask;

        if (ev.type == KeyPress) {
#ifdef DEBUG
            printf("Got key press\n");
#endif
            if ((wKeyBindings[WKBD_FOCUSNEXT].keycode == ev.xkey.keycode
                && wKeyBindings[WKBD_FOCUSNEXT].modifier == modifiers)
                || ev.xkey.keycode == rightKey) {

#ifdef MOX_CYCLING
                if (swpanel) {
                    newFocused = wSwitchPanelSelectNext(swpanel, False);
                    wWindowFocus(newFocused, oldFocused);
                    oldFocused = newFocused;
                }
#else /* !MOX_CYCLING */
                newFocused = nextToFocusAfter(newFocused);
                wWindowFocus(newFocused, oldFocused);
                oldFocused = newFocused;

                if (wPreferences.circ_raise) {
                    /* restore order */
                    CommitStacking(scr);
                    XRaiseWindow(dpy, newFocused->frame->core->window);
                }
#endif /* !MOX_CYCLING */
            } else if ((wKeyBindings[WKBD_FOCUSPREV].keycode == ev.xkey.keycode
                        && wKeyBindings[WKBD_FOCUSPREV].modifier == modifiers)
                       || ev.xkey.keycode == leftKey) {

#ifdef MOX_CYCLING
                if (swpanel) {
                    newFocused = wSwitchPanelSelectNext(swpanel, True);
                    wWindowFocus(newFocused, oldFocused);
                    oldFocused = newFocused;
                }
#else /* !MOX_CYCLING */
                newFocused = nextToFocusBefore(newFocused);
                wWindowFocus(newFocused, oldFocused);
                oldFocused = newFocused;

                if (wPreferences.circ_raise) {
                    /* restore order */
                    CommitStacking(scr);
                    XRaiseWindow(dpy, newFocused->frame->core->window);
                }
#endif /* !MOX_CYCLING */
            } else if (ev.type == MotionNotify) {
                WWindow *tmp;
                if (swpanel) {
                    tmp = wSwitchPanelHandleEvent(swpanel, &ev);
                    if (tmp) {
                        newFocused = tmp;
                        wWindowFocus(newFocused, oldFocused);
                        oldFocused = newFocused;
                    }
                }
            } else {
#ifdef DEBUG
                printf("Got something else\n");
#endif
                somethingElse = True;
                done = True;
            }
        } else if (ev.type == KeyRelease) {
            int i;

#ifdef DEBUG
            printf("Got key release\n");
#endif
            for (i = 0; i < 8 * keymap->max_keypermod; i++) {
                if (keymap->modifiermap[i] == ev.xkey.keycode &&
                    wKeyBindings[WKBD_FOCUSNEXT].modifier
                    & 1<<(i/keymap->max_keypermod)) {
                    done = True;
                    break;
                }
            }
        }
    }
    if (keymap)
        XFreeModifiermap(keymap);

    if (hasModifier) {
#ifdef DEBUG
        printf("Ungrabbing keyboard\n");
#endif
        XUngrabKeyboard(dpy, CurrentTime);
    }
    wSetFocusTo(scr, newFocused);

#ifdef MOX_CYCLING
    if (swpanel)
        wSwitchPanelDestroy(swpanel);
#endif

    if (wPreferences.circ_raise && newFocused) {
        wRaiseFrame(newFocused->frame->core);
        CommitStacking(scr);
    }

    scr->flags.doing_alt_tab = 0;

    if (somethingElse) {
        WMHandleEvent(&ev);
    }
}


