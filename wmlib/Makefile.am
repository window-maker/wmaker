
AUTOMAKE_OPTIONS = no-dependencies

libWMaker_la_LDFLAGS = -version-info 1:1:0
lib_LTLIBRARIES = libWMaker.la

include_HEADERS = WMaker.h

AM_CPPFLAGS = $(DFLAGS) @XCFLAGS@

libWMaker_la_LIBADD = -lX11

libWMaker_la_SOURCES = 	\
	menu.c 		\
	app.c 		\
	event.c 	\
	command.c	\
	app.h		\
	menu.h	

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wmlib.pc

DISTCLEANFILES = $(pkgconfig_DATA)

wmlib.pc: Makefile
	@echo "Generating $@"
	@echo 'Name: wmlib' > $@
	@echo 'Description: Window Maker interface definitions' >> $@
	@echo 'Version: $(VERSION)' >> $@
	@echo 'Requires: x11' >> $@
	@echo 'Libs: $(lib_search_path) -lWMaker' >> $@
	@echo 'Cflags: $(inc_search_path)' >> $@
