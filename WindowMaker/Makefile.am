SUBDIRS = Backgrounds . Defaults IconSets Icons Pixmaps Styles Themes

prefsdatadir = $(pkgdatadir)

wpexecbindir = @wprefs_bindir@

# menus which need generated to have correct path to WPrefs or data directory
GENERATED_MENUS =\
	appearance.menu\
	appearance.menu.fy\
	appearance.menu.nl\
	appearance.menu.sr\
	menu\
	menu.bg\
	menu.fi\
	menu.fy\
	menu.hu\
	menu.ko\
	menu.nl\
	menu.ro\
	menu.sk\
	menu.sr\
	menu.zh_TW\
	plmenu\
	plmenu.bg\
	plmenu.es\
	plmenu.fi\
	plmenu.fy\
	plmenu.ja\
	plmenu.ko\
	plmenu.nl\
	plmenu.pl\
	plmenu.ro\
	plmenu.sk\
	plmenu.sr\
	plmenu.zh_CN\
	plmenu.zh_TW\
	wmmacros

# menus which do not mention WPrefs or data directory
NONGENERATED_MENUS =\
	menu.ca\
	menu.cz\
	menu.da\
	menu.de\
	menu.el\
	menu.es\
	menu.fr\
	menu.gl\
	menu.he\
	menu.hr\
	menu.it\
	menu.ja\
	menu.no\
	menu.pl\
	menu.pt\
	menu.ru\
	menu.se\
	menu.sl\
	menu.tr\
	menu.zh_CN\
	plmenu.da\
	plmenu.de\
	plmenu.fr\
	plmenu.hr\
	plmenu.it

dist_prefsdata_DATA =\
	autostart.sh\
	background.menu\
	background.menu.fy\
	background.menu.nl\
	background.menu.sr\
	exitscript.sh\
	README\
	README.themes\
	$(NONGENERATED_MENUS)

nodist_prefsdata_DATA = $(GENERATED_MENUS)

xsessionsdir = @datadir@/xsessions
xsessions_DATA = wmaker.desktop

EXTRA_DIST = $(addsuffix .in, $(GENERATED_MENUS)) wmaker.desktop

CLEANFILES = $(GENERATED_MENUS)

$(GENERATED_MENUS):
	$(AM_V_GEN)sed -e "s:#wprefs#:$(wpexecbindir)/WPrefs:" \
		-e "s:#wmdatadir#:$(pkgdatadir):" $(srcdir)/$@.in > $@
