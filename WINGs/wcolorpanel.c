/*
 * ColorPanel for WINGs
 *
 * by	]d							: Original idea and basic initial code
 *		Pascal Hofstee				: Code for wheeldrawing and calculating colors from it
 *		Alban Hertroys				: Suggesting optimizations for algorithms for color-
 *										wheel, and implementation of final algorithm.
 *		Alfredo K. Kojima			: For pointing out some memory-allocation problems.
 *		Marco van Hylckama-Vlieg	: For once again doing the artwork ;-)
*/


#include "WINGsP.h"
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>


#ifndef PATH_MAX
#define PATH_MAX	1024
#endif


typedef struct WheelMatrix {
	unsigned int	width, height;	/* Size of the colorwheel */
	unsigned char	*data[3];		/* Wheel data (R,G,B) */
	unsigned char	values[256];	/* Precalculated values for R,G & B values 0..255 */
} wheelMatrix;

typedef struct W_ColorPanel {
	WMWindow	*win;
	
	/* Common Stuff */
	WMColorWell	*colorWell;
	WMButton	*magnifyBtn;
	WMButton	*wheelBtn;
	WMButton	*slidersBtn;
	WMButton	*customPaletteBtn;
	WMButton	*colorListBtn;
	

	/* ColorWheel Panel */
	WMFrame		*wheelFrm;
	WMSlider	*wheelBrightnessS;
	WMView		*wheelView;
	
	/* Slider Panels */
	WMFrame		*slidersFrm;
	WMFrame		*seperatorFrm;
	WMButton	*grayBtn;
	WMButton	*rgbBtn;
	WMButton	*cmykBtn;
	WMButton	*hsbBtn;
		/* Gray Scale Panel */
		WMFrame		*grayFrm;
		WMLabel		*grayMinL;
		WMLabel		*grayMaxL;
		WMSlider	*grayBrightnessS;
		WMTextField	*grayBrightnessT;
		WMButton	*grayPresetBtn[7];
		
		/* RGB Panel */
		WMFrame		*rgbFrm;
		WMLabel		*rgbMinL;
		WMLabel		*rgbMaxL;
		WMSlider	*rgbRedS;
		WMSlider	*rgbGreenS;
		WMSlider	*rgbBlueS;
		WMTextField	*rgbRedT;
		WMTextField	*rgbGreenT;
		WMTextField	*rgbBlueT;
		
		/* CMYK Panel */
		WMFrame		*cmykFrm;
		WMLabel		*cmykMinL;
		WMLabel		*cmykMaxL;
		WMSlider	*cmykCyanS;
		WMSlider	*cmykMagentaS;
		WMSlider	*cmykYellowS;
		WMSlider	*cmykBlackS;
		WMTextField	*cmykCyanT;
		WMTextField	*cmykMagentaT;
		WMTextField	*cmykYellowT;
		WMTextField	*cmykBlackT;
		
		/* HSB Panel */
		WMFrame		*hsbFrm;
		WMSlider	*hsbHueS;
		WMSlider	*hsbSaturationS;
		WMSlider	*hsbBrightnessS;
		WMTextField	*hsbHueT;
		WMTextField	*hsbSaturationT;
		WMTextField	*hsbBrightnessT;
	
	/* Custom Palette Panel*/
	WMFrame			*customPaletteFrm;
	WMPopUpButton	*customPaletteHistoryBtn;
	WMFrame			*customPaletteContentFrm;
	WMPopUpButton	*customPaletteMenuBtn;
	WMView			*customPaletteContentView;

	/* Color List Panel */
	WMFrame		*colorListFrm;
	WMPopUpButton	*colorListHistoryBtn;
	WMList			*colorListContentLst;
	WMPopUpButton	*colorListColorMenuBtn;
	WMPopUpButton	*colorListListMenuBtn;

	/* Look-Up Tables and Images */
	wheelMatrix	*wheelMtrx;
	Pixmap		wheelImg;
	Pixmap		selectionImg;
	Pixmap		selectionBackImg;
	RImage		*customPaletteImg;
	char			*lastBrowseDir;

	/* Common Data Fields */
	RColor				color;	/* Current color */
	RHSVColor			hsvcolor;	/* Backup HSV Color */
	WMColorPanelMode	mode;	/* Current color selection mode */
	WMColorPanelMode	slidersmode;	/* Current color selection mode at sliders panel */
	WMColorPanelMode	lastChanged;	/* Panel that last changed the color */
	int					colx, coly;		/* (x,y) of selection-marker in WheelMode */
	int					palx, paly;		/* (x,y) of selection-marker in CustomPaletteMode */
	float					palXRatio, palYRatio;	/* Ratios in x & y between original and scaled palettesize */
	int					currentPalette;
	char					*configurationPath;
	
	struct {
		unsigned int	dragging:1;
		unsigned int	done:1;
	} flags;
} W_ColorPanel;

enum {
	menuNewFromFile,
	menuRename,
	menuRemove,
	menuCopy,
	menuNewFromClipboard
} customPaletteMenuItem;

#define	PWIDTH			194
#define	PHEIGHT			266
#define	colorWheelSize	150
#define	customPaletteWidth	182
#define	customPaletteHeight	106
#define	knobThickness	8

#define	SPECTRUM_WIDTH	511
#define	SPECTRUM_HEIGHT	360

#define	COLORWHEEL_PART	1
#define	CUSTOMPALETTE_PART	2
#define	BUFSIZE	1024

#undef	EASTEREGG

#define RGBTXT "/usr/X11R6/lib/X11/rgb.txt"
#define MAX_LENGTH  1024


WMFont	*globalFont; 


static int	fetchImageFile(char* toPath, char *imageSrcFile, char *imageDestFileName);
char *generateNewFilename(char *Path, char *curName);

static void	modeButtonCallback(WMWidget *w, void *data);
static int	getPickerPart(W_ColorPanel *panel, int x, int y);
static void readConfiguration(W_ColorPanel *panel);
static void readXColors(W_ColorPanel *panel);

static wheelMatrix*	wheelCreateMatrix(unsigned int width , unsigned int height);
static void	wheelDestroyMatrix(wheelMatrix *matrix);
static wheelMatrix*	wheelInitMatrix(W_ColorPanel *panel);
static void	wheelRender(W_ColorPanel *panel);
static Bool	wheelInsideColorWheel(W_ColorPanel *panel, unsigned long ofs);
static void	wheelPaint(W_ColorPanel *panel);

static void wheelHandleEvents(XEvent *event, void *data);
static void wheelHandleActionEvents(XEvent *event, void *data);
static void wheelBrightnessSliderCallback(WMWidget *w, void *data);
static void wheelUpdateSelection(W_ColorPanel *panel);
static void wheelUndrawSelection(W_ColorPanel *panel);

static void wheelPositionSelection(W_ColorPanel *panel, int x, int y);
static void wheelPositionSelectionOutBounds(W_ColorPanel *panel, int x, int y);
static void	wheelUpdateBrightnessGradientFromHSV (W_ColorPanel *panel, RHSVColor topColor);
static void wheelUpdateBrightnessGradientFromLocation (W_ColorPanel *panel);
static void wheelUpdateBrightnessGradient(W_ColorPanel *panel, RColor topColor);

static void	grayBrightnessSliderCallback(WMWidget *w, void *data);
static void grayPresetButtonCallback(WMWidget *w, void *data);
static void	grayBrightnessTextFieldCallback(void *observerData, WMNotification *notification);

static void rgbSliderCallback(WMWidget *w, void *data);
static void rgbTextFieldCallback(void *observerData, WMNotification *notification);

static void cmykSliderCallback(WMWidget *w, void *data);
static void cmykTextFieldCallback(void *observerData, WMNotification *notification);

static void hsbSliderCallback(WMWidget *w, void *data);
static void hsbTextFieldCallback(void *observerData, WMNotification *notification);
static void	hsbUpdateBrightnessGradient(W_ColorPanel *panel);
static void	hsbUpdateSaturationGradient(W_ColorPanel *panel);
static void	hsbUpdateHueGradient(W_ColorPanel *panel);

static void customRenderSpectrum(W_ColorPanel *panel);
static void customSetPalette(W_ColorPanel *panel);
static void customPaletteHandleEvents(XEvent *event, void *data);
static void customPaletteHandleActionEvents(XEvent *event, void *data);
static void customPalettePositionSelection(W_ColorPanel *panel, int x, int y);
static void customPalettePositionSelectionOutBounds(W_ColorPanel *panel, int x, int y);
static void customPaletteMenuCallback(WMWidget *w, void *data);
static void customPaletteHistoryCallback(WMWidget *w, void *data);

static void customPaletteMenuNewFromFile(W_ColorPanel *panel);
static void customPaletteMenuRename(W_ColorPanel *panel);
static void customPaletteMenuRemove(W_ColorPanel *panel);

static void colorListPaintItem(WMList *lPtr, int index, Drawable d, char *text, int state, WMRect *rect);
static void colorListSelect(WMWidget *w, void *data);

static void	wheelInit(W_ColorPanel *panel);
static void	grayInit(W_ColorPanel *panel);
static void	rgbInit(W_ColorPanel *panel);
static void	cmykInit(W_ColorPanel *panel);
static void hsbInit(W_ColorPanel *panel);


static WMColorPanel*
makeColorPanel(WMScreen *scrPtr, char *name)
{
	WMColorPanel	*panel;
	RImage		*image;
	WMPixmap	*pixmap;
	RColor		from;
	RColor		to;
	int			i;
	int			x,y;

	
	globalFont = WMSystemFontOfSize(scrPtr, 12);
	panel = wmalloc(sizeof(WMColorPanel));
	memset(panel, 0, sizeof(WMColorPanel));
	
	panel->win = WMCreateWindowWithStyle(scrPtr, name, WMTitledWindowMask | WMClosableWindowMask | WMResizableWindowMask);
	WMResizeWidget(panel->win, PWIDTH, PHEIGHT);
	WMSetWindowTitle(panel->win, "Colors");
	

	/* Set Default ColorPanel Mode(s) */
	panel->mode = WMWheelModeColorPanel;
	panel->lastChanged = WMWheelModeColorPanel;
	panel->slidersmode = WMRGBModeColorPanel;
	panel->configurationPath = wstrappend(wusergnusteppath(), "/Library/Colors/");
	
	/* Some Generic Purpose Widgets */
	panel->colorWell = WMCreateColorWell(panel->win);
	WMResizeWidget(panel->colorWell, 134, 36);
	WSetColorWellBordered(panel->colorWell, False);
	WMMoveWidget(panel->colorWell, 56, 4);

	panel->magnifyBtn = WMCreateCommandButton(panel->win);
	WMResizeWidget(panel->magnifyBtn, 46, 36);
	WMMoveWidget(panel->magnifyBtn, 6,4);
	WMSetButtonImagePosition(panel->magnifyBtn, WIPImageOnly);
	WMSetButtonImage(panel->magnifyBtn, scrPtr->magnifyIcon);
	WMSetButtonAltImage(panel->magnifyBtn, scrPtr->magnifyAltIcon);
	
	panel->wheelBtn = WMCreateCustomButton(panel->win, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->wheelBtn, 46, 32);
	WMMoveWidget(panel->wheelBtn, 6, 44);
	WMSetButtonAction(panel->wheelBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->wheelBtn, WIPImageOnly);
	WMSetButtonImage(panel->wheelBtn, scrPtr->wheelIcon);
	WMSetButtonAltImage(panel->wheelBtn, scrPtr->wheelAltIcon);

	panel->slidersBtn = WMCreateCustomButton(panel->win, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->slidersBtn, 46, 32);
	WMMoveWidget(panel->slidersBtn, 52, 44);
	WMSetButtonAction(panel->slidersBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->slidersBtn, WIPImageOnly);
	WMSetButtonImage(panel->slidersBtn, scrPtr->rgbIcon);
	WMSetButtonAltImage(panel->slidersBtn, scrPtr->rgbAltIcon);
	
	panel->customPaletteBtn = WMCreateCustomButton(panel->win, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->customPaletteBtn, 46, 32);
	WMMoveWidget(panel->customPaletteBtn, 98, 44);
	WMSetButtonAction(panel->customPaletteBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->customPaletteBtn, WIPImageOnly);
	WMSetButtonImage(panel->customPaletteBtn, scrPtr->customPaletteIcon);
	WMSetButtonAltImage(panel->customPaletteBtn, scrPtr->customPaletteAltIcon);

	panel->colorListBtn = WMCreateCustomButton(panel->win, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->colorListBtn, 46, 32);
	WMMoveWidget(panel->colorListBtn, 144, 44);
	WMSetButtonAction(panel->colorListBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->colorListBtn, WIPImageOnly);
	WMSetButtonImage(panel->colorListBtn, scrPtr->colorListIcon);
	WMSetButtonAltImage(panel->colorListBtn, scrPtr->colorListAltIcon);
	
	/* Let's Group some of them together */
	WMGroupButtons(panel->wheelBtn, panel->slidersBtn);
	WMGroupButtons(panel->wheelBtn, panel->customPaletteBtn);
	WMGroupButtons(panel->wheelBtn, panel->colorListBtn);
	
	/* Widgets for the ColorWheel Panel */
	panel->wheelFrm = WMCreateFrame(panel->win);
	WMSetFrameRelief(panel->wheelFrm, WRFlat);
	WMResizeWidget(panel->wheelFrm, PWIDTH - 8, PHEIGHT - 80 - 26);
	WMMoveWidget(panel->wheelFrm, 5, 80);

	panel->wheelView = W_CreateView(W_VIEW(panel->wheelFrm));
	/* XXX Can we create a view ? */
	W_ResizeView(panel->wheelView, colorWheelSize+4, colorWheelSize+4);
	W_MoveView(panel->wheelView, 0, 0);

	/* Create an event handler to handle expose/click events in the ColorWheel */
	WMCreateEventHandler(panel->wheelView, ButtonPressMask|ButtonReleaseMask|EnterWindowMask
			|LeaveWindowMask|ButtonMotionMask, wheelHandleActionEvents, panel);

	WMCreateEventHandler(panel->wheelView, ExposureMask, wheelHandleEvents, panel);

	panel->wheelBrightnessS = WMCreateSlider(panel->wheelFrm);
	WMResizeWidget(panel->wheelBrightnessS, 16, 153);
	WMMoveWidget(panel->wheelBrightnessS, 5+colorWheelSize+14, 1);
	WMSetSliderMinValue(panel->wheelBrightnessS, 0);
	WMSetSliderMaxValue(panel->wheelBrightnessS, 255);
	WMSetSliderAction(panel->wheelBrightnessS, wheelBrightnessSliderCallback, panel);
	WMSetSliderKnobThickness(panel->wheelBrightnessS, knobThickness);
	

	/* Widgets for the Slider Panels */
	panel->slidersFrm = WMCreateFrame(panel->win);
	WMSetFrameRelief(panel->slidersFrm, WRFlat);
	WMResizeWidget(panel->slidersFrm, PWIDTH - 8, PHEIGHT - 80 - 26);
	WMMoveWidget(panel->slidersFrm, 4, 80);

	panel->seperatorFrm = WMCreateFrame(panel->slidersFrm);
	WMSetFrameRelief(panel->seperatorFrm, WRPushed);
	WMResizeWidget(panel->seperatorFrm, PWIDTH - 8, 2);
	WMMoveWidget(panel->seperatorFrm, 0, 1);

	panel->grayBtn = WMCreateCustomButton(panel->slidersFrm, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->grayBtn, 46, 24);
	WMMoveWidget(panel->grayBtn, 1, 8);
	WMSetButtonAction(panel->grayBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->grayBtn, WIPImageOnly);
	WMSetButtonImage(panel->grayBtn, scrPtr->grayIcon);
	WMSetButtonAltImage(panel->grayBtn, scrPtr->grayAltIcon);

	panel->rgbBtn = WMCreateCustomButton(panel->slidersFrm, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->rgbBtn, 46, 24);
	WMMoveWidget(panel->rgbBtn, 47, 8);
	WMSetButtonAction(panel->rgbBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->rgbBtn, WIPImageOnly);
	WMSetButtonImage(panel->rgbBtn, scrPtr->rgbIcon);
	WMSetButtonAltImage(panel->rgbBtn, scrPtr->rgbAltIcon);
	
	panel->cmykBtn = WMCreateCustomButton(panel->slidersFrm, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->cmykBtn, 46, 24);
	WMMoveWidget(panel->cmykBtn, 93, 8);
	WMSetButtonAction(panel->cmykBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->cmykBtn, WIPImageOnly);
	WMSetButtonImage(panel->cmykBtn, scrPtr->cmykIcon);
	WMSetButtonAltImage(panel->cmykBtn, scrPtr->cmykAltIcon);
	
	panel->hsbBtn = WMCreateCustomButton(panel->slidersFrm, WBBStateLightMask|WBBStateChangeMask);
	WMResizeWidget(panel->hsbBtn, 46, 24);
	WMMoveWidget(panel->hsbBtn, 139, 8);
	WMSetButtonAction(panel->hsbBtn, modeButtonCallback, panel);
	WMSetButtonImagePosition(panel->hsbBtn, WIPImageOnly);
	WMSetButtonImage(panel->hsbBtn, scrPtr->hsbIcon);
	WMSetButtonAltImage(panel->hsbBtn, scrPtr->hsbAltIcon);
	
	/* Let's Group the Slider Panel Buttons Together */
	WMGroupButtons(panel->grayBtn, panel->rgbBtn);
	WMGroupButtons(panel->grayBtn, panel->cmykBtn);
	WMGroupButtons(panel->grayBtn, panel->hsbBtn);
	
		/* Widgets for GrayScale Panel */
		panel->grayFrm = WMCreateFrame(panel->slidersFrm);
		WMSetFrameRelief(panel->grayFrm, WRFlat);
		WMResizeWidget(panel->grayFrm, PWIDTH - 8, PHEIGHT - 80 - 26 - 32);
		WMMoveWidget(panel->grayFrm, 0, 34);
		
		panel->grayMinL = WMCreateLabel(panel->grayFrm);
		WMResizeWidget(panel->grayMinL, 20, 10);
		WMMoveWidget(panel->grayMinL, 2, 2);
		WMSetLabelText(panel->grayMinL, "0");
		WMSetLabelTextAlignment(panel->grayMinL, WALeft);
		WMSetLabelTextColor(panel->grayMinL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->grayMinL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->grayMaxL = WMCreateLabel(panel->grayFrm);
		WMResizeWidget(panel->grayMaxL, 40, 10);
		WMMoveWidget(panel->grayMaxL, 104, 2);
		WMSetLabelText(panel->grayMaxL, "100");
		WMSetLabelTextAlignment(panel->grayMaxL, WARight);
		WMSetLabelTextColor(panel->grayMaxL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->grayMaxL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->grayBrightnessS = WMCreateSlider(panel->grayFrm);
		WMResizeWidget(panel->grayBrightnessS, 141, 16);
		WMMoveWidget(panel->grayBrightnessS, 2, 14);
		WMSetSliderMinValue(panel->grayBrightnessS, 0);
		WMSetSliderMaxValue(panel->grayBrightnessS, 100);
		WMSetSliderKnobThickness(panel->grayBrightnessS, knobThickness);
		WMSetSliderAction(panel->grayBrightnessS, grayBrightnessSliderCallback, panel);
		
		from.red = 0;
		from.green = 0;
		from.blue = 0;
		
		to.red = 255;
		to.green = 255;
		to.blue = 255;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		RDestroyImage(image);
		W_PaintText(W_VIEW(panel->grayBrightnessS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->white), False, "Brightness", strlen("Brightness"));
		WMSetSliderImage(panel->grayBrightnessS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->grayBrightnessT = WMCreateTextField(panel->grayFrm);
		WMResizeWidget(panel->grayBrightnessT, 40, 18);
		WMMoveWidget(panel->grayBrightnessT, 146, 13);
		WMSetTextFieldAlignment(panel->grayBrightnessT, WALeft);
		WMAddNotificationObserver(grayBrightnessTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->grayBrightnessT);
	
		image = RCreateImage(13,13,False);
		for (i=0; i < 7; i++) {
			for (x=0; x < 13; x++) {
				for (y=0; y < 13; y++) {
					image->data[0][y*13+x] = 255/6*i;
					image->data[1][y*13+x] = 255/6*i;
					image->data[2][y*13+x] = 255/6*i;
				}
			}
			panel->grayPresetBtn[i] = WMCreateCommandButton(panel->grayFrm);
			WMResizeWidget(panel->grayPresetBtn[i], 20, 24);
			WMMoveWidget(panel->grayPresetBtn[i], 2+(i*20), 34);
			WMSetButtonAction(panel->grayPresetBtn[i], grayPresetButtonCallback, panel);
		    pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
			WMSetButtonImage(panel->grayPresetBtn[i], pixmap);
			WMSetButtonImagePosition(panel->grayPresetBtn[i], WIPImageOnly);
			WMReleasePixmap(pixmap);	
		}
		RDestroyImage(image);
		/* End of GrayScale Panel */
		
		/* Widgets for RGB Panel */
		panel->rgbFrm = WMCreateFrame(panel->slidersFrm);
		WMSetFrameRelief(panel->rgbFrm, WRFlat);
		WMResizeWidget(panel->rgbFrm, PWIDTH - 8, PHEIGHT - 80 - 26 - 32);
		WMMoveWidget(panel->rgbFrm, 0, 34);
		
		panel->rgbMinL = WMCreateLabel(panel->rgbFrm);
		WMResizeWidget(panel->rgbMinL, 20, 10);
		WMMoveWidget(panel->rgbMinL, 2, 2);
		WMSetLabelText(panel->rgbMinL, "0");
		WMSetLabelTextAlignment(panel->rgbMinL, WALeft);
		WMSetLabelTextColor(panel->rgbMinL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->rgbMinL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->rgbMaxL = WMCreateLabel(panel->rgbFrm);
		WMResizeWidget(panel->rgbMaxL, 40, 10);
		WMMoveWidget(panel->rgbMaxL, 104, 2);
		WMSetLabelText(panel->rgbMaxL, "255");
		WMSetLabelTextAlignment(panel->rgbMaxL, WARight);
		WMSetLabelTextColor(panel->rgbMaxL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->rgbMaxL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->rgbRedS = WMCreateSlider(panel->rgbFrm);
		WMResizeWidget(panel->rgbRedS, 141, 16);
		WMMoveWidget(panel->rgbRedS, 2, 14);
		WMSetSliderMinValue(panel->rgbRedS, 0);
		WMSetSliderMaxValue(panel->rgbRedS, 255);
		WMSetSliderKnobThickness(panel->rgbRedS, knobThickness);
		WMSetSliderAction(panel->rgbRedS, rgbSliderCallback, panel);
		
		to.red = 255;
		to.green = 0;
		to.blue = 0;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->rgbRedS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->white), False, "Red", strlen("Red"));
		RDestroyImage(image);
		WMSetSliderImage(panel->rgbRedS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->rgbRedT = WMCreateTextField(panel->rgbFrm);
		WMResizeWidget(panel->rgbRedT, 40, 18);
		WMMoveWidget(panel->rgbRedT, 146, 13);
		WMSetTextFieldAlignment(panel->rgbRedT, WALeft);
		WMAddNotificationObserver(rgbTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->rgbRedT);
		

		panel->rgbGreenS = WMCreateSlider(panel->rgbFrm);
		WMResizeWidget(panel->rgbGreenS, 141, 16);
		WMMoveWidget(panel->rgbGreenS, 2, 36);
		WMSetSliderMinValue(panel->rgbGreenS, 0);
		WMSetSliderMaxValue(panel->rgbGreenS, 255);
		WMSetSliderKnobThickness(panel->rgbGreenS, knobThickness);
		WMSetSliderAction(panel->rgbGreenS, rgbSliderCallback, panel);
		
		to.red = 0;
		to.green = 255;
		to.blue = 0;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->rgbGreenS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->white), False, "Green", strlen("Green"));
		RDestroyImage(image);
		WMSetSliderImage(panel->rgbGreenS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->rgbGreenT = WMCreateTextField(panel->rgbFrm);
		WMResizeWidget(panel->rgbGreenT, 40, 18);
		WMMoveWidget(panel->rgbGreenT, 146, 35);
		WMSetTextFieldAlignment(panel->rgbGreenT, WALeft);
		WMAddNotificationObserver(rgbTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->rgbGreenT);
	

		panel->rgbBlueS = WMCreateSlider(panel->rgbFrm);
		WMResizeWidget(panel->rgbBlueS, 141, 16);
		WMMoveWidget(panel->rgbBlueS, 2, 58);
		WMSetSliderMinValue(panel->rgbBlueS, 0);
		WMSetSliderMaxValue(panel->rgbBlueS, 255);
		WMSetSliderKnobThickness(panel->rgbBlueS, knobThickness);
		WMSetSliderAction(panel->rgbBlueS, rgbSliderCallback, panel);
	
		to.red = 0;
		to.green = 0;
		to.blue = 255;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->rgbBlueS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->white), False, "Blue", strlen("Blue"));
		RDestroyImage(image);
		WMSetSliderImage(panel->rgbBlueS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->rgbBlueT = WMCreateTextField(panel->rgbFrm);
		WMResizeWidget(panel->rgbBlueT, 40, 18);
		WMMoveWidget(panel->rgbBlueT, 146, 57);
		WMSetTextFieldAlignment(panel->rgbBlueT, WALeft);
		WMAddNotificationObserver(rgbTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->rgbBlueT);
		/* End of RGB Panel */

		/* Widgets for CMYK Panel */
		panel->cmykFrm = WMCreateFrame(panel->slidersFrm);
		WMSetFrameRelief(panel->cmykFrm, WRFlat);
		WMResizeWidget(panel->cmykFrm, PWIDTH - 8, PHEIGHT - 80 - 26 - 32);
		WMMoveWidget(panel->cmykFrm, 0, 34);
			
		panel->cmykMinL = WMCreateLabel(panel->cmykFrm);
		WMResizeWidget(panel->cmykMinL, 20, 10);
		WMMoveWidget(panel->cmykMinL, 2, 2);
		WMSetLabelText(panel->cmykMinL, "0");
		WMSetLabelTextAlignment(panel->cmykMinL, WALeft);
		WMSetLabelTextColor(panel->cmykMinL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->cmykMinL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->cmykMaxL = WMCreateLabel(panel->cmykFrm);
		WMResizeWidget(panel->cmykMaxL, 40, 10);
		WMMoveWidget(panel->cmykMaxL, 104, 2);
		WMSetLabelText(panel->cmykMaxL, "100");
		WMSetLabelTextAlignment(panel->cmykMaxL, WARight);
		WMSetLabelTextColor(panel->cmykMaxL, WMDarkGrayColor(scrPtr));
		WMSetLabelFont(panel->cmykMaxL, WMSystemFontOfSize(scrPtr, 8));
		
		panel->cmykCyanS = WMCreateSlider(panel->cmykFrm);
		WMResizeWidget(panel->cmykCyanS, 141, 16);
		WMMoveWidget(panel->cmykCyanS, 2, 14);
		WMSetSliderMinValue(panel->cmykCyanS, 0);
		WMSetSliderMaxValue(panel->cmykCyanS, 100);
		WMSetSliderKnobThickness(panel->cmykCyanS, knobThickness);
		WMSetSliderAction(panel->cmykCyanS, cmykSliderCallback, panel);
		
		from.red = 255;
		from.green = 255;
		from.blue = 255;
		
		to.red = 0;
		to.green = 255;
		to.blue = 255;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->cmykCyanS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->black), False, "Cyan", strlen("Cyan"));
		RDestroyImage(image);
		WMSetSliderImage(panel->cmykCyanS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->cmykCyanT = WMCreateTextField(panel->cmykFrm);
		WMResizeWidget(panel->cmykCyanT, 40, 18);
		WMMoveWidget(panel->cmykCyanT, 146, 13);
		WMSetTextFieldAlignment(panel->cmykCyanT, WALeft);
		WMAddNotificationObserver(cmykTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->cmykCyanT);
		

		panel->cmykMagentaS = WMCreateSlider(panel->cmykFrm);
		WMResizeWidget(panel->cmykMagentaS, 141, 16);
		WMMoveWidget(panel->cmykMagentaS, 2, 36);
		WMSetSliderMinValue(panel->cmykMagentaS, 0);
		WMSetSliderMaxValue(panel->cmykMagentaS, 100);
		WMSetSliderKnobThickness(panel->cmykMagentaS, knobThickness);
		WMSetSliderAction(panel->cmykMagentaS, cmykSliderCallback, panel);
		
		to.red = 255;
		to.green = 0;
		to.blue = 255;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->cmykMagentaS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->black), False, "Magenta", strlen("Magenta"));
		RDestroyImage(image);
		WMSetSliderImage(panel->cmykMagentaS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->cmykMagentaT = WMCreateTextField(panel->cmykFrm);
		WMResizeWidget(panel->cmykMagentaT, 40, 18);
		WMMoveWidget(panel->cmykMagentaT, 146, 35);
		WMSetTextFieldAlignment(panel->cmykMagentaT, WALeft);
		WMAddNotificationObserver(cmykTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->cmykMagentaT);
	

		panel->cmykYellowS = WMCreateSlider(panel->cmykFrm);
		WMResizeWidget(panel->cmykYellowS, 141, 16);
		WMMoveWidget(panel->cmykYellowS, 2, 58);
		WMSetSliderMinValue(panel->cmykYellowS, 0);
		WMSetSliderMaxValue(panel->cmykYellowS, 100);
		WMSetSliderKnobThickness(panel->cmykYellowS, knobThickness);
		WMSetSliderAction(panel->cmykYellowS, cmykSliderCallback, panel);
	
		to.red = 255;
		to.green = 255;
		to.blue = 0;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->cmykYellowS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->black), False, "Yellow", strlen("Yellow"));
		RDestroyImage(image);
		WMSetSliderImage(panel->cmykYellowS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->cmykYellowT = WMCreateTextField(panel->cmykFrm);
		WMResizeWidget(panel->cmykYellowT, 40, 18);
		WMMoveWidget(panel->cmykYellowT, 146, 57);
		WMSetTextFieldAlignment(panel->cmykYellowT, WALeft);
		WMAddNotificationObserver(cmykTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->cmykYellowT);
	

		panel->cmykBlackS = WMCreateSlider(panel->cmykFrm);
		WMResizeWidget(panel->cmykBlackS, 141, 16);
		WMMoveWidget(panel->cmykBlackS, 2, 80);
		WMSetSliderMinValue(panel->cmykBlackS, 0);
		WMSetSliderMaxValue(panel->cmykBlackS, 100);
		WMSetSliderValue(panel->cmykBlackS, 0);
		WMSetSliderKnobThickness(panel->cmykBlackS, knobThickness);
		WMSetSliderAction(panel->cmykBlackS, cmykSliderCallback, panel);
	
		to.red = 0;
		to.green = 0;
		to.blue = 0;
		
		image = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
		pixmap = WMCreatePixmapFromRImage(scrPtr, image, 0);
		W_PaintText(W_VIEW(panel->cmykBlackS), pixmap->pixmap, WMSystemFontOfSize(scrPtr, 12), 2, 0, 100, WALeft, W_GC(scrPtr->black), False, "Black", strlen("Black"));
		RDestroyImage(image);
		WMSetSliderImage(panel->cmykBlackS, pixmap);
		WMReleasePixmap(pixmap);
		
		panel->cmykBlackT = WMCreateTextField(panel->cmykFrm);
		WMResizeWidget(panel->cmykBlackT, 40, 18);
		WMMoveWidget(panel->cmykBlackT, 146, 79);
		WMSetTextFieldAlignment(panel->cmykBlackT, WALeft);
		WMAddNotificationObserver(cmykTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->cmykBlackT);
		/* End of CMYK Panel */

		/* Widgets for HSB Panel */
		panel->hsbFrm = WMCreateFrame(panel->slidersFrm);
		WMSetFrameRelief(panel->hsbFrm, WRFlat);
		WMResizeWidget(panel->hsbFrm, PWIDTH - 8, PHEIGHT - 80 - 26 - 32);
		WMMoveWidget(panel->hsbFrm, 0, 34);

		panel->hsbHueS = WMCreateSlider(panel->hsbFrm);
		WMResizeWidget(panel->hsbHueS, 141, 16);
		WMMoveWidget(panel->hsbHueS, 2, 14);
		WMSetSliderMinValue(panel->hsbHueS, 0);
		WMSetSliderMaxValue(panel->hsbHueS, 359);
		WMSetSliderKnobThickness(panel->hsbHueS, knobThickness);
		WMSetSliderAction(panel->hsbHueS, hsbSliderCallback, panel);
		
		panel->hsbHueT = WMCreateTextField(panel->hsbFrm);
		WMResizeWidget(panel->hsbHueT, 40, 18);
		WMMoveWidget(panel->hsbHueT, 146, 13);
		WMSetTextFieldAlignment(panel->hsbHueT, WALeft);
		WMAddNotificationObserver(hsbTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->hsbHueT);
		

		panel->hsbSaturationS = WMCreateSlider(panel->hsbFrm);
		WMResizeWidget(panel->hsbSaturationS, 141, 16);
		WMMoveWidget(panel->hsbSaturationS, 2, 36);
		WMSetSliderMinValue(panel->hsbSaturationS, 0);
		WMSetSliderMaxValue(panel->hsbSaturationS, 100);
		WMSetSliderKnobThickness(panel->hsbSaturationS, knobThickness);
		WMSetSliderAction(panel->hsbSaturationS, hsbSliderCallback, panel);
		
		panel->hsbSaturationT = WMCreateTextField(panel->hsbFrm);
		WMResizeWidget(panel->hsbSaturationT, 40, 18);
		WMMoveWidget(panel->hsbSaturationT, 146, 35);
		WMSetTextFieldAlignment(panel->hsbSaturationT, WALeft);
		WMAddNotificationObserver(hsbTextFieldCallback, panel, \
					WMTextDidEndEditingNotification, panel->hsbSaturationT);
	

		panel->hsbBrightnessS = WMCreateSlider(panel->hsbFrm);
		WMResizeWidget(panel->hsbBrightnessS, 141, 16);
		WMMoveWidget(panel->hsbBrightnessS, 2, 58);
		WMSetSliderMinValue(panel->hsbBrightnessS, 0);
		WMSetSliderMaxValue(panel->hsbBrightnessS, 100);
		WMSetSliderKnobThickness(panel->hsbBrightnessS, knobThickness);
		WMSetSliderAction(panel->hsbBrightnessS, hsbSliderCallback, panel);
		
		panel->hsbBrightnessT = WMCreateTextField(panel->hsbFrm);
		WMResizeWidget(panel->hsbBrightnessT, 40, 18);
		WMMoveWidget(panel->hsbBrightnessT, 146, 57);
		WMSetTextFieldAlignment(panel->hsbBrightnessT, WALeft);
		WMAddNotificationObserver(hsbTextFieldCallback, panel, \
			WMTextDidEndEditingNotification, panel->hsbBrightnessT);
		/* End of HSB Panel */

	/* Widgets for the CustomPalette Panel */
	panel->customPaletteFrm = WMCreateFrame(panel->win);
	WMSetFrameRelief(panel->customPaletteFrm, WRFlat);
	WMResizeWidget(panel->customPaletteFrm, PWIDTH - 8, PHEIGHT - 80 - 26);
	WMMoveWidget(panel->customPaletteFrm, 5, 80);

	panel->customPaletteHistoryBtn = WMCreatePopUpButton(panel->customPaletteFrm);
	WMAddPopUpButtonItem(panel->customPaletteHistoryBtn, "Spectrum");
	WMSetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn, WMGetPopUpButtonNumberOfItems(panel->customPaletteHistoryBtn)-1);
	WMSetPopUpButtonAction(panel->customPaletteHistoryBtn, customPaletteHistoryCallback, panel);
	WMResizeWidget(panel->customPaletteHistoryBtn, PWIDTH - 8, 20);
	WMMoveWidget(panel->customPaletteHistoryBtn, 0, 0);
	
	panel->customPaletteContentFrm = WMCreateFrame(panel->customPaletteFrm);
	WMSetFrameRelief(panel->customPaletteContentFrm, WRSunken);
	WMResizeWidget(panel->customPaletteContentFrm, PWIDTH - 8, PHEIGHT - 156);
	WMMoveWidget(panel->customPaletteContentFrm, 0, 23);

	panel->customPaletteContentView = W_CreateView(W_VIEW(panel->customPaletteContentFrm));
	/* XXX Can we create a view ? */
	W_ResizeView(panel->customPaletteContentView, customPaletteWidth, customPaletteHeight);
	W_MoveView(panel->customPaletteContentView, 2, 2);

	/* Create an event handler to handle expose/click events in the CustomPalette */
	WMCreateEventHandler(panel->customPaletteContentView, ButtonPressMask|ButtonReleaseMask|EnterWindowMask
			|LeaveWindowMask|ButtonMotionMask, customPaletteHandleActionEvents, panel);

	WMCreateEventHandler(panel->customPaletteContentView, ExposureMask, customPaletteHandleEvents, panel);

	panel->customPaletteMenuBtn = WMCreatePopUpButton(panel->customPaletteFrm);
	WMSetPopUpButtonPullsDown(panel->customPaletteMenuBtn, 1);
	WMSetPopUpButtonText(panel->customPaletteMenuBtn, "Palette");
	WMSetPopUpButtonAction(panel->customPaletteMenuBtn, customPaletteMenuCallback, panel);
	WMResizeWidget(panel->customPaletteMenuBtn, PWIDTH - 8, 20);
	WMMoveWidget(panel->customPaletteMenuBtn, 0, PHEIGHT - 130);

	WMAddPopUpButtonItem(panel->customPaletteMenuBtn, "New from File...");		/* item 0 */
	WMAddPopUpButtonItem(panel->customPaletteMenuBtn, "Rename...");				/* item 1 */
	WMAddPopUpButtonItem(panel->customPaletteMenuBtn, "Remove");					/* item 2 */
	WMAddPopUpButtonItem(panel->customPaletteMenuBtn, "Copy");						/* item 3 */
	WMAddPopUpButtonItem(panel->customPaletteMenuBtn, "New from Clipboard");	/* item 4 */

	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, 1, 0);
	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, 2, 0);
	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, 3, 0);
	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, 4, 0);

	customRenderSpectrum(panel);
	panel->currentPalette = 0;


	/* Widgets for the ColorList Panel */
	panel->colorListFrm = WMCreateFrame(panel->win);
	WMSetFrameRelief(panel->colorListFrm, WRFlat);
	WMResizeWidget(panel->colorListFrm, PWIDTH - 8, PHEIGHT - 80 - 26);
	WMMoveWidget(panel->colorListFrm, 5, 80);

	panel->colorListHistoryBtn = WMCreatePopUpButton(panel->colorListFrm);
	WMAddPopUpButtonItem(panel->colorListHistoryBtn, "X11-Colors");
	WMSetPopUpButtonSelectedItem(panel->colorListHistoryBtn, WMGetPopUpButtonNumberOfItems(panel->colorListHistoryBtn)-1);
//	WMSetPopUpButtonAction(panel->colorListHistoryBtn, colorListHistoryCallback, panel);
	WMResizeWidget(panel->colorListHistoryBtn, PWIDTH - 8, 20);
	WMMoveWidget(panel->colorListHistoryBtn, 0, 0);
	
	panel->colorListContentLst = WMCreateList(panel->colorListFrm);
	WMSetListAction(panel->colorListContentLst, colorListSelect, panel);
	WMSetListUserDrawProc(panel->colorListContentLst, colorListPaintItem);
	WMResizeWidget(panel->colorListContentLst, PWIDTH - 8, PHEIGHT - 156);
	WMMoveWidget(panel->colorListContentLst, 0, 23);

	panel->colorListColorMenuBtn = WMCreatePopUpButton(panel->colorListFrm);
	WMSetPopUpButtonPullsDown(panel->colorListColorMenuBtn, 1);
	WMSetPopUpButtonText(panel->colorListColorMenuBtn, "Color");
//	WMSetPopUpButtonAction(panel->colorListColorMenuBtn, colorListColorMenuCallback, panel);
	WMResizeWidget(panel->colorListColorMenuBtn, (PWIDTH - 16)/2, 20);
	WMMoveWidget(panel->colorListColorMenuBtn, 0, PHEIGHT - 130);

	panel->colorListListMenuBtn = WMCreatePopUpButton(panel->colorListFrm);
	WMSetPopUpButtonPullsDown(panel->colorListListMenuBtn, 1);
	WMSetPopUpButtonText(panel->colorListListMenuBtn, "List");
//	WMSetPopUpButtonAction(panel->colorListListMenuBtn, colorListListMenuCallback, panel);
	WMResizeWidget(panel->colorListListMenuBtn, (PWIDTH - 16)/2, 20);
	WMMoveWidget(panel->colorListListMenuBtn, (PWIDTH - 16)/2 + 8, PHEIGHT - 130);


	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);

	WMMapSubwidgets(panel->wheelFrm);
	WMMapSubwidgets(panel->slidersFrm);
		WMMapSubwidgets(panel->grayFrm);
		WMMapSubwidgets(panel->rgbFrm);
		WMMapSubwidgets(panel->cmykFrm);
		WMMapSubwidgets(panel->hsbFrm);
	WMMapSubwidgets(panel->customPaletteFrm);
		WMMapSubwidgets(panel->customPaletteContentFrm);
	WMMapSubwidgets(panel->colorListFrm);

	readConfiguration(panel);
	readXColors(panel);
 
	return panel;
}


WMColorPanel*
WMGetColorPanel(WMScreen *scrPtr)
{
	WMColorPanel	*panel;
	
	if (scrPtr->sharedColorPanel)
		return scrPtr->sharedColorPanel;
	
	panel = makeColorPanel(scrPtr, "colorPanel");
	
	scrPtr->sharedColorPanel = panel;
	
	return panel;
}


void
WMFreeColorPanel(WMColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);

	if (panel == scr->sharedColorPanel) {
		WMWidgetScreen(panel->win)->sharedColorPanel = NULL;
	}
	WMRemoveNotificationObserver(panel);	/* XXX */
	WMUnmapWidget(panel->win);
	WMDestroyWidget(panel->win);
	if (panel->wheelMtrx)
		wheelDestroyMatrix(panel->wheelMtrx);
	if (panel->wheelImg)
		XFreePixmap(scr->display, panel->wheelImg);
	if (panel->selectionImg)
		XFreePixmap(scr->display, panel->selectionImg);
	if (panel->selectionBackImg)
		XFreePixmap(scr->display, panel->selectionBackImg);
	if (panel->customPaletteImg)
		RDestroyImage(panel->customPaletteImg);
	if (panel->lastBrowseDir)
		free(panel->lastBrowseDir);
	if (panel->configurationPath)
		free(panel->configurationPath);

	free(panel);
}


int
WMRunColorPanel(WMColorPanel *panel, WMWindow *owner, RColor color)
{
	WMScreen	*scr = WMWidgetScreen(panel->win);
	XEvent 		event;
	RHSVColor	hsvcolor;
	GC			bgc = W_GC(scr->black);
	GC			wgc = W_GC(scr->white);
	float		W_PI = 4*atan(1);
	
	WMChangePanelOwner(panel->win, owner);
	
	WMSetColorPanelColor(panel, color);
	panel->wheelMtrx = wheelInitMatrix(panel);	/* Needs to be done After Color is set */

	/* Maybe put this in sub-function ... Initialising selection images */
	RRGBtoHSV(&color, &hsvcolor);
	panel->colx = 2 + rint((colorWheelSize / 2.0) * (1 + (hsvcolor.saturation/255.0) * cos( hsvcolor.hue*W_PI/180.0)));
	panel->coly = 2 + rint((colorWheelSize / 2.0) * (1 + (hsvcolor.saturation/255.0) * sin(-hsvcolor.hue*W_PI/180.0)));
	wheelUpdateBrightnessGradientFromHSV(panel, hsvcolor);
	WMSetSliderValue(panel->wheelBrightnessS, 255 - hsvcolor.value);

	panel->selectionImg = XCreatePixmap(scr->display, W_VIEW(panel->wheelFrm)->window, 4, 4, scr->depth);
	XFillRectangle(scr->display, panel->selectionImg, bgc, 0, 0, 4, 4);
	XFillRectangle(scr->display, panel->selectionImg, wgc, 1, 1, 2, 2);
	/* End of initialisation section */

	panel->palx = customPaletteWidth/2;
	panel->paly = customPaletteHeight/2;
	
	WMSetPickerMode(panel, WMWheelModeColorPanel);

	panel->flags.done = 0;
	
	WMMapWidget(panel->win);
	
	while (!panel->flags.done) {
		WMNextEvent(scr->display, &event);
		WMHandleEvent(&event);
	}
	
	WMCloseWindow(panel->win);
	
	return 0;
}



static void
readConfiguration(W_ColorPanel *panel)
{
	/* XXX Doesn't take care of "invalid" files */

	DIR	*dPtr;
	struct dirent	*dp;
	struct stat		stat_buf;


	if (stat(panel->configurationPath, &stat_buf)!=0) {
		if (mkdir(panel->configurationPath, S_IRWXU|S_IRGRP|S_IROTH|S_IXGRP|S_IXOTH)!=0) {
				wfatal("ColorPanel could not create directory %s needed to store configurations", panel->configurationPath);
				WMFreeColorPanel(panel);
				wAbort();
		}
		return;
	}
	
	dPtr = opendir(panel->configurationPath);
	while ((dp = readdir(dPtr)) != NULL) {
		if (dp->d_name[0] != '.')
			WMAddPopUpButtonItem(panel->customPaletteHistoryBtn, dp->d_name);
	}
	(void)closedir(dPtr);
}


static void
readXColors(W_ColorPanel *panel)
{
	struct stat		stat_buf;
	FILE			*rgbtxt;
	char			line[MAX_LENGTH];
	int				red, green, blue;
	char			name[48];
	RColor			*color;
	WMListItem		*item;
	
	if (stat(RGBTXT, &stat_buf) != 0) {
		wwarning("Cannot find file %s", RGBTXT);
		return;
	}
	else {
		rgbtxt = fopen(RGBTXT, "r");
		if (rgbtxt) {
			while (fgets(line, MAX_LENGTH, rgbtxt)) {
				if (sscanf(line, "%d%d%d %[^\n]", &red, &green, &blue, name)) {
					color = wmalloc(sizeof(RColor));
					color->red = (unsigned char)red;
					color->green = (unsigned char)green;
					color->blue = (unsigned char)blue;
					item = WMAddListItem(panel->colorListContentLst, name);
					item->clientData = (void *)color;
				}
			}
			fclose(rgbtxt);
		}
		else {
			wwarning("Unable to open file %s for reading", RGBTXT);
		}
	}
}


void
WMSetPickerMode(WMColorPanel *panel, WMColorPanelMode mode)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);

	if (mode != WMWheelModeColorPanel) {
		WMUnmapWidget(panel->wheelFrm);
		if (panel->selectionBackImg) {
			XFreePixmap(WMWidgetScreen(panel->win)->display, panel->selectionBackImg);
			panel->selectionBackImg = None;
		}
	}
	if (mode != WMGrayModeColorPanel)
		WMUnmapWidget(panel->grayFrm);
	if (mode != WMRGBModeColorPanel)
		WMUnmapWidget(panel->rgbFrm);
	if (mode != WMCMYKModeColorPanel)
		WMUnmapWidget(panel->cmykFrm);
	if (mode != WMHSBModeColorPanel)
		WMUnmapWidget(panel->hsbFrm);
	if (mode != WMCustomPaletteModeColorPanel) {
		WMUnmapWidget(panel->customPaletteFrm);
		if (panel->selectionBackImg) {
			XFreePixmap(WMWidgetScreen(panel->win)->display, panel->selectionBackImg);
			panel->selectionBackImg = None;
		}
	}
	if (mode != WMColorListModeColorPanel)
		WMUnmapWidget(panel->colorListFrm);
	if ((mode != WMGrayModeColorPanel) && (mode != WMRGBModeColorPanel) && \
		(mode != WMCMYKModeColorPanel) && (mode != WMHSBModeColorPanel))
		WMUnmapWidget(panel->slidersFrm);
	else
		panel->slidersmode = mode;

	if (mode == WMWheelModeColorPanel) {
		WMMapWidget(panel->wheelFrm);
		WMSetButtonSelected(panel->wheelBtn, True);
		if (panel->lastChanged != WMWheelModeColorPanel)
			wheelInit(panel);
		wheelRender(panel);
		wheelPaint(panel);
	} else if (mode == WMGrayModeColorPanel) {
		WMMapWidget(panel->slidersFrm);
		WMSetButtonSelected(panel->slidersBtn, True);
		WMMapWidget(panel->grayFrm);
		WMSetButtonSelected(panel->grayBtn, True);
		WMSetButtonImage(panel->slidersBtn, scr->grayIcon);
		WMSetButtonAltImage(panel->slidersBtn,scr->grayAltIcon);
		if (panel->lastChanged != WMGrayModeColorPanel)
			grayInit(panel);
	} else if (mode == WMRGBModeColorPanel) {
		WMMapWidget(panel->slidersFrm);
		WMSetButtonSelected(panel->slidersBtn, True);
		WMMapWidget(panel->rgbFrm);
		WMSetButtonSelected(panel->rgbBtn, True);
		WMSetButtonImage(panel->slidersBtn, scr->rgbIcon);
		WMSetButtonAltImage(panel->slidersBtn, scr->rgbAltIcon);
		if (panel->lastChanged != WMRGBModeColorPanel)
			rgbInit(panel);
	} else if (mode == WMCMYKModeColorPanel) {
		WMMapWidget(panel->slidersFrm);
		WMSetButtonSelected(panel->slidersBtn, True);
		WMMapWidget(panel->cmykFrm);
		WMSetButtonSelected(panel->cmykBtn, True);
		WMSetButtonImage(panel->slidersBtn, scr->cmykIcon);
		WMSetButtonAltImage(panel->slidersBtn, scr->cmykAltIcon);
		if (panel->lastChanged != WMCMYKModeColorPanel)
			cmykInit(panel);
	} else if (mode == WMHSBModeColorPanel) {
		WMMapWidget(panel->slidersFrm);
		WMSetButtonSelected(panel->slidersBtn, True);
		WMMapWidget(panel->hsbFrm);
		WMSetButtonSelected(panel->hsbBtn, True);
		WMSetButtonImage(panel->slidersBtn, scr->hsbIcon);
		WMSetButtonAltImage(panel->slidersBtn, scr->hsbAltIcon);
		if (panel->lastChanged != WMHSBModeColorPanel)
			hsbInit(panel);
	} else if (mode == WMCustomPaletteModeColorPanel) {
		WMMapWidget(panel->customPaletteFrm);
		WMSetButtonSelected(panel->customPaletteBtn, True);
		customSetPalette(panel);
	} else if (mode == WMColorListModeColorPanel) {
		WMMapWidget(panel->colorListFrm);
		WMSetButtonSelected(panel->colorListBtn, True);
	}
	

	panel->mode = mode;
}

void
WMSetColorPanelColor(WMColorPanel *panel, RColor color)
{
	WMScreen	*scr = WMWidgetScreen(panel->win);
	WMColor 	*wellcolor;

	wellcolor = WMCreateRGBColor(scr, color.red << 8, color.green << 8, color.blue << 8, True);
	WMSetColorWellColor(panel->colorWell, wellcolor);
	WMReleaseColor(wellcolor);
	
	panel->color = color;
	if (panel->lastChanged != WMHSBModeColorPanel)	/* If color was set by HSB-Panel do not convert back to hsv */
		RRGBtoHSV(&panel->color, &panel->hsvcolor);
}

static void
modeButtonCallback(WMWidget *w, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)(data);
	
	if (w == panel->wheelBtn)
		WMSetPickerMode(panel, WMWheelModeColorPanel);
	else if (w == panel->slidersBtn)
		WMSetPickerMode(panel, panel->slidersmode);
	else if (w == panel->customPaletteBtn)
		WMSetPickerMode(panel, WMCustomPaletteModeColorPanel);
	else if (w == panel->colorListBtn)
		WMSetPickerMode(panel, WMColorListModeColorPanel);
	else if (w == panel->grayBtn)
		WMSetPickerMode(panel, WMGrayModeColorPanel);
	else if (w == panel->rgbBtn)
		WMSetPickerMode(panel, WMRGBModeColorPanel);
	else if (w == panel->cmykBtn)
		WMSetPickerMode(panel, WMCMYKModeColorPanel);
	else if (w == panel->hsbBtn)
		WMSetPickerMode(panel, WMHSBModeColorPanel);
}

/******************  WheelMatrix Functions  ************************/

static wheelMatrix*
wheelCreateMatrix(unsigned int width, unsigned int height)
{
	wheelMatrix	*matrix = NULL;
	int	i;
	
	assert((width > 0) && (height > 0));
	
	matrix = malloc(sizeof(wheelMatrix));
	if (!matrix) {
		RErrorCode = RERR_NOMEMORY;
		return NULL;
	}
	
	memset(matrix, 0, sizeof(wheelMatrix));
	matrix->width = width;
	matrix->height = height;
	for (i = 0; i < 3; i++) {
		matrix->data[i] = malloc(width*height*sizeof(unsigned char));
		if (!matrix->data[i])
			goto error;
	}
	
	return matrix;
	
error:
	for (i = 0; i < 3; i++) {
		if (matrix->data[i])
			free(matrix->data[i]);
	}
	if (matrix)
		free(matrix);
	RErrorCode = RERR_NOMEMORY;
	return NULL;
}


static void
wheelDestroyMatrix(wheelMatrix *matrix)
{
	int i;
	
	assert (matrix!=NULL);
	
	for (i = 0; i < 3; i++) {
		if (matrix->data[i])
			free(matrix->data[i]);
	}
	free(matrix);
}


static wheelMatrix*
wheelInitMatrix(W_ColorPanel *panel)
{
	int				i;
	int				x,y;
	wheelMatrix		*matrix;
	unsigned char	*rp, *gp, *bp;
	RHSVColor		cur_hsv;
	RColor			cur_rgb;
	long	ofs[4];
	float			hue;
	int			sat;
	float			xcor, ycor;
	float			W_PI = 4 *atan(1);
	int				dhue[4];
	
	matrix = wheelCreateMatrix(colorWheelSize+4, colorWheelSize+4);
	if (!matrix)
		return NULL;
	
	RRGBtoHSV(&panel->color, &cur_hsv);
	
	for (i = 0; i < 256; i++)
		matrix->values[i] = (unsigned char)(rint(i*cur_hsv.value/255));
	
	cur_hsv.value = 255;
	
	ofs[0] = -1;
	ofs[1] = -(colorWheelSize + 4);
/*	ofs[2] = 0;		superfluous
	ofs[3] = 0;
*/
	for (y = 0; y < (colorWheelSize+4)/2; y++) {
		for (x = y; x < (colorWheelSize+4-y); x++) {
			xcor = ((float)(2.0*x - 4.0) / colorWheelSize) - 1;
			ycor = ((float)(2.0*y - 4.0) / colorWheelSize) - 1;

			sat = rint(255.0 * sqrt((xcor*xcor) + (ycor*ycor)));



			/* offsets are counterclockwise (in triangles) */
			if (y < (colorWheelSize+4)/2)
				ofs[0]++;
									/* top quarter */
			ofs[1] += colorWheelSize + 4;			/* left quarter */


			ofs[2] = (colorWheelSize + 4) * (colorWheelSize + 4) - 1 - ofs[0];	/* bottom quarter */
			ofs[3] = (colorWheelSize + 4) * (colorWheelSize + 4) - 1 - ofs[1];							/* right quarter */

			if (sat < 256) {
				cur_hsv.saturation = (unsigned char)sat;
				
				if (xcor != 0)
					hue = atan(ycor/xcor);
				else {
					if (ycor > 0)
						hue = W_PI * 0.5;
					else
						hue = W_PI * 1.5;
				}
				
				if (xcor < 0)
					hue += W_PI;
				if ((xcor > 0) && (ycor < 0))
					hue += W_PI * 2.0;
				
				hue = -hue;		/* Reverse direction of ColorWheel */
				
				if (hue < 0)
					hue += W_PI * 2.0;
				
				dhue[0] = (hue*360) / (W_PI * 2.0);
				
				for (i = 0; i < 4; i++) {

					if (i > 0)
						dhue[i] = (dhue[i-1] + 90) % 360;
				
					if ((i == 1) || (i == 3))
						dhue[i] = 360 - dhue[i];

					if (i == 2)
						dhue[i] = 360 - dhue[i] + 180;
	
					rp = matrix->data[0] + (ofs[i] * sizeof(unsigned char));
					gp = matrix->data[1] + (ofs[i] * sizeof(unsigned char));
					bp = matrix->data[2] + (ofs[i] * sizeof(unsigned char));
					
					cur_hsv.hue = dhue[i];
					RHSVtoRGB(&cur_hsv, &cur_rgb);

					*rp = (unsigned char)(cur_rgb.red);
					*gp = (unsigned char)(cur_rgb.green);
					*bp = (unsigned char)(cur_rgb.blue);
				}
			}
			else {
				for (i = 0; i < 4; i++) {
					rp = matrix->data[0] + (ofs[i] * sizeof(unsigned char));
					gp = matrix->data[1] + (ofs[i] * sizeof(unsigned char));
					bp = matrix->data[2] + (ofs[i] * sizeof(unsigned char));
					
					*rp = (unsigned char)(0);
					*gp = (unsigned char)(0);
					*bp = (unsigned char)(0);
				}
			}
		}
		if (y < (colorWheelSize+4)/2)
			ofs[0] += 2*y+1;

		ofs[1] += 1 - (colorWheelSize + 4) * (colorWheelSize + 4 - 1 - 2*y);
	}
	
	return matrix;
}

/****************** ColorWheel Functions *******************/

static void
wheelRender(W_ColorPanel *panel)
{
	W_Screen		*scr = WMWidgetScreen(panel->win);
	int				x,y;
	RImage			*image;
	unsigned char	*rp, *gp, *bp;
	RColor			gray;
	unsigned long	ofs;
	
	image = RCreateImage(colorWheelSize+4, colorWheelSize+4, False);
	if (!image)
		return;
	
	gray.red = gray.green = gray.blue = 0xaa;
	
	for (x = 0; x < colorWheelSize+4; x++) {
		for (y = 0; y < colorWheelSize+4; y++) {
			
			ofs = (y * image->width) + x;
			rp = image->data[0] + ofs;
			gp = image->data[1] + ofs;
			bp = image->data[2] + ofs;
	
			if (wheelInsideColorWheel(panel, ofs)) {
				*rp = (unsigned int)(panel->wheelMtrx->values[ panel->wheelMtrx->data[0][ofs] ]);
				*gp = (unsigned int)(panel->wheelMtrx->values[ panel->wheelMtrx->data[1][ofs] ]);
				*bp = (unsigned int)(panel->wheelMtrx->values[ panel->wheelMtrx->data[2][ofs] ]);
			}
			else {
				*rp = (unsigned char)(gray.red);
				*gp = (unsigned char)(gray.green);
				*bp = (unsigned char)(gray.blue);
			}
		}
	}
	
	if (panel->wheelImg)
		XFreePixmap(scr->display, panel->wheelImg);
	
	panel->wheelImg = XCreatePixmap(scr->display, W_DRAWABLE(scr), colorWheelSize+4, colorWheelSize+4, scr->depth);
	RConvertImage(scr->rcontext, image, &panel->wheelImg);

	/* Check backimage existence. If it doesn't exist, allocate it and fill it */
	if (!panel->selectionBackImg) {
		panel->selectionBackImg = XCreatePixmap(scr->display, W_VIEW(panel->wheelFrm)->window, 4, 4, scr->depth);
		XCopyArea(scr->display, panel->wheelImg, panel->selectionBackImg, scr->copyGC, panel->colx -2, panel->coly -2, 4, 4, 0, 0); /* -2 is for hot spot correction */
	}

	RDestroyImage(image);
}

static Bool
wheelInsideColorWheel(W_ColorPanel *panel, unsigned long ofs)
{
	if ((panel->wheelMtrx->data[0][ofs] != 0) &&
			(panel->wheelMtrx->data[1][ofs] != 0) &&
			(panel->wheelMtrx->data[2][ofs] != 0))
		return True;
	return False;
}

static void
wheelPaint (W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	
	XCopyArea(scr->display, panel->wheelImg, panel->wheelView->window, scr->copyGC, 0, 0, colorWheelSize+4, colorWheelSize+4, 0, 0);

	/* Draw selection image */
	XCopyArea(scr->display, panel->selectionImg, panel->wheelView->window, scr->copyGC, 0 , 0, 4, 4, panel->colx -2, panel->coly -2);
}

static void
wheelHandleEvents(XEvent *event, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)data;

	switch (event->type) {
		case Expose:
			if (event->xexpose.count != 0)
				break;
			wheelPaint(panel);
			break;
	}
}

static void
wheelHandleActionEvents(XEvent *event, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)data;
	
	switch (event->type) {
		case ButtonPress:
			if (getPickerPart(panel, event->xbutton.x, event->xbutton.y) == COLORWHEEL_PART) {
				panel->flags.dragging = 1;
				wheelPositionSelection(panel, event->xbutton.x, event->xbutton.y);
			}
			break;
			
		case ButtonRelease:
			panel->flags.dragging = 0;
			break;
			
		case MotionNotify:
			if (panel->flags.dragging) {
				if (getPickerPart(panel, event->xmotion.x, event->xmotion.y) == COLORWHEEL_PART) {
					wheelPositionSelection(panel, event->xmotion.x, event->xmotion.y);
				}
				else
					wheelPositionSelectionOutBounds(panel, event->xmotion.x, event->xmotion.y);
			}
			break;
	}
}


static int
getPickerPart(W_ColorPanel *panel, int x, int y)
{
	int				lx, ly;
	unsigned long	ofs;
	
	lx = x;
	ly = y;
	
	if (panel->mode == WMWheelModeColorPanel) {
		if ((lx >= 2) && (lx <= 2+colorWheelSize) && (ly >= 2) && (ly <= 2+colorWheelSize)) {
			ofs = ly*panel->wheelMtrx->width+lx;
		
			if (wheelInsideColorWheel(panel, ofs))
				return COLORWHEEL_PART;
		}
	}

	if (panel->mode == WMCustomPaletteModeColorPanel) {
		if ((lx >= 2) && (lx < customPaletteWidth-2) && (ly >= 2) && (ly < customPaletteHeight-2)) {
			return CUSTOMPALETTE_PART;
		}
	}
	
	return 0;
}




static void
wheelBrightnessSliderCallback(WMWidget *w, void *data)
{
	int	i;
	unsigned int	v;
	int	value;
	unsigned long	ofs;
	RColor			cur_rgb;
	
	W_ColorPanel *panel = (W_ColorPanel*)data;
	
	value = 255-WMGetSliderValue(panel->wheelBrightnessS);
	
	for (i = 0; i < 256; i++) {
		/* We divide by 128 in advance, and check whether that number divides
		 * by 2 properly. If not, we add one to round the number correctly
		 */
		v = (i*value) >> 7;
#ifdef EASTEREGG
		panel->wheelMtrx->values[i] = (unsigned char)((v >> 1) + v);
#else
		panel->wheelMtrx->values[i] = (unsigned char)((v >> 1) +(v & 0x01));
#endif
	}
	
	ofs = (panel->coly * panel->wheelMtrx->width) + panel->colx;
	
	if (!wheelInsideColorWheel(panel, ofs)) {
		panel->hsvcolor.saturation = 255;
		panel->hsvcolor.value = value;
		RHSVtoRGB(&panel->hsvcolor, &cur_rgb);
	
		panel->color = cur_rgb;
	}
	else {
		panel->color.red	= panel->wheelMtrx->values[ panel->wheelMtrx->data[0][ofs] ];
		panel->color.green	= panel->wheelMtrx->values[ panel->wheelMtrx->data[1][ofs] ];
		panel->color.blue	= panel->wheelMtrx->values[ panel->wheelMtrx->data[2][ofs] ];
	}

	wheelRender(panel);
	wheelPaint(panel);
	wheelUpdateSelection(panel);
}

static void
wheelUpdateSelection(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	WMSetColorPanelColor(panel, panel->color);
	panel->lastChanged = WMWheelModeColorPanel;

	/* Redraw color selector (and make a backup of the part it will cover) */
	XCopyArea(scr->display, panel->wheelImg, panel->selectionBackImg, scr->copyGC, panel->colx -2, panel->coly -2, 4, 4, 0, 0);	/* "-2" is correction for hotspot location */
	XCopyArea(scr->display, panel->selectionImg, panel->wheelView->window, scr->copyGC, 0, 0, 4, 4, panel->colx -2, panel->coly -2);	/* see above */
}

static void
wheelUndrawSelection(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);

	XCopyArea(scr->display, panel->selectionBackImg, panel->wheelView->window, scr->copyGC, 0, 0, 4, 4, panel->colx -2, panel->coly -2);	/* see above */
}

static void
wheelPositionSelection(W_ColorPanel *panel, int x, int y)
{
	unsigned long	ofs = (y * panel->wheelMtrx->width)+ x;
	
	
	panel->color.red	= panel->wheelMtrx->values[ panel->wheelMtrx->data[0][ofs] ];
	panel->color.green	= panel->wheelMtrx->values[ panel->wheelMtrx->data[1][ofs] ];
	panel->color.blue	= panel->wheelMtrx->values[ panel->wheelMtrx->data[2][ofs] ];
	
	wheelUndrawSelection(panel);
	
	panel->colx = x;
	panel->coly = y;
	
	wheelUpdateSelection(panel);
	wheelUpdateBrightnessGradientFromLocation(panel);
}

static void
wheelPositionSelectionOutBounds(W_ColorPanel *panel, int x, int y)
{
	RHSVColor	cur_hsv;
	float		hue;
	float		xcor, ycor;
	float		W_PI = 4*atan(1);
	
	xcor = ((x*2.0) / (colorWheelSize+4)) - 1.0;
	ycor = ((y*2.0) / (colorWheelSize+4)) - 1.0;
	
	cur_hsv.saturation = 255;
	cur_hsv.value = 255 - WMGetSliderValue(panel->wheelBrightnessS);
	
	if (xcor != 0)
		hue = atan(ycor/xcor);
	else {
		if (ycor > 0)
			hue = W_PI * 0.5;
		else
			hue = W_PI * 1.5;
	}
	
	if (xcor < 0)
		hue += W_PI;
	if ((xcor > 0) && (ycor < 0))
		hue += W_PI * 2.0;
	
	hue = -hue;
	
	if (hue < 0)
		hue += W_PI * 2.0;
	
	cur_hsv.hue = (hue*360.0)/(W_PI*2.0);
	RHSVtoRGB(&cur_hsv, &panel->color);
	
	wheelUndrawSelection(panel);
	
	panel->colx = 2 + rint((colorWheelSize * (1.0 + cos( cur_hsv.hue*W_PI/180))) /2.0); /* "+2" because of "colorWheelSize + 4" */
	panel->coly = 2 + rint((colorWheelSize * (1.0 + sin(-cur_hsv.hue*W_PI/180))) /2.0);
	
	wheelUpdateSelection(panel);
	wheelUpdateBrightnessGradientFromHSV(panel, cur_hsv);
}

static void
wheelUpdateBrightnessGradientFromHSV(W_ColorPanel *panel, RHSVColor topColor)
{
	RColor	from;
	
	/* Update Brightness-Slider */
	topColor.value = 255;
	RHSVtoRGB(&topColor, &from);
	
	wheelUpdateBrightnessGradient(panel, from);
}

static void
wheelUpdateBrightnessGradientFromLocation(W_ColorPanel *panel)
{
	RColor	from;
	unsigned long	ofs;
	
	ofs = panel->coly * panel->wheelMtrx->width + panel->colx;
	
	from.red	= panel->wheelMtrx->data[0][ofs];
	from.green	= panel->wheelMtrx->data[1][ofs];
	from.blue	= panel->wheelMtrx->data[2][ofs];
	
	wheelUpdateBrightnessGradient(panel, from);
}

static void
wheelUpdateBrightnessGradient(W_ColorPanel *panel, RColor topColor)
{
	RColor		to;
	RImage		*sliderImg;
	WMPixmap	*sliderPxmp;
	
	to.red = to.green = to.blue = 0;
	
	sliderImg = RRenderGradient(16, 153, &topColor, &to, RGRD_VERTICAL);
	sliderPxmp = WMCreatePixmapFromRImage(WMWidgetScreen(panel->win), sliderImg, 0);
	RDestroyImage(sliderImg);
	WMSetSliderImage(panel->wheelBrightnessS, sliderPxmp);
	WMReleasePixmap(sliderPxmp);
}

/****************** Grayscale Panel Functions ***************/

static void
grayBrightnessSliderCallback(WMWidget *w, void *data)
{
	RColor	color;
	int		value;
	char	tmp[3];
	
	W_ColorPanel *panel = (W_ColorPanel*)data;
	
	value = WMGetSliderValue(panel->grayBrightnessS);
	
	sprintf(tmp, "%d", value);
	
	WMSetTextFieldText(panel->grayBrightnessT, tmp);
	color.red = color.green = color.blue = rint(2.55*value);
	
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMGrayModeColorPanel;
}

static void
grayPresetButtonCallback(WMWidget *w, void *data)
{
	RColor	color;
	char	tmp[3];
	int		value;
	int		i=0;
	
	W_ColorPanel *panel = (W_ColorPanel*)data;

	while (i < 7) {
		if (w == panel->grayPresetBtn[i])
			break;
		i++;
	}
	
	value = rint(100.0/6.0*i);
	sprintf(tmp, "%d", value);
	
	WMSetTextFieldText(panel->grayBrightnessT, tmp);
	color.red = color.green = color.blue = rint(255.0*i/6.0);
	
	WMSetSliderValue(panel->grayBrightnessS, rint(100.0*i/6.0));
	
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMGrayModeColorPanel;
}

static void
grayBrightnessTextFieldCallback(void *observerData, WMNotification *notification)
{
	RColor	color;
	char	tmp[3];
	int		value;
	W_ColorPanel	*panel = (W_ColorPanel*)observerData;

	value = atoi(WMGetTextFieldText(panel->grayBrightnessT));
	if (value > 100)
		value = 100;
	if (value < 0)
		value = 0;

	sprintf(tmp, "%d", value);
	WMSetTextFieldText(panel->grayBrightnessT, tmp);
	WMSetSliderValue(panel->grayBrightnessS, value);

	color.red = color.green = color.blue = rint(255.0*value/100.0);
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMGrayModeColorPanel;
}

/******************* RGB Panel Functions *****************/

static void
rgbSliderCallback(WMWidget *w, void *data)
{
	RColor	color;
	int		value[3];
	char	tmp[3];
	
	W_ColorPanel *panel = (W_ColorPanel*)data;

	value[0] = WMGetSliderValue(panel->rgbRedS);
	value[1] = WMGetSliderValue(panel->rgbGreenS);
	value[2] = WMGetSliderValue(panel->rgbBlueS);
	
	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->rgbRedT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->rgbGreenT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->rgbBlueT, tmp);

	color.red = value[0];
	color.green = value[1];
	color.blue = value[2];
	
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMRGBModeColorPanel;
}

static void
rgbTextFieldCallback(void *observerData, WMNotification *notification)
{
	RColor	color;
	char	tmp[3];
	int		value[3];
	int		n;
	W_ColorPanel	*panel = (W_ColorPanel*)observerData;

	value[0] = atoi(WMGetTextFieldText(panel->rgbRedT));
	value[1] = atoi(WMGetTextFieldText(panel->rgbGreenT));
	value[2] = atoi(WMGetTextFieldText(panel->rgbBlueT));

	for (n=0; n < 3; n++) {
		if (value[n] > 255)
			value[n] = 255;
		if (value[n] < 0)
			value[n] = 0;
	}

	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->rgbRedT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->rgbGreenT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->rgbBlueT, tmp);

	WMSetSliderValue(panel->rgbRedS, value[0]);
	WMSetSliderValue(panel->rgbGreenS, value[1]);
	WMSetSliderValue(panel->rgbBlueS, value[2]);
	
	color.red = value[0];
	color.green = value[1];
	color.blue = value[2];
	
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMRGBModeColorPanel;
}


/******************* CMYK Panel Functions *****************/

static void
cmykSliderCallback(WMWidget *w, void *data)
{
	RColor	color;
	int		value[4];
	char	tmp[3];
	
	W_ColorPanel *panel = (W_ColorPanel*)data;

	value[0] = WMGetSliderValue(panel->cmykCyanS);
	value[1] = WMGetSliderValue(panel->cmykMagentaS);
	value[2] = WMGetSliderValue(panel->cmykYellowS);
	value[3] = WMGetSliderValue(panel->cmykBlackS);
	
	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->cmykCyanT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->cmykMagentaT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->cmykYellowT, tmp);
	sprintf(tmp, "%d", value[3]);
	WMSetTextFieldText(panel->cmykBlackT, tmp);

	color.red = rint((255.0 - (value[0] * 2.55)) * (1.0 - (value[3] / 100.0)));
	color.green = rint((255.0 - (value[1] * 2.55)) * (1.0 - (value[3] / 100.0)));
	color.blue = rint((255.0 - (value[2] * 2.55)) * (1.0 - (value[3] / 100.0)));
			
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMCMYKModeColorPanel;
}

static void
cmykTextFieldCallback(void *observerData, WMNotification *notification)
{
	RColor	color;
	char	tmp[3];
	int		value[4];
	int		n;
	W_ColorPanel	*panel = (W_ColorPanel*)observerData;

	value[0] = atoi(WMGetTextFieldText(panel->cmykCyanT));
	value[1] = atoi(WMGetTextFieldText(panel->cmykMagentaT));
	value[2] = atoi(WMGetTextFieldText(panel->cmykYellowT));
	value[3] = atoi(WMGetTextFieldText(panel->cmykBlackT));

	for (n=0; n < 4; n++) {
		if (value[n] > 100)
			value[n] = 100;
		if (value[n] < 0)
			value[n] = 0;
	}

	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->cmykCyanT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->cmykMagentaT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->cmykYellowT, tmp);
	sprintf(tmp, "%d", value[3]);
	WMSetTextFieldText(panel->cmykBlackT, tmp);

	WMSetSliderValue(panel->cmykCyanS, value[0]);
	WMSetSliderValue(panel->cmykMagentaS, value[1]);
	WMSetSliderValue(panel->cmykYellowS, value[2]);
	WMSetSliderValue(panel->cmykBlackS, value[3]);
	
	color.red = rint((255.0 - (value[0] * 2.55)) * (1.0 - (value[3] / 100.0)));
	color.green = rint((255.0 - (value[1] * 2.55)) * (1.0 - (value[3] / 100.0)));
	color.blue = rint((255.0 - (value[2] * 2.55)) * (1.0 - (value[3] / 100.0)));
	
	WMSetColorPanelColor(panel, color);
	panel->lastChanged = WMCMYKModeColorPanel;
}

/********************** HSB Panel Functions ***********************/

static void
hsbSliderCallback(WMWidget *w, void *data)
{
	RColor	color;
	int		value[3];
	char	tmp[3];
	
	W_ColorPanel *panel = (W_ColorPanel*)data;

	value[0] = WMGetSliderValue(panel->hsbHueS);
	value[1] = WMGetSliderValue(panel->hsbSaturationS);
	value[2] = WMGetSliderValue(panel->hsbBrightnessS);
	
	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->hsbHueT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->hsbSaturationT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->hsbBrightnessT, tmp);

	panel->hsvcolor.hue = value[0];
	panel->hsvcolor.saturation = value[1]*2.55;
	panel->hsvcolor.value = value[2]*2.55;
	
	RHSVtoRGB(&panel->hsvcolor, &color);

	panel->lastChanged = WMHSBModeColorPanel;
	WMSetColorPanelColor(panel, color);

	if (w != panel->hsbBrightnessS)
		hsbUpdateBrightnessGradient(panel);
	if (w != panel->hsbSaturationS)
		hsbUpdateSaturationGradient(panel);
	if (w != panel->hsbHueS)
		hsbUpdateHueGradient(panel);
}

static void
hsbTextFieldCallback(void *observerData, WMNotification *notification)
{
	RColor	color;
	char	tmp[3];
	int		value[3];
	int		n;
	W_ColorPanel	*panel = (W_ColorPanel*)observerData;

	value[0] = atoi(WMGetTextFieldText(panel->hsbHueT));
	value[1] = atoi(WMGetTextFieldText(panel->hsbSaturationT));
	value[2] = atoi(WMGetTextFieldText(panel->hsbBrightnessT));
	
	if (value[0] > 359)
		value[0] = 359;
	if (value[0] < 0)
	   value[0] = 0;

	for (n=1; n < 3; n++) {
		if (value[n] > 100)
			value[n] = 100;
		if (value[n] < 0)
			value[n] = 0;
	}

	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->hsbHueT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->hsbSaturationT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->hsbBrightnessT, tmp);

	WMSetSliderValue(panel->hsbHueS, value[0]);
	WMSetSliderValue(panel->hsbSaturationS, value[1]);
	WMSetSliderValue(panel->hsbBrightnessS, value[2]);

	panel->hsvcolor.hue = value[0];
	panel->hsvcolor.saturation = value[1]*2.55;
	panel->hsvcolor.value = value[2]*2.55;
	
	RHSVtoRGB(&panel->hsvcolor, &color);

	panel->lastChanged = WMHSBModeColorPanel;
	WMSetColorPanelColor(panel, color);
	
	hsbUpdateBrightnessGradient(panel);
	hsbUpdateSaturationGradient(panel);
	hsbUpdateHueGradient(panel);
}

static void
hsbUpdateBrightnessGradient(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	RColor		from;
	RColor		to;
	RHSVColor	hsvcolor;
	RImage		*sliderImg;
	WMPixmap	*sliderPxmp;
	
	from.red = from.green = from.blue = 0;
	hsvcolor = panel->hsvcolor;
	hsvcolor.value = 255;
	
	RHSVtoRGB(&hsvcolor, &to);
	
	sliderImg = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
	sliderPxmp = WMCreatePixmapFromRImage(scr, sliderImg, 0);
	RDestroyImage(sliderImg);
	W_PaintText(W_VIEW(panel->hsbBrightnessS), sliderPxmp->pixmap, WMSystemFontOfSize(scr, 12), 2, 0, 100, WALeft, W_GC(scr->white), False, "Brightness", strlen("Brightness"));
	WMSetSliderImage(panel->hsbBrightnessS, sliderPxmp);
	WMReleasePixmap(sliderPxmp);
}

static void
hsbUpdateSaturationGradient(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	RColor		from;
	RColor		to;
	RHSVColor	hsvcolor;
	RImage		*sliderImg;
	WMPixmap	*sliderPxmp;
	
	hsvcolor = panel->hsvcolor;
	hsvcolor.saturation = 0;
	RHSVtoRGB(&hsvcolor, &from);
	
	hsvcolor.saturation = 255;
	RHSVtoRGB(&hsvcolor, &to);
	
	sliderImg = RRenderGradient(141, 16, &from, &to, RGRD_HORIZONTAL);
	sliderPxmp = WMCreatePixmapFromRImage(scr, sliderImg, 0);
	RDestroyImage(sliderImg);
	if (hsvcolor.value < 128)
		W_PaintText(W_VIEW(panel->hsbSaturationS), sliderPxmp->pixmap, WMSystemFontOfSize(scr, 12), 2, 0, 100, WALeft, W_GC(scr->white), False, "Saturation", strlen("Saturation"));
	else
		W_PaintText(W_VIEW(panel->hsbSaturationS), sliderPxmp->pixmap, WMSystemFontOfSize(scr, 12), 2, 0, 100, WALeft, W_GC(scr->black), False, "Saturation", strlen("Saturation"));

	WMSetSliderImage(panel->hsbSaturationS, sliderPxmp);
	WMReleasePixmap(sliderPxmp);
}

static void
hsbUpdateHueGradient(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	RColor		**colors = NULL;
	RHSVColor	hsvcolor;
	RImage		*sliderImg;
	WMPixmap	*sliderPxmp;
	int			i;
	
	hsvcolor = panel->hsvcolor;
	
	colors = malloc(sizeof(RColor*)*(8));
	for (i=0; i<7; i++) {
		hsvcolor.hue = (360*i)/6;
		colors[i] = malloc(sizeof(RColor));
		RHSVtoRGB(&hsvcolor, colors[i]);
	}
	colors[7] = NULL;
	
	sliderImg = RRenderMultiGradient(141, 16, colors, RGRD_HORIZONTAL);
	sliderPxmp = WMCreatePixmapFromRImage(scr, sliderImg, 0);
	RDestroyImage(sliderImg);
	if (hsvcolor.value < 128)
		W_PaintText(W_VIEW(panel->hsbHueS), sliderPxmp->pixmap, WMSystemFontOfSize(scr, 12), 2, 0, 100, WALeft, W_GC(scr->white), False, "Hue", strlen("Hue"));
	else
		W_PaintText(W_VIEW(panel->hsbHueS), sliderPxmp->pixmap, WMSystemFontOfSize(scr, 12), 2, 0, 100, WALeft, W_GC(scr->black), False, "Hue", strlen("Hue"));

	WMSetSliderImage(panel->hsbHueS, sliderPxmp);
	WMReleasePixmap(sliderPxmp);

	for (i=0; i<7; i++) {
		if (colors[i])
			free(colors[i]);
	}
	if (colors)
		free(colors);
}

/*************** Custom Palette Functions ****************/

static void
customRenderSpectrum(W_ColorPanel *panel)
{
	RImage	*spectrum;
	int		hue, sat, val;
	int		x,y;
	unsigned long	ofs;
	unsigned char *rp, *gp, *bp;
	RColor	color;
	RHSVColor	cur_hsv;

	spectrum = RCreateImage(SPECTRUM_WIDTH, SPECTRUM_HEIGHT, 0);

	for (y=0; y<360; y++) {
		val = 255;
		sat = 0;
		hue = y;
		for (x=0; x<511; x++) {
			ofs = (y * 511) + x;

			cur_hsv.hue = hue;
			cur_hsv.saturation = sat;
			cur_hsv.value = val;

			RHSVtoRGB (&cur_hsv, &color);

			rp = spectrum->data[0] + ofs;
			gp = spectrum->data[1] + ofs;
			bp = spectrum->data[2] + ofs;

			*rp = (unsigned char)color.red;
			*gp = (unsigned char)color.green;
			*bp = (unsigned char)color.blue;

			if (x<255)
				sat++;

			if (x>255)
				val--;
		}
	}
	if (panel->customPaletteImg)	{
		RDestroyImage(panel->customPaletteImg);
		panel->customPaletteImg = NULL;
	}
	panel->customPaletteImg = spectrum;
}



static void
customSetPalette(W_ColorPanel *panel)
{
   W_Screen    *scr = WMWidgetScreen(panel->win);
	RImage		*scaledImg;
	Pixmap		image;
	int			item;

	image = XCreatePixmap(scr->display, W_DRAWABLE(scr), customPaletteWidth, customPaletteHeight, scr->depth);

	scaledImg = RScaleImage(panel->customPaletteImg, customPaletteWidth, customPaletteHeight);
	RConvertImage(scr->rcontext, scaledImg, &image);
	RDestroyImage(scaledImg);

	XCopyArea(scr->display, image, panel->customPaletteContentView->window, scr->copyGC, 0, 0, customPaletteWidth, customPaletteHeight, 0, 0);

	/* Check backimage existence. If it doesn't exist, allocate it and fill it */
	if (!panel->selectionBackImg) {
		panel->selectionBackImg = XCreatePixmap(scr->display, panel->customPaletteContentView->window, 4, 4, scr->depth);
	}

	XCopyArea(scr->display, image, panel->selectionBackImg, scr->copyGC, panel->palx-2, panel->paly-2, 4, 4, 0, 0); 	
	XCopyArea(scr->display, panel->selectionImg, panel->customPaletteContentView->window, scr->copyGC, 0 , 0, 4, 4, panel->palx-2, panel->paly-2);
	XFreePixmap(scr->display, image);

	panel->palXRatio = (float)(panel->customPaletteImg->width) / (float)(customPaletteWidth);
	panel->palYRatio = (float)(panel->customPaletteImg->height) / (float)(customPaletteHeight);

	item = WMGetPopUpButtonSelectedItem (panel->customPaletteHistoryBtn);

	/* if palette != "Spectrum", we are allowed to rename and remove it */	
	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, menuRename, (item > 0) );
	WMSetPopUpButtonItemEnabled(panel->customPaletteMenuBtn, menuRemove, (item > 0) );
}


static void
customPalettePositionSelection(W_ColorPanel *panel, int x, int y)
{
	W_Screen *scr = WMWidgetScreen(panel->win);
	unsigned long		ofs;


	/* undraw selection */
	XCopyArea(scr->display, panel->selectionBackImg, panel->customPaletteContentView->window, scr->copyGC, 0, 0, 4, 4, panel->palx-2, panel->paly-2);

	panel->palx = x;
	panel->paly = y;

	ofs = rint(x * panel->palXRatio) + rint(y * panel->palYRatio) * panel->customPaletteImg->width;

	panel->color.red   = panel->customPaletteImg->data[0][ofs];
	panel->color.green = panel->customPaletteImg->data[1][ofs];
	panel->color.blue  = panel->customPaletteImg->data[2][ofs];

	WMSetColorPanelColor(panel, panel->color);
	panel->lastChanged = WMCustomPaletteModeColorPanel;

	/* Redraw color selector (and make a backup of the part it will cover) */
	XCopyArea(scr->display, panel->customPaletteContentView->window, panel->selectionBackImg, scr->copyGC, panel->palx-2, panel->paly-2, 4, 4, 0, 0);	/* "-2" is correction for hotspot location */
	XCopyArea(scr->display, panel->selectionImg, panel->customPaletteContentView->window, scr->copyGC, 0, 0, 4, 4, panel->palx-2, panel->paly-2);	/* see above */
}


static void
customPalettePositionSelectionOutBounds(W_ColorPanel *panel, int x, int y)
{
	if (x < 2)
		x = 2;
	if (y < 2)
		y = 2;
	if (x >= customPaletteWidth)
		x = customPaletteWidth -2;
	if (y >= customPaletteHeight)
		y = customPaletteHeight -2;

	customPalettePositionSelection(panel, x, y);
}


static void
customPaletteHandleEvents(XEvent *event, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)data;

	switch (event->type) {
		case Expose:
			if (event->xexpose.count != 0)
				break;
			customSetPalette(panel);
			break;
	}
}

static void
customPaletteHandleActionEvents(XEvent *event, void *data)
{
	W_ColorPanel   *panel = (W_ColorPanel*)data;
	int				x, y;

	switch (event->type) {
		case ButtonPress:
			x = event->xbutton.x;
			y = event->xbutton.y;

			if (getPickerPart(panel, x, y) == CUSTOMPALETTE_PART) {
				panel->flags.dragging = 1;
				customPalettePositionSelection(panel, x, y);
			}
			break;

		case ButtonRelease:
			panel->flags.dragging = 0;
			break;

		case MotionNotify:
			x = event->xmotion.x;
			y = event->xmotion.y;

			if (panel->flags.dragging) {
				if (getPickerPart(panel, x, y) == CUSTOMPALETTE_PART) {
					customPalettePositionSelection(panel, x, y);
				}
				else
					customPalettePositionSelectionOutBounds(panel, x, y);
			}
			break;
	}
}


static void
customPaletteMenuCallback(WMWidget *w, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)data;
	int	item;

	item = WMGetPopUpButtonSelectedItem(panel->customPaletteMenuBtn);

	switch (item) {
		case menuNewFromFile:
			customPaletteMenuNewFromFile(panel);
			break;
		case menuRename:
			customPaletteMenuRename(panel);
			break;
		case menuRemove:
			customPaletteMenuRemove(panel);
			break;
		case menuCopy:
			break;
		case menuNewFromClipboard:
			break;
	}
}


static void
customPaletteMenuNewFromFile(W_ColorPanel *panel)
{
	W_Screen			*scr = WMWidgetScreen(panel->win);
	WMOpenPanel			*browseP;
	char				*filepath;
	char				*filename = NULL;
	char				path[PATH_MAX] = "\0";
	char				*spath;
	char				*tmp;
	int				i;
	RImage			*tmpImg = NULL;

	if ((!panel->lastBrowseDir) || (strcmp(panel->lastBrowseDir,"\0") == 0)) {
		strcpy(path, wgethomedir());
		spath = wmalloc(strlen(path)+2);
		strcpy(spath, path);
	}
	else {
		spath = wmalloc(strlen(panel->lastBrowseDir)+2);
		strcpy(spath, panel->lastBrowseDir);
	}


	browseP = WMGetOpenPanel(scr);
	WMSetFilePanelCanChooseDirectories(browseP, 0);
	WMSetFilePanelCanChooseFiles(browseP, 1);

	/* Get a filename */
	if (WMRunModalOpenPanelForDirectory(browseP, panel->win, wexpandpath(spath), "Open Palette", RSupportedFileFormats()) ) {
		filepath = WMGetFilePanelFileName(browseP);

		/* Get seperation position between path and filename */ 
		i = strrchr(filepath, '/') - filepath + 1;
		if (i > strlen(filepath))
			i = strlen(filepath);

		/* Store last browsed path */
		if (panel->lastBrowseDir)
			free(panel->lastBrowseDir);
		panel->lastBrowseDir = wmalloc((i+1)*sizeof(char));
		memset(panel->lastBrowseDir, 0, (i+1)*sizeof(char));
		strncpy(panel->lastBrowseDir, filepath, i);

		/* Get filename from path */
		filename = wstrdup(filepath + i);

		/* Check for duplicate files, and rename it if there are any */
		tmp = wstrappend(panel->configurationPath, filename);
		while (access (tmp, F_OK) == 0) {
			free(tmp);
			tmp = generateNewFilename(panel->configurationPath, filename);
			filename = wstrdup(tmp);

			free(tmp);
			tmp = wstrappend(panel->configurationPath, filename);
		}
		free(tmp);

		/* Copy the image to $(gnustepdir)/Library/Colors/ & Add the filename to the history menu */
		if (!fetchImageFile (panel->configurationPath, filepath, filename)) {		/* error = -1 */

			/* filepath is a "local" path now the file has been copied */
			free(filepath);
			filepath = wstrappend(panel->configurationPath, filename);

			/* load the image & add menu entries */
			tmpImg = RLoadImage(scr->rcontext, filepath, 0);
			if (tmpImg) {
				if (panel->customPaletteImg)
					RDestroyImage(panel->customPaletteImg);
				panel->customPaletteImg = tmpImg;

				customSetPalette(panel);
				WMAddPopUpButtonItem(panel->customPaletteHistoryBtn, filename);
				WMSetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn, WMGetPopUpButtonNumberOfItems(panel->customPaletteHistoryBtn)-1);

				panel->currentPalette = WMGetPopUpButtonNumberOfItems(panel->customPaletteHistoryBtn)-1;
			}
		}
		else
		{
			tmp = wstrappend(panel->configurationPath, filename);

			i = remove(tmp);	/* Delete the file, it doesn't belong here */			
			WMRunAlertPanel(scr, panel->win, "File Error", "Invalid file format !", "OK", NULL, NULL);
			if (i != 0)
				WMRunAlertPanel(scr, panel->win, "File Error", "Couldn't remove file from Configuration Directory !", "OK", NULL, NULL);

			free(tmp);
		}
		free(filepath);
		free(filename);
	}
	WMFreeFilePanel(browseP);
}


static void
customPaletteMenuRename(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	char		*toName = NULL;
	char		*fromName;
	char		*toPath, *fromPath;
	int		item;
	int		index;

	item = WMGetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn);
	fromName = WMGetPopUpButtonItem(panel->customPaletteHistoryBtn, item);

	toName = WMRunInputPanel(scr, panel->win, "Rename", "Rename palette to:", 
		fromName, "OK", "Cancel");

	if (toName) {

		/* As some people do certain stupid things... */
		if (strcmp(toName, fromName) == 0) {
			free(toName);
			return;
		}

		/* For normal people */
		fromPath = wstrappend(panel->configurationPath, fromName);
		toPath   = wstrappend(panel->configurationPath, toName);

		if (access (toPath, F_OK) == 0)	{	/* Careful, this palette exists already */
			if (WMRunAlertPanel(scr, panel->win, "Warning", 
					"Palette already exists !\n\nOverwrite ?", "No", "Yes", NULL) == 1) {
				/* "No" = 0, "Yes" = 1 */

				remove(toPath);

				/* Remove from History list too */

				/*
				 * XXX This code is not functional yet !!! (And not really necessary)
				 */

				/*
				index = 1;
				while ((index < WMGetPopUpButtonNumberOfItems(panel->customPaletteHistoryBtn)) &&
						 (strcmp(WMGetPopUpButtonItem(panel->customPaletteHistoryBtn, index), toName) != 0 ))
					index++;

				if (index < WMGetPopUpButtonNumberOfItems(panel->customPaletteHistoryBtn)) {
					WMRemovePopUpButtonItem(panel->customPaletteHistoryBtn, index);
					if (index < item)
						item--;
				}
				*/

			} else {
				free(fromPath);
				free(toName);
				free(toPath);

				return;
			}
		}

		if ( rename(fromPath, toPath) != 0)
			wwarning("Couldn't rename palette %s to %s\n", fromName, toName);
		else {
			WMRemovePopUpButtonItem(panel->customPaletteHistoryBtn, item);
			WMInsertPopUpButtonItem(panel->customPaletteHistoryBtn, item, toName);

			WMSetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn, item);
		}
		free(fromPath);
		free(toPath);
		free(toName);
	}
}


static void
customPaletteMenuRemove(W_ColorPanel *panel)
{
	W_Screen	*scr = WMWidgetScreen(panel->win);
	char		*text;
	char		*tmp;
	int		choice;
	int		item;

	item = WMGetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn);

	tmp = wstrappend( "This will permanently remove the palette ", 
				WMGetPopUpButtonItem(panel->customPaletteHistoryBtn, item ));
	text = wstrappend( tmp, ".\n\nAre you sure you want to remove this palette ?");
	free(tmp);

	choice = WMRunAlertPanel(scr, panel->win, NULL, text, "Yes", "No", NULL);
	/* returns 0 (= "Yes") or 1 (="No") */
	free(text);

	if (choice == 0) {

		tmp = wstrappend(panel->configurationPath, 
				WMGetPopUpButtonItem(panel->customPaletteHistoryBtn, item ));

		if ( remove(tmp) != 0)
			wwarning("Couldn't remove palette %s\n", tmp);
		free(tmp);

		WMSetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn, item-1);	/* item -1 always exists */
		customPaletteHistoryCallback(panel->customPaletteHistoryBtn, panel);
		customSetPalette(panel);

		WMRemovePopUpButtonItem(panel->customPaletteHistoryBtn, item);
	}
}


static void
customPaletteHistoryCallback(WMWidget *w, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel*)data;
	W_Screen			*scr = WMWidgetScreen(panel->win);
	int				item;
	char				*filename;
	RImage			*tmp = NULL;

	item = WMGetPopUpButtonSelectedItem(panel->customPaletteHistoryBtn);
	if (item == panel->currentPalette)
		return;

	if (item == 0)
		customRenderSpectrum(panel);
	else {
		/* Load file from configpath */
		filename = wstrappend( panel->configurationPath, WMGetPopUpButtonItem(panel->customPaletteHistoryBtn, item) );

		/* XXX To do: Check existence of file and remove it from the history if it doesn't exist */

		tmp = RLoadImage(scr->rcontext,  filename, 0);
		if (tmp) {
			if (panel->customPaletteImg) {
				RDestroyImage(panel->customPaletteImg);
				panel->customPaletteImg = NULL;
			}
			panel->customPaletteImg = tmp;
		}
		free(filename);
	}
	customSetPalette(panel);

	panel->currentPalette = item;
}


/*************** Panel Initialisation Functions *****************/

static void
wheelInit(W_ColorPanel *panel)
{
	RHSVColor	cur_hsv;
	float		W_PI = 4 * atan(1);
	int			i;
	int			v;

	RRGBtoHSV(&panel->color, &cur_hsv);
	
	WMSetSliderValue(panel->wheelBrightnessS, 255-cur_hsv.value);
	wheelUpdateBrightnessGradientFromHSV(panel, cur_hsv);
	
	panel->colx = 2 + rint((colorWheelSize / 2.0) * (1 + (cur_hsv.saturation/255.0) * cos( cur_hsv.hue*W_PI/180.0)));
	panel->coly = 2 + rint((colorWheelSize / 2.0) * (1 + (cur_hsv.saturation/255.0) * sin(-cur_hsv.hue*W_PI/180.0)));
	
	for (i = 0; i < 256; i++) {
		/* We divide by 128 in advance, and check whether that number divides
		 * by 2 properly. If not, we add one to round the number correctly
		 */
		v = (i*cur_hsv.value) >> 7;
		panel->wheelMtrx->values[i] = (unsigned char)((v >> 1) + (v & 1));
	}
}

static void
grayInit(W_ColorPanel *panel)
{
	RHSVColor	cur_hsv;
	int			value;
	char		tmp[3];
	
	RRGBtoHSV(&panel->color, &cur_hsv);

	value = rint(cur_hsv.value/2.55);	
	WMSetSliderValue(panel->grayBrightnessS, value);

	sprintf(tmp, "%d", value);
	WMSetTextFieldText(panel->grayBrightnessT, tmp);
}

static void
rgbInit(W_ColorPanel *panel)
{
	char	tmp[3];

	WMSetSliderValue(panel->rgbRedS,panel->color.red);
	WMSetSliderValue(panel->rgbGreenS,panel->color.green);
	WMSetSliderValue(panel->rgbBlueS,panel->color.blue);
	
	sprintf(tmp, "%d", panel->color.red);
	WMSetTextFieldText(panel->rgbRedT, tmp);
	sprintf(tmp, "%d", panel->color.green);
	WMSetTextFieldText(panel->rgbGreenT, tmp);
	sprintf(tmp, "%d", panel->color.blue);
	WMSetTextFieldText(panel->rgbBlueT, tmp);
}

static void
cmykInit(W_ColorPanel *panel)
{
	int		value[3];
	char	tmp[3];

	value[0] = rint((255-panel->color.red)/2.55);
	value[1] = rint((255-panel->color.green)/2.55);
	value[2] = rint((255-panel->color.blue)/2.55);
	
	WMSetSliderValue(panel->cmykCyanS, value[0]);
	WMSetSliderValue(panel->cmykMagentaS, value[1]);
	WMSetSliderValue(panel->cmykYellowS, value[2]);
	WMSetSliderValue(panel->cmykBlackS, 0);
	
	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->cmykCyanT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->cmykMagentaT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->cmykYellowT, tmp);
	WMSetTextFieldText(panel->cmykBlackT, "0");
}

static void
hsbInit(W_ColorPanel *panel)
{
	int		value[3];
	char	tmp[3];
	
	value[0] = panel->hsvcolor.hue;
	value[1] = rint(panel->hsvcolor.saturation/2.55);
	value[2] = rint(panel->hsvcolor.value/2.55);
	
	WMSetSliderValue(panel->hsbHueS,value[0]);
	WMSetSliderValue(panel->hsbSaturationS,value[1]);
	WMSetSliderValue(panel->hsbBrightnessS,value[2]);
	
	sprintf(tmp, "%d", value[0]);
	WMSetTextFieldText(panel->hsbHueT, tmp);
	sprintf(tmp, "%d", value[1]);
	WMSetTextFieldText(panel->hsbSaturationT, tmp);
	sprintf(tmp, "%d", value[2]);
	WMSetTextFieldText(panel->hsbBrightnessT, tmp);
	
	hsbUpdateBrightnessGradient(panel);
	hsbUpdateSaturationGradient(panel);
	hsbUpdateHueGradient(panel);
}




/************************* ColorList Panel Functions **********************/

static void
colorListPaintItem(WMList *lPtr, int index, Drawable d, char *text, int state, WMRect *rect)
{
	int		width, height, x, y;
	RColor			color = *((RColor *)WMGetListItem(lPtr, index)->clientData);
	WMScreen		*scr = WMWidgetScreen(lPtr);
	Display			*dpy = WMScreenDisplay(scr);
	RImage			*image;
	WMPixmap		*pixmap;
	int				i, j;

	width = rect->size.width;
	height = rect->size.height;
	x = rect->pos.x;
	y = rect->pos.y;
	
	if (state & WLDSSelected)
		XFillRectangle(dpy, d, WMColorGC(WMWhiteColor(scr)), x, y, width, height);
	else
		XClearArea(dpy, d, x, y, width, height, False);
	
	image = RCreateImage(15, 15, False);
	for (i=0; i < 15 ; i++) {
		for (j=0; j < 15; j++) {
			image->data[0][j*15+i] = color.red;
			image->data[1][j*15+i] = color.green;
			image->data[2][j*15+i] = color.blue;
		}
	}
	
	pixmap = WMCreatePixmapFromRImage(scr, image, 0);
	RDestroyImage(image);
	WMDrawPixmap(pixmap, d, x, y);
	WMReleasePixmap(pixmap);
	WMDrawString(scr, d, WMColorGC(WMBlackColor(scr)), globalFont, x+18, y, text, strlen(text));
}


static void
colorListSelect(WMWidget *w, void *data)
{
	W_ColorPanel	*panel = (W_ColorPanel *)data;
	RColor			color = *((RColor *)WMGetListSelectedItem(w)->clientData);
	
	panel->lastChanged = WMColorListModeColorPanel;
	WMSetColorPanelColor(panel, color);
}




/************************** Common utility functions ************************/

static int
fetchImageFile(char *toPath, char *imageSrcFile, char *imageDestFile)
{
	int	src, dest;
	int	n;
	char	*tmp;
	char	buf[BUFSIZE];

	if ((src = open(imageSrcFile, O_RDONLY)) == 0) {
		wwarning("Could not open %s", imageSrcFile);
		return -1;
	}

	tmp = wstrappend(toPath, imageDestFile);
	if ((dest = open( tmp, O_RDWR|O_CREAT, S_IRWXU|S_IRGRP|S_IROTH)) == 0) {
		wwarning("Could not create %s", tmp);
		free(tmp);
		return -1;
	}
	free(tmp);


	/* Copy the file */
	while ((n = read(src, buf, BUFSIZE)) > 0)
	{
		if (write (dest, buf, n) != n)	{
			wwarning("Write error on file %s", imageDestFile);
			return -1;
		}
	}

	return 0;
}



char*
generateNewFilename(char *Path, char *curName)
{
	char	*newName;
	char	*numStart, *numEnd;
	char	*pre, *num, *post;
	int	n, numLength;
	char	*tmp;

	newName = wstrdup(curName);

	if (newName == NULL)
		return wstrdup("Unnamed");

	/*
	 *	Modify the name to contain a number to make it unique 
	 */


	/* Get number-string from filename. */
	numStart = strrchr(newName, '(');
	if (numStart) {
		if (strlen(numStart) > 1)
			numStart++;						/* Next character, please */


		numEnd = strrchr(numStart, ')');	/* Already past "numbers" */

		if (numEnd) {
			/* Now it gets interesting */

			pre = wmalloc((numStart - newName +1) * sizeof(char));	/* a trailing '\0' is useful here */
			memset(pre, 0, (numStart - newName +1) * sizeof(char));

			strncpy (pre, newName, numStart-newName);

			num = wmalloc((numEnd - numStart +1) * sizeof(char));
			memset (num, 0, (numEnd - numStart +1) * sizeof(char));

			strncpy (num, numStart, numEnd-numStart);

			post = wstrdup( numEnd);

			free(newName);	/* We don't need it anymore */

			if (strcspn(num, "0123456789") == 0) {	
				n = (int)strtol(num, NULL, 10);

				/* Increase number between brackets (in filename); This is what it all is about */
				n++;

				free(num);
				num = wmalloc(3 * sizeof(char));		/* Nobody uses more than 999 duplicate filenames, do you ? */
				memset(num, 0, 3 * sizeof(char));

				numLength = sprintf(num, "%d", n);

				free(num);
				num = wmalloc(numLength * sizeof(char));
				memset(num, 0, numLength * sizeof(char));

				sprintf(num, "%d", n);

				tmp = wstrappend (num, post);
				newName = wstrappend (pre, tmp);

				free (pre);
				free (num);
				free (post);
				free (tmp);
			}
		}
	}


	if (strcmp( curName, newName) == 0) {
		/* The name is still unchanged, so there was no number (between brackets) in it.
			Add a number 1 to the end. 
		*/
		tmp = wstrdup(newName);
		if (newName)
			free(newName);
		newName = wstrappend(tmp, " (1)");
		free(tmp);
	}

	return newName;
}

