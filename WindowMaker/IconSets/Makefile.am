
defsdatadir = $(pkgdatadir)/IconSets

defsdata_DATA = Default.iconset

EXTRA_DIST = 

CLEANFILES = Default.iconset

Default.iconset: $(top_srcdir)/WindowMaker/Defaults/WMWindowAttributes ./Makefile
	-rm -f Default.iconset
	cp $(top_srcdir)/WindowMaker/Defaults/WMWindowAttributes Default.iconset
	chmod 644 Default.iconset


