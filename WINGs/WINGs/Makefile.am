## automake input file for WINGs - Headers subdir

AUTOMAKE_OPTIONS =

# is this a kluge? if so, how should i do it?
includedir = @includedir@/WINGs

BUILT_SOURCES = WINGsP.h
CLEANFILES = WINGsP.h
include_HEADERS = WINGs.h WUtil.h
nodist_include_HEADERS = WINGsP.h
EXTRA_DIST = WINGsP.h.in

WINGsP.h: WINGsP.h.in $(top_builddir)/config.h
	$(AM_V_GEN)$(top_srcdir)/script/replace-ac-keywords.sh \
		--header "$(top_builddir)/config.h" \
		--filter "USE_PANGO" \
		-o $@  $<
