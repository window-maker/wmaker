## automake input file for WINGs - Examples

AUTOMAKE_OPTIONS = no-dependencies


noinst_PROGRAMS = connect fontl puzzle


LDADD= $(top_builddir)/WINGs/libWINGs.a $(top_builddir)/wrlib/libwraster.la \
	@LIBPL@


fontl_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

puzzle_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

connect_DEPENDENCIES = $(top_builddir)/WINGs/libWUtil.a

connect_LDADD = $(top_builddir)/WINGs/libWUtil.a @LIBRARY_SEARCH_PATH@ \
	@NETLIBS@ @LIBPL@


INCLUDES = -I$(top_srcdir)/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG


