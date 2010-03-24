
#include "WINGsP.h"

typedef struct W_Label {
	W_Class widgetClass;
	W_View *view;

	char *caption;

	WMColorSpec textColor;
	WMFont *font;		/* if NULL, use default */

	WMImage *image;

	struct {
		WMReliefType relief:3;
		WMImagePosition imagePosition:4;
		WMAlignment alignment:2;

		unsigned int noWrap:1;

		unsigned int redrawPending:1;
	} flags;
} Label;

#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		14
#define DEFAULT_ALIGNMENT	WALeft
#define DEFAULT_RELIEF		WRFlat
#define DEFAULT_IMAGE_POSITION	WIPNoImage

static void destroyLabel(Label * lPtr);
static void paintLabel(Label * lPtr);

static void handleEvents(XEvent * event, void *data);

WMLabel *WMCreateLabel(WMWidget * parent)
{
	Label *lPtr;

	lPtr = wmalloc(sizeof(Label));
	memset(lPtr, 0, sizeof(Label));

	lPtr->widgetClass = WC_Label;

	lPtr->view = W_CreateView(W_VIEW(parent));
	if (!lPtr->view) {
		wfree(lPtr);
		return NULL;
	}
	lPtr->view->self = lPtr;

	lPtr->textColor = WMBlackColorSpec();

	WMCreateEventHandler(lPtr->view, ExposureMask | StructureNotifyMask, handleEvents, lPtr);

	W_ResizeView(lPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
	lPtr->flags.alignment = DEFAULT_ALIGNMENT;
	lPtr->flags.relief = DEFAULT_RELIEF;
	lPtr->flags.imagePosition = DEFAULT_IMAGE_POSITION;
	lPtr->flags.noWrap = 1;

	return lPtr;
}

void WMSetLabelImage(WMLabel * lPtr, WMImage * image)
{
	if (lPtr->image != NULL)
		WMDestroyImage(lPtr->image);

	if (image)
		lPtr->image = cairo_surface_reference(image);
	else
		lPtr->image = NULL;

	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

WMImage *WMGetLabelImage(WMLabel * lPtr)
{
	return lPtr->image;
}

char *WMGetLabelText(WMLabel * lPtr)
{
	return lPtr->caption;
}

void WMSetLabelImagePosition(WMLabel * lPtr, WMImagePosition position)
{
	lPtr->flags.imagePosition = position;
	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

void WMSetLabelTextAlignment(WMLabel * lPtr, WMAlignment alignment)
{
	lPtr->flags.alignment = alignment;
	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

void WMSetLabelRelief(WMLabel * lPtr, WMReliefType relief)
{
	lPtr->flags.relief = relief;
	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

void WMSetLabelText(WMLabel * lPtr, char *text)
{
	if (lPtr->caption)
		wfree(lPtr->caption);

	if (text != NULL) {
		lPtr->caption = wstrdup(text);
	} else {
		lPtr->caption = NULL;
	}
	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

WMFont *WMGetLabelFont(WMLabel * lPtr)
{
	return lPtr->font;
}

void WMSetLabelFont(WMLabel * lPtr, WMFont * font)
{
	if (lPtr->font != NULL)
		WMReleaseFont(lPtr->font);
	if (font)
		lPtr->font = WMRetainFont(font);
	else
		lPtr->font = NULL;

	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

void WMSetLabelTextColor(WMLabel *lPtr, WMColorSpec *color)
{
	lPtr->textColor = *color;
	if (lPtr->view->flags.realized) {
		paintLabel(lPtr);
	}
}

void WMSetLabelWraps(WMLabel * lPtr, Bool flag)
{
	flag = ((flag == 0) ? 0 : 1);
	if (lPtr->flags.noWrap != !flag) {
		lPtr->flags.noWrap = !flag;
		if (lPtr->view->flags.realized)
			paintLabel(lPtr);
	}
}

static void paintLabel(Label * lPtr)
{
	W_Screen *scrPtr = lPtr->view->screen;
	cairo_t *cairo= W_CreateCairoForView(lPtr->view);

	WMColorSpecSet(cairo, &lPtr->view->backColor);

	cairo_rectangle(cairo, 0, 0, WMWidgetWidth(lPtr), WMWidgetHeight(lPtr));
	cairo_fill(cairo);

	WMColorSpecSet(cairo, &lPtr->textColor);

	W_PaintTextAndImage(scrPtr, cairo, lPtr->view, !lPtr->flags.noWrap, &lPtr->textColor,
			(lPtr->font!=NULL ? lPtr->font : scrPtr->normalFont),
			lPtr->flags.relief, lPtr->caption,
			lPtr->flags.alignment, lPtr->image,
			lPtr->flags.imagePosition, NULL, 0);

	cairo_destroy(cairo);

}

static void handleEvents(XEvent * event, void *data)
{
	Label *lPtr = (Label *) data;

	CHECK_CLASS(data, WC_Label);

	switch (event->type) {
	case Expose:
		if (event->xexpose.count != 0)
			break;
		paintLabel(lPtr);
		break;

	case DestroyNotify:
		destroyLabel(lPtr);
		break;
	}
}

static void destroyLabel(Label * lPtr)
{
	if (lPtr->caption)
		wfree(lPtr->caption);

	if (lPtr->font)
		WMReleaseFont(lPtr->font);

	wfree(lPtr);
}
