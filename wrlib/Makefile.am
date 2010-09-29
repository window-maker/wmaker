## automake input file for wrlib

SUBDIRS = .

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

INCLUDES = $(DFLAGS) @HEADER_SEARCH_PATH@

libwraster_la_LIBADD = @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ @LIBXMU@ -lm

DISTCLEANFILES = wrlib.pc get-wraster-flags

wrlib.pc: Makefile
	@echo "Generating $@"
	@echo 'Name: wrlib' > $@
	@echo 'Description: Image manipulation and conversion library' >> $@
	@echo 'Version: $(VERSION)' >> $@
	@echo 'Libs: $(lib_search_path) -lwraster $(GFXLIBS) $(XLIBS) -lm' >> $@
	@echo 'Cflags: $(inc_search_path)' >> $@

get-wraster-flags: get-wraster-flags.in Makefile
	@echo "Generating $@"
	@$(SED) -e 's#$${inc_search_path}#$(inc_search_path)#;' \
	        -e 's#$${lib_search_path}#$(lib_search_path)#;' \
	        -e 's#$${GFXLIBS}#$(GFXLIBS)#;' \
	        -e 's#$${XLIBS}#$(XLIBS)#;' < $< > $@
	@chmod 755 $@


install-exec-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/pkgconfig
	@list='wrlib.pc'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/; \
	  else :; fi; \
	done


