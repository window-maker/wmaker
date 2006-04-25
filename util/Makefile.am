
AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/@PACKAGE@

bin_PROGRAMS = wxcopy wxpaste wdwrite wdread getstyle setstyle convertfonts \
	seticons geticonset wmsetbg wmsetup wmagnify

bin_SCRIPTS = wmaker.inst wm-oldmenu2new wkdemenu.pl

EXTRA_DIST = wmaker.inst.in bughint wm-oldmenu2new directjpeg.c wkdemenu.pl

INCLUDES = $(DFLAGS) -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib \
	@HEADER_SEARCH_PATH@ \
	-DETCDIR=\"sysconfdir\" -DDATADIR=\"pkgdatadir\"

liblist= @LIBRARY_SEARCH_PATH@ @INTLIBS@

wdwrite_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

wdread_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

wxcopy_LDADD = @XLFLAGS@ @XLIBS@ 

wxpaste_LDADD = @XLFLAGS@ @XLIBS@

getstyle_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

getstyle_SOURCES = getstyle.c fontconv.c

setstyle_LDADD = \
	$(top_builddir)/WINGs/libWUtil.a \
	@XLFLAGS@ @XLIBS@ $(liblist)

setstyle_SOURCES = setstyle.c fontconv.c

convertfonts_LDADD = $(top_builddir)/WINGs/libWUtil.a $(liblist)

convertfonts_SOURCES = convertfonts.c fontconv.c

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
	@XLFLAGS@ @XLIBS@ @XFTLIBS@ @INTLIBS@ @DLLIBS@

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

