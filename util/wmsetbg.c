/* wmsetbg.c- sets root window background image 
 *
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1998 Dan Pascu
 *  Copyright (c) 1998 Alfredo K. Kojima
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
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>
#include <wraster.h>
#include <pwd.h>
#include <sys/types.h>

#include "../src/wconfig.h"

#include <proplist.h>


#ifdef DEBUG
#include <sys/time.h>
#include <time.h>
#endif

#define WTP_TILE  1
#define WTP_SCALE 2
#define WTP_CENTER 3

char *ProgName;


/* Alfredo please take a look at this function. I don't like the way
 * it sends the XKillClient. Should it interfere this way with the rest?
 * This was added by the patch to allow the transparent background for Eterm.
 * Also for this purpose Eterm have a program named Esetroot.
 * People wanting that feature can use that instead of wmsetbg. Why do we
 * need to patch wmsetbg to do this?
 * In case you want to keep it, please also check the way it does
 * the PropModeAppend. -Dan */
static void
setPixmapProperty(Pixmap pixmap, Display *dpy, Window root)
{
    Atom prop, type;
    int format;
    unsigned long length, after;
    unsigned char *data;

    /* This will locate the property, creating it if it doesn't exist */
    prop = XInternAtom(dpy, "_XROOTPMAP_ID", False);

    if (prop == None)
        return;

    /* Clear out the old pixmap */
    XGetWindowProperty(dpy, root, prop, 0L, 1L, True, AnyPropertyType,
                       &type, &format, &length, &after, &data);

    if ((type == XA_PIXMAP) && (format == 32) && (length == 1)) {
        XKillClient(dpy, *((Pixmap *)data));
    }
    XDeleteProperty(dpy, root, prop);

    prop = XInternAtom(dpy, "_XROOTPMAP_ID", True);
    if (prop == None)
        return;

    /* Now add the new one.  We use PropModeAppend because PropModeReplace
     doesn't seem to work if there isn't already a property there. */
    XChangeProperty(dpy, root, prop, XA_PIXMAP, 32, PropModeAppend,
                    (unsigned char *) &pixmap, 1);

    XFlush(dpy);
    XSetCloseDownMode(dpy, RetainPermanent);
    XFlush(dpy);
}


void*
wmalloc(size_t size)
{
    void *ptr;
    ptr = malloc(size);
    if (!ptr) {
	perror(ProgName);
	exit(1);
    }
    return ptr;
}

char*
gethomedir()
{
    char *home = getenv("HOME");
    struct passwd *user;

    if (home)
      return home;
    
    user = getpwuid(getuid());
    if (!user) {
        perror(ProgName);
        return "/";
    }
    if (!user->pw_dir) {
        return "/";
    } else {
        return user->pw_dir;
    }
}



void wAbort()
{
    exit(1);
}


void
print_help()
{
    printf("usage: %s [-options] image\n", ProgName);
    puts("options:");
    puts(" -d		dither image");
    puts(" -m		match  colors");
    puts(" -b <color>	background color");
    puts(" -t		tile   image");
    puts(" -e		center image");
    puts(" -s		scale  image (default)");
    puts(" -u		update WindowMaker domain database");
    puts(" -D <domain>	update <domain> database");
    puts(" -c <cpc>	colors per channel to use");
}


char*
defaultsPathForDomain(char *domain)
{
    char path[1024];
    char *gspath, *tmp;

    gspath = getenv("GNUSTEP_USER_ROOT");
    if (gspath) {
        strcpy(path, gspath);
        strcat(path, "/");
    } else {
        strcpy(path, gethomedir());
        strcat(path, "/GNUstep/");
    }
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    tmp = wmalloc(strlen(path)+2);
    strcpy(tmp, path);

    return tmp;
}


char *wstrdup(char *str)
{
    return strcpy(wmalloc(strlen(str)+1), str);
}


/* Returns an array of pointers to the pixmap paths, doing ~ expansion */
static char**
getPixmapPath(char *domain)
{
    char **ret;
    char *path;
    proplist_t prop, pixmap_path, key, value;
    int count, i;

    path = defaultsPathForDomain(domain);
    if (!path)
        return NULL;

    prop = PLGetProplistWithPath(path);
    if (!prop || !PLIsDictionary(prop))
        return NULL;

    key = PLMakeString("PixmapPath");
    pixmap_path = PLGetDictionaryEntry(prop, key);
    PLRelease(key);
    if (!pixmap_path || !PLIsArray(pixmap_path))
        return NULL;

    count = PLGetNumberOfElements(pixmap_path);
    if (count < 1)
        return NULL;

    ret = wmalloc(sizeof(char*)*(count+1));

    for (i=0; i<count; i++) {
        value = PLGetArrayElement(pixmap_path, i);
        if (!value || !PLIsString(value))
            break;
        ret[i] = wstrdup(PLGetString(value));
        if (ret[i][0]=='~' && ret[i][1]=='/') {
            /* home is statically allocated. Don't free it */
            char *fullpath, *home=gethomedir();

            fullpath = wmalloc(strlen(home)+strlen(ret[i]));
            strcpy(fullpath, home);
            strcat(fullpath, &ret[i][1]);
            free(ret[i]);
            ret[i] = fullpath;
        }
    }
    ret[i] = NULL;
    return ret;
}


int
main(int argc, char **argv)
{
    Display *dpy;
    Window root_win;
    RContextAttributes rattr;
    int screen_number, default_depth, i, style = WTP_SCALE;
    int scr_width, scr_height;
    RContext *rcontext;
    RImage *image, *tmp;
    Pixmap secretBuffer = None;
    Pixmap pixmap;
    XColor xcolor;
    char *back_color = "black";
    char *image_name = NULL;
    char *domain = "WindowMaker";
    char *program = "wdwrite";
    int update=0, cpc=4, render_mode=RM_MATCH, obey_user=0;
#ifdef DEBUG
    double t1, t2, total, t;
    struct timeval timev;
#endif


    ProgName = strrchr(argv[0],'/');
    if (!ProgName)
      ProgName = argv[0];
    else
      ProgName++;

    if (argc>1) {
	for (i=1; i<argc; i++) {
            if (strcmp(argv[i], "-s")==0) {
                style = WTP_SCALE;
            } else if (strcmp(argv[i], "-t")==0) {
                style = WTP_TILE;
            } else if (strcmp(argv[i], "-e")==0) {
                style = WTP_CENTER;
            } else if (strcmp(argv[i], "-d")==0) {
                render_mode = RM_DITHER;
                obey_user++;
            } else if (strcmp(argv[i], "-m")==0) {
                render_mode = RM_MATCH;
                obey_user++;
            } else if (strcmp(argv[i], "-u")==0) {
                update++;
            } else if (strcmp(argv[i], "-D")==0) {
                update++;
                i++;
                if (i>=argc) {
                    fprintf(stderr, "too few arguments for %s\n", argv[i-1]);
                    exit(0);
                }
                domain = wstrdup(argv[i]);
            } else if (strcmp(argv[i], "-c")==0) {
                i++;
                if (i>=argc) {
		    fprintf(stderr, "too few arguments for %s\n", argv[i-1]);
                    exit(0);
                }
                if (sscanf(argv[i], "%i", &cpc)!=1) {
                    fprintf(stderr, "bad value for colors per channel: \"%s\"\n", argv[i]);
                    exit(0);
                }
            } else if (strcmp(argv[i], "-b")==0) {
                i++;
                if (i>=argc) {
		    fprintf(stderr, "too few arguments for %s\n", argv[i-1]);
                    exit(0);
                }
		back_color = argv[i];
            } else if (strcmp(argv[i], "-x")==0) {
		/* secret option:renders the pixmap in the supplied drawable */
                i++;
                if (i>=argc || 
		    sscanf(argv[i], "%x", (unsigned*)&secretBuffer)!=1) {
		    print_help();
                    exit(1);
                }
            } else if (argv[i][0] != '-') {
                image_name = argv[i];
            } else {
                print_help();
                exit(1);
            }
	}
    }

    if (image_name == NULL) {
        print_help();
        exit(1);
    }
    if (update) {
        char *value = wmalloc(sizeof(image_name) + 30);
        char *tmp=image_name, **paths;
        int i;

        /* should we read PixmapPath from the same file as we write into ? */
        paths = getPixmapPath("WindowMaker");
        if (paths) {
            for(i=0; paths[i]!=NULL; i++) {
                if ((tmp = strstr(image_name, paths[i])) != NULL &&
                    tmp == image_name) {
                    tmp += strlen(paths[i]);
                    while(*tmp=='/') tmp++;
                    break;
                }
            }
        }

        if (!tmp)
            tmp = image_name;

        if (style == WTP_TILE)
            strcpy(value, "(tpixmap, \"");
        else if (style == WTP_SCALE)
            strcpy(value, "(spixmap, \"");
        else
            strcpy(value, "(cpixmap, \"");
        strcat(value, tmp);
        strcat(value, "\", \"");
	strcat(value, back_color);
	strcat(value, "\")");
        execlp(program, program, domain, "WorkspaceBack", value, NULL);
        printf("%s: warning could not run \"%s\"\n", ProgName, program);
        /* Do not exit. At least try to put the image in the background */
	/* Won't this waste CPU for nothing? We're going to be called again, 
	 * anyways. -Alfredo */
        /* If it fails to update the WindowMaker domain with "wdwrite" we
         * won't be called again, because Window Maker will not notice any
         * change. If it reaches this point, this means it failed.
         * On success it will never get here. -Dan */
        /*exit(0);*/
    }

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("Could not open display!");
	exit(1);
    }
#ifdef DEBUG
    XSynchronize(dpy, True);
#endif
    screen_number = DefaultScreen(dpy);
    root_win = RootWindow(dpy, screen_number);
    default_depth = DefaultDepth(dpy, screen_number);
    scr_width = WidthOfScreen(ScreenOfDisplay(dpy, screen_number));
    scr_height = HeightOfScreen(ScreenOfDisplay(dpy, screen_number));

    if (!XParseColor(dpy, DefaultColormap(dpy, screen_number), back_color,
		     &xcolor)) {
	printf("invalid color %s\n", back_color);
	exit(1);
    }

    if (!obey_user && default_depth <= 8)
        render_mode = RM_DITHER;

    rattr.flags = RC_RenderMode | RC_ColorsPerChannel | RC_DefaultVisual;
    rattr.render_mode = render_mode;
    rattr.colors_per_channel = cpc;

    rcontext = RCreateContext(dpy, screen_number, &rattr);
    if (!rcontext) {
	printf("could not initialize graphics library context: %s\n",
               RErrorString);
	exit(1);
    }

#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    t = t1;
#endif
    image = RLoadImage(rcontext, image_name, 0);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("load image in %f sec\n", total);
#endif

    if (!image) {
        printf("could not load image %s:%s\n", image_name, RErrorString);
        exit(1);
    }

#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
#endif
    if (style == WTP_SCALE) {
        tmp = RScaleImage(image, scr_width, scr_height);
        if (!tmp) {
            printf("could not scale image: %s\n", image_name);
            exit(1);
        }
        RDestroyImage(image);
        image = tmp;
    } else if (style==WTP_CENTER && (image->width!=scr_width
	       || image->height!=scr_height)) {
        RColor color;

        color.red = xcolor.red>>8;
        color.green = xcolor.green>>8;
        color.blue = xcolor.blue>>8;
        color.alpha = 255;
        tmp = RMakeCenteredImage(image, scr_width, scr_height, &color);
	if (!tmp) {
	    printf("could not create centered image: %s\n", image_name);
	    exit(1);
	}
        RDestroyImage(image);
        image = tmp;
    }
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("scale image in %f sec\n", total);

    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
#endif
    RConvertImage(rcontext, image, &pixmap);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("convert image to pixmap in %f sec\n", total);
    total = t2 - t;
    printf("total image proccessing in %f sec\n", total);
#endif
    RDestroyImage(image);
    if (secretBuffer==None) {
	setPixmapProperty(pixmap, dpy, root_win);
	XSetWindowBackgroundPixmap(dpy, root_win, pixmap);
	XClearWindow(dpy, root_win);
    } else {
	XCopyArea(dpy, pixmap, secretBuffer, DefaultGC(dpy, screen_number),
		  0, 0, scr_width, scr_height, 0, 0);
    }
    XSync(dpy, False);
    XCloseDisplay(dpy);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t;
    printf("total proccessing time: %f sec\n", total);
#endif
    if (secretBuffer)
	exit(123);
    else
	exit(0);
}
