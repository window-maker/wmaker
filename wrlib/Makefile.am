## automake input file for wrlib

AUTOMAKE_OPTIONS = no-dependencies

lib_LTLIBRARIES = libwraster.la

libwraster_la_LDFLAGS = -version-info 3:1:2

bin_SCRIPTS = get-wraster-flags

noinst_PROGRAMS = testgrad testdraw view 

EXTRA_DIST = test.png tile.xpm ballot_box.xpm 
include_HEADERS = wraster.h

libwraster_la_SOURCES = 	\
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


INCLUDES = @DFLAGS@ @HEADER_SEARCH_PATH@

LIBLIST = $(top_builddir)/wrlib/libwraster.la @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ -lm

view_SOURCES= view.c

view_LDADD = $(LIBLIST)

testgrad_SOURCES = testgrad.c

testgrad_LDADD = $(LIBLIST)

testdraw_SOURCES = testdraw.c

testdraw_LDADD = $(LIBLIST)


libwraster_la_LIBADD = @ALLOCA@


