## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies



include_HEADERS = wtableview.h tabledelegates.h fieldeditor.h


lib_LIBRARIES = libExtraWINGs.a 



EXTRA_DIST = 

libExtraWINGs_a_SOURCES = 	\
	wtableview.c \
	tabledelegates.c \
	fieldeditor.c \
	wtableview.h \
	tabledelegates.h \
	fieldeditor.h


INCLUDES = -I$(top_srcdir)/wrlib -I$(top_srcdir)/WINGs \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" @HEADER_SEARCH_PATH@ -DDEBUG

