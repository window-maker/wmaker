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

LIBLIST = -L. -lwraster @GFXLFLAGS@ @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

view_SOURCES= view.c

view_LDADD = $(LIBLIST)

test_SOURCES = test.c

test_LDADD = $(LIBLIST)

testgrad_SOURCES = testgrad.c

testgrad_LDADD = $(LIBLIST)

testdraw_SOURCES = testdraw.c

testdraw_LDADD = $(LIBLIST)


libwraster_a_LIBADD = @ALLOCA@


