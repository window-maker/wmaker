## automake input file for WINGs - Tests

AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = wtest wmquery wmfile testmywidget 

LDADD= $(top_builddir)/WINGs/.libs/libWINGs.a \
		$(top_builddir)/WINGs/.libs/libWUtil.a \
		@XLIBS@ @XFTLIBS@ @INTLIBS@ @CAIRO_LIBS@ @GLIB_LIBS@

testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h 

wtest_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la


EXTRA_DIST = logo.xpm upbtn.xpm wm.html wm.png

INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @XFTFLAGS@ @HEADER_SEARCH_PATH@ \
	-DDEBUG @XCFLAGS@ @GLIB_CFLAGS@ @CAIRO_CFLAGS@
	
LIBTOOL = $(QUIET) $(SHELL) $(top_builddir)/libtool $(LIBTOOL_ARG)

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
