nlsdir      = @NLSDIR@

CATALOGS = @WPMOFILES@

CLEANFILES = $(CATALOGS)

EXTRA_DIST = pt.po hr.po fr.po ko.po cs.po ja.po

POTFILES  = \
	$(top_builddir)/WPrefs.app/Configurations.c \
	$(top_builddir)/WPrefs.app/Expert.c \
	$(top_builddir)/WPrefs.app/Focus.c \
	$(top_builddir)/WPrefs.app/Icons.c \
	$(top_builddir)/WPrefs.app/KeyboardSettings.c \
	$(top_builddir)/WPrefs.app/KeyboardShortcuts.c \
	$(top_builddir)/WPrefs.app/Menu.c \
	$(top_builddir)/WPrefs.app/MenuGuru.c \
	$(top_builddir)/WPrefs.app/MenuPreferences.c \
	$(top_builddir)/WPrefs.app/MouseSettings.c \
	$(top_builddir)/WPrefs.app/Paths.c \
	$(top_builddir)/WPrefs.app/Preferences.c \
	$(top_builddir)/WPrefs.app/Text.c \
	$(top_builddir)/WPrefs.app/TextureAndColor.c \
	$(top_builddir)/WPrefs.app/Themes.c \
	$(top_builddir)/WPrefs.app/WPrefs.c \
	$(top_builddir)/WPrefs.app/WindowHandling.c \
	$(top_builddir)/WPrefs.app/Workspace.c \
	$(top_builddir)/WPrefs.app/main.c \
	$(top_builddir)/WPrefs.app/xmodifier.c	

SUFFIXES = .po .mo

.po.mo: 
	msgfmt -o $@ $<


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
			$(INSTALL) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/WPrefs.mo; \
		fi; \
	done

