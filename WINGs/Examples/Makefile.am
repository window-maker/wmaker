## automake input file for WINGs - Examples

AUTOMAKE_OPTIONS =


noinst_PROGRAMS = connect server fontl puzzle colorpick


LDADD= $(top_builddir)/WINGs/libWINGs.la $(top_builddir)/wrlib/libwraster.la \
	$(top_builddir)/WINGs/libWUtil.la \
	@XFTLIBS@ @INTLIBS@

colorpick_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

fontl_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

puzzle_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.la

connect_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.la

connect_LDADD = $(top_builddir)/WINGs/libWUtil.la @LIBRARY_SEARCH_PATH@ @INTLIBS@

server_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.la

server_LDADD = $(top_builddir)/WINGs/libWUtil.la @LIBRARY_SEARCH_PATH@ @INTLIBS@


AM_CPPFLAGS = -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@
