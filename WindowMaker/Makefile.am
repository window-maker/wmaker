SUBDIRS = Defaults Styles Themes Icons Pixmaps Backgrounds IconSets 

prefsdatadir = $(pkgdatadir)

prefsdata_DATA = wmmacros README menu plmenu autostart.sh exitscript.sh \
	menu.cz menu.de menu.fr menu.gl menu.ja menu.ko menu.nl menu.pt  \
	menu.it menu.no menu.he menu.es menu.ru menu.tr menu.el menu.se \
	menu.fi menu.hr menu.dk menu.sl


EXTRA_DIST = $(prefsdata_DATA) wmmacros.in plmenu.in plmenu.fr.in plmenu.hr.in

CLEANFILES = wmmacros plmenu

wmmacros: $(srcdir)/wmmacros.in ./Makefile
	-rm -f wmmacros

	sed -e "s:#pkgdatadir#:$(pkgdatadir):" -e "s://:/:" \
		$(srcdir)/wmmacros.in > wmmacros; \
	chmod 644 wmmacros


plmenu: $(srcdir)/plmenu.in ./Makefile
	-rm -f plmenu

	sed -e "s:#pkgdatadir#:$(pkgdatadir):" -e "s://:/:" \
		$(srcdir)/plmenu.in > plmenu; \
	chmod 644 plmenu

