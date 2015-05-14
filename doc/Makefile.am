## Process this file with automake to produce Makefile.in

SUBDIRS = build sk cs ru

dist_man_MANS = \
	geticonset.1x \
	getstyle.1x \
	get-wings-flags.1 \
	get-wraster-flags.1 \
	get-wutil-flags.1 \
	seticons.1x \
	setstyle.1x \
	upgrade-windowmaker-defaults.8 \
	wdread.1x \
	wdwrite.1x \
	WindowMaker.1x \
	wmagnify.1x \
	wmgenmenu.1 \
	wmmenugen.1 \
	WPrefs.1x \
	wxcopy.1x \
	wxpaste.1x

man_MANS = \
	wmaker.1x \
	wmsetbg.1x

MOSTLYCLEANFILES = wmaker.1x wmsetbg.1x

EXTRA_DIST = wmaker.in wmsetbg.in

wmaker.1x: wmaker.in Makefile $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" --filter "HAVE_INOTIFY" \
		--filter "USE_ICCCM_WMREPLACE" \
		-D"sysconfdir=$(sysconfdir)" --replace "sysconfdir" \
		-D"pkgdatadir=$(pkgdatadir)" --replace "pkgdatadir" \
		-o "wmaker.1x"  "$(srcdir)/wmaker.in"

wmsetbg.1x: wmsetbg.in Makefile $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" --filter "USE_XINERAMA" \
		-o "wmsetbg.1x"  "$(srcdir)/wmsetbg.in"

# Create a 'silent rule' for our make check the same way automake does
AM_V_CHKOPTS = $(am__v_CHKOPTS_$(V))
am__v_CHKOPTS_  = $(am__v_CHKOPTS_$(AM_DEFAULT_VERBOSITY))
am__v_CHKOPTS_0 = @echo "  CHK      $@" ;
am__v_CHKOPTS_1 =

check-local: wmaker-args WPrefs-args wmagnify-args geticonset-args getstyle-args seticons-args setstyle-args \
	wdread-args wdwrite-args wmgenmenu-args wmsetbg-args wxcopy-args wxpaste-args

wmaker-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/src/wmaker" --man-page "wmaker.1x"

WPrefs-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/WPrefs.app/WPrefs" --man-page "$(top_srcdir)/doc/WPrefs.1x"

wmagnify-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmagnify" --man-page "$(top_srcdir)/doc/wmagnify.1x"

geticonset-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/geticonset" --man-page "$(top_srcdir)/doc/geticonset.1x"

getstyle-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/getstyle" --man-page "$(top_srcdir)/doc/getstyle.1x"

seticons-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/seticons" --man-page "$(top_srcdir)/doc/seticons.1x"

setstyle-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/setstyle" --man-page "$(top_srcdir)/doc/setstyle.1x"

wdread-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wdread" --man-page "$(top_srcdir)/doc/wdread.1x"

wdwrite-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wdwrite" --man-page "$(top_srcdir)/doc/wdwrite.1x"

wmgenmenu-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmgenmenu" --man-page "$(top_srcdir)/doc/wmgenmenu.1"

wmsetbg-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmsetbg" --man-page "wmsetbg.1x"

wxcopy-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wxcopy" --man-page "$(top_srcdir)/doc/wxcopy.1x"

wxpaste-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wxpaste" --man-page "$(top_srcdir)/doc/wxpaste.1x"

.PHONY: wmaker-args WPrefs-args wmagnify-args geticonset-args getstyle-args seticons-args setstyle-args \
	wdread-args wdwrite-args wmgenmenu-args wmsetbg-args wxcopy-args wxpaste-args
