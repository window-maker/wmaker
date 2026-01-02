/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2004 Alfredo K. Kojima
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "WindowMaker.h"
#include "screen.h"
#include "framewin.h"
#include "icon.h"
#include "window.h"
#include "defaults.h"
#include "switchpanel.h"
#include "misc.h"
#include "xinerama.h"


#ifdef USE_XSHAPE
#include <X11/extensions/shape.h>
#endif

struct SwitchPanel {
	WScreen *scr;
	WMWindow *win;
	WMFrame *iconBox;

	WMArray *icons;
	WMArray *images;
	WMArray *windows;
	WMArray *flags;
	RImage *bg;
	int current;
	int firstVisible;
	int visibleCount;

	WMLabel *label;

	RImage *tileTmp;
	RImage *tile;

	WMFont *font;
	WMColor *white;
	WMColor *gray;
};

/* these values will be updated whenever the switch panel
 * is created to to match the size defined in switch_panel_icon_size
 * and the selected font size */
static short int icon_size;
static short int border_space;
static short int icon_tile_size;
static short int label_height;

#define SCREEN_BORDER_SPACING 2*20

#define ICON_SELECTED (1<<1)
#define ICON_DIM (1<<2)

static int canReceiveFocus(WWindow *wwin)
{
	if (!wPreferences.cycle_all_workspaces && wwin->frame->workspace != wwin->screen_ptr->current_workspace)
		return 0;

	if (wPreferences.cycle_active_head_only &&
	    wGetHeadForWindow(wwin) != wGetHeadForPointerLocation(wwin->screen_ptr))
		return 0;

	if (WFLAGP(wwin, no_focusable))
		return 0;

	if (!wwin->flags.mapped) {
		if (!wwin->flags.shaded && !wwin->flags.miniaturized && !wwin->flags.hidden && !wPreferences.cycle_all_workspaces)
			return 0;
		else
			return -1;
	}

	return 1;
}

static Bool sameWindowClass(WWindow *wwin, WWindow *curwin)
{
	if (!wwin->wm_class || !curwin->wm_class)
		return False;
	if (strcmp(wwin->wm_class, curwin->wm_class))
		return False;

	return True;
}

static void changeImage(WSwitchPanel *panel, int idecks, int selected, Bool dim, Bool force)
{
	WMFrame *icon = NULL;
	RImage *image = NULL;
	int flags;
	int desired = 0;

	/* This whole function is a no-op if we aren't drawing the panel */
	if (!wPreferences.swtileImage)
		return;

	icon = WMGetFromArray(panel->icons, idecks);
	image = WMGetFromArray(panel->images, idecks);
	flags = (int) (uintptr_t) WMGetFromArray(panel->flags, idecks);

	if (selected)
		desired |= ICON_SELECTED;
	if (dim)
		desired |= ICON_DIM;

	if (flags == desired && !force)
		return;

	WMReplaceInArray(panel->flags, idecks, (void *) (uintptr_t) desired);

	if (!panel->bg && !panel->tile && !selected)
		WMSetFrameRelief(icon, WRFlat);

	if (image && icon) {
		RImage *back;
		int opaq = (dim) ? 75 : 255;
		RImage *tile;
		WMPoint pos;
		Pixmap p;

		if (canReceiveFocus(WMGetFromArray(panel->windows, idecks)) < 0)
			opaq = 50;

		pos = WMGetViewPosition(WMWidgetView(icon));
		back = panel->tileTmp;
		if (panel->bg) {
			RCopyArea(back, panel->bg,
				  border_space + pos.x - panel->firstVisible * icon_tile_size,
				  border_space + pos.y, back->width, back->height, 0, 0);
		} else {
			RColor color;
			color.red = WMRedComponentOfColor(panel->gray) >> 8;
			color.green = WMGreenComponentOfColor(panel->gray) >> 8;
			color.blue = WMBlueComponentOfColor(panel->gray) >> 8;
			color.alpha = 255;
			RFillImage(back, &color);
		}

		if (selected) {
			tile = panel->tile;
			RCombineArea(back, tile, 0, 0, tile->width, tile->height,
				     (back->width - tile->width) / 2, (back->height - tile->height) / 2);
		}

		RCombineAreaWithOpaqueness(back, image, 0, 0, image->width, image->height,
					   (back->width - image->width) / 2, (back->height - image->height) / 2,
					   opaq);

		RConvertImage(panel->scr->rcontext, back, &p);
		XSetWindowBackgroundPixmap(dpy, WMWidgetXID(icon), p);
		XClearWindow(dpy, WMWidgetXID(icon));
		XFreePixmap(dpy, p);
	}

	if (!panel->bg && !panel->tile && selected)
		WMSetFrameRelief(icon, WRSimple);
}

static void addIconForWindow(WSwitchPanel *panel, WMWidget *parent, WWindow *wwin, int x, int y)
{
	WMFrame *icon = WMCreateFrame(parent);
	RImage *image = NULL;

	WMSetFrameRelief(icon, WRFlat);
	WMResizeWidget(icon, icon_tile_size, icon_tile_size);
	WMMoveWidget(icon, x, y);

	if (!WFLAGP(wwin, always_user_icon) && wwin->net_icon_image)
		image = RRetainImage(wwin->net_icon_image);

	/* get_icon_image() includes the default icon image */
	if (!image)
		image = get_icon_image(panel->scr, wwin->wm_instance, wwin->wm_class, icon_tile_size);

	/* We must resize the icon size (~64) to the switch panel icon size (~48) */
	image = wIconValidateIconSize(image, icon_size);

	WMAddToArray(panel->images, image);
	WMAddToArray(panel->icons, icon);
}

static void scrollIcons(WSwitchPanel *panel, int delta)
{
	int nfirst = panel->firstVisible + delta;
	int i;
	int count = WMGetArrayItemCount(panel->windows);
	Bool dim;

	if (count <= panel->visibleCount)
		return;

	if (nfirst < 0)
		nfirst = 0;
	else if (nfirst >= count - panel->visibleCount)
		nfirst = count - panel->visibleCount;

	if (nfirst == panel->firstVisible)
		return;

	WMMoveWidget(panel->iconBox, -nfirst * icon_tile_size, 0);

	panel->firstVisible = nfirst;

	for (i = panel->firstVisible; i < panel->firstVisible + panel->visibleCount; i++) {
		if (i == panel->current)
			continue;
		dim = ((int) (uintptr_t) WMGetFromArray(panel->flags, i) & ICON_DIM);
		changeImage(panel, i, 0, dim, True);
	}
}

/*
 * 0 1 2
 * 3 4 5
 * 6 7 8
 */
static RImage *assemblePuzzleImage(RImage **images, int width, int height)
{
	RImage *img;
	RImage *tmp;
	int tw, th;
	RColor color;

	tw = width - images[0]->width - images[2]->width;
	th = height - images[0]->height - images[6]->height;

	if (tw <= 0 || th <= 0)
		return NULL;

	img = RCreateImage(width, height, 1);
	if (!img)
		return NULL;

	color.red   =   0;
	color.green =   0;
	color.blue  =   0;
	color.alpha = 255;
	RFillImage(img, &color);

	/* top */
	tmp = RSmoothScaleImage(images[1], tw, images[1]->height);
	RCopyArea(img, tmp, 0, 0, tmp->width, tmp->height, images[0]->width, 0);
	RReleaseImage(tmp);

	/* bottom */
	tmp = RSmoothScaleImage(images[7], tw, images[7]->height);
	RCopyArea(img, tmp, 0, 0, tmp->width, tmp->height, images[6]->width, height - images[6]->height);
	RReleaseImage(tmp);

	/* left */
	tmp = RSmoothScaleImage(images[3], images[3]->width, th);
	RCopyArea(img, tmp, 0, 0, tmp->width, tmp->height, 0, images[0]->height);
	RReleaseImage(tmp);

	/* right */
	tmp = RSmoothScaleImage(images[5], images[5]->width, th);
	RCopyArea(img, tmp, 0, 0, tmp->width, tmp->height, width - images[5]->width, images[2]->height);
	RReleaseImage(tmp);

	/* center */
	tmp = RSmoothScaleImage(images[4], tw, th);
	RCopyArea(img, tmp, 0, 0, tmp->width, tmp->height, images[0]->width, images[0]->height);
	RReleaseImage(tmp);

	/* corners */
	RCopyArea(img, images[0], 0, 0, images[0]->width, images[0]->height, 0, 0);
	RCopyArea(img, images[2], 0, 0, images[2]->width, images[2]->height, width - images[2]->width, 0);
	RCopyArea(img, images[6], 0, 0, images[6]->width, images[6]->height, 0, height - images[6]->height);
	RCopyArea(img, images[8], 0, 0, images[8]->width, images[8]->height,
		  width - images[8]->width, height - images[8]->height);

	return img;
}

static RImage *createBackImage(int width, int height)
{
	return assemblePuzzleImage(wPreferences.swbackImage, width, height);
}

static RImage *getTile(void)
{
	RImage *stile;

	if (!wPreferences.swtileImage)
		return NULL;

	stile = RScaleImage(wPreferences.swtileImage, icon_tile_size, icon_tile_size);
	if (!stile)
		return wPreferences.swtileImage;

	return stile;
}

static void drawTitle(WSwitchPanel *panel, int idecks, const char *title)
{
	char *ntitle;
	int width = WMWidgetWidth(panel->win);
	int x;

	if (title)
		ntitle = ShrinkString(panel->font, title, width - 2 * border_space);
	else
		ntitle = NULL;

	if (panel->bg) {
		if (ntitle) {
			if (strcmp(ntitle, title) != 0) {
				x = border_space;
			} else {
				int w = WMWidthOfString(panel->font, ntitle, strlen(ntitle));

				x = border_space + (idecks - panel->firstVisible) * icon_tile_size+
				    icon_tile_size/ 2 - w / 2;
				if (x < border_space)
					x = border_space;
				else if (x + w > width - border_space)
					x = width - border_space - w;
			}
		}

		XClearWindow(dpy, WMWidgetXID(panel->win));
		if (ntitle)
			WMDrawString(panel->scr->wmscreen,
				     WMWidgetXID(panel->win),
				     panel->white, panel->font,
				     x,
				     WMWidgetHeight(panel->win) - border_space - label_height +
				     WMFontHeight(panel->font) / 2, ntitle, strlen(ntitle));
	} else {
		if (ntitle)
			WMSetLabelText(panel->label, ntitle);
	}

	if (ntitle)
		free(ntitle);
}

static WMArray *makeWindowListArray(WScreen *scr, int include_unmapped, Bool class_only)
{
	WMArray *windows = WMCreateArray(10);
	WWindow *wwin = scr->focused_window;

	while (wwin) {
		if ((canReceiveFocus(wwin) != 0) &&
		    (wwin->flags.mapped || wwin->flags.shaded || include_unmapped)) {
			if (class_only)
				if (!sameWindowClass(scr->focused_window, wwin)) {
					wwin = wwin->prev;
					continue;
				}
			if (!WFLAGP(wwin, skip_switchpanel))
				WMAddToArray(windows, wwin);
		}
		wwin = wwin->prev;
	}
	return windows;
}

static WMArray *makeWindowFlagsArray(int count)
{
	WMArray *flags = WMCreateArray(count);
	int i;

	for (i = 0; i < count; i++)
		WMAddToArray(flags, (void *) 0);

	return flags;
}

WSwitchPanel *wInitSwitchPanel(WScreen *scr, WWindow *curwin, Bool class_only)
{
	int wmScaleWidth, wmScaleHeight;
	WMGetScaleBaseFromSystemFont(scr->wmscreen, &wmScaleWidth, &wmScaleHeight);

	icon_size = wPreferences.switch_panel_icon_size;
	icon_tile_size = (short int)(((float)icon_size * 1.2F) + 0.5F);
	border_space = WMScaleY(10);
	label_height = WMScaleY(25);

	WWindow *wwin;
	WSwitchPanel *panel = wmalloc(sizeof(WSwitchPanel));
	WMFrame *viewport;
	int i, width, height, iconsThatFitCount, count;
	WMRect rect = wGetRectForHead(scr, wGetHeadForPointerLocation(scr));

	panel->scr = scr;
	panel->windows = makeWindowListArray(scr, wPreferences.swtileImage != NULL, class_only);
	count = WMGetArrayItemCount(panel->windows);
	if (count)
		panel->flags = makeWindowFlagsArray(count);

	if (count == 0) {
		WMFreeArray(panel->windows);
		wfree(panel);
		return NULL;
	}

	width = icon_tile_size* count;
	iconsThatFitCount = count;

	if (width > (int)rect.size.width) {
		iconsThatFitCount = (rect.size.width - SCREEN_BORDER_SPACING) / icon_tile_size;
		width = iconsThatFitCount * icon_tile_size;
	}

	panel->visibleCount = iconsThatFitCount;

	if (!wPreferences.swtileImage)
		return panel;

	height = label_height + icon_tile_size;

	panel->tileTmp = RCreateImage(icon_tile_size, icon_tile_size, 1);
	panel->tile = getTile();
	if (panel->tile && wPreferences.swbackImage[8])
		panel->bg = createBackImage(width + 2 * border_space, height + 2 * border_space);

	if (!panel->tileTmp || !panel->tile) {
		if (panel->bg)
			RReleaseImage(panel->bg);
		panel->bg = NULL;
		if (panel->tile)
			RReleaseImage(panel->tile);
		panel->tile = NULL;
		if (panel->tileTmp)
			RReleaseImage(panel->tileTmp);
		panel->tileTmp = NULL;
	}

	panel->white = WMWhiteColor(scr->wmscreen);
	panel->gray = WMGrayColor(scr->wmscreen);
	panel->font = WMBoldSystemFontOfSize(scr->wmscreen, WMScaleY(12));
	panel->icons = WMCreateArray(count);
	panel->images = WMCreateArray(count);

	panel->win = WMCreateWindow(scr->wmscreen, "");

	if (!panel->bg) {
		WMFrame *frame = WMCreateFrame(panel->win);
		WMColor *darkGray = WMDarkGrayColor(scr->wmscreen);
		WMSetFrameRelief(frame, WRSimple);
		WMSetViewExpandsToParent(WMWidgetView(frame), 0, 0, 0, 0);

		panel->label = WMCreateLabel(panel->win);
		WMResizeWidget(panel->label, width, label_height);
		WMMoveWidget(panel->label, border_space, border_space + icon_tile_size+ 5);
		WMSetLabelRelief(panel->label, WRSimple);
		WMSetWidgetBackgroundColor(panel->label, darkGray);
		WMSetLabelFont(panel->label, panel->font);
		WMSetLabelTextColor(panel->label, panel->white);

		WMReleaseColor(darkGray);
		height += 5;
	}

	WMResizeWidget(panel->win, width + 2 * border_space, height + 2 * border_space);

	viewport = WMCreateFrame(panel->win);
	WMResizeWidget(viewport, width, icon_tile_size);
	WMMoveWidget(viewport, border_space, border_space);
	WMSetFrameRelief(viewport, WRFlat);

	panel->iconBox = WMCreateFrame(viewport);
	WMMoveWidget(panel->iconBox, 0, 0);
	WMResizeWidget(panel->iconBox, icon_tile_size* count, icon_tile_size);
	WMSetFrameRelief(panel->iconBox, WRFlat);

	WM_ITERATE_ARRAY(panel->windows, wwin, i) {
		addIconForWindow(panel, panel->iconBox, wwin, i * icon_tile_size, 0);
	}

	WMMapSubwidgets(panel->win);
	WMRealizeWidget(panel->win);

	WM_ITERATE_ARRAY(panel->windows, wwin, i) {
		changeImage(panel, i, 0, False, True);
	}

	if (panel->bg) {
		Pixmap pixmap, mask;

		RConvertImageMask(scr->rcontext, panel->bg, &pixmap, &mask, 250);

		XSetWindowBackgroundPixmap(dpy, WMWidgetXID(panel->win), pixmap);

#ifdef USE_XSHAPE
		if (mask && w_global.xext.shape.supported)
			XShapeCombineMask(dpy, WMWidgetXID(panel->win), ShapeBounding, 0, 0, mask, ShapeSet);
#endif
		if (pixmap)
			XFreePixmap(dpy, pixmap);

		if (mask)
			XFreePixmap(dpy, mask);
	}

	{
		WMPoint center;
		center = wGetPointToCenterRectInHead(scr, wGetHeadForPointerLocation(scr),
						     width + 2 * border_space, height + 2 * border_space);
		WMMoveWidget(panel->win, center.x, center.y);
	}

	panel->current = WMGetFirstInArray(panel->windows, curwin);
	if (panel->current >= 0)
		changeImage(panel, panel->current, 1, False, False);

	WMMapWidget(panel->win);

	return panel;
}

void wSwitchPanelDestroy(WSwitchPanel *panel)
{
	int i;
	RImage *image;

	if (panel->win) {
		Window info_win = panel->scr->info_window;
		XEvent ev;
		ev.xclient.type = ClientMessage;
		ev.xclient.message_type = w_global.atom.wm.ignore_focus_events;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = True;

		XSendEvent(dpy, info_win, True, EnterWindowMask, &ev);
		WMUnmapWidget(panel->win);

		ev.xclient.data.l[0] = False;
		XSendEvent(dpy, info_win, True, EnterWindowMask, &ev);
	}

	if (panel->images) {
		WM_ITERATE_ARRAY(panel->images, image, i) {
			if (image)
				RReleaseImage(image);
		}
		WMFreeArray(panel->images);
	}

	if (panel->win)
		WMDestroyWidget(panel->win);

	if (panel->icons)
		WMFreeArray(panel->icons);

	if (panel->flags)
		WMFreeArray(panel->flags);

	WMFreeArray(panel->windows);

	if (panel->tile)
		RReleaseImage(panel->tile);

	if (panel->tileTmp)
		RReleaseImage(panel->tileTmp);

	if (panel->bg)
		RReleaseImage(panel->bg);

	if (panel->font)
		WMReleaseFont(panel->font);

	if (panel->white)
		WMReleaseColor(panel->white);

	if (panel->gray)
		WMReleaseColor(panel->gray);

	wfree(panel);
}

WWindow *wSwitchPanelSelectNext(WSwitchPanel *panel, int back, int ignore_minimized, Bool class_only)
{
	WWindow *wwin, *curwin, *tmpwin;
	int count = WMGetArrayItemCount(panel->windows);
	int orig = panel->current;
	int i;
	Bool dim = False;

	if (count == 0)
		return NULL;

	if (!wPreferences.cycle_ignore_minimized)
		ignore_minimized = False;

	if (ignore_minimized && canReceiveFocus(WMGetFromArray(panel->windows, (count + panel->current) % count)) < 0)
		ignore_minimized = False;

	curwin = WMGetFromArray(panel->windows, orig);
	do {
		do {
			if (back)
				panel->current--;
			else
				panel->current++;

			panel->current = (count + panel->current) % count;
			wwin = WMGetFromArray(panel->windows, panel->current);

			if (!class_only)
				break;
			if (panel->current == orig)
				break;
		} while (!sameWindowClass(wwin, curwin));
	} while (ignore_minimized && panel->current != orig && canReceiveFocus(wwin) < 0);

	WM_ITERATE_ARRAY(panel->windows, tmpwin, i) {
		if (i == panel->current)
			continue;
		if (!class_only || sameWindowClass(tmpwin, curwin))
			changeImage(panel, i, 0, False, False);
		else {
			if (i == orig)
				dim = True;
			changeImage(panel, i, 0, True, False);
		}

	}

	if (panel->current < panel->firstVisible)
		scrollIcons(panel, panel->current - panel->firstVisible);
	else if (panel->current - panel->firstVisible >= panel->visibleCount)
		scrollIcons(panel, panel->current - panel->firstVisible - panel->visibleCount + 1);

	if (panel->win) {
		drawTitle(panel, panel->current, wwin->frame->title);
		if (panel->current != orig)
			changeImage(panel, orig, 0, dim, False);
		changeImage(panel, panel->current, 1, False, False);
	}

	return wwin;
}

WWindow *wSwitchPanelSelectFirst(WSwitchPanel *panel, int back)
{
	WWindow *wwin;
	int count = WMGetArrayItemCount(panel->windows);
	char *title;
	int i;

	if (count == 0)
		return NULL;

	if (back) {
		panel->current = count - 1;
		scrollIcons(panel, count);
	} else {
		panel->current = 0;
		scrollIcons(panel, -count);
	}

	wwin = WMGetFromArray(panel->windows, panel->current);
	title = wwin->frame->title;

	if (panel->win) {
		for (WMArrayFirst(panel->windows, &(i)); (i) != WANotFound; WMArrayNext(panel->windows, &(i)))
			changeImage(panel, i, i == panel->current, False, False);

		drawTitle(panel, panel->current, title);
	}

	return wwin;
}

WWindow *wSwitchPanelHandleEvent(WSwitchPanel *panel, XEvent *event)
{
	WMFrame *icon;
	int i;
	int focus = -1;

	if (!panel->win)
		return NULL;

	if (event->type == MotionNotify) {
		WM_ITERATE_ARRAY(panel->icons, icon, i) {
			if (WMWidgetXID(icon) == event->xmotion.window) {
				focus = i;
				break;
			}
		}
	}

	if (focus >= 0 && panel->current != focus) {
		WWindow *wwin;

		WM_ITERATE_ARRAY(panel->windows, wwin, i) {
			changeImage(panel, i, i == focus, False, False);
		}
		panel->current = focus;

		wwin = WMGetFromArray(panel->windows, focus);

		drawTitle(panel, panel->current, wwin->frame->title);

		return wwin;
	}

	return NULL;
}

Window wSwitchPanelGetWindow(WSwitchPanel *swpanel)
{
	if (!swpanel->win)
		return None;

	return WMWidgetXID(swpanel->win);
}
