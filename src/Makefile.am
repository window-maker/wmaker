AUTOMAKE_OPTIONS = no-dependencies

BUILT_SOURCES = wconfig.h

bin_PROGRAMS = wmaker

EXTRA_DIST = wmnotify.c wmnotdef.h wmnotify.h


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
	def_pixmaps.h \
	defaults.c \
	defaults.h \
	dialog.c \
	dialog.h \
	dock.c \
	dockedapp.c \
	dock.h \
	event.c \
	framewin.c \
	framewin.h \
	gnome.c \
	gnome.h \
	funcs.h \
	icon.c \
	icon.h \
	keybind.h \
	kwm.h \
	kwm.c \
	list.c \
	list.h \
	main.c \
	menu.c \
	menu.h \
	misc.c \
	motif.c \
	motif.h \
	moveres.c \
	openlook.c \
	openlook.h \
	pixmap.c \
	pixmap.h \
	placement.c \
	properties.c \
	properties.h \
	proplist.c \
	resources.c \
	resources.h \
	rootmenu.c \
	screen.c \
	screen.h \
	session.h \
	session.c \
	shutdown.c \
	stacking.c \
	stacking.h \
	startup.c \
	superfluous.c \
	superfluous.h \
	switchmenu.c \
	texture.c \
	texture.h \
	xde.h \
	xde.c \
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
	workspace.c \
	workspace.h \
	wmsound.c \
	wmsound.h \
	text.c \
	text.h 


CPPFLAGS = @CPPFLAGS@ @DFLAGS@ -DLOCALEDIR=\"$(NLSDIR)\"


INCLUDES = \
	-I$(top_srcdir)/wrlib \
	-I$(top_srcdir)/WINGs @HEADER_SEARCH_PATH@ 


wmaker_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a\
	$(top_builddir)/wrlib/libwraster.la\
	@LIBRARY_SEARCH_PATH@ \
	@LIBPL@ \
	@GFXLIBS@ \
	@XLIBS@ \
	@INTLIBS@ \
	@DLLIBS@ \
	-lm 

