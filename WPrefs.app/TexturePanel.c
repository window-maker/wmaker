/* TexturePanel.c- texture editting panel
 * 
 *  WPrefs - WindowMaker Preferences Program
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
 *  Copyright (c) 1998 James Thompson
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>

#include <WINGs.h>
#include <WUtil.h>


#include "TexturePanel.h"
#include "TexturePanel.icons"

typedef struct _TexturePanel {
    WMWindow *win;

    WMFrame *typeF;
    WMButton *tsoliB;
    WMButton *tgradB;
    WMButton *tpixmB;
    
    WMButton *okB;
    WMButton *cancB;
    
    /* solid */
    WMFrame *scolorF;
    WMColorWell *scolorW;
    
    /* gradient */
    WMFrame *gcolorF;
    WMList *gcolorLs;
    WMColorWell *gcolorW;
    WMButton *gaddB;
    WMButton *gremB;
    WMButton *gupdB;

    WMFrame *gdirF;
    WMButton *ghorB;
    WMButton *gverB;
    WMButton *gdiaB;
    
    /* pixmap */
    WMFrame *pimagF;
    WMLabel *pimagL;
    WMTextField *pimagT;
    WMButton *pbrowB;
    
    WMFrame *pcolorF;
    WMColorWell *pcolorW;
    
    WMFrame *pmodeF;
    WMButton *pscalB;
    WMButton *ptileB;
    
    WMAction *okAction;
    void *okData;
    
    WMAction *cancelAction;
    void *cancelData;

} _TexturePanel;



char *WMGetColorWellRGBString(WMColorWell *cPtr) {
  char *rgbString;
  WMColor *color;
  
 
  rgbString = wmalloc(13);
  color = WMGetColorWellColor(cPtr);

  if (color) {
    sprintf(rgbString,"rgb:%02x/%02x/%02x",
	    (WMRedComponentOfColor(color) >> 8), 
	    (WMGreenComponentOfColor(color) >> 8),
	    (WMBlueComponentOfColor(color) >> 8));
  }

  return rgbString;
}



/*
 *--------------------------------------------------------------------------
 * Private Functions
 *--------------------------------------------------------------------------
 */
static void buttonCallback(WMWidget *self, void *data);
static void renderTextureButtons (_TexturePanel *panel);
static void paintListItem(WMList *lPtr, int index, Drawable d, char *text, int state, WMRect *rect);
static void notificationObserver(void *self, WMNotification *notif);

static void
notificationObserver(void *self, WMNotification *notif)
{
    _TexturePanel *panel = (_TexturePanel*)self;
    void *object = WMGetNotificationObject(notif);
    char *text;

    if (WMGetNotificationName(notif) == WMTextDidChangeNotification) {
      if (object == panel->pimagT) {
	text = WMGetTextFieldText(panel->pimagT);
	if (strlen(text)) {
	  WMSetButtonEnabled(panel->okB, True);
	} else {
	  WMSetButtonEnabled(panel->okB, False);
        }
	free(text); 
      } 
    }
}


static void
paintListItem(WMList *lPtr, int index, Drawable d, char *text, int state, WMRect *rect)
{
    WMScreen *scr;
    int width, height, x, y;
    GC gc;
    Display *dpy;
    
    scr = WMWidgetScreen(lPtr);
    dpy = WMScreenDisplay(scr);
    
    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
        XFillRectangle(dpy, d, WMColorGC(WMWhiteColor(scr)), x, y,width, height);
    else
        XClearArea(WMScreenDisplay(scr), d, x, y, width, height, False);

    gc = XCreateGC(dpy, RootWindow(dpy, 0),0,NULL);
    WMSetColorInGC(WMCreateNamedColor(scr, text, True),gc);
    XFillRectangle (dpy, d, gc,x+5,y+3,width-8,height-6);
    XFreeGC(dpy,gc);
}


static void
buttonCallback(WMWidget *self, void *data)
{
  _TexturePanel *panel = (_TexturePanel*)data;
  char *text, *color;
  WMOpenPanel *op;
  int itemRow;
  
  WMSetButtonEnabled(panel->okB, True);
  
  /* Global Buttons */

  if (self == panel->okB) {
    if (panel->okAction) {
      (*panel->okAction)(self, panel->okData);
    } else {
     wwarning ("Texture panel OK button undefined");
    }
  } else if (self == panel->cancB) {
    if (panel->cancelAction) {
      (*panel->cancelAction)(self, panel->cancelData);
    } else {
     wwarning ("Texture panel CANCEL button undefined");
    }
  } else if (self == panel->tsoliB) {
    WMMapWidget(panel->scolorF);
    WMUnmapWidget(panel->gcolorF);
    WMUnmapWidget(panel->gdirF);
    WMUnmapWidget(panel->pimagF);
    WMUnmapWidget(panel->pcolorF);
    WMUnmapWidget(panel->pmodeF);	
  } else if (self == panel->tgradB) {
    WMMapWidget(panel->gcolorF);
    WMMapWidget(panel->gdirF);
    WMUnmapWidget(panel->scolorF);
    WMUnmapWidget(panel->pimagF);
    WMUnmapWidget(panel->pcolorF);
    WMUnmapWidget(panel->pmodeF);
  } else if (self == panel->tpixmB) {
    WMMapWidget(panel->pimagF);
    WMMapWidget(panel->pcolorF);
    WMMapWidget(panel->pmodeF);
    WMUnmapWidget(panel->gcolorF);
    WMUnmapWidget(panel->gdirF);
    WMUnmapWidget(panel->scolorF);
    
    text = WMGetTextFieldText(panel->pimagT);
    if (!strlen(text)) {
      WMSetButtonEnabled(panel->okB, False);
    }
    free(text);

    /* Gradient Panel Buttons */

  } else if (self == panel->gaddB) {
    color = WMGetColorWellRGBString(panel->gcolorW);
    itemRow = WMGetListSelectedItemRow(panel->gcolorLs);
    WMInsertListItem(panel->gcolorLs,itemRow, color);
    free(color);
    renderTextureButtons(panel);

  } else if (self == panel->gremB) {
    if (WMGetListNumberOfRows(panel->gcolorLs) != 1) {
      itemRow = WMGetListSelectedItemRow(panel->gcolorLs);
      WMRemoveListItem(panel->gcolorLs,itemRow);
      renderTextureButtons(panel);
    }
    
  } else if (self == panel->gupdB) {
    color = WMGetColorWellRGBString(panel->gcolorW);
    itemRow = WMGetListSelectedItemRow(panel->gcolorLs);
    WMRemoveListItem(panel->gcolorLs,itemRow);
    WMInsertListItem(panel->gcolorLs,itemRow, color);
    free(color);
    renderTextureButtons(panel);

  /* Pixmap Panel Buttons */
  
  } else if (self == panel->pbrowB) {
    
    op = WMGetOpenPanel(WMWidgetScreen(panel->pbrowB));    
    if (WMRunModalOpenPanelForDirectory(op, NULL, "/usr/local", NULL, NULL)) {
      char *path;
      path = WMGetFilePanelFileName(op);
      WMSetTextFieldText(panel->pimagT, path);
      if (strlen(path)) {
	WMSetButtonEnabled(panel->okB, True);
      }
      free(path);  
    }
    WMFreeFilePanel(op);
  }
  
}

#if 0

static void
changePanel(WMWidget *self, void *data)
{    
    _TexturePanel *panel = (_TexturePanel*)data;
    char *text = NULL;

    WMSetButtonEnabled(panel->okB, True);
    if (self == panel->tsoliB) {
	WMMapWidget(panel->scolorF);
	WMUnmapWidget(panel->gcolorF);
	WMUnmapWidget(panel->gdirF);
	WMUnmapWidget(panel->pimagF);
	WMUnmapWidget(panel->pcolorF);
	WMUnmapWidget(panel->pmodeF);	
    } else if (self == panel->tgradB) {
	WMMapWidget(panel->gcolorF);
	WMMapWidget(panel->gdirF);
	WMUnmapWidget(panel->scolorF);
	WMUnmapWidget(panel->pimagF);
	WMUnmapWidget(panel->pcolorF);
	WMUnmapWidget(panel->pmodeF);
    } else {
	WMMapWidget(panel->pimagF);
	WMMapWidget(panel->pcolorF);
	WMMapWidget(panel->pmodeF);
	WMUnmapWidget(panel->gcolorF);
	WMUnmapWidget(panel->gdirF);
	WMUnmapWidget(panel->scolorF);
 
        text = WMGetTextFieldText(panel->pimagT);
        if (!strlen(text)) {
          WMSetButtonEnabled(panel->okB, False);
	}
        free(text);
    }
    
}

static void 
modifyGradientList(WMWidget *self, void *data)
{
   _TexturePanel *panel = (_TexturePanel*)data;
   char *color = NULL;
   int itemRow;

   itemRow = WMGetListSelectedItemRow(panel->gcolorLs);

   color = WMGetColorWellRGBString(panel->gcolorW);

   if (self == panel->gaddB) {
      WMInsertListItem(panel->gcolorLs,itemRow, color);
   } else if (self == panel->gremB) {
     if (WMGetListNumberOfRows(panel->gcolorLs) != 1) {
       WMRemoveListItem(panel->gcolorLs,itemRow);
     }
     
   } else {
     WMRemoveListItem(panel->gcolorLs,itemRow);
     WMInsertListItem(panel->gcolorLs,itemRow, color);
   }
   free (color);

   renderTextureButtons(panel);
}
#endif


static void
renderTextureButtons(_TexturePanel *panel)
{ 
  RColor **colors = NULL;
  XColor color;
  WMPixmap *icon;
  RImage *imgh, *imgv, *imgd;
  WMScreen *scr;

  int i, listRows;

  scr = WMWidgetScreen(panel->gcolorLs);
  listRows = WMGetListNumberOfRows(panel->gcolorLs);
  colors = wmalloc(sizeof(RColor*)*(listRows+1));

  for (i=0; i < listRows; i++) {
    if (!XParseColor(WMScreenDisplay(scr), (WMScreenRContext(scr))->cmap, 
		     wstrdup(WMGetListItem(panel->gcolorLs, i)->text),&color)) {
      wfatal("could not parse color \"%s\"\n", WMGetListItem(panel->gcolorLs, i)->text);
      exit(1);
    } else {
      colors[i] = malloc(sizeof(RColor));
      colors[i]->red = color.red >> 8;
      colors[i]->green = color.green >> 8;
      colors[i]->blue = color.blue >> 8;
    }
  }
  colors[i] = NULL;
  
  imgh = RRenderMultiGradient(50, 20, colors, RGRD_HORIZONTAL);
  if (!imgh) {
    wwarning("internal error:%s", RMessageForError(RErrorCode));
  } else {
    icon = WMCreatePixmapFromRImage(scr, imgh, 120);
    RDestroyImage(imgh);
    WMSetButtonImage(panel->ghorB, icon);
    WMReleasePixmap(icon);
  }
    
  imgv = RRenderMultiGradient(50, 20, colors, RGRD_VERTICAL);
  if (!imgv) {
    wwarning("internal error:%s", RMessageForError(RErrorCode));
  } else {
    icon = WMCreatePixmapFromRImage(scr, imgv, 120);
    RDestroyImage(imgv);
    WMSetButtonImage(panel->gverB, icon);
    WMReleasePixmap(icon);
  }

  imgd = RRenderMultiGradient(50, 20, colors, RGRD_DIAGONAL);
  if (!imgd) {
    wwarning("internal error:%s", RMessageForError(RErrorCode));
  } else {
    icon = WMCreatePixmapFromRImage(scr, imgd, 120);
    RDestroyImage(imgd);
    WMSetButtonImage(panel->gdiaB, icon);
    WMReleasePixmap(icon);
  }

  free(colors);
}


/*
 *--------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------
 */
void
DestroyTexturePanel(TexturePanel *panel)
{
  WMUnmapWidget(panel->win);
  WMDestroyWidget(panel->win);
  free(panel);
}


void
ShowTexturePanel(TexturePanel *panel)
{
  WMMapWidget(panel->win);
}


void
HideTexturePanel(TexturePanel *panel)
{
   WMUnmapWidget(panel->win);
}

void
SetTexturePanelOkAction(TexturePanel *panel, WMAction *action, void *clientData)
{
  panel->okAction = action;
  panel->okData = clientData;
}


void
SetTexturePanelCancelAction(TexturePanel *panel, WMAction *action, void *clientData)
{
  panel->cancelAction = action;
  panel->cancelData = clientData;
}

void
SetTexturePanelTexture(TexturePanel *panel, char *texture)
{
  char *parseString;
  char *parseStringPosition;
  char *enclosures = "( )";
  char *seperators = " ,\"";
  char *filename;

  WMSetButtonSelected(panel->tsoliB, False); 
  WMSetButtonSelected(panel->tgradB, False); 
  WMSetButtonSelected(panel->tpixmB, False); 

  parseString = wstrdup(texture);
  parseStringPosition = parseString;

  parseStringPosition = strtok(parseStringPosition,seperators);
  wwarning ("Parsing...");

  while (parseStringPosition) {

    if (!strpbrk(parseStringPosition,enclosures)) {
      if (strcasecmp(parseStringPosition,"solid") == 0) {
        wwarning("Switch to solid");
	WMPerformButtonClick(panel->tsoliB);

        parseStringPosition = strtok(NULL,seperators);
	
        while (parseStringPosition) {  
          if (!strpbrk(parseStringPosition,enclosures)) {
            WMSetColorWellColor(panel->scolorW,WMCreateNamedColor(WMWidgetScreen(panel->scolorW), parseStringPosition, True));
          } 
          parseStringPosition = strtok(NULL,seperators);
        }
	
      } else if ((strstr(parseStringPosition,"pixmap")) && (strcasecmp(strstr(parseStringPosition,"pixmap"),"pixmap") == 0)) {
        WMSetButtonSelected(panel->ptileB, False); 
        WMSetButtonSelected(panel->pscalB, False); 
        
	WMPerformButtonClick(panel->tpixmB);

        if  (tolower(*parseStringPosition) == 't') {
          wwarning ("Switch to Tiled Pixmap");
          WMPerformButtonClick(panel->ptileB);
	} else {
	  wwarning ("Switch to Scaled Pixmap");
          WMPerformButtonClick(panel->pscalB);
	}


        filename = NULL;
        parseStringPosition = strtok(NULL,seperators);
        do {
          if (filename) filename = wstrappend(filename," ");
	  filename = wstrappend(filename,parseStringPosition);
          parseStringPosition = strtok(NULL,seperators);
	} while (!strstr(parseStringPosition,"rgb:"));

  	WMSetTextFieldText(panel->pimagT, filename);
        free(filename);   
        
        /* parseStringPosition = strtok(NULL,seperators); */

        WMSetColorWellColor(panel->pcolorW,WMCreateNamedColor(WMWidgetScreen(panel->scolorW), parseStringPosition, True));

      } else if ((strstr(parseStringPosition,"gradient")) && (strcasecmp(strstr(parseStringPosition,"gradient"),"gradient") == 0)) {
        WMSetButtonSelected(panel->ghorB, False); 
        WMSetButtonSelected(panel->gverB, False); 
        WMSetButtonSelected(panel->gdiaB, False); 

	WMPerformButtonClick(panel->tgradB);

        if (strstr(parseStringPosition,"h")) {
          WMPerformButtonClick(panel->ghorB);
        } else if (strstr(parseStringPosition,"v")) {
          WMPerformButtonClick(panel->gverB);
        } else {
          WMPerformButtonClick(panel->gdiaB);
        }

       WMClearList(panel->gcolorLs);

       parseStringPosition = strtok(NULL,seperators);
       while (parseStringPosition) {  
          if (!strpbrk(parseStringPosition,enclosures)) {
            WMAddListItem(panel->gcolorLs, parseStringPosition);
            WMSetColorWellColor(panel->gcolorW,WMCreateNamedColor(WMWidgetScreen(panel->scolorW), parseStringPosition, True));
          } 
          parseStringPosition = strtok(NULL,seperators);
       }
      } else {
	wfatal("Unknown Texture Type");
      }

       while (parseStringPosition) {  
          parseStringPosition = strtok(NULL,seperators);
       }

    }

    parseStringPosition = strtok(NULL,seperators);
    }
    renderTextureButtons (panel);
    free(parseString);
}


char*
GetTexturePanelTextureString(TexturePanel *panel)
{ 
  char *colorString = NULL;
  char *start       = "( ";
  char *finish      = " )";
  char *seperator   = ", ";
  char *quote       = "\"";
  int  i, listRows;
  
  colorString = wstrappend(colorString,start);
  
  if (WMGetButtonSelected(panel->tsoliB)) {
    colorString = wstrappend(colorString,"solid");
    colorString = wstrappend(colorString,seperator);
    colorString = wstrappend(colorString, quote);
    colorString = wstrappend(colorString,WMGetColorWellRGBString(panel->scolorW));
    colorString = wstrappend(colorString, quote);
    
  } else if (WMGetButtonSelected(panel->tgradB)) {
    listRows = WMGetListNumberOfRows(panel->gcolorLs);
    
    if (listRows > 2) {
      colorString = wstrappend(colorString,"m");
    }
    if (WMGetButtonSelected(panel->ghorB)) {
      colorString = wstrappend(colorString,"hgradient");
    } else if (WMGetButtonSelected(panel->gverB)) {
      colorString = wstrappend(colorString,"vgradient");
    } else {
      colorString = wstrappend(colorString,"dgradient");
    }
    
    for (i=0; i < listRows; i++) {
      colorString = wstrappend(colorString, seperator);
      colorString = wstrappend(colorString, quote);
      colorString = wstrappend(colorString, wstrdup(WMGetListItem(panel->gcolorLs, i)->text));
      colorString = wstrappend(colorString, quote);
    }
  } else if (WMGetButtonSelected(panel->tpixmB)) {
    if (WMGetButtonSelected(panel->pscalB)) {
      colorString = wstrappend(colorString,"spixmap");
    } else {
      colorString = wstrappend(colorString,"tpixmap");
    }
    colorString = wstrappend(colorString,seperator);
    colorString = wstrappend(colorString, WMGetTextFieldText(panel->pimagT));
    colorString = wstrappend(colorString,seperator);
    colorString = wstrappend(colorString, quote);
    colorString = wstrappend(colorString, WMGetColorWellRGBString(panel->pcolorW));
    colorString = wstrappend(colorString, quote);
  }
  
  colorString = wstrappend(colorString,finish);
  
  return colorString;
}


RImage*
RenderTexturePanelTexture(TexturePanel *panel, unsigned width, unsigned height)
{
  XColor color;
  RContext *ctx;
  RColor  solidColor;
  RColor **colors = NULL;
  RImage *image=NULL, *fileImage;
  WMScreen *scr;
  WMColor *wellColor;

  int i, listRows, style;

  scr = WMWidgetScreen(panel->gcolorLs);
  ctx = WMScreenRContext(scr);

  if (WMGetButtonSelected(panel->tsoliB)) {
    
    image = RCreateImage(width, height, 1);
    wellColor = WMGetColorWellColor(panel->scolorW);

    solidColor.red = (WMRedComponentOfColor(wellColor)  >> 8);
    solidColor.green = (WMGreenComponentOfColor(wellColor) >> 8) ;
    solidColor.blue = (WMBlueComponentOfColor(wellColor) >> 8);
    solidColor.alpha = 0xff;

    WMReleaseColor(wellColor);
    
    RClearImage(image, &solidColor);
    
  } else if (WMGetButtonSelected(panel->tgradB)) {
    
    if (WMGetButtonSelected(panel->ghorB)) {
      style = RGRD_HORIZONTAL;
    } else if (WMGetButtonSelected(panel->gverB)) {
      style = RGRD_VERTICAL;
    } else {
      style = RGRD_DIAGONAL;
    }

    listRows = WMGetListNumberOfRows(panel->gcolorLs);
    colors = wmalloc(sizeof(RColor*)*(listRows+1));
    
    for (i=0; i < listRows; i++) {
      if (!XParseColor(WMScreenDisplay(scr), ctx->cmap, 
		       wstrdup(WMGetListItem(panel->gcolorLs, i)->text),&color)) {
	wfatal("could not parse color \"%s\"\n", WMGetListItem(panel->gcolorLs, i)->text);
	exit(1);
      } else {
	colors[i] = malloc(sizeof(RColor));
	colors[i]->red = color.red >> 8;
	colors[i]->green = color.green >> 8;
	colors[i]->blue = color.blue >> 8;
      }
    }
    colors[i] = NULL;
    
    image =  RRenderMultiGradient(width, height, colors, style);
    
  } else if (WMGetButtonSelected(panel->tpixmB)) {

    if (fopen(WMGetTextFieldText(panel->pimagT),"r")) {

      fileImage = RLoadImage(ctx, WMGetTextFieldText(panel->pimagT), 0);
      if (WMGetButtonSelected(panel->pscalB)) {
	image = RScaleImage(fileImage, width, height);
      } else {
	image = RMakeTiledImage(fileImage, width, height);;
      }
    } else {      
      wwarning ("Invalid File Name");
      return  RCreateImage(1,1,1);
    }
  }
  
  return image;
}

TexturePanel*
CreateTexturePanel(WMScreen *scr)
{
    RImage *image;
    WMPixmap *icon;
    WMColor *red;
    TexturePanel *panel;
    RColor white,black;

    white.red=255;
    white.blue=255;
    white.green=255;
    white.alpha=255;

    black.red=0;
    black.blue=0;
    black.green=0;
    black.alpha=0;

    panel = wmalloc(sizeof(_TexturePanel));
    memset(panel, 0, sizeof(_TexturePanel));
    red = WMCreateRGBColor(scr,100,100,100,True);
    
    panel->win = WMCreateWindow(scr, "textureBuilder");
    WMResizeWidget(panel->win, 275, 365);
    WMSetWindowTitle(panel->win, "Texture Builder");

    /***************** Generic stuff ****************/
    
    panel->typeF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->typeF, 245, 80);
    WMMoveWidget(panel->typeF, 15, 10);
    WMSetFrameTitle(panel->typeF, "Texture Type");

    panel->tsoliB = WMCreateButton(panel->typeF, WBTOnOff);
    WMResizeWidget(panel->tsoliB, 48, 48);
    WMMoveWidget(panel->tsoliB, 42, 20);
    WMSetButtonImagePosition(panel->tsoliB, WIPImageOnly);
    image = RGetImageFromXPMData(WMScreenRContext(scr), solid_xpm);
    if (!image) {
	wwarning("internal error:%s", RMessageForError(RErrorCode));
    } else {
	icon = WMCreatePixmapFromRImage(scr, image, 120);
	RDestroyImage(image);
	WMSetButtonImage(panel->tsoliB, icon);
	WMReleasePixmap(icon);
    }
    WMSetButtonAction(panel->tsoliB, buttonCallback, panel);
    
    panel->tgradB = WMCreateButton(panel->typeF, WBTOnOff);
    WMGroupButtons(panel->tsoliB, panel->tgradB);
    WMResizeWidget(panel->tgradB, 48, 48);
    WMMoveWidget(panel->tgradB, 98, 20);
    WMSetButtonImagePosition(panel->tgradB, WIPImageOnly);
    image = RGetImageFromXPMData(WMScreenRContext(scr), gradient_xpm);
    if (!image) {
	wwarning("internal error:%s", RMessageForError(RErrorCode));
    } else {
	icon = WMCreatePixmapFromRImage(scr, image, 120);
	RDestroyImage(image);
	WMSetButtonImage(panel->tgradB, icon);
	WMReleasePixmap(icon);
    }
    WMSetButtonAction(panel->tgradB, buttonCallback, panel);
    
    panel->tpixmB = WMCreateButton(panel->typeF, WBTOnOff);
    WMGroupButtons(panel->tsoliB, panel->tpixmB);
    WMResizeWidget(panel->tpixmB, 48, 48);
    WMMoveWidget(panel->tpixmB, 154, 20);
    WMSetButtonImagePosition(panel->tpixmB, WIPImageOnly);
    image = RGetImageFromXPMData(WMScreenRContext(scr), pixmap_xpm);
    if (!image) {
	wwarning("internal error:%s", RMessageForError(RErrorCode));
    } else {
	icon = WMCreatePixmapFromRImage(scr, image, 120);
	RDestroyImage(image);
	WMSetButtonImage(panel->tpixmB, icon);
	WMReleasePixmap(icon);
    }
    WMSetButtonAction(panel->tpixmB, buttonCallback, panel);

    panel->okB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->okB, 70, 28);
    WMMoveWidget(panel->okB, 110, 325);
    WMSetButtonText(panel->okB, "OK");
    WMSetButtonAction(panel->okB,buttonCallback,panel);

    panel->cancB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancB, 70, 28);
    WMMoveWidget(panel->cancB, 190, 325);
    WMSetButtonText(panel->cancB, "Cancel");
    WMSetButtonAction(panel->cancB,buttonCallback,panel);
    
    WMMapSubwidgets(panel->typeF);
    
    /***************** Solid *****************/
    panel->scolorF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->scolorF, 245, 125);
    WMMoveWidget(panel->scolorF, 15, 140);
    WMSetFrameTitle(panel->scolorF, "Color");
    
    panel->scolorW = WMCreateColorWell(panel->scolorF);
    WMResizeWidget(panel->scolorW, 60, 45);
    WMMoveWidget(panel->scolorW, 95, 40);
    WMSetColorWellColor(panel->scolorW,WMCreateRGBColor(scr, 0xddff, 0xddff, 0, True));

    WMMapSubwidgets(panel->scolorF);
    
    /***************** Gradient *****************/
    panel->gcolorF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->gcolorF, 245, 145);
    WMMoveWidget(panel->gcolorF, 15, 95);
    WMSetFrameTitle(panel->gcolorF, "Colors");
    
    panel->gcolorLs = WMCreateList(panel->gcolorF);
    WMResizeWidget(panel->gcolorLs, 120, 84);
    WMMoveWidget(panel->gcolorLs, 20, 20);

    WMSetListUserDrawProc(panel->gcolorLs, paintListItem);

    WMAddListItem(panel->gcolorLs, "rgb:ff/ff/ff");
    WMAddListItem(panel->gcolorLs, "rgb:00/00/ff");
    
    panel->gcolorW = WMCreateColorWell(panel->gcolorF);
    WMResizeWidget(panel->gcolorW, 60, 45);
    WMMoveWidget(panel->gcolorW, 160, 40);
    WMSetColorWellColor(panel->gcolorW,WMCreateRGBColor(scr, 0xffff, 0, 0, True));    
    
    panel->gremB = WMCreateCommandButton(panel->gcolorF);
    WMResizeWidget(panel->gremB, 64, 24);
    WMMoveWidget(panel->gremB, 20, 110);
    WMSetButtonText(panel->gremB, "Remove");
    WMSetButtonAction(panel->gremB,(WMAction*)buttonCallback,panel);

    panel->gupdB = WMCreateCommandButton(panel->gcolorF);
    WMResizeWidget(panel->gupdB, 64, 24);
    WMMoveWidget(panel->gupdB, 90, 110);
    WMSetButtonText(panel->gupdB, "Update");
    WMSetButtonAction(panel->gupdB,(WMAction*)buttonCallback,panel);

    panel->gaddB = WMCreateCommandButton(panel->gcolorF);
    WMResizeWidget(panel->gaddB, 64, 24);
    WMMoveWidget(panel->gaddB, 160, 110);
    WMSetButtonText(panel->gaddB, "Add");
    WMSetButtonAction(panel->gaddB,(WMAction*)buttonCallback,panel);
    WMMapSubwidgets(panel->gcolorF);
    
    panel->gdirF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->gdirF, 245, 65);
    WMMoveWidget(panel->gdirF, 15, 245);
    WMSetFrameTitle(panel->gdirF, "Direction");
    
    panel->ghorB = WMCreateButton(panel->gdirF, WBTOnOff);
    WMResizeWidget(panel->ghorB, 64, 34);
    WMMoveWidget(panel->ghorB, 20, 20);
    WMSetButtonImagePosition(panel->ghorB, WIPImageOnly);
    WMPerformButtonClick(panel->ghorB);


    panel->gverB = WMCreateButton(panel->gdirF, WBTOnOff);
    WMResizeWidget(panel->gverB, 64, 34);
    WMMoveWidget(panel->gverB, 90, 20);
    WMSetButtonImagePosition(panel->gverB, WIPImageOnly);
    WMGroupButtons(panel->ghorB, panel->gverB);

    panel->gdiaB = WMCreateButton(panel->gdirF, WBTOnOff);
    WMResizeWidget(panel->gdiaB, 64, 34);
    WMMoveWidget(panel->gdiaB, 160, 20);
    WMSetButtonImagePosition(panel->gdiaB, WIPImageOnly);
    WMGroupButtons(panel->ghorB, panel->gdiaB);
    
    WMMapSubwidgets(panel->gdirF);
    
    /***************** Pixmap *****************/
    panel->pimagF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->pimagF, 245, 140);
    WMMoveWidget(panel->pimagF, 15, 96);
    WMSetFrameTitle(panel->pimagF, "Image");

    panel->pimagL = WMCreateLabel(panel->pimagF);
    WMResizeWidget(panel->pimagL, 220, 83);
    WMMoveWidget(panel->pimagL, 10, 20);
    WMSetLabelImagePosition(panel->pimagL, WIPImageOnly);
    
    panel->pimagT = WMCreateTextField(panel->pimagF);
    WMResizeWidget(panel->pimagT, 147, 20);
    WMMoveWidget(panel->pimagT, 10, 110);

    panel->pbrowB = WMCreateCommandButton(panel->pimagF);
    WMResizeWidget(panel->pbrowB, 68, 24);
    WMMoveWidget(panel->pbrowB, 165, 108);
    WMSetButtonText(panel->pbrowB, "Browse...");
    WMSetButtonAction(panel->pbrowB,buttonCallback,panel);

    WMMapSubwidgets(panel->pimagF);

    panel->pcolorF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->pcolorF, 90, 75);
    WMMoveWidget(panel->pcolorF, 15, 240);
    WMSetFrameTitle(panel->pcolorF, "Color");

    panel->pcolorW = WMCreateColorWell(panel->pcolorF);
    WMResizeWidget(panel->pcolorW, 60, 45);
    WMMoveWidget(panel->pcolorW, 15, 20);
    WMSetColorWellColor(panel->pcolorW,WMCreateRGBColor(scr, 0x00, 0xddff, 0xffff, True));    

    
    WMMapSubwidgets(panel->pcolorF);
    
    panel->pmodeF = WMCreateFrame(panel->win);
    WMResizeWidget(panel->pmodeF, 145, 70);
    WMMoveWidget(panel->pmodeF, 115, 245);
    
    panel->pscalB = WMCreateButton(panel->pmodeF, WBTOnOff);
    WMResizeWidget(panel->pscalB, 54, 50);
    WMMoveWidget(panel->pscalB, 10, 10);
    WMSetButtonImagePosition(panel->pscalB, WIPImageOnly);
    WMPerformButtonClick(panel->pscalB);

    image = RGetImageFromXPMData(WMScreenRContext(scr), scaled_xpm);
    if (!image) {
	wwarning("internal error:%s", RMessageForError(RErrorCode));
    } else {
	icon = WMCreatePixmapFromRImage(scr, image, 120);
	RDestroyImage(image);
	WMSetButtonImage(panel->pscalB, icon);
	WMReleasePixmap(icon);
    }

    panel->ptileB = WMCreateButton(panel->pmodeF, WBTOnOff);
    WMResizeWidget(panel->ptileB, 54, 50);
    WMMoveWidget(panel->ptileB, 75, 10);
    WMSetButtonImagePosition(panel->ptileB, WIPImageOnly);
    image = RGetImageFromXPMData(WMScreenRContext(scr), tiled_xpm);
    if (!image) {
	wwarning("internal error:%s", RMessageForError(RErrorCode));
    } else {
	icon = WMCreatePixmapFromRImage(scr, image, 120);
	RDestroyImage(image);
	WMSetButtonImage(panel->ptileB, icon);
	WMReleasePixmap(icon);
    }
    WMGroupButtons(panel->pscalB, panel->ptileB);

    WMMapSubwidgets(panel->pmodeF);
    
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    WMPerformButtonClick(panel->tsoliB);

    WMMapWidget(panel->win);
    
    renderTextureButtons (panel);


    /* register notification observers */
    WMAddNotificationObserver(notificationObserver, panel,
                              WMTextDidChangeNotification,
                              panel->pimagT);

    return panel;
}



/*
 *--------------------------------------------------------------------------
 * Test stuff
 *--------------------------------------------------------------------------
 */

#ifdef test

char *ProgName = "test";

void testOKButton(WMWidget *self, void *data){
  char *test;
  Display *dpy;
  Window win;
  Pixmap pix;
  RImage *image;

  TexturePanel *panel = (TexturePanel*)data;
  test = GetTexturePanelTextureString(panel);
  
  wwarning(test);
  
  dpy = WMScreenDisplay(WMWidgetScreen(panel->okB));
  win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, 250, 250,
			    0, 0, 0);
  XMapRaised(dpy, win);
  XFlush(dpy);
  
  image = RenderTexturePanelTexture(panel, 250, 250);

  RConvertImage(WMScreenRContext(WMWidgetScreen(panel->okB)), image, &pix);
  
  XCopyArea(dpy, pix, win, (WMScreenRContext(WMWidgetScreen(panel->okB)))->copy_gc, 0, 0, image->width, image->height, 
	    0, 0);

  SetTexturePanelTexture(panel, test);
  free (test);
  
}
void testCancelButton(WMWidget *self, void *data){
  wwarning("Exiting test....");
  exit(0);
}

void wAbort()
{
exit(1);
}

int main(int argc, char **argv)
{
    TexturePanel *panel;

    Display *dpy = XOpenDisplay("");
    WMScreen *scr;

    /* char *test; */

    WMInitializeApplication("Test", &argc, argv);

    if (!dpy) {
        wfatal("could not open display");
        exit(1);
    }
    
    scr = WMCreateSimpleApplicationScreen(dpy);

    panel = CreateTexturePanel(scr);

    SetTexturePanelOkAction(panel,(WMAction*)testOKButton,panel);
    SetTexturePanelCancelAction(panel,(WMAction*)testCancelButton,panel);

    ShowTexturePanel(panel);

    SetTexturePanelTexture(panel,"  ( tpixmap, ballot box.xpm, \"rgb:ff/de/ff\" )  ");

    WMScreenMainLoop(scr);
    return 0;
}
#endif
