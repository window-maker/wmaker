## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies



include_HEADERS = wtableview.h wtabledelegates.h 


lib_LIBRARIES = libExtraWINGs.a 

noinst_PROGRAMS = test

EXTRA_DIST = 

libExtraWINGs_a_SOURCES = 	\
	wtableview.c \
	wtabledelegates.c \
	wtableview.h \
	wtabledelegates.h 


INCLUDES = -I$(top_srcdir)/wrlib -I$(top_srcdir)/WINGs \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG

LDADD= $(top_builddir)/WINGs/libWINGs.a $(top_builddir)/wrlib/libwraster.la \
        @LIBPL@ 

test_LDADD = wtableview.o wtabledelegates.o $(LDADD)

