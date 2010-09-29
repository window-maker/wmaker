## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS =

BUILT_SOURCES = config-paths.h

DISTCLEANFILES = config-paths.h

config.h: config-paths.h

config-paths.h: Makefile
	@echo "Generating $@"
	@echo '/* define to the path to cpp */' > $@
	@echo '#define CPP_PATH "$(CPP_PATH)"' >> $@
	@echo '' >> $@
	@echo '/* gettext domain used for menu translations */' >> $@
	@if test -z "$(menutextdomain)"; then \
	  echo '/* #undef MENU_TEXTDOMAIN "$(menutextdomain)" */' >> $@; \
	else \
	  echo '#define MENU_TEXTDOMAIN "$(menutextdomain)"' >> $@; \
	fi
	@echo '' >> $@
	@echo '/* define an extra path for pixmaps */' >> $@
	@echo '#define PIXMAPDIR "$(pixmapdir)"' >> $@
	@echo '' >> $@
	@echo '/* where shared data is stored */' >> $@
	@echo '#define PKGDATADIR "$(pkgdatadir)/WindowMaker"' >> $@
	@echo '' >> $@
	@echo '/* where the configuration is stored */' >> $@
	@echo '#define SYSCONFDIR "$(sysconfdir)"' >> $@

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = wrlib WINGs src util po WindowMaker WPrefs.app doc

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N FAQ.I18N.cs FAQ.I18N.sk \
	INSTALL-WMAKER INSTALL-WMAKER.cs INSTALL-WMAKER.fr INSTALL-WMAKER.es \
	INSTALL-WMAKER.pt INSTALL-WMAKER.sk README.definable-cursor \
	The-perfect-Window-Maker-patch.txt \
	README.es README.pt COPYING.WTFPL autogen.sh

if USE_LCOV
coverage-reset:
	find . -type f -name '*.gcda' -exec rm -f '{}' ';'
	lcov --directory . --zerocounters

coverage:
	rm -rf coverage @lcov_output_directory@
	-mkdir -p coverage @lcov_output_directory@
	lcov --compat-libtool --capture --directory . --output-file coverage/coverage.info
	genhtml --output-directory @lcov_output_directory@ coverage/coverage.info

.PHONY:	coverage-reset coverage
endif
