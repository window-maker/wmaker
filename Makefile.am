## Process this file with automake to produce Makefile.in

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
