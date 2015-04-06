## Process this file with automake to produce Makefile.in

SUBDIRS = build sk cs ru

man_MANS = \
	geticonset.1x \
	getstyle.1x \
	get-wings-flags.1 \
	get-wraster-flags.1 \
	get-wutil-flags.1 \
	seticons.1x \
	setstyle.1x \
	upgrade-windowmaker-defaults.8 \
	wdread.1 \
	wdwrite.1x \
	WindowMaker.1x \
	wmagnify.1x \
	wmaker.1x \
	wmgenmenu.1 \
	wmmenugen.1 \
	wmsetbg.1x \
	WPrefs.1x \
	wxcopy.1x \
	wxpaste.1x

EXTRA_DIST = $(man_MANS)

# Create a 'silent rule' for our make check the same way automake does
AM_V_CHKOPTS = $(am__v_CHKOPTS_$(V))
am__v_CHKOPTS_  = $(am__v_CHKOPTS_$(AM_DEFAULT_VERBOSITY))
am__v_CHKOPTS_0 = @echo "  CHK      $@" ;
am__v_CHKOPTS_1 =

check-local: wmaker-args

wmaker-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/src/wmaker" --man-page "$(top_srcdir)/doc/wmaker.1x"

.PHONY: wmaker-args
