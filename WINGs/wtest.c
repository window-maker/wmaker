/*
 * WINGs test application
 */

#include "WINGs.h"

#include <stdio.h>



/*
 * You need to define this function to link any program to WINGs.
 * This will be called when the application will be terminated because
 * on a fatal error.
 */
void
wAbort()
{
    exit(1);
}




Display *dpy;

int windowCount = 0;

void
closeAction(WMWidget *self, void *data)
{
    WMDestroyWidget(self);
    windowCount--;
    if (windowCount < 1)
	exit(0);
}


void
testOpenFilePanel(WMScreen *scr)
{
    WMOpenPanel *panel;
    
    windowCount++;
    
    /* get the shared Open File panel */
    panel = WMGetOpenPanel(scr);
    
    WMRunModalFilePanelForDirectory(panel, NULL, "/usr/local", NULL, NULL);
    
    /* free the panel to save some memory. Not needed otherwise. */
    WMFreeFilePanel(WMGetOpenPanel(scr));
}


void
testFontPanel(WMScreen *scr)
{
    WMFontPanel *panel;
    
    windowCount++;
    
    panel = WMGetFontPanel(scr);
    
    WMShowFontPanel(panel);
    
/*    WMFreeFontPanel(panel);*/
}



void
testList(WMScreen *scr)
{
    WMWindow *win;
    WMList *list;
    char text[100];
    int i;

    windowCount++;

    win = WMCreateWindow(scr, "testList");
    WMSetWindowTitle(win, "List");
    WMSetWindowCloseAction(win, closeAction, NULL);

    list = WMCreateList(win);
    for (i=0; i<50; i++) {
	sprintf(text, "Item %i", i);
	WMAddListItem(list, text);
    }
    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);

    
}


void
testGradientButtons(WMScreen *scr)
{
    WMWindow *win;
    WMButton *btn;
    WMPixmap *pix1, *pix2;
    RImage *back;
    RColor light, dark;
    WMColor *color;

    windowCount++;
    
    /* creates the top-level window */
    win = WMCreateWindow(scr, "testGradientButtons");
    WMSetWindowTitle(win, "Gradiented Button Demo");
    WMResizeWidget(win, 300, 200);

    light.red = 0x90;
    light.green = 0x85;
    light.blue = 0x90;
    dark.red = 0x35;
    dark.green = 0x30;
    dark.blue = 0x35;
    
    color = WMCreateRGBColor(scr, 0x5900, 0x5100, 0x5900, True);
    WMSetWidgetBackgroundColor(win, color);
    WMReleaseColor(color);
    
    back = RRenderGradient(60, 24, &dark, &light, RGRD_DIAGONAL);
    RBevelImage(back, RBEV_RAISED2);
    pix1 = WMCreatePixmapFromRImage(scr, back, 0);
    RDestroyImage(back);

    back = RRenderGradient(60, 24, &dark, &light, RGRD_DIAGONAL);
    RBevelImage(back, RBEV_SUNKEN);
    pix2 = WMCreatePixmapFromRImage(scr, back, 0);
    RDestroyImage(back);

    btn = WMCreateButton(win, WBTMomentaryChange);
    WMResizeWidget(btn, 60, 24);
    WMMoveWidget(btn, 20, 100);
    WMSetButtonBordered(btn, False);
    WMSetButtonImagePosition(btn, WIPOverlaps);
    WMSetButtonImage(btn, pix1);
    WMSetButtonAltImage(btn, pix2);
    WMSetButtonText(btn, "Cool");

    WMSetBalloonTextForView("This is a button", WMWidgetView(btn));

    btn = WMCreateButton(win, WBTMomentaryChange);
    WMResizeWidget(btn, 60, 24);
    WMMoveWidget(btn, 90, 100);
    WMSetButtonBordered(btn, False);
    WMSetButtonImagePosition(btn, WIPOverlaps);
    WMSetButtonImage(btn, pix1);
    WMSetButtonAltImage(btn, pix2);
    WMSetButtonText(btn, "Button");

    WMSetBalloonTextForView("This is another button", WMWidgetView(btn));

    btn = WMCreateButton(win, WBTMomentaryChange);
    WMResizeWidget(btn, 60, 24);
    WMMoveWidget(btn, 160, 100);
    WMSetButtonBordered(btn, False);
    WMSetButtonImagePosition(btn, WIPOverlaps);
    WMSetButtonImage(btn, pix1);
    WMSetButtonAltImage(btn, pix2);
    WMSetButtonText(btn, "Test");

    WMSetBalloonTextForView("This is yet another button.\nBut the balloon has 3 lines.\nYay!",
			    WMWidgetView(btn));

    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
}


void
testScrollView(WMScreen *scr)
{
    WMWindow *win;
    WMScrollView *sview;
    WMFrame *f;
    WMLabel *l;
    char buffer[128];
    int i;

    windowCount++;
    
    /* creates the top-level window */
    win = WMCreateWindow(scr, "testScroll");
    WMSetWindowTitle(win, "Scrollable View");
    
    WMSetWindowCloseAction(win, closeAction, NULL);
    
    /* set the window size */
    WMResizeWidget(win, 300, 300);
    
    /* creates a scrollable view inside the top-level window */
    sview = WMCreateScrollView(win);
    WMResizeWidget(sview, 200, 200);
    WMMoveWidget(sview, 30, 30);
    WMSetScrollViewRelief(sview, WRSunken);
    WMSetScrollViewHasVerticalScroller(sview, True);
    WMSetScrollViewHasHorizontalScroller(sview, True);

    /* create a frame with a bunch of labels */
    f = WMCreateFrame(win);
    WMResizeWidget(f, 400, 400);
    WMSetFrameRelief(f, WRFlat);

    for (i=0; i<20; i++) {
	l = WMCreateLabel(f);
	WMResizeWidget(l, 50, 18);
	WMMoveWidget(l, 10, 20*i);
	sprintf(buffer, "Label %i", i);
	WMSetLabelText(l, buffer);
	WMSetLabelRelief(l, WRSimple);
    }
    WMMapSubwidgets(f);
    WMMapWidget(f);
    
    WMSetScrollViewContentView(sview, WMWidgetView(f));
    
    /* make the windows of the widgets be actually created */
    WMRealizeWidget(win);
    
    /* Map all child widgets of the top-level be mapped.
     * You must call this for each container widget (like frames),
     * even if they are childs of the top-level window.
     */
    WMMapSubwidgets(win);

    /* map the top-level window */
    WMMapWidget(win);
}


void
testColorWell(WMScreen *scr)
{
    WMWindow *win;
    WMColorWell *well1, *well2;

    windowCount++;

    win = WMCreateWindow(scr, "testColor");
    WMResizeWidget(win, 300, 300);

    WMSetWindowCloseAction(win, closeAction, NULL);

    well1 = WMCreateColorWell(win);
    WMResizeWidget(well1, 60, 40);
    WMMoveWidget(well1, 100, 100);
    WMSetColorWellColor(well1, WMCreateRGBColor(scr, 0x8888, 0, 0x1111, True));
    well2 = WMCreateColorWell(win);
    WMResizeWidget(well2, 60, 40);
    WMMoveWidget(well2, 200, 100);
    WMSetColorWellColor(well2, WMCreateRGBColor(scr, 0, 0, 0x8888, True));
    
    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
}

void
sliderCallback(WMWidget *w, void *data)
{
    printf("SLIEDER == %i\n", WMGetSliderValue(w));
}


void
testSlider(WMScreen *scr)
{
    WMWindow *win;
    WMSlider *s;

    windowCount++;
    
    win = WMCreateWindow(scr, "testSlider");
    WMResizeWidget(win, 300, 300);
    WMSetWindowTitle(win, "Sliders");

    WMSetWindowCloseAction(win, closeAction, NULL);

    s = WMCreateSlider(win);
    WMResizeWidget(s, 16, 100);
    WMMoveWidget(s, 100, 100);
    WMSetSliderKnobThickness(s, 8);
    WMSetSliderContinuous(s, False);
    WMSetSliderAction(s, sliderCallback, s);

    s = WMCreateSlider(win);
    WMResizeWidget(s, 100, 16);
    WMMoveWidget(s, 100, 10);
    WMSetSliderKnobThickness(s, 8);

    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
}

void
testTextField(WMScreen *scr)
{
    WMWindow *win;
    WMTextField *field, *field2;

    windowCount++;
    
    win = WMCreateWindow(scr, "testText");
    WMResizeWidget(win, 400, 300);

    WMSetWindowCloseAction(win, closeAction, NULL);    

    field = WMCreateTextField(win);
    WMResizeWidget(field, 200, 20);
    WMMoveWidget(field, 20, 20);

    field2 = WMCreateTextField(win);
    WMResizeWidget(field2, 200, 20);
    WMMoveWidget(field2, 20, 50);
    WMSetTextFieldAlignment(field2, WARight);

    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
    
}


void
testPullDown(WMScreen *scr)
{
    WMWindow *win;
    WMPopUpButton *pop, *pop2;

    windowCount++;
    
    win = WMCreateWindow(scr, "pullDown");
    WMResizeWidget(win, 400, 300);

    WMSetWindowCloseAction(win, closeAction, NULL);    

    pop = WMCreatePopUpButton(win);
    WMResizeWidget(pop, 100, 20);
    WMMoveWidget(pop, 50, 60);
    WMSetPopUpButtonPullsDown(pop, True);
    WMSetPopUpButtonText(pop, "Commands");
    WMAddPopUpButtonItem(pop, "Add");
    WMAddPopUpButtonItem(pop, "Remove");
    WMAddPopUpButtonItem(pop, "Check");
    WMAddPopUpButtonItem(pop, "Eat");

    pop2 = WMCreatePopUpButton(win);
    WMResizeWidget(pop2, 100, 20);
    WMMoveWidget(pop2, 200, 60);
    WMSetPopUpButtonText(pop2, "Select");
    WMAddPopUpButtonItem(pop2, "Apples");
    WMAddPopUpButtonItem(pop2, "Bananas");
    WMAddPopUpButtonItem(pop2, "Strawberries");
    WMAddPopUpButtonItem(pop2, "Blueberries");

    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
    
}



#include "WUtil.h"

int main(int argc, char **argv)
{
    WMScreen *scr;
    WMPixmap *pixmap;    
    
    /* Initialize the application */
    WMInitializeApplication("Test", &argc, argv);
    
    /*
     * Open connection to the X display.
     */
    dpy = XOpenDisplay("");
    
    if (!dpy) {
	puts("could not open display");
	exit(1);
    }
    
    /* This is used to disable buffering of X protocol requests. 
     * Do NOT use it unless when debugging. It will cause a major 
     * slowdown in your application
     */
#ifdef DEBUG
    XSynchronize(dpy, True);
#endif
    /*
     * Create screen descriptor.
     */
    scr = WMCreateScreen(dpy, DefaultScreen(dpy));

    /*
     * Loads the logo of the application.
     */
    pixmap = WMCreatePixmapFromFile(scr, "logo.xpm");
    
    /*
     * Makes the logo be used in standard dialog panels.
     */
    WMSetApplicationIconImage(scr, pixmap); WMReleasePixmap(pixmap);
    
    /*
     * Do some test stuff.
     * 
     * Put the testSomething() function you want to test here.
     */
    testGradientButtons(scr);

#if 0
    testOpenFilePanel(scr);
    testFontPanel(scr);
    testList(scr);
    testGradientButtons(scr);
    testScrollView(scr);

    testColorWell(scr);

    testSlider(scr);
    testTextField(scr);
    testPullDown(scr);
#endif
    /*
     * The main event loop.
     * 
     */
    WMScreenMainLoop(scr);

    return 0;
}
