## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test WPrefs.app doc\
	contrib

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N MIRRORS COPYING.OPL \
	Install acconfig.h \
	mkpatch README.KDE README.GNOME WindowMaker.lsm.in\
	docklib-0.2.tar.gz libwmfun-0.0.1.tar.gz 

WindowMaker.lsm: WindowMaker.lsm.in
	size=`wc -c $(distdir).tar.gz|cut -d\  -f1`;\
	size=`echo scale=2\;$$size/1048576|bc`;\
	sed -e "s/VERSION/"$(VERSION)"/" \
		-e "s/DATE/`date +%d%b%Y|tr a-z A-Z`/"\
		-e "s/SIZE/$$size M/"\
		WindowMaker.lsm.in > WindowMaker-$(VERSION).lsm

#dist-hook:
#	touch `find -name configure.in -print`
#	touch `find -name aclocal.m4 -print`
#	touch `find -name Makefile.am -print`
#	touch `find -name Makefile.in -print`
#	touch `find -name configure -print`

