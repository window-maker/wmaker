## automake input file for wrlib

AUTOMAKE_OPTIONS = no-dependencies

lib_LTLIBRARIES = libwraster.la

libwraster_la_LDFLAGS = -version-info 2:0:0

bin_SCRIPTS = get-wraster-flags

noinst_PROGRAMS = testgrad testdraw view testrot

EXTRA_DIST = test.png tile.xpm ballot_box.xpm 
include_HEADERS = wraster.h

libwraster_la_SOURCES = 	\
	LookupCmap.c	\
	StdCmap.c	\
	StdCmap.h	\
	CrCmap.c	\
	DelCmap.c	\
	CmapAlloc.c	\
	raster.c 	\
	draw.c		\
	color.c		\
	load.c 		\
	save.c		\
	gradient.c 	\
	xpixmap.c	\
	convert.c 	\
	x86_specific.c	\
	context.c 	\
	misc.c 		\
	scale.c		\
	rotate.c	\
	convolve.c	\
	nxpm.c		\
	xpm.c 		\
	xutil.c		\
	ppm.c		\
	png.c 		\
	jpeg.c 		\
	tiff.c		\
	gif.c


LTCOMPILE2=`echo $(LTCOMPILE) | sed -e s/-fomit-frame-pointer//`
COMPILE2=`echo $(COMPILE) | sed -e s/-fomit-frame-pointer//`

# cant compile asm stuff with optimizations
x86_specific.lo: x86_specific.c
	$(LTCOMPILE2) -O0 -c $<

x86_specific.o: x86_specific.c
	$(COMPILE2) -O0 -c $<




INCLUDES = @HEADER_SEARCH_PATH@

LIBLIST = $(top_builddir)/wrlib/libwraster.la @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ -lm

view_SOURCES= view.c

view_LDADD = $(LIBLIST)

testgrad_SOURCES = testgrad.c

testgrad_LDADD = $(LIBLIST)

testrot_SOURCES = testrot.c

testrot_LDADD = $(LIBLIST)


testdraw_SOURCES = testdraw.c

testdraw_LDADD = $(LIBLIST)


libwraster_la_LIBADD = @ALLOCA@ 


