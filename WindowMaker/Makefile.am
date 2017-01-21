SUBDIRS = Backgrounds . Defaults IconSets Icons Pixmaps Styles Themes

prefsdatadir = $(pkgdatadir)

wpexecbindir = @wprefs_bindir@

# menus which need generated to have correct path to WPrefs
GENERATED_MENUS =\
	menu\
	menu.bg\
	menu.fi\
	menu.fy\
	menu.hu\
	menu.ko\
	menu.nl\
	menu.ro\
	menu.sk\
	menu.zh_TW\
	plmenu\
	plmenu.bg\
	plmenu.es\
	plmenu.fi\
	plmenu.fy\
	plmenu.ja\
	plmenu.ko\
	plmenu.nl\
	plmenu.ro\
	plmenu.sk\
	plmenu.zh_TW

# menus which do not mention WPrefs
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
	plmenu.it\
	plmenu.pl\
	plmenu.zh_CN

dist_prefsdata_DATA =\
	appearance.menu\
	autostart.sh\
	background.menu\
	exitscript.sh\
	README\
	README.themes\
	wmmacros\
	$(NONGENERATED_MENUS)

nodist_prefsdata_DATA = $(GENERATED_MENUS)

EXTRA_DIST = $(addsuffix .in, $(GENERATED_MENUS))

CLEANFILES = $(GENERATED_MENUS)

$(GENERATED_MENUS):
	$(AM_V_GEN)sed -e "s:#wprefs#:$(wpexecbindir)/WPrefs:" \
		$(srcdir)/$@.in > $@
