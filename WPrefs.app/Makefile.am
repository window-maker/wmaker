SUBDIRS = xpm tiff po

nlsdir = @NLSDIR@

AUTOMAKE_OPTIONS = no-dependencies

wpexecbindir = @wprefsdir@

wpexecbin_PROGRAMS = WPrefs 

wpdatadir = @wprefsdir@

wpdata_DATA = WPrefs.tiff WPrefs.xpm

EXTRA_DIST = $(wpdata_DATA) TexturePanel.icons 

WPrefs_SOURCES = \
	main.c \
	WPrefs.c \
	WPrefs.h \
	Configurations.c \
	Expert.c \
	Focus.c \
	Icons.c \
	KeyboardSettings.c \
	KeyboardShortcuts.c \
	Menu.c \
	MenuPreferences.c \
	MouseSettings.c \
	Paths.c \
	Preferences.c \
	Text.c \
	TextureAndColor.c \
	TexturePanel.c \
	TexturePanel.h \
	WindowHandling.c \
	Workspace.c \
	double.c \
	double.h \
	MenuGuru.c \
	xmodifier.c

CPPFLAGS = \
	@CPPFLAGS@ \
	@SHAPE@ @I18N@ @X_LOCALE@ \
	-DNLSDIR="\"$(nlsdir)\"" 


INCLUDES = \
	-I$(top_srcdir)/wrlib \
	-I$(top_srcdir)/WINGs \
	-I$(top_srcdir)/libPropList \
	@XCFLAGS@ 

WPrefs_DEPENDENCIES = $(top_builddir)/WINGs/libWINGs.a

WPrefs_LDADD = \
	$(top_builddir)/WINGs/libWINGs.a\
	$(top_builddir)/wrlib/libwraster.a \
        $(top_builddir)/libPropList/libPropList.a \
	@GFXLFLAGS@ \
	@XLFLAGS@ \
	@GFXLIBS@ \
	@XLIBS@ \
	@X_EXTRA_LIBS@ \
	@INTLIBS@ \
	-lm 

