
AUTOMAKE_OPTIONS = no-dependencies

lib_LIBRARIES = libWMaker.a

include_HEADERS = WMaker.h

INCLUDES = $(DFLAGS) @XCFLAGS@

libWMaker_a_SOURCES = 	\
	menu.c 		\
	app.c 		\
	event.c 	\
	command.c	\
	app.h		\
	menu.h	

DISTCLEANFILES = wmlib.pc

install-exec-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/pkgconfig
	@list='wmlib.pc'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/pkgconfig/; \
	  else :; fi; \
	done
