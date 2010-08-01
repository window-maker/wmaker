## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = WINGs . po Documentation Resources

LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LIBTOOL_ARG)

libWINGs_a_AR = $(QUIET_AR) $(AR) $(ARFLAGS)

libWUtil_a_AR = $(QUIET_AR) $(AR) $(ARFLAGS)

libWINGs_la_LDFLAGS = -version-info 2:1:0
libWUtil_la_LDFLAGS = -version-info 1:2:0

bin_SCRIPTS = get-wings-flags get-wutil-flags

lib_LTLIBRARIES = libWUtil.la libWINGs.la


LDADD= libWUtil.la libWINGs.la $(top_builddir)/wrlib/libwraster.la @INTLIBS@
libWINGs_la_LIBADD = libWUtil.la $(top_builddir)/wrlib/libwraster.la @XFTLIBS@

EXTRA_DIST = BUGS make-rgb Examples Extras Tests


# wbutton.c 
libWINGs_la_SOURCES = 	\
	configuration.c \
	dragcommon.c \
	dragdestination.c \
	dragsource.c \
	rgb.h \
	selection.c \
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

libWUtil_la_SOURCES = 	\
	array.c \
	bagtree.c \
	data.c \
	error.c \
	findfile.c \
	handlers.c \
	hashtable.c \
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


AM_CPPFLAGS = -DLOCALEDIR=\"$(NLSDIR)\" -DRESOURCE_PATH=\"$(datadir)/WINGs\" -DDEBUG

AM_CFLAGS = @NOSTRICTALIASING@

INCLUDES = -I$(top_srcdir)/WINGs/WINGs -I$(top_srcdir)/wrlib -I$(top_srcdir)/src \
	@XFTFLAGS@ @HEADER_SEARCH_PATH@

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

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
