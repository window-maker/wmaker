/* wxpaste.c- paste contents of cutbuffer to stdout
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>


#define LINESIZE	(4*1024)

void
help(char *progn)
{
    printf("usage: %s [-cutbuffer number]\n", progn);
}

int
main(int argc, char **argv)
{
    Display *dpy;
    int i, l;
    int buffer=0;
    char *buf;
    
    for (i=1; i<argc; i++) {
	if (argv[i][0]=='-') {
	    if (argv[i][1]=='h') {
		help(argv[0]);
		exit(0);
	    } else if (strcmp(argv[i],"-cutbuffer")==0) {
		if (i<argc-1) {
		    i++;
		    if (sscanf(argv[i],"%i", &buffer)!=1) {
			printf("%s: could not convert \"%s\" to int\n", 
			       argv[0], argv[i]);
			exit(1);
		    }
		    if (buffer<0 || buffer > 7) {
			printf("%s: invalid buffer number %i\n", argv[0],
			       buffer);
			exit(1);
		    }
		} else {
		    help(argv[0]);
		    exit(1);
		}
	    }
	} else {
	    help(argv[0]);
	    exit(1);
	}
    }
    dpy = XOpenDisplay("");
    if (!dpy) {
	printf("%s: could not open display\n", argv[0]);
	exit(1);
    }
    buf=XFetchBuffer(dpy, &l, buffer);
    if (buf)
      printf("%s", buf);
    XCloseDisplay(dpy);
    exit(1);
}

