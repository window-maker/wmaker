
AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/@PACKAGE@

bin_PROGRAMS = wxcopy wxpaste wdwrite getstyle setstyle seticons geticonset wmsetbg

bin_SCRIPTS = wmaker.inst wm-oldmenu2new

EXTRA_DIST = wmaker.inst.in bughint wm-oldmenu2new

INCLUDES = @DFLAGS@ @XCFLAGS@ -I$(top_srcdir)/wrlib -I$(top_srcdir)/libPropList @GFXFLAGS@ 

# X_EXTRA_LIBS is for libproplist in systems that need -lsocket 
liblist= -L$(top_builddir)/libPropList -lPropList @X_EXTRA_LIBS@ 


wdwrite_LDADD = $(liblist)

wxcopy_LDADD = @XLFLAGS@ @XLIBS@ 

wxpaste_LDADD = @XLFLAGS@ @XLIBS@ 

getstyle_LDADD = $(liblist)

setstyle_LDADD = $(liblist)

seticons_LDADD= $(liblist)

geticonset_LDADD= $(liblist)

wmsetbg_LDADD = -L$(top_builddir)/libPropList -lPropList \
	-L$(top_builddir)/wrlib -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm 

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


