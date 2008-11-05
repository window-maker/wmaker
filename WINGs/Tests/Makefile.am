## automake input file for WINGs - Tests

AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = wtest wmquery wmfile testmywidget 

LDADD= $(top_builddir)/WINGs/libWINGs.a $(top_builddir)/wrlib/libwraster.la \
	@XFTLIBS@ @INTLIBS@


testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h 

wtest_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a


EXTRA_DIST = logo.xpm upbtn.xpm wm.html wm.png

INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @XFTFLAGS@ @HEADER_SEARCH_PATH@ \
	-DDEBUG
	
LIBTOOL = $(QUIET) $(SHELL) $(top_srcdir)/libtool $(LIBTOOL_ARG)

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
