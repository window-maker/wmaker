## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test WPrefs.app doc\
	contrib

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N MIRRORS COPYING.WTFPL \
	Install INSTALL.pt README.pt FAQ.I18N.cs INSTALL.cs\
	mkpatch README.KDE README.GNOME WindowMaker.lsm.in\
	README.definable-cursor \
	FAQ.I18N.sk INSTALL.sk INSTALL.es INSTALL.fr
#	libwmfun-0.0.3.tar.gz

WindowMaker.lsm: WindowMaker.lsm.in
	size=`wc -c $(distdir).tar.gz|cut -d\  -f1`;\
	size=`echo scale=2\;$$size/1048576|bc`;\
	sed -e "s/VERSION/"$(VERSION)"/" \
		-e "s/DATE/`date +%d%b%Y|tr a-z A-Z`/"\
		-e "s/SIZE/$$size M/"\
		WindowMaker.lsm.in > WindowMaker-$(VERSION).lsm

