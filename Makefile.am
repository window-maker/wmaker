## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test WPrefs.app doc

EXTRA_DIST = TODO BUGS BUGFORM FAQ FAQ.I18N MIRRORS Install acconfig.h\
	mkpatch README.KDE README.GNOME WindowMaker.lsm.in\
	docklib-0.0.tar.gz


windowmaker.lsm:
	size=`wc -c $(distdir).tar.gz|cut -d\  -f1`;\
	size=`echo scale=2\;$$size/1048576|bc`;\
	sed -e "s/VERSION/"$(VERSION)"/" \
		-e "s/DATE/`date +%d%b%Y|tr a-z A-Z`/"\
		-e "s/SIZE/$$size M/"\
		WindowMaker.lsm.in > WindowMaker.lsm
