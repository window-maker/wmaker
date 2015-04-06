# The list of sources are distributed, but none are to be
# installed along with Window Maker:
EXTRA_DIST = Readme \
	Compilation.texi \
	Translations.texi

# How to re-generate automatically the top-level text files
all-local: $(top_srcdir)/INSTALL-WMAKER $(top_srcdir)/README.i18n

$(top_srcdir)/INSTALL-WMAKER: $(srcdir)/Compilation.texi $(top_srcdir)/script/generate-txt-from-texi.sh
	$(AM_V_GEN)if test ! -e "$(top_srcdir)/INSTALL-WMAKER" -o -w "$(top_srcdir)/INSTALL-WMAKER" ; then \
	    $(top_srcdir)/script/generate-txt-from-texi.sh \
	        $(srcdir)/Compilation.texi -o $(top_srcdir)/INSTALL-WMAKER \
	        -Dversion="$(PACKAGE_VERSION)" -e "$(PACKAGE_BUGREPORT)" ; \
	else \
	    echo "Warning: \"$(top_srcdir)/INSTALL-WMAKER\" is not writeable, not regenerated" ; \
	fi

$(top_srcdir)/README.i18n: $(srcdir)/Translations.texi $(top_srcdir)/script/generate-txt-from-texi.sh
	$(AM_V_GEN)if test ! -e "$(top_srcdir)/README.i18n" -o -w "$(top_srcdir)/README.i18n" ; then \
	    $(top_srcdir)/script/generate-txt-from-texi.sh \
	        $(srcdir)/Translations.texi -o $(top_srcdir)/README.i18n \
	        -Dversion="$(PACKAGE_VERSION)" -e "$(PACKAGE_BUGREPORT)" ; \
	else \
	    echo "Warning: \"$(top_srcdir)/README.i18n\" is not writeable, not regenerated" ; \
	fi
