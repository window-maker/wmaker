
AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/@PACKAGE@

bin_PROGRAMS = wxcopy wxpaste wdwrite getstyle setstyle seticons geticonset wmsetbg

bin_SCRIPTS = wmaker.inst wm-oldmenu2new wsetfont wkdemenu.pl

EXTRA_DIST = wmaker.inst.in bughint wm-oldmenu2new wsetfont directjpeg.c \
		wkdemenu.pl

INCLUDES = $(DFLAGS) -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib \
	@HEADER_SEARCH_PATH@

# X_EXTRA_LIBS is for libproplist in systems that need -lsocket
liblist= @LIBRARY_SEARCH_PATH@ @LIBPL@ @X_EXTRA_LIBS@

wdwrite_LDADD = $(liblist)

wxcopy_LDADD = @XLFLAGS@ @XLIBS@ 

wxpaste_LDADD = @XLFLAGS@ @XLIBS@

getstyle_LDADD = $(liblist)

setstyle_LDADD = @XLFLAGS@ @XLIBS@ $(liblist)

seticons_LDADD= $(liblist)

geticonset_LDADD= $(liblist)

wmsetbg_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a \
	$(top_builddir)/wrlib/libwraster.la \
	@LIBRARY_SEARCH_PATH@ \
	@LIBPL@ \
	@GFXLIBS@ @XLIBS@ @DLLIBS@ -lm

getstyle_SOURCES = getstyle.c

setstyle_SOURCES = setstyle.c

seticons_SOURCES = seticons.c

geticonset_SOURCES = geticonset.c

wxcopy_SOURCES = wxcopy.c

wxpaste_SOURCES = wxpaste.c

wdwrite_SOURCES = wdwrite.c

wmsetbg_SOURCES = wmsetbg.c 

CLEANFILES = wmaker.inst

wmaker.inst: $(srcdir)/wmaker.inst.in ./Makefile
	-rm -f wmaker.inst
	sed -e "s|#pkgdatadir#|$(pkgdatadir)|" \
		-e "s|#sysconfdir#|$(sysconfdir)/WindowMaker|" \
		-e "s|#version#|$(VERSION)|" \
		-e "s|#bindir#|$(bindir)|" \
		-e "s|#LITE#|@LITE@|" \
			$(srcdir)/wmaker.inst.in >wmaker.inst

	chmod 755 wmaker.inst


