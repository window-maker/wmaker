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
  WMColor	*startcolor;
  char		*colorname = NULL;
  int		i;

#if 0
    XSynchronize(dpy, True);
    fprintf(stderr, "...Running Synchronous...\n");
#endif

  WMInitializeApplication("WMColorPicker", &argc, argv);
    
  ProgName = argv[0];

  if (!dpy) {
    puts("could not open display");
    exit(1);
  }

  for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0) {
	  printf("testcolorpanel [-h] [--help] [-c <color>]"
		 "[--color <color>]\n");
	  exit(0);
      }
      if (strcmp(argv[i], "-c")==0 || strcmp(argv[i], "--color")==0) {
	  i++;
	  if (i >= argc) {
	      printf("%s: missing argument for option '%s'\n",
		      argv[0], argv[i-1]);
	      exit(1);
	  }
	  colorname = argv[i];
      }
  }

  scr = WMCreateSimpleApplicationScreen(dpy);
	
  pixmap = WMCreatePixmapFromXPMData(scr, GNUSTEP_XPM);
  WMSetApplicationIconImage(scr, pixmap);
  WMReleasePixmap(pixmap);
  
  panel = WMGetColorPanel(scr);

  if (colorname) {
      startcolor = WMCreateNamedColor(scr, colorname, False);
      WMSetColorPanelColor(panel, startcolor);
      WMReleaseColor(startcolor);
  }
  
  WMShowColorPanel(panel);

  WMScreenMainLoop(scr);
  return 0;
}
