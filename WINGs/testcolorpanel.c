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
  
  WMShowColorPanel(panel);

  WMScreenMainLoop(scr);
  return 0;
}
