## automake input file for WINGs - Examples

AUTOMAKE_OPTIONS = no-dependencies


noinst_PROGRAMS = connect server fontl

LDADD= $(top_builddir)/WINGs/.libs/libWINGs.a \
		$(top_builddir)/WINGs/.libs/libWUtil.a \
		@XLIBS@ @XFTLIBS@ @INTLIBS@ @GLIB_LIBS@ @CAIRO_LIBS@

colorpick_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

fontl_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

puzzle_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

connect_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.la

connect_LDADD = $(top_builddir)/WINGs/libWUtil.la @LIBRARY_SEARCH_PATH@ @INTLIBS@ @CAIRO_LIBS@

server_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.la

server_LDADD = $(top_builddir)/WINGs/libWUtil.la @LIBRARY_SEARCH_PATH@ @INTLIBS@


INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ \
	@XCFLAGS@ @GLIB_CFLAGS@ @CAIRO_CFLAGS@ -DDEBUG

LIBTOOL = $(QUIET) $(SHELL) $(top_builddir)/libtool $(LIBTOOL_ARG)

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
