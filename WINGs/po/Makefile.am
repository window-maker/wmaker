nlsdir      = $(NLSDIR)

CATALOGS = @WINGSMOFILES@

CLEANFILES = $(CATALOGS) WINGs.pot

EXTRA_DIST = bg.po ca.po cs.po de.po fr.po hu.po sk.po

POTFILES  = \
	$(top_srcdir)/WINGs/error.c \
	$(top_srcdir)/WINGs/findfile.c \
	$(top_srcdir)/WINGs/proplist.c \
	$(top_srcdir)/WINGs/userdefaults.c \
	$(top_srcdir)/WINGs/wcolor.c \
	$(top_srcdir)/WINGs/wcolorpanel.c \
	$(top_srcdir)/WINGs/wfilepanel.c \
	$(top_srcdir)/WINGs/wfont.c \
	$(top_srcdir)/WINGs/wfontpanel.c \
	$(top_srcdir)/WINGs/widgets.c \
	$(top_srcdir)/WINGs/wruler.c

SUFFIXES = .po .mo

.po.mo: 
	msgfmt -c -o $@ $<


all-local: $(CATALOGS)

WINGs.pot: $(POTFILES)
	xgettext --default-domain=WINGs \
	--add-comments --keyword=_ --keyword=N_ $(POTFILES)
	if cmp -s WINGs.po WINGs.pot; then \
	    rm -f WINGs.po; \
	else \
	    mv -f WINGs.po WINGs.pot; \
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
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/WINGs.mo; \
		fi; \
	done
