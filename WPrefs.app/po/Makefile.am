CATALOGS = @WPMOFILES@

nlsdir = $(NLSDIR)

CLEANFILES = WPrefs.pot $(CATALOGS)

EXTRA_DIST = bg.po ca.po cs.po de.po es.po et.po fi.po fr.po hr.po hu.po \
	it.po ja.po ko.po pt.po ru.po sk.po zh_CN.po zh_TW.Big5.po

POTFILES  = \
	$(top_builddir)/WPrefs.app/Appearance.c \
	$(top_builddir)/WPrefs.app/Configurations.c \
	$(top_builddir)/WPrefs.app/Expert.c \
	$(top_builddir)/WPrefs.app/Focus.c \
	$(top_builddir)/WPrefs.app/Font.c \
	$(top_builddir)/WPrefs.app/Icons.c \
	$(top_builddir)/WPrefs.app/KeyboardSettings.c \
	$(top_builddir)/WPrefs.app/KeyboardShortcuts.c \
	$(top_builddir)/WPrefs.app/Menu.c \
	$(top_builddir)/WPrefs.app/MenuPreferences.c \
	$(top_builddir)/WPrefs.app/MouseSettings.c \
	$(top_builddir)/WPrefs.app/Paths.c \
	$(top_builddir)/WPrefs.app/Preferences.c \
	$(top_builddir)/WPrefs.app/TexturePanel.c \
	$(top_builddir)/WPrefs.app/Themes.c \
	$(top_builddir)/WPrefs.app/WPrefs.c \
	$(top_builddir)/WPrefs.app/WindowHandling.c \
	$(top_builddir)/WPrefs.app/Workspace.c \
	$(top_builddir)/WPrefs.app/imagebrowser.c \
	$(top_builddir)/WPrefs.app/main.c

SUFFIXES = .po .mo

.po.mo: 
	msgfmt -o $@ $<


all-local: $(CATALOGS)

WPrefs.pot: $(POTFILES)
	xgettext --default-domain=WPrefs \
	--add-comments --keyword=_ $(POTFILES)
	if cmp -s WPrefs.po WPrefs.pot; then \
	    rm -f WPrefs.po; \
	else \
	    mv -f WPrefs.po WPrefs.pot; \
	fi


install-data-local:  $(CATALOGS)
	$(mkinstalldirs) $(DESTDIR)$(nlsdir)
	chmod 755 $(DESTDIR)$(nlsdir) 
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			chmod 755 $(DESTDIR)$(nlsdir)/$$l; \
			chmod 755 $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/WPrefs.mo; \
		fi; \
	done

