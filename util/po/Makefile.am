DOMAIN = wmgenmenu

CATALOGS = @UTILMOFILES@

nlsdir = $(NLSDIR)

CLEANFILES = $(DOMAIN).pot $(CATALOGS)

EXTRA_DIST = de.po es.po fr.po nl.po pt.po

POTFILES  = \
	$(top_srcdir)/util/wmgenmenu.c \
	$(top_srcdir)/util/wmgenmenu.h

SUFFIXES = .po .mo

.po.mo:
	msgfmt -c -o $@ $<


all-local: $(CATALOGS)

$(DOMAIN).pot: $(POTFILES)
	xgettext --default-domain=$(DOMAIN) \
	--add-comments --keyword=_ --keyword=N_ $(POTFILES)
	if cmp -s $(DOMAIN).po $(DOMAIN).pot; then \
	    rm -f $(DOMAIN).po; \
	else \
	    mv -f $(DOMAIN).po $(DOMAIN).pot; \
	fi


install-data-local:  $(CATALOGS)
	$(mkinstalldirs) $(DESTDIR)$(nlsdir)
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(nlsdir)/$$l/LC_MESSAGES/$(DOMAIN).mo; \
		fi; \
	done
