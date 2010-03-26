## automake input file for WINGs - Tests

AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = draw

LDADD= @XLIBS@ @XFTLIBS@ @INTLIBS@ @CAIRO_LIBS@ @GLIB_LIBS@


INCLUDES = @XFTFLAGS@ @HEADER_SEARCH_PATH@ \
	-DDEBUG @GLIB_CFLAGS@ @CAIRO_CFLAGS@

LIBTOOL = $(QUIET) $(SHELL) $(top_srcdir)/libtool $(LIBTOOL_ARG)

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
