
AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/@PACKAGE@

bin_PROGRAMS = wxcopy wxpaste wdwrite wdread getstyle setstyle seticons geticonset wmsetbg wmsetup wmagnify

bin_SCRIPTS = wmaker.inst wm-oldmenu2new wsetfont wmchlocale wkdemenu.pl\
	wcopy wpaste

EXTRA_DIST = wmaker.inst.in bughint wm-oldmenu2new wsetfont directjpeg.c \
		wkdemenu.pl wmchlocale.in wcopy wpaste

INCLUDES = $(DFLAGS) -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib \
	@HEADER_SEARCH_PATH@ \
	-DETCDIR=\"sysconfdir\" -DDATADIR=\"pkgdatadir\"

liblist= @LIBRARY_SEARCH_PATH@ @INTLIBS@

wdwrite_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

wdread_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

wxcopy_LDADD = @XLFLAGS@ @XLIBS@ 

wxpaste_LDADD = @XLFLAGS@ @XLIBS@

getstyle_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

setstyle_LDADD = \
	$(top_builddir)/WINGs/libWUtil.a \
	@XLFLAGS@ @XLIBS@ $(liblist)

seticons_LDADD= $(top_builddir)/WINGs/libWUtil.a $(liblist)

geticonset_LDADD= $(top_builddir)/WINGs/libWUtil.a $(liblist)

wmagnify_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a \
	$(top_builddir)/wrlib/libwraster.la \
	@XFTLIBS@ @INTLIBS@ @DLLIBS@

wmsetup_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a \
	$(top_builddir)/wrlib/libwraster.la \
	@XFTLIBS@ @INTLIBS@ @DLLIBS@

wmsetbg_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a \
	$(top_builddir)/wrlib/libwraster.la \
	@XFTLIBS@ @INTLIBS@ @DLLIBS@

CLEANFILES = wmaker.inst wmchlocale

wmaker.inst: $(srcdir)/wmaker.inst.in ./Makefile
	-rm -f wmaker.inst
	sed -e "s|#pkgdatadir#|$(pkgdatadir)|" \
		-e "s|#sysconfdir#|$(sysconfdir)/WindowMaker|" \
		-e "s|#version#|$(VERSION)|" \
		-e "s|#bindir#|$(bindir)|" \
		-e "s|#LITE#|@LITE@|" \
			$(srcdir)/wmaker.inst.in >wmaker.inst

	chmod 755 wmaker.inst

wmchlocale: $(srcdir)/wmchlocale.in ./Makefile
	-rm -f wmchlocale
	sed -e "s|#pkgdatadir#|$(pkgdatadir)|" $(srcdir)/wmchlocale.in >wmchlocale

	chmod 755 wmchlocale

