## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = Resources



#libWINGs_la_LDFLAGS = -version-info 1:1:0

include_HEADERS = WINGs.h WUtil.h WINGsP.h

bin_SCRIPTS = get-wings-flags get-wutil-flags

noinst_PROGRAMS = wtest wmquery wmfile fontl testmywidget testcolorpanel\
	connect puzzle # testtext

lib_LIBRARIES = libWINGs.a libWUtil.a


LDADD= libWINGs.a $(top_builddir)/wrlib/libwraster.la\
	 @LIBRARY_SEARCH_PATH@ @GFXLIBS@ @XLIBS@ \
	-lm @LIBPL@


testmywidget_SOURCES = testmywidget.c mywidget.c mywidget.h 

wtest_DEPENDENCIES = libWINGs.a

connect_LDADD = libWUtil.a @LIBRARY_SEARCH_PATH@ @NETLIBS@ @LIBPL@


EXTRA_DIST = logo.xpm BUGS wm.png upbtn.xpm wm.html

# wbutton.c 
libWINGs_a_SOURCES = 	\
	WINGs.h \
	WINGsP.h \
	configuration.c \
	dragdestination.c \
	dragsource.c \
	fontmanager.c \
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
	wruler.c \
	wscroller.c \
	wscrollview.c \
	wslider.c \
	wsplitview.c \
	wtabview.c \
	wtext.c \
	wtextfield.c \
	wwindow.c \
	wview.c \
	error.c \
	findfile.c \
	bagarray.c \
	bagtree.c \
	connection.c \
	data.c \
	hashtable.c \
	host.c \
	memory.c \
	usleep.c


libWUtil_a_SOURCES = 	\
	WINGs.h \
	WINGsP.h \
	bagarray.c \
	bagtree.c \
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
