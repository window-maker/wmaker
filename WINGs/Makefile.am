## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = Resources


LIBLIST= $(top_builddir)/wrlib/libwraster.la\
	 @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ \
	-lm @LIBPL@


#lib_LTLIBRARIES = libWINGs.la

lib_LIBRARIES = libWINGs.a libWUtil.a

#libWINGs_la_LDFLAGS = -version-info 1:1:0

include_HEADERS = WINGs.h WUtil.h WINGsP.h

noinst_PROGRAMS = wtest wmquery wmfile fontl testmywidget testcolorpanel connect

testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h

testmywidget_LDADD = libWINGs.a $(LIBLIST)

fontl_SOURCES = fontl.c

fontl_LDADD = libWINGs.a $(LIBLIST)

wtest_SOURCES = wtest.c

wtest_LDADD = libWINGs.a $(LIBLIST)

wtest_DEPENDENCIES = libWINGs.a

wmfile_SOURCES = wmfile.c 

wmfile_LDADD = libWINGs.a $(LIBLIST)
	
testcolorpanel_SOURCES = testcolorpanel.c

testcolorpanel_LDADD = libWINGs.a $(LIBLIST)

wmquery_SOURCES = wmquery.c 

wmquery_LDADD = libWINGs.a $(LIBLIST)

connect_SOURCES = connect.c

connect_LDADD = libWUtil.a @NETLIBS@ @LIBPL@


EXTRA_DIST = logo.xpm BUGS

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
	wappresource.c \
	wballoon.c \
	wbrowser.c \
	wbutton.c \
	wcolor.c \
	wcolorpanel.c \
	wcolorwell.c \
	wevent.c \
	wfilepanel.c \
	wframe.c \
	wfont.c \
	wfontpanel.c \
	widgets.c \
	wlabel.c \
	wlist.c \
	wmenuitem.c \
	wmisc.c \
	wpanel.c \
	wpixmap.c \
	wpopupbutton.c \
	wprogressindicator.c \
	wscroller.c \
	wscrollview.c \
	wslider.c \
	wsplitview.c \
	wtabview.c \
	wtextfield.c \
	wwindow.c \
	wview.c \
	error.c \
	findfile.c \
	bag.c \
	connection.c \
	data.c \
	hashtable.c \
	host.c \
	memory.c \
	usleep.c


libWUtil_a_SOURCES = 	\
	WINGs.h \
	WINGsP.h \
	bag.c \
	connection.c \
	data.c \
	host.c \
	international.c \
	notification.c \
	userdefaults.c \
	wapplication.c \
	wutil.c \
	error.c \
	findfile.c \
	hashtable.c \
	memory.c \
	usleep.c


INCLUDES = -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG


wcolorpanel.o: wcolorpanel.c
	$(COMPILE) -c -DRGBTXT="\"@X_LIBRARY_PATH@/X11/rgb.txt\"" $<
