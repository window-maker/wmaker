SUBDIRS = Backgrounds Defaults IconSets Icons Pixmaps Styles Themes

prefsdatadir = $(pkgdatadir)

prefsdata_DATA =\
	README\
	README.themes\
	autostart.sh\
	exitscript.sh\
	menu\
	menu.ca\
	menu.cz\
	menu.de\
	menu.dk\
	menu.el\
	menu.es\
	menu.fi\
	menu.fr\
	menu.gl\
	menu.he\
	menu.hr\
	menu.hu\
	menu.it\
	menu.ja\
	menu.ko\
	menu.nl\
	menu.no\
	menu.pt\
	menu.ru\
	menu.se\
	menu.sl\
	menu.tr

EXTRA_DIST = $(prefsdata_DATA) mkMakefile wmmacros.in plmenu.in\
	plmenu.fr.in plmenu.hr.in

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

