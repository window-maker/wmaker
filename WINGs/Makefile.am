## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = Resources


LIBLIST= $(top_builddir)/wrlib/libwraster.la\
	 @GFXLFLAGS@ @XLFLAGS@ @GFXLIBS@ @XLIBS@ \
	-lm $(top_builddir)/libPropList/libPropList.la


#lib_LTLIBRARIES = libWINGs.la

lib_LIBRARIES = libWINGs.a

#libWINGs_la_LDFLAGS = -version-info 1:1:0

include_HEADERS = WINGs.h WUtil.h WINGsP.h

noinst_PROGRAMS = wtest wmquery wmfile fontl testmywidget

testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h

testmywidget_LDADD = libWINGs.a $(LIBLIST)

fontl_SOURCES = fontl.c

fontl_LDADD = libWINGs.a $(LIBLIST)

wtest_SOURCES = wtest.c

wtest_LDADD = libWINGs.a $(LIBLIST)

wtest_DEPENDENCIES = libWINGs.a

wmfile_SOURCES = wmfile.c 

wmfile_LDADD = libWINGs.a $(LIBLIST)


wmquery_SOURCES = wmquery.c 

wmquery_LDADD = libWINGs.a $(LIBLIST)


EXTRA_DIST = logo.xpm 

# wbutton.c 
libWINGs_a_SOURCES = 	\
	WINGs.h \
	WINGsP.h \
	configuration.c \
	international.c \
	notification.c \
	selection.c \
	userdefaults.c \
	wapplication.c \
	wbrowser.c \
	wbutton.c \
	wcolor.c \
	wcolorwell.c \
	wevent.c \
	wfilepanel.c \
	wframe.c \
	wfont.c \
	wfontpanel.c \
	widgets.c \
	wlabel.c \
	wlist.c \
	wmisc.c \
	wpanel.c \
	wpixmap.c \
	wpopupbutton.c \
	wscroller.c \
	wscrollview.c \
	wslider.c \
	wsplitview.c \
	wtextfield.c \
	wwindow.c \
	wview.c \
	error.c \
	findfile.c \
	hashtable.c \
	memory.c \
	usleep.c

##
## Find a better way than $(GFXFLAGS) to inform widgets.c wich of 
## tiff or xpm images should be used

INCLUDES = -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" $(GFXFLAGS) -DDEBUG \
	-I$(top_srcdir)/libPropList @XCFLAGS@

