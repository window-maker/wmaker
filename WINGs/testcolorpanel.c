/*
 * Author: Pascal Hofstee <daeron@shadowmere.student.utwente.nl> 
 */


#include "WINGs.h"

#include <unistd.h>
#include <stdio.h>

#include "logo.xpm"

void
wAbort()
{
    exit(1);
}

char *ProgName;


int main(int argc, char **argv)
{
  Display *dpy = XOpenDisplay("");
  WMScreen		*scr;
  WMPixmap		*pixmap;
  WMColorPanel	*panel;
  RColor		startcolor;

  WMInitializeApplication("WMColorPicker", &argc, argv);
    
  ProgName = argv[0];

  if (!dpy) {
    puts("could not open display");
    exit(1);
  }

  scr = WMCreateSimpleApplicationScreen(dpy);


	
  pixmap = WMCreatePixmapFromXPMData(scr, GNUSTEP_XPM);
  WMSetApplicationIconImage(scr, pixmap); WMReleasePixmap(pixmap);
  panel = WMGetColorPanel(scr);
  
  startcolor.red = 0;
  startcolor.green = 0;
  startcolor.blue = 255;
  
  WMRunColorPanel(panel, NULL, startcolor);
  return 0;
}
