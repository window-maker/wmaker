## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = WINGs . po Documentation Resources Examples Extras Tests



#libWINGs_la_LDFLAGS = -version-info 1:1:0

bin_SCRIPTS = get-wings-flags get-wutil-flags

lib_LIBRARIES = libWINGs.a libWUtil.a


LDADD= libWINGs.a $(top_builddir)/wrlib/libwraster.la @LIBPL@ @INTLIBS@


EXTRA_DIST = BUGS

# wbutton.c 
libWINGs_a_SOURCES = 	\
	array.c \
	bagtree.c \
	configuration.c \
	connection.c \
	data.c \
	dragdestination.c \
	dragsource.c \
	error.c \
	findfile.c \
	handlers.c \
	hashtable.c \
	host.c \
	international.c \
	memory.c \
	misc.c \
	notification.c \
	selection.c \
	string.c \
	tree.c \
	userdefaults.c \
	usleep.c \
	wapplication.c \
	wappresource.c \
	wballoon.c \
	wbox.c \
	wbrowser.c \
	wbutton.c \
	wcolor.c \
	wcolorpanel.c \
	wcolorwell.c \
	wconfig.h \
	wevent.c \
	wfilepanel.c \
	wframe.c \
	wfont.c \
	wfontpanel.c \
	widgets.c \
	winputmethod.c \
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
	wview.c \
	wwindow.c


libWUtil_a_SOURCES = 	\
	array.c \
	bagtree.c \
	connection.c \
	data.c \
	error.c \
	findfile.c \
	handlers.c \
	hashtable.c \
	host.c \
	international.c \
	memory.c \
	misc.c \
	notification.c \
	string.c \
	tree.c \
	userdefaults.c \
	usleep.c \
	wapplication.c \
	wconfig.h \
	wutil.c

CPPFLAGS = @CPPFLAGS@ -DLOCALEDIR=\"$(NLSDIR)\"

INCLUDES = -I$(top_srcdir)/WINGs/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG


wcolorpanel.o: wcolorpanel.c
	$(COMPILE) -c -DRGBTXT="\"@X_LIBRARY_PATH@/X11/rgb.txt\"" $<
