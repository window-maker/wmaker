## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = WINGs . po Documentation Resources Examples Extras Tests



#libWINGs_la_LDFLAGS = -version-info 1:1:0

bin_SCRIPTS = get-wings-flags get-wutil-flags

lib_LIBRARIES = libWINGs.a libWUtil.a


LDADD= libWINGs.a $(top_builddir)/wrlib/libwraster.la @INTLIBS@


EXTRA_DIST = BUGS python/Makefile python/README python/WINGs.i \
	python/WINGs.py python/setup.py python/test.py


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
	proplist.c \
	selection.c \
	snprintf.c \
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
	proplist.c \
	snprintf.c \
	string.c \
	tree.c \
	userdefaults.c \
	usleep.c \
	wapplication.c \
	wconfig.h \
	wutil.c


AM_CPPFLAGS = @CPPFLAGS@ -DLOCALEDIR=\"$(NLSDIR)\"

INCLUDES = -I$(top_srcdir)/WINGs/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @XFTFLAGS@ @HEADER_SEARCH_PATH@ -DDEBUG


wcolorpanel.o: wcolorpanel.c
	$(COMPILE) -c -DRGBTXT="\"@X_LIBRARY_PATH@/X11/rgb.txt\"" $<

DISTCLEANFILES = WINGs.pc

install-exec-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/pkgconfig
	@list='WINGs.pc'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/; \
	  else :; fi; \
	done
