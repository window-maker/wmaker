nlsdir      = $(NLSDIR)

CATALOGS = @MOFILES@

CLEANFILES = $(CATALOGS) WindowMaker.pot

# Please keep this sorted in alphabetic order!
EXTRA_DIST = be.po bg.po bs.po ca.po cs.po da.po de.po el.po es.po et.po \
	fi.po fr.po gl.po hr.po hu.po it.po ja.po ko.po ms.po nl.po no.po \
	pl.po pt.po ro.po ru.po sk.po sv.po tr.po zh_CN.po zh_TW.po

POTFILES  = \
	$(top_builddir)/src/appicon.c \
	$(top_builddir)/src/application.c \
	$(top_builddir)/src/defaults.c \
	$(top_builddir)/src/dialog.c \
	$(top_builddir)/src/dock.c \
	$(top_builddir)/src/dockedapp.c \
	$(top_builddir)/src/framewin.c \
	$(top_builddir)/src/icon.c \
	$(top_builddir)/src/main.c \
	$(top_builddir)/src/menu.c \
	$(top_builddir)/src/misc.c \
	$(top_builddir)/src/pixmap.c \
	$(top_builddir)/src/resources.c \
	$(top_builddir)/src/rootmenu.c \
	$(top_builddir)/src/screen.c \
	$(top_builddir)/src/session.c \
	$(top_builddir)/src/stacking.c \
	$(top_builddir)/src/startup.c \
	$(top_builddir)/src/switchmenu.c \
	$(top_builddir)/src/switchpanel.c \
	$(top_builddir)/src/texture.c \
	$(top_builddir)/src/wdefaults.c \
	$(top_builddir)/src/window.c \
	$(top_builddir)/src/winmenu.c \
	$(top_builddir)/src/winspector.c \
	$(top_builddir)/src/wmspec.c \
	$(top_builddir)/src/workspace.c

SUFFIXES = .po .mo

.po.mo: 
	msgfmt -c -o $@ $<


all-local: $(CATALOGS)

WindowMaker.pot: $(POTFILES)
	xgettext --default-domain=WindowMaker \
	--add-comments --keyword=_ $(POTFILES)
	if cmp -s WindowMaker.po WindowMaker.pot; then \
	    rm -f WindowMaker.po; \
	else \
	    mv -f WindowMaker.po WindowMaker.pot; \
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
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/WindowMaker.mo; \
		fi; \
	done
