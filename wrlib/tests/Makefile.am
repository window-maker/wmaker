## automake input file for wrlib

AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = testdraw testgrad testrot view

EXTRA_DIST = test.png tile.xpm ballot_box.xpm 
include_HEADERS = $(top_builddir)/wrlib/wraster.h

INCLUDES = $(DFLAGS) @HEADER_SEARCH_PATH@

LIBLIST = $(top_builddir)/wrlib/libwraster.la

testdraw_SOURCES = testdraw.c
testdraw_LDADD = $(LIBLIST)

testgrad_SOURCES = testgrad.c
testgrad_LDADD = $(LIBLIST)

testrot_SOURCES = testrot.c
testrot_LDADD = $(LIBLIST)

view_SOURCES= view.c
view_LDADD = $(LIBLIST)

