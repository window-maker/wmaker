/* randr.c - RandR multi-monitor support
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 2026 Window Maker Team
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

#include "wconfig.h"

#ifdef USE_RANDR

#include <string.h>
#include <X11/Xlib.h>

#include "randr.h"
#include "window.h"
#include "framewin.h"
#include "xinerama.h"
#include "wmspec.h"
#include "dock.h"
#include "workspace.h"
#include "actions.h"

#define RANDR_MAX_OUTPUTS     32
#define RANDR_DEBOUNCE_DELAY  100   /* milliseconds */

typedef struct {
    RROutput xid;
    RRCrtc crtc;
    int x, y, w, h;
    Rotation rotation;     /* current CRTC rotation */
    Bool connected;        /* RR_Connected */
    Bool stale;            /* mark-scan scratch flag */
    char name[64];
} WRandROutput;

typedef struct {
    WRandROutput outputs[RANDR_MAX_OUTPUTS];
    int n_outputs;
} WRandRState;

/* Initial output scan to populate state from scratch */
static void wRandR_Scan(WScreen *scr, WRandRState *state)
{
    XRRScreenResources *sr;
    int i;

    state->n_outputs = 0;

    sr = XRRGetScreenResourcesCurrent(dpy, scr->root_win);
    if (!sr)
        return;

    for (i = 0; i < sr->noutput && state->n_outputs < RANDR_MAX_OUTPUTS; i++) {
        XRROutputInfo *oi;
        WRandROutput *o;

        oi = XRRGetOutputInfo(dpy, sr, sr->outputs[i]);
        if (!oi)
            continue;

        o = &state->outputs[state->n_outputs++];
        memset(o, 0, sizeof(*o));
        o->xid = sr->outputs[i];
        o->connected = (oi->connection == RR_Connected);
        o->crtc = oi->crtc;
        strncpy(o->name, oi->name, sizeof(o->name) - 1);

        if (oi->crtc != None) {
            XRRCrtcInfo *ci = XRRGetCrtcInfo(dpy, sr, oi->crtc);
            if (ci) {
                o->x = ci->x;
                o->y = ci->y;
                o->w = ci->width;
                o->h = ci->height;
                o->rotation = ci->rotation;
                XRRFreeCrtcInfo(ci);
            }
        }

        XRRFreeOutputInfo(oi);
    }

    XRRFreeScreenResources(sr);
}

/* Primary head detection */
static int wRandR_PrimaryHeadIndex(Window root, const RROutput *xids, int count)
{
    RROutput primary_xid = XRRGetOutputPrimary(dpy, root);
    int i;

    if (primary_xid != None) {
        for (i = 0; i < count; i++) {
            if (xids[i] == primary_xid)
                return i;
        }
    }
    return 0;
}

/* Push RandR geometry into the Xinerama head layer */
static void wRandR_ApplyToXinerama(WScreen *scr, WRandRState *state)
{
    WMRect new_screens[RANDR_MAX_OUTPUTS];
    RROutput new_xids[RANDR_MAX_OUTPUTS];   /* parallel: XID of each head */
    int count = 0;
    int old_count;
    int i;

    for (i = 0; i < state->n_outputs && count < RANDR_MAX_OUTPUTS; i++) {
        WRandROutput *o = &state->outputs[i];
        int j, dup = 0;

        /* Collect outputs that are truly active */
        if (o->crtc == None || o->w == 0 || o->h == 0)
            continue;

        /* Deduplicate mirrored outputs that share the same CRTC origin */
        for (j = 0; j < count; j++) {
            if (new_screens[j].pos.x == o->x && new_screens[j].pos.y == o->y) {
                dup = 1;
                break;
            }
        }
        if (dup)
            continue;

        new_screens[count].pos.x = o->x;
        new_screens[count].pos.y = o->y;
        new_screens[count].size.width = o->w;
        new_screens[count].size.height = o->h;
        new_xids[count] = o->xid;
        count++;
    }

    /* Fallback: if every output disappeared, use the full virtual screen so
     * we never end up with zero heads */
    if (count == 0) {
        new_screens[0].pos.x = 0;
        new_screens[0].pos.y = 0;
        new_screens[0].size.width = scr->scr_width;
        new_screens[0].size.height = scr->scr_height;
        new_xids[0] = None;
        count = 1;
    }

    old_count = wXineramaHeads(scr);

    /* Replace the screens array */
    if (scr->xine_info.screens)
        wfree(scr->xine_info.screens);

    scr->xine_info.screens = wmalloc(sizeof(WMRect) * (count + 1));
    memcpy(scr->xine_info.screens, new_screens, sizeof(WMRect) * count);
    scr->xine_info.count = count;

    scr->xine_info.primary_head = wRandR_PrimaryHeadIndex(scr->root_win, new_xids, count);

    /* Refresh cached screen dimensions (updated by XRRUpdateConfiguration) */
    scr->scr_width = WidthOfScreen(ScreenOfDisplay(dpy, scr->screen));
    scr->scr_height = HeightOfScreen(ScreenOfDisplay(dpy, scr->screen));

    /* Reallocate per-head usable area arrays if the head count changed */
    if (old_count != count) {
        wfree(scr->usableArea);
        wfree(scr->totalUsableArea);
        scr->usableArea = wmalloc(sizeof(WArea) * count);
        scr->totalUsableArea = wmalloc(sizeof(WArea) * count);
    }

    /* Seed usable areas from new geometry */
    for (i = 0; i < count; i++) {
        WMRect *r = &scr->xine_info.screens[i];

        scr->usableArea[i].x1 = scr->totalUsableArea[i].x1 = r->pos.x;
        scr->usableArea[i].y1 = scr->totalUsableArea[i].y1 = r->pos.y;
        scr->usableArea[i].x2 = scr->totalUsableArea[i].x2 = r->pos.x + r->size.width;
        scr->usableArea[i].y2 = scr->totalUsableArea[i].y2 = r->pos.y + r->size.height;
    }
}

/* Bring any off-screen windows in */
static void wRandR_BringAllWindowsInside(WScreen *scr)
{
    WWindow *wwin;
    int i;

    for (wwin = scr->focused_window; wwin != NULL; wwin = wwin->prev) {
        int bw, wx, wy, ww, wh;
        int fully_inside = 0;

        if (!wwin->flags.mapped || wwin->flags.hidden)
            continue;

        bw = HAS_BORDER(wwin) ? scr->frame_border_width : 0;
        wx = wwin->frame_x - bw;
        wy = wwin->frame_y - bw;
        ww = (int)wwin->frame->core->width  + 2 * bw;
        wh = (int)wwin->frame->core->height + 2 * bw;

        /* Skip windows already fully contained within a surviving head */
        for (i = 0; i < wXineramaHeads(scr); i++) {
            WMRect r = wGetRectForHead(scr, i);
            if (wx >= r.pos.x && wy >= r.pos.y &&
                wx + ww <= r.pos.x + (int)r.size.width &&
                wy + wh <= r.pos.y + (int)r.size.height) {
                fully_inside = 1;
                break;
            }
        }

        if (!fully_inside)
            wWindowSnapToHead(wwin);
    }
}

/* Auto-deactivate a physically disconnected output that still holds a CRTC
 *
 * When a cable is pulled or monitor turned off, the X server sets
 * connection=RR_Disconnected but does NOT free the CRTC automatically.
 * We must call XRRSetCrtcConfig with mode=None to release it,
 * which causes the server to fire a fresh batch of RandR events.
 *
 * Returns True if at least one CRTC was freed */
static Bool wRandR_AutoDeactivate(WScreen *scr, WRandRState *state, XRRScreenResources *sr)
{
    Bool deactivated = False;
    int i;

    for (i = 0; i < state->n_outputs; i++) {
        WRandROutput *o = &state->outputs[i];
        Status ret;

        if (o->connected || o->crtc == None || o->stale)
            continue;

        /* Release the CRTC: no mode, no outputs */
        ret = XRRSetCrtcConfig(dpy, sr, o->crtc, sr->timestamp,
                               0, 0, None, RR_Rotate_0, NULL, 0);
        if (ret == RRSetConfigSuccess) {
            wwarning("RandR: released CRTC for disconnected output %s", o->name);
            o->crtc = None;
            o->w = o->h = 0;
            deactivated = True;
        } else {
            wwarning("RandR: failed to release CRTC for output %s", o->name);
        }
    }

    /* Compact and shrink the virtual framebuffer to the bounding box of remaining active outputs */
    if (deactivated) {
        int min_x = scr->scr_width;
        int max_x = 0, max_y = 0;
        int j;

        for (i = 0; i < state->n_outputs; i++) {
            WRandROutput *a = &state->outputs[i];

            if (a->crtc == None || a->w == 0 || a->h == 0)
                continue;

            if (a->x < min_x) min_x = a->x;
            if (a->x + a->w > max_x) max_x = a->x + a->w;
            if (a->y + a->h > max_y) max_y = a->y + a->h;
        }

        /* Slide surviving outputs left to eliminate dead space at the origin */
        if (min_x > 0 && max_x > 0) {
            for (i = 0; i < state->n_outputs; i++) {
                WRandROutput *a = &state->outputs[i];
                XRRCrtcInfo *ci;

                if (a->crtc == None || a->w == 0 || a->h == 0)
                    continue;

                ci = XRRGetCrtcInfo(dpy, sr, a->crtc);
                if (!ci)
                    continue;

                XRRSetCrtcConfig(dpy, sr, a->crtc, sr->timestamp,
                                 a->x - min_x, a->y,
                                 ci->mode, ci->rotation,
                                 ci->outputs, ci->noutput);

                for (j = 0; j < state->n_outputs; j++) {
                    if (state->outputs[j].crtc == a->crtc)
                        state->outputs[j].x -= min_x;
                }

                XRRFreeCrtcInfo(ci);
            }
            max_x -= min_x;
        }

        /* Shrink the virtual framebuffer down to the compacted bounding box */
        if (max_x > 0 && max_y > 0 &&
            (max_x < scr->scr_width || max_y < scr->scr_height)) {
            int mm_w = (int)((long)WidthMMOfScreen(ScreenOfDisplay(dpy, scr->screen))
                              * max_x / scr->scr_width);
            int mm_h = (int)((long)HeightMMOfScreen(ScreenOfDisplay(dpy, scr->screen))
                              * max_y / scr->scr_height);
            XRRSetScreenSize(dpy, scr->root_win, max_x, max_y, mm_w, mm_h);
        }
    }

    return deactivated;
}

/* Auto-activate a newly connected output that has no CRTC assigned yet
 *
 * Mirrors "xrandr --auto": find a free CRTC, pick the preferred mode,
 * place the new head to the right of all active outputs,
 * expand the virtual framebuffer if needed, then call XRRSetCrtcConfig.
 *
 * Returns True if at least one output was activated */
static Bool wRandR_AutoActivate(WScreen *scr, WRandRState *state, XRRScreenResources *sr)
{
    Bool activated = False;
    int i, j;

    for (i = 0; i < state->n_outputs; i++) {
        WRandROutput *o = &state->outputs[i];
        XRROutputInfo *oi;
        RRCrtc free_crtc = None;
        Rotation rotation = RR_Rotate_0;
        RRMode best_mode = None;
        int mode_w = 0, mode_h = 0;
        int new_x = 0;
        int new_vw, new_vh;
        Status ret;

        /* Only process newly connected outputs that have no CRTC yet */
        if (!o->connected || o->crtc != None)
            continue;

        oi = XRRGetOutputInfo(dpy, sr, o->xid);
        if (!oi)
            continue;

        if (oi->ncrtc == 0 || oi->nmode == 0) {
            XRRFreeOutputInfo(oi);
            continue;
        }

        /* Find a free CRTC that is idle and capable of driving this output */
        for (j = 0; j < oi->ncrtc; j++) {
            XRRCrtcInfo *ci = XRRGetCrtcInfo(dpy, sr, oi->crtcs[j]);
            if (ci) {
                if (ci->noutput == 0) {
                    int k;

                    for (k = 0; k < (int)ci->npossible; k++) {
                        if (ci->possible[k] == o->xid) {
                            free_crtc = oi->crtcs[j];
                            if (ci->rotations & RR_Rotate_0)
                                rotation = RR_Rotate_0;
                            else if (ci->rotations & RR_Rotate_90)
                                rotation = RR_Rotate_90;
                            else if (ci->rotations & RR_Rotate_180)
                                rotation = RR_Rotate_180;
                            else
                                rotation = RR_Rotate_270;
                            break;
                        }
                    }
                }
                XRRFreeCrtcInfo(ci);
            }
            if (free_crtc != None)
                break;
        }

        if (free_crtc == None) {
            wwarning("RandR: no free CRTC for output %s", o->name);
            XRRFreeOutputInfo(oi);
            continue;
        }

        /* Preferred mode is first in the list (per RandR spec) */
        best_mode = oi->modes[0];
        XRRFreeOutputInfo(oi);

        /* Look up its pixel dimensions in sr->modes[] */
        for (j = 0; j < sr->nmode; j++) {
            if (sr->modes[j].id == best_mode) {
                mode_w = (int)sr->modes[j].width;
                mode_h = (int)sr->modes[j].height;
                break;
            }
        }

        if (mode_w == 0 || mode_h == 0)
            continue;

        /* Position it to the right of all currently active outputs, same as GNOME */
        for (j = 0; j < state->n_outputs; j++) {
            WRandROutput *a = &state->outputs[j];

            if (a->crtc != None && a->w > 0) {
                int right = a->x + a->w;

                if (right > new_x)
                    new_x = right;
            }
        }

        /* Expand virtual framebuffer if the new head exceeds current size */
        new_vw = new_x + mode_w;
        new_vh = mode_h > scr->scr_height ? mode_h : scr->scr_height;

        if (new_vw > scr->scr_width || new_vh > scr->scr_height) {
            int mm_w = (int)((long)WidthMMOfScreen(ScreenOfDisplay(dpy, scr->screen))
                          * new_vw / scr->scr_width);
            int mm_h = (int)((long)HeightMMOfScreen(ScreenOfDisplay(dpy, scr->screen))
                          * new_vh / scr->scr_height);
            XRRSetScreenSize(dpy, scr->root_win, new_vw, new_vh, mm_w, mm_h);
        }

        ret = XRRSetCrtcConfig(dpy, sr, free_crtc, sr->timestamp,
                               new_x, 0, best_mode, rotation,
                               &o->xid, 1);
        if (ret == RRSetConfigSuccess) {
            wwarning("RandR: auto-activated output %s at +%d+0", o->name, new_x);
            activated = True;
        } else {
            wwarning("RandR: failed to auto-activate output %s", o->name);
        }
    }

    return activated;
}

/* Synchronize RandR state with the X server */
static void wRandR_Update(WScreen *scr)
{
    WRandRState *state = (WRandRState *)scr->randr_state;
    XRRScreenResources *sr;
    int i, j;

    /* Assume all outputs removed until server confirms */
    for (i = 0; i < state->n_outputs; i++)
        state->outputs[i].stale = True;

    /* Re-query server, update or add outputs */
    sr = XRRGetScreenResourcesCurrent(dpy, scr->root_win);
    if (!sr) {
        wwarning("wRandR_Update: XRRGetScreenResourcesCurrent failed");
        return;
    }

    for (i = 0; i < sr->noutput; i++) {
        XRROutputInfo *oi;
        WRandROutput *found = NULL;

        oi = XRRGetOutputInfo(dpy, sr, sr->outputs[i]);
        if (!oi)
            continue;

        /* Match by XID (stable across reconfigures) */
        for (j = 0; j < state->n_outputs; j++) {
            if (state->outputs[j].xid == sr->outputs[i]) {
                found = &state->outputs[j];
                break;
            }
        }

        /* Append new output not seen before */
        if (!found && state->n_outputs < RANDR_MAX_OUTPUTS) {
            found = &state->outputs[state->n_outputs++];
            memset(found, 0, sizeof(*found));
            found->xid = sr->outputs[i];
            strncpy(found->name, oi->name, sizeof(found->name) - 1);
        }

        if (found) {
            found->stale = False;
            found->connected = (oi->connection == RR_Connected);
            found->crtc = oi->crtc;
            found->w = 0;
            found->h = 0;
            found->rotation = RR_Rotate_0;

            if (oi->crtc != None) {
                XRRCrtcInfo *ci = XRRGetCrtcInfo(dpy, sr, oi->crtc);
                if (ci) {
                    found->x = ci->x;
                    found->y = ci->y;
                    found->w = ci->width;
                    found->h = ci->height;
                    found->rotation = ci->rotation;
                    XRRFreeCrtcInfo(ci);
                }
            }
        }

        XRRFreeOutputInfo(oi);
    }

    /* When HotplugMonitor is enabled, actively manage CRTC lifecycle */
    if (wPreferences.hotplug_monitor) {
        Bool changed;

        changed  = wRandR_AutoDeactivate(scr, state, sr);
        changed |= wRandR_AutoActivate(scr, state, sr);
        if (changed) {
            XRRFreeScreenResources(sr);
            return;
        }
    }

    XRRFreeScreenResources(sr);

    /* Remove outputs not reported by server at all */
    for (i = 0, j = 0; i < state->n_outputs; i++) {
        if (!state->outputs[i].stale) {
            if (j != i)
                state->outputs[j] = state->outputs[i];
            j++;
        }
    }
    state->n_outputs = j;

    /* Apply new geometry to the Xinerama head layer */
    i = scr->xine_info.primary_head;
    wRandR_ApplyToXinerama(scr, state);

    /* Move the dock if needed */
    if (scr->dock &&
        ((scr->xine_info.primary_head != i && wPreferences.keep_dock_on_primary_head) ||
         scr->dock->x_pos < 0 ||
         scr->dock->x_pos >= scr->scr_width))
        wDockSwap(scr->dock);

    /* Snap each workspace clip back onto a visible head if it ended up
     * outside the (now smaller or rearranged) virtual framebuffer */
    if (!wPreferences.flags.noclip) {
        int k;

        for (k = 0; k < scr->workspace_count; k++) {
            WDock *clip = scr->workspaces[k]->clip;

            if (clip)
                wClipSnapToHead(clip);
        }
    }

    /* Refresh usable areas and EWMH hints */
    wScreenUpdateUsableArea(scr);
    wNETWMUpdateWorkarea(scr);

    /* Bring any windows that ended up in dead space to an active head */
    wRandR_BringAllWindowsInside(scr);

    /* Rearrange miniaturized and appicons into the icon yard */
    wArrangeIcons(scr, True);
}

static void wRandRDebounceTimerFired(void *data)
{
    WScreen *scr = (WScreen *)data;

    scr->randr_debounce_timer = NULL;
    wRandR_Update(scr);
}

/* end of local stuff */

void wRandRInit(WScreen *scr)
{
    WRandRState *state;

    if (!w_global.xext.randr.supported)
        return;

    state = wmalloc(sizeof(WRandRState));
    memset(state, 0, sizeof(*state));
    scr->randr_state = state;

    wRandR_Scan(scr, state);
    wRandR_ApplyToXinerama(scr, state);
}

void wRandRTeardown(WScreen *scr)
{
    if (!scr->randr_state)
        return;

    if (scr->randr_debounce_timer) {
        WMDeleteTimerHandler(scr->randr_debounce_timer);
        scr->randr_debounce_timer = NULL;
    }

    wfree(scr->randr_state);
    scr->randr_state = NULL;
}

void wRandRHandleNotify(WScreen *scr, XEvent *event)
{
    if (!scr || !scr->randr_state)
        return;

    if (event->type == (w_global.xext.randr.event_base + RRScreenChangeNotify))
        XRRUpdateConfiguration(event);

    /* Debounce: cancel any pending timer, then restart it */
    if (scr->randr_debounce_timer) {
        WMDeleteTimerHandler(scr->randr_debounce_timer);
        scr->randr_debounce_timer = NULL;
    }
    scr->randr_debounce_timer =
        WMAddTimerHandler(RANDR_DEBOUNCE_DELAY, wRandRDebounceTimerFired, scr);
}

#endif /* USE_RANDR */
