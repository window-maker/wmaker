
AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/@PACKAGE@

bin_PROGRAMS = wxcopy wxpaste wdwrite getstyle setstyle seticons geticonset wmsetbg

bin_SCRIPTS = wmaker.inst dockit 

EXTRA_DIST = wmaker.inst.in bughint dockit

INCLUDES = @DFLAGS@ -I$(top_srcdir)/wrlib @XCFLAGS@ @LIBPL_INC_PATH@ @GFXFLAGS@ 

wdwrite_LDADD = @LIBPL_LIBS@ @X_EXTRA_LIBS@ 

wxcopy_LDADD = @XLFLAGS@ @XLIBS@ 

wxpaste_LDADD = @XLFLAGS@ @XLIBS@ 

getstyle_LDADD = @LIBPL_LIBS@ @X_EXTRA_LIBS@

setstyle_LDADD = @LIBPL_LIBS@ @X_EXTRA_LIBS@

seticons_LDADD= @LIBPL_LIBS@ @X_EXTRA_LIBS@

geticonset_LDADD= @LIBPL_LIBS@ @X_EXTRA_LIBS@

wmsetbg_LDADD = @LIBPL_LIBS@ -L$(top_builddir)/wrlib -lwraster @XLFLAGS@\
	 @GFXLIBS@ @XLIBS@ -lm 

getstyle_SOURCES = getstyle.c

setstyle_SOURCES = setstyle.c

seticons_SOURCES = seticons.c

geticonset_SOURCES = geticonset.c

wxcopy_SOURCES = wxcopy.c

wxpaste_SOURCES = wxpaste.c

wdwrite_SOURCES = wdwrite.c

wmsetbg_SOURCES = wmsetbg.c

CLEANFILES = wmaker.inst

wmaker.inst: $(srcdir)/wmaker.inst.in $(srcdir)/Makefile
	-rm -f wmaker.inst
	sed -e "s:#pkgdatadir#:$(pkgdatadir):" \
		-e "s/#version#/$(VERSION)/" \
		-e "s:#bindir#:$(bindir):" \
			$(srcdir)/wmaker.inst.in >wmaker.inst

	chmod 755 wmaker.inst


