## automake input file for wrlib

SUBDIRS = . tests

AUTOMAKE_OPTIONS = no-dependencies

lib_LTLIBRARIES = libwraster.la

libwraster_la_LDFLAGS = -version-info 4:0:1

bin_SCRIPTS = get-wraster-flags

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


INCLUDES = $(DFLAGS) @HEADER_SEARCH_PATH@

libwraster_la_LIBADD = @ALLOCA@ @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ -lm

DISTCLEANFILES = wrlib.pc

install-exec-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/pkgconfig
	@list='wrlib.pc'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/; \
	  else :; fi; \
	done
