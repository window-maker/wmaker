## automake input file for wrlib

SUBDIRS = . po
DIST_SUBDIRS = $(SUBDIRS) tests

AUTOMAKE_OPTIONS =

EXTRA_DIST = tests

lib_LTLIBRARIES = libwraster.la

libwraster_la_LDFLAGS = -version-info @WRASTER_VERSION@

CLEANFILES = wraster.h
EXTRA_DIST += wraster.h.in

if HAVE_LD_VERSION_SCRIPT
libwraster_la_LDFLAGS += -Wl,--version-script=libwraster.map
EXTRA_libwraster_la_DEPENDENCIES = libwraster.map
CLEANFILES += libwraster.map
endif

BUILT_SOURCES = wraster.h
nodist_include_HEADERS = wraster.h

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
	wr_i18n.h	\
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

DISTCLEANFILES = $(pkgconfig_DATA)

wrlib.pc: Makefile
	@echo "Generating $@"
	@echo 'Name: wrlib' > $@
	@echo 'Description: Image manipulation and conversion library' >> $@
	@echo 'Version: $(VERSION)' >> $@
	@echo 'Libs: $(lib_search_path) -lwraster' >> $@
	@echo 'Libs.private: $(GFXLIBS) $(MAGICKLIBS) $(XLIBS) -lm' >> $@
	@echo 'Cflags: $(inc_search_path)' >> $@

wraster.h: wraster.h.in $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" \
		--filter "USE_XSHM" \
		-o $@  $<


if HAVE_LD_VERSION_SCRIPT
libwraster.map: $(nodist_include_HEADERS) $(top_srcdir)/script/generate-mapfile-from-header.sh
	$(AM_V_GEN)$(top_srcdir)/script/generate-mapfile-from-header.sh \
		-n LIBWRASTER -v $(WRASTER_VERSION) $<  >  $@
endif
