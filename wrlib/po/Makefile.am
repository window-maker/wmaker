DOMAIN = WRaster

CATALOGS = @WRASTERMOFILES@

CLEANFILES = $(CATALOGS) $(DOMAIN).pot

EXTRA_DIST = fr.po

POTFILES  = \
	$(top_srcdir)/wrlib/raster.c	\
	$(top_srcdir)/wrlib/alpha_combine.c	\
	$(top_srcdir)/wrlib/draw.c	\
	$(top_srcdir)/wrlib/color.c	\
	$(top_srcdir)/wrlib/load.c	\
	$(top_srcdir)/wrlib/save.c	\
	$(top_srcdir)/wrlib/gradient.c	\
	$(top_srcdir)/wrlib/xpixmap.c	\
	$(top_srcdir)/wrlib/convert.c	\
	$(top_srcdir)/wrlib/context.c	\
	$(top_srcdir)/wrlib/misc.c	\
	$(top_srcdir)/wrlib/scale.c	\
	$(top_srcdir)/wrlib/rotate.c	\
	$(top_srcdir)/wrlib/flip.c	\
	$(top_srcdir)/wrlib/convolve.c	\
	$(top_srcdir)/wrlib/save_xpm.c	\
	$(top_srcdir)/wrlib/xutil.c	\
	$(top_srcdir)/wrlib/load_ppm.c	\
	$(top_srcdir)/wrlib/load_gif.c	\
	$(top_srcdir)/wrlib/load_jpeg.c	\
	$(top_srcdir)/wrlib/load_png.c	\
	$(top_srcdir)/wrlib/load_tiff.c	\
	$(top_srcdir)/wrlib/load_xpm.c	\
	$(top_srcdir)/wrlib/load_xpm_normalized.c	\
	$(top_srcdir)/wrlib/load_webp.c	\
	$(top_srcdir)/wrlib/load_magick.c

SUFFIXES = .po .mo

.po.mo:
	$(AM_V_GEN)$(MSGFMT) -c -o $@ $<


all-local: $(CATALOGS)


.PHONY: update-lang

if HAVE_XGETTEXT
update-lang: $(DOMAIN).pot
	$(AM_V_GEN)$(top_srcdir)/script/generate-po-from-template.sh \
	    -n "$(PACKAGE_NAME)" -v "$(PACKAGE_VERSION)" -b "$(PACKAGE_BUGREPORT)" \
	    -t "$(DOMAIN).pot"  "$(srcdir)/$(PO).po"

$(DOMAIN).pot: $(POTFILES)
	$(AM_V_GEN)$(XGETTEXT) --default-domain=$(DOMAIN) \
	    --add-comments --keyword=_ --keyword=N_ $(POTFILES)
	@if cmp -s $(DOMAIN).po $(DOMAIN).pot; then \
	    rm -f $(DOMAIN).po; \
	else \
	    mv -f $(DOMAIN).po $(DOMAIN).pot; \
	fi
endif


install-data-local:  $(CATALOGS)
	$(mkinstalldirs) $(DESTDIR)$(localedir)
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(localedir)/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(localedir)/$$l/LC_MESSAGES/$(DOMAIN).mo; \
		fi; \
	done

uninstall-local:
	for n in $(CATALOGS) ; do \
		l=`basename $$n .mo`; \
		rm -f $(DESTDIR)$(localedir)/$$l/LC_MESSAGES/$(DOMAIN).mo; \
	done

# Create a 'silent rule' for our make check the same way automake does
AM_V_CHKTRANS = $(am__v_CHKTRANS_$(V))
am__v_CHKTRANS_ = $(am__v_CHKTRANS_$(AM_DEFAULT_VERBOSITY))
am__v_CHKTRANS_0 = @echo "  CHK     translations" ;
am__v_CHKTRANS_1 =

# 'make check' will make sure the tranlation sources are in line with the compiled source
check-local:
	$(AM_V_CHKTRANS)$(top_srcdir)/script/check-translation-sources.sh \
		"$(srcdir)" -s "$(top_srcdir)/wrlib/Makefile.am"
