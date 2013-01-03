AUTOMAKE_OPTIONS =

BUILT_SOURCES = wconfig.h

bin_PROGRAMS = wmaker

EXTRA_DIST = 

wmaker_SOURCES = 	\
	GNUstep.h \
	WindowMaker.h \
	actions.c \
	actions.h \
	appicon.c \
	appicon.h \
	application.c \
	application.h \
	appmenu.c \
	appmenu.h \
	balloon.c \
	balloon.h \
	client.c \
	client.h \
	colormap.c \
	colormap.h \
	cycling.c \
	def_pixmaps.h \
	defaults.c \
	defaults.h \
	dialog.c \
	dialog.h \
	dock.c \
	dockedapp.c \
	dockedapp.h \
	dock.h \
	event.c \
	extend_pixmaps.h \
	framewin.c \
	framewin.h \
	geomview.c \
	geomview.h \
	funcs.h \
	icon.c \
	icon.h \
	keybind.h \
	main.c \
	main.h \
	menu.c \
	menu.h \
	misc.c \
	osdep_@WM_OSDEP@.c \
	monitor.c \
	motif.c \
	motif.h \
	moveres.c \
	pixmap.c \
	pixmap.h \
	placement.c \
	properties.c \
	properties.h \
	resources.c \
	resources.h \
	rootmenu.c \
	rootmenu.h \
	screen.c \
	screen.h \
	session.h \
	session.c \
	shutdown.c \
	switchpanel.c \
	switchpanel.h \
	stacking.c \
	stacking.h \
	startup.c \
	superfluous.c \
	superfluous.h \
	switchmenu.c \
	texture.c \
	texture.h \
	usermenu.c \
	usermenu.h \
	xdnd.h \
	xdnd.c \
	xinerama.h \
	xinerama.c \
	xmodifier.h \
	xmodifier.c \
	xutil.c \
	xutil.h \
	wconfig.h \
	wcore.c \
	wcore.h \
	wdefaults.c \
	wdefaults.h \
	window.c \
	window.h \
	winmenu.c \
	winspector.h \
	winspector.c \
	wmspec.h \
	wmspec.c \
	workspace.c \
	workspace.h

EXTRA_wmaker_SOURCES = osdep_bsd.c osdep_darwin.c osdep_linux.c osdep_stub.c

AM_CFLAGS =

AM_CPPFLAGS = \
	$(DFLAGS) -DLOCALEDIR=\"$(NLSDIR)\" \
	-I$(top_srcdir)/wrlib \
	-I$(top_srcdir)/WINGs @HEADER_SEARCH_PATH@ 


wmaker_LDADD = \
	$(top_builddir)/WINGs/libWINGs.la\
	$(top_builddir)/WINGs/libWUtil.la\
	$(top_builddir)/wrlib/libwraster.la\
	@XLFLAGS@ \
	@LIBXRANDR@ \
	@LIBXINERAMA@ \
	@XLIBS@ \
	@LIBM@ \
	@INTLIBS@
