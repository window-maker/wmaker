## automake input file for wrlib

SUBDIRS = .
DIST_SUBDIRS = $(SUBDIRS) tests

AUTOMAKE_OPTIONS =

EXTRA_DIST = tests libwraster.map get-wraster-flags.in

lib_LTLIBRARIES = libwraster.la

libwraster_la_LDFLAGS = -version-info @WRASTER_VERSION@

if HAVE_LD_VERSION_SCRIPT
libwraster_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libwraster.map
endif

bin_SCRIPTS = get-wraster-flags

include_HEADERS = wraster.h

libwraster_la_SOURCES = 	\
	imgformat.h 	\
	raster.c 	\
	alpha_combine.c \
	draw.c		\
	color.c		\
	load.c 		\
	save.c		\
	gradient.c 	\
	xpixmap.c	\
	convert.h 	\
	convert.c 	\
	context.c 	\
	misc.c 		\
	scale.c		\
	scale.h		\
	rotate.c	\
	rotate.h	\
	flip.c		\
	convolve.c	\
	save_xpm.c	\
	xutil.c		\
	xutil.h		\
	load_ppm.c

if USE_GIF
libwraster_la_SOURCES += load_gif.c
endif

if USE_JPEG
libwraster_la_SOURCES += load_jpeg.c
endif

if USE_PNG
libwraster_la_SOURCES += load_png.c
endif

if USE_TIFF
libwraster_la_SOURCES += load_tiff.c
endif

if USE_XPM
libwraster_la_SOURCES += load_xpm.c
else
libwraster_la_SOURCES += load_xpm_normalized.c
endif

if USE_WEBP
libwraster_la_SOURCES += load_webp.c
endif

if USE_MAGICK
libwraster_la_SOURCES += load_magick.c
endif

AM_CFLAGS = @MAGICKFLAGS@
AM_CPPFLAGS = $(DFLAGS) @HEADER_SEARCH_PATH@

libwraster_la_LIBADD = @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @MAGICKLIBS@ @XLIBS@ @LIBXMU@ -lm

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wrlib.pc

DISTCLEANFILES = $(pkgconfig_DATA) get-wraster-flags

wrlib.pc: Makefile
	@echo "Generating $@"
	@echo 'Name: wrlib' > $@
	@echo 'Description: Image manipulation and conversion library' >> $@
	@echo 'Version: $(VERSION)' >> $@
	@echo 'Libs: $(lib_search_path) -lwraster' >> $@
	@echo 'Libs.private: $(GFXLIBS) $(MAGICKLIBS) $(XLIBS) -lm' >> $@
	@echo 'Cflags: $(inc_search_path)' >> $@

get-wraster-flags: get-wraster-flags.in Makefile
	@echo "Generating $@"
	@$(SED) -e 's#$${inc_search_path}#$(inc_search_path)#;' \
	        -e 's#$${lib_search_path}#$(lib_search_path)#;' \
	        -e 's#$${GFXLIBS}#$(GFXLIBS)#;' \
	        -e 's#$${XLIBS}#$(XLIBS)#;' < $(abs_srcdir)/get-wraster-flags.in > $@
	@chmod 755 $@

