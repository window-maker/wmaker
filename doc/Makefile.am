## Process this file with automake to produce Makefile.in

SUBDIRS = build @MANLANGDIRS@
DIST_SUBDIRS = build cs ru sk

dist_man_MANS = \
	geticonset.1 \
	getstyle.1 \
	seticons.1 \
	setstyle.1 \
	wdread.1 \
	wdwrite.1 \
	WindowMaker.1 \
	wmagnify.1 \
	wmgenmenu.1 \
	wmiv.1 \
	wmmenugen.1 \
	WPrefs.1 \
	wxcopy.1 \
	wxpaste.1

man_MANS = \
	wmaker.1 \
	wmsetbg.1

MOSTLYCLEANFILES = wmaker.1 wmsetbg.1

EXTRA_DIST = wmaker.in wmsetbg.in


################################################################################
# Generation of man pages that need processing
################################################################################

wmaker.1: wmaker.in Makefile $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" --filter "HAVE_INOTIFY" \
		--filter "USE_ICCCM_WMREPLACE" \
		-D"pkgdatadir=$(pkgdatadir)" --replace "pkgdatadir" \
		-D"pkgconfdir=$(pkgconfdir)" --replace "pkgconfdir" \
		-o "wmaker.1"  "$(srcdir)/wmaker.in"

wmsetbg.1: wmsetbg.in Makefile $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" --filter "USE_XINERAMA" \
		-o "wmsetbg.1"  "$(srcdir)/wmsetbg.in"


################################################################################
# Section for checking the man pages against the program's --help text
################################################################################

# Create a 'silent rule' for our make check the same way automake does
AM_V_CHKOPTS = $(am__v_CHKOPTS_$(V))
am__v_CHKOPTS_  = $(am__v_CHKOPTS_$(AM_DEFAULT_VERBOSITY))
am__v_CHKOPTS_0 = @echo "  CHK      $@" ;
am__v_CHKOPTS_1 =

check-local: wmaker-args WPrefs-args wmagnify-args geticonset-args getstyle-args seticons-args setstyle-args \
	wdread-args wdwrite-args wmgenmenu-args wmiv-args wmmenugen-args wmsetbg-args wxcopy-args wxpaste-args

wmaker-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/src/wmaker" --man-page "wmaker.1"

WPrefs-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/WPrefs.app/WPrefs" --man-page "$(top_srcdir)/doc/WPrefs.1"

wmagnify-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmagnify" --man-page "$(top_srcdir)/doc/wmagnify.1"

geticonset-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/geticonset" --man-page "$(top_srcdir)/doc/geticonset.1"

getstyle-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/getstyle" --man-page "$(top_srcdir)/doc/getstyle.1"

seticons-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/seticons" --man-page "$(top_srcdir)/doc/seticons.1"

setstyle-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/setstyle" --man-page "$(top_srcdir)/doc/setstyle.1"

wdread-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wdread" --man-page "$(top_srcdir)/doc/wdread.1"

wdwrite-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wdwrite" --man-page "$(top_srcdir)/doc/wdwrite.1"

wmgenmenu-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmgenmenu" --man-page "$(top_srcdir)/doc/wmgenmenu.1"

wmiv-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmiv" --man-page "$(top_srcdir)/doc/wmiv.1"

wmmenugen-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmmenugen" --man-page "$(top_srcdir)/doc/wmmenugen.1"

wmsetbg-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wmsetbg" --man-page "wmsetbg.1"

wxcopy-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wxcopy" --man-page "$(top_srcdir)/doc/wxcopy.1"

wxpaste-args:
	$(AM_V_CHKOPTS)$(top_srcdir)/script/check-cmdline-options-doc.sh \
		--program "$(top_builddir)/util/wxpaste" --man-page "$(top_srcdir)/doc/wxpaste.1"

.PHONY: wmaker-args WPrefs-args wmagnify-args geticonset-args getstyle-args seticons-args setstyle-args \
	wdread-args wdwrite-args wmgenmenu-args wmiv-args wmmenugen-args wmsetbg-args wxcopy-args wxpaste-args


################################################################################
# Section related to generating HTML version of man pages for the website
################################################################################

if WITH_WEB_REPO
# We convert all man pages except those that are a link to other man page (.so command)
website: $(MANS) website.menu
	@local_pages=`echo "$^" | sed -e 's/ [^ ]*\.menu$$// ; s,[^ /]*/,,g' `; \
	generated_pages=""; \
	for man in $^; do \
	  [ "$$man" = "website.menu" ] && continue; \
	  grep -i '^\.so[ \t]' "$$man" > /dev/null && continue; \
	  echo "  MAN2HTML $$man"; \
	  $(top_srcdir)/script/generate-html-from-man.sh --groff $(GROFF) \
	        --output $(WEB_REPO_ROOT)/docs/manpages/`echo "$$man" | sed -e 's,[^ /]*/,,g ; s/\.[^.]*$$//' `.html \
	        --local-pages "$$local_pages"  --external-url 'http://linux.die.net/man/%s/%l' \
	        --with-menu "website.menu"  --package '$(PACKAGE_STRING)' \
	        $$man || exit $$?; \
	  generated_pages="$$generated_pages $$man"; \
	done; \
	echo "  UPDATE   index.md"; \
	$(top_srcdir)/script/replace-generated-content.sh  --man-pages "$$generated_pages" \
	        --template '<tr><td class="name"><a href="\1">\2</a></td><td class="section">\3</td><td>\4</td></tr>' \
	        --marker LIST_MANPAGES_COMMANDS  $(WEB_REPO_ROOT)/docs/manpages/index.md

MOSTLYCLEANFILES += website.menu

# This menu is the icon bar to navigate on the website, which we want to keep on all man pages
# We extract it from the Template defined for all pages of the site
website.menu: $(WEB_REPO_ROOT)/_layouts/default.html
	$(AM_V_GEN)sed -n -e '/<aside>/,/<\/aside>/ { s/{{ site.baseurl }}// ; s/^    // ; p }'  $<  >  $@
endif

.PHONY: website
