## automake input file for WINGs - Examples

AUTOMAKE_OPTIONS = no-dependencies


noinst_PROGRAMS = connect server fontl puzzle colorpick


LDADD= $(top_builddir)/WINGs/libWINGs.a $(top_builddir)/wrlib/libwraster.la \
	@XFTLIBS@ @INTLIBS@

colorpick_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

fontl_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

puzzle_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

connect_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.a

connect_LDADD = $(top_builddir)/WINGs/libWUtil.a @LIBRARY_SEARCH_PATH@ \
	@NETLIBS@ @INTLIBS@

server_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.a

server_LDADD = $(top_builddir)/WINGs/libWUtil.a @LIBRARY_SEARCH_PATH@ \
	@NETLIBS@ @INTLIBS@


INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG


