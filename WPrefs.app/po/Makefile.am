nlsdir      = @NLSDIR@

CATALOGS = @WPMOFILES@

CLEANFILES = $(CATALOGS)

EXTRA_DIST = pt.po hr.po fr.po ko.po cs.po ja.po

POTFILES  = \
	$(top_builddir)/WPrefs/Configurations.c \
	$(top_builddir)/WPrefs/Expert.c \
	$(top_builddir)/WPrefs/Focus.c \
	$(top_builddir)/WPrefs/KeyboardSettings.c \
	$(top_builddir)/WPrefs/KeyboardShortcuts.c \
	$(top_builddir)/WPrefs/Menu.c \
	$(top_builddir)/WPrefs/MenuGuru.c \
	$(top_builddir)/WPrefs/MenuPreferences.c \
	$(top_builddir)/WPrefs/MouseSettings.c \
	$(top_builddir)/WPrefs/Paths.c \
	$(top_builddir)/WPrefs/Preferences.c \
	$(top_builddir)/WPrefs/Text.c \
	$(top_builddir)/WPrefs/TextureAndColor.c \
	$(top_builddir)/WPrefs/WPrefs.c \
	$(top_builddir)/WPrefs/WindowHandling.c \
	$(top_builddir)/WPrefs/Workspace.c \
	$(top_builddir)/WPrefs/main.c \
	$(top_builddir)/WPrefs/xmodifier.c	

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
	$(mkinstalldirs) $(nlsdir) 
	chmod 755 $(nlsdir) 
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(nlsdir)/$$l/LC_MESSAGES; \
			chmod 755 $(nlsdir)/$$l; \
			chmod 755 $(nlsdir)/$$l/LC_MESSAGES; \
			$(INSTALL) -m 644 $$n $(nlsdir)/$$l/LC_MESSAGES/WPrefs.mo; \
		fi; \
	done
