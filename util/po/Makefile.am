CATALOGS = @UTILMOFILES@

nlsdir = $(NLSDIR)

CLEANFILES = wmgenmenu.pot $(CATALOGS)

EXTRA_DIST = de.po es.po fr.po nl.po pt.po

POTFILES  = \
	$(top_srcdir)/util/wmgenmenu.c \
	$(top_srcdir)/util/wmgenmenu.h

SUFFIXES = .po .mo

.po.mo:
	msgfmt -c -o $@ $<


all-local: $(CATALOGS)

wmgenmenu.pot: $(POTFILES)
	xgettext --default-domain=wmgenmenu \
	--add-comments --keyword=_ --keyword=N_ $(POTFILES)
	if cmp -s wmgenmenu.po wmgenmenu.pot; then \
	    rm -f wmgenmenu.po; \
	else \
	    mv -f wmgenmenu.po wmgenmenu.pot; \
	fi


install-data-local:  $(CATALOGS)
	$(mkinstalldirs) $(DESTDIR)$(nlsdir)
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/wmgenmenu.mo; \
		fi; \
	done
