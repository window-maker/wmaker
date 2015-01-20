## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS =

BUILT_SOURCES = config-paths.h

DISTCLEANFILES = config-paths.h

config.h: config-paths.h

config-paths.h: Makefile
	@echo "Generating $@"
	@echo '/* this is a generated file - do not edit */' > $@
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
	@echo '#define PKGDATADIR "$(datadir)/WindowMaker"' >> $@
	@echo '' >> $@
	@echo '/* where the configuration is stored */' >> $@
	@echo '#define SYSCONFDIR "$(sysconfdir)"' >> $@

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib WPrefs.app doc
DIST_SUBDIRS = $(SUBDIRS) test

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N INSTALL \
	INSTALL-WMAKER README.i18n README.definable-cursor \
	The-perfect-Window-Maker-patch.txt \
	README COPYING.WTFPL autogen.sh \
	email-clients.txt checkpatch.pl update-changelog.pl \
	script/check-translation-sources.sh \
	script/generate-mapfile-from-header.sh \
	script/generate-txt-from-texi.sh \
	script/nested-func-to-macro.sh

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
