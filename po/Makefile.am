nlsdir      = $(NLSDIR)

CATALOGS = @MOFILES@

CLEANFILES = $(CATALOGS) WindowMaker.pot

EXTRA_DIST = cs.po de.po es.po et.po fr.po gl.po ja.po ko.po nl.po pt.po \
    sv.po it.po no.po ru.po tr.po fi.po hr.po el.po pl.po ro.po zh_TW.Big5.po\
	zh_CN.po sk.po da.po bg.po hu.po

POTFILES  = \
	$(top_builddir)/src/appicon.c \
	$(top_builddir)/src/defaults.c \
	$(top_builddir)/src/dialog.c \
	$(top_builddir)/src/dock.c \
	$(top_builddir)/src/dockedapp.c \
	$(top_builddir)/src/event.c \
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
	$(top_builddir)/src/texture.c \
	$(top_builddir)/src/wdefaults.c \
	$(top_builddir)/src/window.c \
	$(top_builddir)/src/winmenu.c \
	$(top_builddir)/src/winspector.c \
	$(top_builddir)/src/workspace.c \
	$(top_builddir)/src/xutil.c 


SUFFIXES = .po .mo

.po.mo: 
	msgfmt -o $@ $<


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
