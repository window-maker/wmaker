## automake input file for WINGs - Tests

AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = wtest wmquery wmfile testmywidget testcolorpanel testtext

LDADD= $(top_builddir)/WINGs/libWINGs.a $(top_builddir)/wrlib/libwraster.la \
	@LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ \
	-lm @LIBPL@


testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h 

wtest_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

testtext_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a


EXTRA_DIST = logo.xpm upbtn.xpm

INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG


