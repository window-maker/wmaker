## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test WPrefs.app doc\
	contrib

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N FAQ.I18N.cs FAQ.I18N.sk \
	Install INSTALL.cs INSTALL.fr INSTALL.es INSTALL.pt INSTALL.sk \
	README.definable-cursor README.pt README.KDE README.GNOME \
	MIRRORS COPYING.WTFPL mkpatch WindowMaker.lsm.in
#	libwmfun-0.0.3.tar.gz

WindowMaker.lsm: WindowMaker.lsm.in
	size=`wc -c $(distdir).tar.gz|cut -d\  -f1`;\
	size=`echo scale=2\;$$size/1048576|bc`;\
	sed -e "s/VERSION/"$(VERSION)"/" \
		-e "s/DATE/`date +%d%b%Y|tr a-z A-Z`/"\
		-e "s/SIZE/$$size M/"\
		WindowMaker.lsm.in > WindowMaker-$(VERSION).lsm

