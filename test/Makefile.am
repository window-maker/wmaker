## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

EXTRA_DIST = notest.c

noinst_PROGRAMS = wtest

wtest_SOURCES = wtest.c

wtest_LDADD = $(top_builddir)/wmlib/libWMaker.a @XLFLAGS@ @XLIBS@ 


INCLUDES = -g -D_BSD_SOURCE @XCFLAGS@ -I$(top_srcdir)/wmlib 

