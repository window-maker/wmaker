
/*
 * Author: Len Trigg <trigg@cs.waikato.ac.nz> 
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

void usage(void)
{
  fprintf(stderr,
	  "usage:\n"
	  "\t%s [-options]\n"
	  "\n"
	  "options:\n"
	  "  -i <str>\tInitial directory (default /)\n"
	  "  -t <str>\tQuery window title (default none)\n"
	  "\n"
	  "information:\n"
	  "\t%s pops up a WindowMaker style file selection panel.\n"
	  "\n"
	  "version:\n"
	  "\t%s\n"
	  ,ProgName,ProgName,__DATE__
	  );
  exit(0);
}

int main(int argc, char **argv)
{
  Display *dpy = XOpenDisplay("");
  WMScreen *scr;
  WMPixmap *pixmap;
  WMOpenPanel *panel;
/*  RImage *image;*/
  char *title = NULL;
  char *initial = "/";
  int ch;
  extern char *optarg;
  extern int optind;

  WMInitializeApplication("WMFile", &argc, argv);
    
  ProgName = argv[0];

  if (!dpy) {
    puts("could not open display");
    exit(1);
  }
  while((ch = getopt(argc, argv, "i:ht:")) != -1)
    switch(ch) 
    { 
    case 'i':
      initial = optarg;
      break;
    case 't':
      title = optarg;
      break;
    default:
      usage();
    }

  for(; optind <argc; optind++)
    usage();

  scr = WMCreateSimpleApplicationScreen(dpy);


	
  pixmap = WMCreatePixmapFromXPMData(scr, GNUSTEP_XPM);
  WMSetApplicationIconImage(scr, pixmap); WMReleasePixmap(pixmap);
  panel = WMGetOpenPanel(scr);
  
 /* The 3rd argument for this function is the initial name of the file,
  * not the name of the window, although it's not implemented yet */
  if (WMRunModalOpenPanelForDirectory(panel, NULL, initial, /*title*/ NULL, NULL) == True)
    printf("%s\n", WMGetFilePanelFileName(panel));
  else
    printf("\n");
  return 0;
}
