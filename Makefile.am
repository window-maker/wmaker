## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test WPrefs.app doc


EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N MIRRORS Install acconfig.h\
	libPropList.tar.gz mkpatch

libPropList.tar.gz:
	-(cd libPropList; $(MAKE) distclean)
	-rm -f libPropList.tar.gz
	tar cf libPropList.tar libPropList
	gzip -9 libPropList.tar

