## automake input file for wrlib

AUTOMAKE_OPTIONS = no-dependencies

lib_LIBRARIES = libwraster.a

noinst_PROGRAMS = test testgrad testdraw view 

EXTRA_DIST = test.png tile.xpm ballot_box.xpm ballot_box.tiff

include_HEADERS = wraster.h

libwraster_a_SOURCES = 	\
	raster.c 	\
	draw.c		\
	color.c		\
	load.c 		\
	save.c		\
	gradient.c 	\
	xpixmap.c	\
	convert.c 	\
	context.c 	\
	misc.c 		\
	scale.c		\
	convolve.c	\
	nxpm.c		\
	xpm.c 		\
	xutil.c		\
	ppm.c		\
	png.c 		\
	jpeg.c 		\
	tiff.c		\
	gif.c


INCLUDES = @DFLAGS@ @XCFLAGS@ @GFXFLAGS@ @XSHM@

view_SOURCES= view.c

view_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

test_SOURCES = test.c

test_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

testgrad_SOURCES = testgrad.c

testgrad_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

testdraw_SOURCES = testdraw.c

testdraw_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm


libwraster_a_LIBADD = @ALLOCA@


