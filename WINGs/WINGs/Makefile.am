## automake input file for WINGs - Headers subdir

AUTOMAKE_OPTIONS = no-dependencies

# is this a kluge? if so, how should i do it?
includedir = @includedir@/WINGs

include_HEADERS = WINGs.h WUtil.h WINGsP.h proplist-compat.h

.c.o:
	$(QUIET)$(COMPILE) -c $<

.c.obj:
	$(QUIET)$(COMPILE) -c `$(CYGPATH_W) '$<'`

.c.lo:
	$(QUIET)$(LTCOMPILE) -c -o $@ $<
