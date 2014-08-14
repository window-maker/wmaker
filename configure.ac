dnl
dnl Window Maker autoconf input.
dnl
dnl Process with:
dnl               aclocal
dnl               autoheader
dnl               autoconf
dnl               libtoolize --force --automake
dnl               automake -a --gnu --include-deps
dnl
dnl


AC_INIT(WindowMaker, 0.95.5, , WindowMaker, http://www.windowmaker.org/)
AC_CONFIG_SRCDIR(src/WindowMaker.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([1.11 silent-rules])

AH_BOTTOM([#include "config-paths.h"])

dnl libtool library versioning
dnl =======================
dnl
dnl current
dnl revision
dnl age
dnl
dnl 1. Start with version information of ‘0:0:0’ for each libtool library.
dnl 2. Update the version information only immediately before a public
dnl release of your software. More frequent updates are unnecessary, and
dnl only guarantee that the current interface number gets larger faster.
dnl 3. If the library source code has changed at all since the last
dnl update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
dnl 4. If any interfaces have been added, removed, or changed since the
dnl last update, increment current, and set revision to 0.
dnl 5. If any interfaces have been added since the last public release,
dnl then increment age.
dnl 6. If any interfaces have been removed or changed since the last
dnl public release, then set age to 0.
dnl
dnl libwraster
WRASTER_CURRENT=5
WRASTER_REVISION=0
WRASTER_AGE=0
WRASTER_VERSION=$WRASTER_CURRENT:$WRASTER_REVISION:$WRASTER_AGE
AC_SUBST(WRASTER_VERSION)
dnl
dnl libWINGs
WINGS_CURRENT=3
WINGS_REVISION=0
WINGS_AGE=0
WINGS_VERSION=$WINGS_CURRENT:$WINGS_REVISION:$WINGS_AGE
AC_SUBST(WINGS_VERSION)
dnl
dnl libWUtil
WUTIL_CURRENT=5
WUTIL_REVISION=0
WUTIL_AGE=0
WUTIL_VERSION=$WUTIL_CURRENT:$WUTIL_REVISION:$WUTIL_AGE
AC_SUBST(WUTIL_VERSION)


dnl Checks for host/os name
dnl =======================
dnl AC_CANONICAL_HOST -- already done by AC_PROG_LIBTOOL

dnl Checks for programs.
dnl ===================
AC_PROG_CC
WM_PROG_CC_C11
AC_PROG_LN_S
AC_PROG_GCC_TRADITIONAL
AC_PROG_LIBTOOL


dnl Debugging Options
dnl =================
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug], [enable debugging options, @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [yes], [debug=yes],
        [no],  [debug=no],
        [AC_MSG_ERROR([bad value $enableval for --enable-debug])] )],
    [debug=no])
AS_IF([test "x$debug" = "xyes"],
    [dnl This flag should have already been detected and added, but if user
     dnl provided an explicit CFLAGS it may not be the case
     AS_IF([echo " $CFLAGS " | grep " -g " 2>&1 > /dev/null],
           [@%:@ Debug symbol already activated],
           [AX_CFLAGS_GCC_OPTION([-g])])
     dnl
     dnl This flag generally makes debugging nightmarish, remove it if present
     CFLAGS="`echo "$CFLAGS" | sed -e 's/-fomit-frame-pointer *//' `"
     dnl
     dnl Enable internal debug code
     CPPFLAGS="$CPPFLAGS -DDEBUG"
],  [dnl
     dnl When debug is not enabled, the user probably does not wants to keep
     dnl assertions in the final program
     CPPFLAGS="$CPPFLAGS -DNDEBUG"
])


AX_CFLAGS_GCC_OPTION(-Wall)
AX_CFLAGS_GCC_OPTION(-Wextra -Wno-sign-compare)
AS_IF([test "x$debug" = "xyes"],
    [dnl When debug is enabled, we try to activate more checks from
     dnl the compiler. They are on independant check because the
     dnl macro checks all the options at once, but we may have cases
     dnl where some options are not supported and we don't want to
     dnl loose all of them.
     dnl
     dnl Floating-point comparison is not a good idea
     AX_CFLAGS_GCC_OPTION([-Wfloat-equal])
     dnl
     dnl Try to report misuses of '&' versus '&&' and similar
     AX_CFLAGS_GCC_OPTION([-Wlogical-op])
     dnl
     dnl Reports declaration of global things that are done inside
     dnl a local environment, instead of using the appropriate
     dnl header
     AX_CFLAGS_GCC_OPTION([-Wnested-externs])
     dnl
     dnl Use of 'sizeof()' on inappropriate pointer types
     AX_CFLAGS_GCC_OPTION([-Wpointer-arith])
     dnl
     dnl Having more than 1 prototype for a function makes code updates
     dnl more difficult, so try to avoid it
     AX_CFLAGS_GCC_OPTION([-Wredundant-decls])
     dnl
     dnl Proper attributes helps the compiler to produce better code
     WM_CFLAGS_CHECK_FIRST([format attribute suggest],
         [-Wsuggest-attribute=format  dnl new gcc syntax
          -Wmissing-format-attribute  dnl old gcc syntax, clang
         ])
     WM_CFLAGS_CHECK_FIRST([no-return attribute suggest],
         [-Wsuggest-attribute=noreturn  dnl gcc syntax
          -Wmissing-noreturn            dnl clang syntax
         ])
     dnl
     dnl The use of trampolines cause code that can crash on some
     dnl secured OS, and in a more general way generate binary code
     dnl that may not be optimal
     AX_CFLAGS_GCC_OPTION([-Wtrampolines])
     dnl
     dnl GCC provides a couple of checks to detect incorrect macro uses
     AX_CFLAGS_GCC_OPTION([-Wundef])
     AX_CFLAGS_GCC_OPTION([-Wunused-macros])
], [dnl
     dnl When debug not enabled, we try to avoid some non-necessary
     dnl messages from the compiler
     dnl
     dnl To support legacy X servers, we have sometime to use
     dnl functions marked as deprecated. We do not wish our users
     dnl to be worried about it
     AX_CFLAGS_GCC_OPTION([-Wno-deprecated])
     AX_CFLAGS_GCC_OPTION([-Wno-deprecated-declarations])
])

dnl Posix thread
dnl =================
AX_PTHREAD


dnl Tracking on what is detected for final status
dnl =============================================
unsupported=""
supported_xext=""
supported_gfx=""


dnl Platform-specific Makefile setup
dnl ================================
AS_CASE(["$host"],
    [*-*-linux*|*-*-cygwin*|*-gnu*], [WM_OSDEP="linux" ; CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=600"],
    [*-*-freebsd*|*-k*bsd-gnu*],     [WM_OSDEP="bsd"   ; CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=600 -DFREEBSD"],
    [*-*-netbsd*],                   [WM_OSDEP="bsd"   ; CPPFLAGS="$CPPFLAGS -DNETBSD"],
    [*-*-openbsd*],                  [WM_OSDEP="bsd"   ; CPPFLAGS="$CPPFLAGS -DOPENBSD"],
    [*-*-dragonfly*],                [WM_OSDEP="bsd"   ; CPPFLAGS="$CPPFLAGS -DDRAGONFLYBSD"],
    [*-apple-darwin*],               [WM_OSDEP="darwin"],
    [*-*-solaris*],                  [WM_OSDEP="stub"],  dnl  solaris.c when done
    [WM_OSDEP="stub"])
AM_CONDITIONAL([WM_OSDEP_LINUX],   [test "x$WM_OSDEP" = "xlinux"])
AM_CONDITIONAL([WM_OSDEP_BSD],     [test "x$WM_OSDEP" = "xbsd"])
AM_CONDITIONAL([WM_OSDEP_DARWIN],  [test "x$WM_OSDEP" = "xdarwin"])
AM_CONDITIONAL([WM_OSDEP_GENERIC], [test "x$WM_OSDEP" = "xstub"])


dnl the prefix
dnl ==========
dnl
dnl move this earlier in the script... anyone know why this is handled
dnl in such a bizarre way?

test "x$prefix" = xNONE && prefix=$ac_default_prefix
dnl Let make expand exec_prefix.
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'


_bindir=`eval echo $bindir`
_bindir=`eval echo $_bindir`

lib_search_path='-L${libdir}'

inc_search_path='-I${includedir}'

dnl ===============================================
dnl Specify paths to look for libraries and headers
dnl ===============================================
AC_ARG_WITH(libs-from, AS_HELP_STRING([--with-libs-from], [pass compiler flags to look for libraries]),
	[lib_search_path="$withval $lib_search_path"])

AC_ARG_WITH(incs-from, AS_HELP_STRING([--with-incs-from], [pass compiler flags to look for header files]),
	[inc_search_path="$withval $inc_search_path"])


dnl Boehm GC
dnl ========
AC_ARG_ENABLE([boehm-gc],
    [AS_HELP_STRING([--enable-boehm-gc], [use Boehm GC instead of the default libc malloc() [default=no]])],
    [AS_CASE(["$enableval"],
        [yes], [with_boehm_gc=yes],
        [no],  [with_boehm_gc=no],
        [AC_MSG_ERROR([bad value $enableval for --enable-boehm-gc])] )],
    [with_boehm_gc=no])
AS_IF([test "x$with_boehm_gc" = "xyes"],
    AC_SEARCH_LIBS([GC_malloc], [gc],
        [AC_DEFINE(USE_BOEHM_GC, 1, [Define if Boehm GC is to be used])],
        [AC_MSG_FAILURE([--enable-boehm-gc specified but test for libgc failed])]))


dnl LCOV
dnl ====
AC_ARG_ENABLE([lcov],
    [AS_HELP_STRING([--enable-lcov[=output-directory]], [enable coverage data generation using LCOV (GCC only) [default=no]])],
    [],
    [enable_lcov=no])

AS_IF([test "x$enable_lcov" != "xno"],
    [AX_CFLAGS_GCC_OPTION(-fprofile-arcs -ftest-coverage)
    AS_IF([test "x$enable_lcov" = "xyes"],
        [lcov_output_directory="coverage-report"],
        [lcov_output_directory="${enable_lcov}/coverage-report"])
    AC_SUBST(lcov_output_directory)])

AM_CONDITIONAL([USE_LCOV], [test "x$enable_lcov" != "xno"])

dnl
dnl ============================
dnl Checks for library functions
dnl ============================
dnl not used anywhere
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
WM_FUNC_SECURE_GETENV
AC_CHECK_FUNCS(gethostname select poll strcasecmp strncasecmp \
	       setsid mallinfo mkstemp sysconf)
AC_SEARCH_LIBS([strerror], [cposix])

dnl Check for strlcat/strlcpy
dnl =========================
AC_ARG_WITH([libbsd],
  [AS_HELP_STRING([--without-libbsd], [do not use libbsd for strlcat and strlcpy [default=check]])],
  [AS_IF([test "x$with_libbsd" != "xno"],
    [with_libbsd=bsd]
    [with_libbsd=]
  )],
  [with_libbsd=bsd])

tmp_libs=$LIBS
AC_SEARCH_LIBS([strlcat],[$with_libbsd],
  [AC_DEFINE(HAVE_STRLCAT, 1, [Define if strlcat is available])],
  [],
  []
)
AC_SEARCH_LIBS([strlcpy],[$with_libbsd],
  [AC_DEFINE(HAVE_STRLCAT, 1, [Define if strlcpy is available])],
  [],
  []
)
LIBS=$tmp_libs

LIBBSD=
AS_IF([test "x$ac_cv_search_strlcat" = "x-lbsd" -o "x$ac_cv_search_strlcpy" = "x-lbsd"],
  [LIBBSD=-lbsd
   AC_CHECK_HEADERS([bsd/string.h])]
)
AC_SUBST(LIBBSD)

dnl Check for OpenBSD kernel memory interface - kvm(3)
dnl ==================================================
AS_IF([test "x$WM_OSDEP" = "xbsd"],
  AC_SEARCH_LIBS([kvm_openfiles], [kvm]) )

dnl Check for inotify
dnl =================
AC_CHECK_HEADERS(sys/inotify.h, AC_DEFINE(HAVE_INOTIFY, 1, Check for inotify))


dnl Check for syslog 
dnl =================
AC_CHECK_HEADERS([syslog.h], [AC_DEFINE([HAVE_SYSLOG], [1], [Check for syslog])])


dnl Checks for header files.
dnl =======================
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h libintl.h poll.h malloc.h ctype.h \
		 string.h strings.h)


dnl Checks for typedefs, structures, and compiler characteristics.
dnl ==============================================================
AC_DECL_SYS_SIGLIST
AC_C_CONST
AC_C_INLINE
WM_C_NORETURN
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL


dnl pkg-config
dnl ==========
dnl AC_ARG_VAR(PKGCONFIG, [pkg-config command])
AC_CHECK_PROG(PKGCONFIG, pkg-config, pkg-config) 

dnl gettext
dnl =======

dnl AM_GNU_GETTEXT

INTLIBS=""

AC_CHECK_FUNC(gettext, [HAVEGETTEXT="yes"], 
	AC_CHECK_LIB(intl, gettext, [INTLIBS="-lintl" HAVEGETTEXT="yes"],
			 INTLIBS="" ))

AC_CHECK_PROG(XGETTEXT, xgettext, xgettext)

if test "$XGETTEXT" != ""; then 
    if $XGETTEXT --help 2>&1 | grep illegal >/dev/null ; then
        echo "xgettext isn't GNU version"
        XGETTEXT=""
    fi
fi

if test "$LINGUAS" != ""; then
    if test "$XGETTEXT" != "" -a "$HAVEGETTEXT" != ""; then
        AC_DEFINE(I18N, 1, [Internationalization (I18N) support (set by configure)])
        PO=""
        echo "xgettext and gettext() exist; will build i18n support for $LINGUAS"
    else
        LINGUAS=""
        PO=""
        echo "xgettext and libintl.a don't both exist; will not build i18n support"
    fi
else
    	INTLIBS=""
	MOFILES=""
	WPMOFILES=""
	UTILMOFILES=""
	PO=""
fi


dnl The Tower of Babel
dnl ==================

dnl List of supported locales
dnl =========================
supported_locales="be bg bs ca cs da de el es et fi fr gl hr hu hy it ja ko ms nl no pl pt ro ru sk sv tr zh_CN zh_TW"
supported_wprefs_locales="bg ca cs de es et fi fr hr hu it ja ko nl pt ru sk zh_CN zh_TW"
supported_wings_locales="bg ca cs de fr hu nl sk"
supported_util_locales="de es fr nl pt"

for lang in $LINGUAS; do
	ok=0
	for l in $supported_locales; do
		if test "$l" = "$lang"; then
			ok=1
			break
		fi
	done
	if test "$ok" = 1; then
		MOFILES="$MOFILES $lang.mo"
	else
		echo "Locale $lang is not supported."
	fi
	ok=0
	for l in $supported_wprefs_locales; do
		if test "$l" = "$lang"; then
			ok=1
			break
		fi
	done
	if test "$ok" = 1; then
		WPMOFILES="$WPMOFILES $lang.mo"
	fi
	ok=0
	for l in $supported_util_locales; do
		if test "$l" = "$lang"; then
			ok=1
			break
		fi
	done
	if test "$ok" = 1; then
		UTILMOFILES="$UTILMOFILES $lang.mo"
	fi
	ok=0
	for l in $supported_wings_locales; do
		if test "$l" = "$lang"; then
			ok=1
			break
		fi
	done
	if test "$ok" = 1; then
		WINGSMOFILES="$WINGSMOFILES $lang.mo"
	fi
done


dnl Added by Oliver - Support for NLSDIR option
dnl ===========================================
AC_ARG_WITH(nlsdir, AS_HELP_STRING([--with-nlsdir=PATH], [specify where the locale stuff should go]))

if test "x$NLSDIR" = "x"; then
	if test "x$with_nlsdir" != "x"; then
		NLSDIR=$with_nlsdir
	else
		NLSDIR='$(prefix)/lib/locale'
	fi
fi

menutextdomain=
AC_ARG_WITH(menu-textdomain, AS_HELP_STRING([--with-menu-textdomain=DOMAIN], [specify gettext domain used for menu translations]),
	[if test "x$withval" != "xno"; then
         menutextdomain=$withval
	 fi])
AC_SUBST(menutextdomain)

AC_SUBST(INTLIBS)
AC_SUBST(NLSDIR)
AC_SUBST(MOFILES)
AC_SUBST(WPMOFILES)
AC_SUBST(UTILMOFILES)
AC_SUBST(WINGSMOFILES)
AC_SUBST(supported_locales)

dnl ===========================================
dnl 		Stuff that uses X
dnl ===========================================

AC_PATH_XTRA

if test $no_x; then
    AC_MSG_ERROR([The path for the X11 files not found!
Make sure you have X and it's headers and libraries (the -devel packages
in Linux) installed.])
fi

X_LIBRARY_PATH=$x_libraries
XCFLAGS="$X_CFLAGS"
XLFLAGS="$X_LIBS"
XLIBS="-lX11 $X_EXTRA_LIBS"

lib_search_path="$lib_search_path $XLFLAGS"
inc_search_path="$inc_search_path $XCFLAGS"

AC_SUBST(X_LIBRARY_PATH)

dnl Decide which locale function to use, setlocale() or _Xsetlocale()
dnl by MANOME Tomonori 
dnl ===========================================
use_locale=yes
AC_ARG_ENABLE(locale, AS_HELP_STRING([--disable-locale], [disable use of X locale support]),
              use_locale=no)

if test "$use_locale" = yes; then
	AC_CHECK_LIB(X11, _Xsetlocale,
		AC_DEFINE(X_LOCALE, 1, [define if you want support for X window's X_LOCALE (set by configure)]),,
		$XLFLAGS $XLIBS)
fi

dnl Check whether XInternAtoms() exist
dnl ==================================
AC_CHECK_LIB(X11, XInternAtoms, 
             AC_DEFINE(HAVE_XINTERNATOMS, 1, [define if your X server has XInternAtoms() (set by configure)]),,
             $XLFLAGS $XLIBS)

dnl Check whether XConvertCase() exist
dnl ==================================
AC_CHECK_LIB(X11, XConvertCase, 
             AC_DEFINE(HAVE_XCONVERTCASE, 1, [define if your X server has XConvertCase() (set by configure)]),,
             $XLFLAGS $XLIBS)

dnl XKB keyboard language status
dnl ============================
AC_ARG_ENABLE(modelock, AS_HELP_STRING([--enable-modelock], [XKB keyboard language status support]),
		AC_DEFINE(XKB_MODELOCK, 1, [whether XKB language MODELOCK should be enabled]))

dnl XDND Drag-nd-Drop support
dnl ============================
AC_ARG_ENABLE([xdnd],
    [AS_HELP_STRING([--disable-xdnd], [disable Drag-nd-Drop support])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --disable-xdnd]) ]) ],
    [enable_xdnd=yes
  supported_xext="$supported_xext XDnD"
  AC_DEFINE(XDND, 1, [whether Drag-nd-Drop support should be enabled])
])

dnl XShape support
dnl ==============
AC_ARG_ENABLE([shape],
    [AS_HELP_STRING([--disable-shape], [disable shaped window extension support])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-shape]) ]) ],
    [enable_shape=auto])
WM_XEXT_CHECK_XSHAPE

dnl MIT-SHM support
dnl ===============
AC_ARG_ENABLE([shm],
    [AS_HELP_STRING([--disable-shm], [disable usage of MIT-SHM extension])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-shm]) ]) ],
    [enable_shm=auto])
WM_XEXT_CHECK_XSHM

dnl X Misceleanous Utility
dnl ======================
# the libXmu is used in WRaster
WM_EXT_CHECK_XMU

dnl XINERAMA support
dnl ================
AC_ARG_ENABLE([xinerama],
    [AS_HELP_STRING([--enable-xinerama], [enable Xinerama extension support])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-xinerama]) ]) ],
    [enable_xinerama=auto])
WM_XEXT_CHECK_XINERAMA

dnl RandR support
dnl ==============
AC_ARG_ENABLE([randr],
    [AS_HELP_STRING([--enable-randr], [enable RandR extension support (NOT recommended, buggy)])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-randr]) ]) ],
    [enable_randr=no])
WM_XEXT_CHECK_XRANDR

dnl
dnl libWINGS uses math functions, check whether usage requires linking
dnl against libm
dnl
WM_CHECK_LIBM

dnl
dnl libWINGS uses FcPatternDel from libfontconfig
dnl
AC_MSG_CHECKING([for fontconfig library])
FCLIBS=`$PKGCONFIG fontconfig --libs`
if test "x$FCLIBS" = "x" ; then
        AC_MSG_RESULT([not found])
else
        AC_MSG_RESULT([found])
fi
AC_SUBST(FCLIBS)


dnl Xft2 antialiased font support
dnl =============================

xft=yes
XFTLIBS=""

if test "x$PKGCONFIG" != x -a "`$PKGCONFIG xft; echo $?`" = 0; then
        XFTCONFIG="$PKGCONFIG xft"
        pkgconfig_xft=yes
else
        AC_CHECK_PROG(XFTCONFIG, xft-config, xft-config)
fi

AC_MSG_CHECKING([for the Xft2 library])

if test "x$XFTCONFIG" != x; then
        XFTLIBS=`$XFTCONFIG --libs`
        XFTFLAGS=`$XFTCONFIG --cflags`
        AC_MSG_RESULT([found])
else
        AC_MSG_RESULT([not found])
        echo
        echo "ERROR!!! libXft2 is not installed or could not be found."
        echo "         Xft2 is a requirement for building Window Maker."
        echo "         Please install it (along with fontconfig) before continuing."
        echo
        exit 1
fi

minXFT="2.1.0"
goodxft="no"

dnl
dnl The macro below will use $XFTFLAGS (defined above) to find Xft.h
dnl
WM_CHECK_XFT_VERSION($minXFT, goodxft=yes, goodxft=no)

if test "$goodxft" = no; then
        echo
        echo "ERROR!!! libXft on this system is an old version."
        echo "         Please consider upgrading to at least version ${minXFT}."
        echo
        exit 1
fi

AC_SUBST(XFTFLAGS)
AC_SUBST(XFTLIBS)


dnl ==============================================
dnl         Graphic Format Libraries
dnl ==============================================

dnl XPM Support
dnl ===========
AC_ARG_ENABLE([xpm],
    [AS_HELP_STRING([--disable-xpm], [disable use of XPM pixmaps through libXpm])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-xpm])] )],
    [enable_xpm=auto])
WM_IMGFMT_CHECK_XPM


# for wmlib
AC_SUBST(XCFLAGS)
# for test
AC_SUBST(XLFLAGS)
AC_SUBST(XLIBS)
AC_SUBST(X_EXTRA_LIBS)

dnl ===============================================
dnl 		End of stuff that uses X
dnl ===============================================

dnl EXIF Support
dnl ===========
WM_CHECK_LIBEXIF
AS_IF([test "x$LIBEXIF" != "x"],
        [AC_DEFINE(HAVE_EXIF, 1, [Define if EXIF can be used])])

dnl PNG Support
dnl ===========
AC_ARG_ENABLE([png],
    [AS_HELP_STRING([--disable-png], [disable PNG support through libpng])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-png])] )],
    [enable_png=auto])
WM_IMGFMT_CHECK_PNG


dnl JPEG Support
dnl ============
AC_ARG_ENABLE([jpeg],
    [AS_HELP_STRING([--disable-jpeg], [disable JPEG support through libjpeg])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-jpeg])] )],
    [enable_jpeg=auto])
WM_IMGFMT_CHECK_JPEG


dnl GIF Support
dnl ============
AC_ARG_ENABLE(gif,
    [AS_HELP_STRING([--disable-gif], [disable GIF support through libgif or libungif])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-gif])] )],
    [enable_gif=auto])
WM_IMGFMT_CHECK_GIF


dnl TIFF Support
dnl ============
AC_ARG_ENABLE([tiff],
    [AS_HELP_STRING([--disable-tiff], [disable use of TIFF images through libtiff])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-tiff])] )],
    [enable_tiff=auto])
WM_IMGFMT_CHECK_TIFF


dnl WEBP Support
dnl ===========
AC_ARG_ENABLE([webp],
    [AS_HELP_STRING([--disable-webp], [disable WEBP support through libwebp])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-webp])] )],
    [enable_webp=auto])
WM_IMGFMT_CHECK_WEBP


dnl MAGICK Support
dnl ===========
AC_ARG_ENABLE([magick],
    [AS_HELP_STRING([--disable-magick], [disable MAGICK support through libMagickWand])],
    [AS_CASE(["$enableval"],
        [yes|no], [],
        [AC_MSG_ERROR([bad value $enableval for --enable-magick])] )],
    [enable_magick=auto])
WM_IMGFMT_CHECK_MAGICK


dnl PPM Support
dnl ===========
# The PPM format is always enabled because we have built-in support for the format
# We are not using any external library like libppm
supported_gfx="$supported_gfx builtin-PPM"


# Choice of the default format for icons
AS_IF([test "x$enable_tiff" != "xno"],
    [ICONEXT="tiff"],
    [ICONEXT="xpm"])


LIBRARY_SEARCH_PATH="$lib_search_path"
HEADER_SEARCH_PATH="$inc_search_path"

AC_SUBST(LIBRARY_SEARCH_PATH)
AC_SUBST(HEADER_SEARCH_PATH)


AC_SUBST(GFXLIBS)
AC_SUBST(ICONEXT)
AM_CONDITIONAL([ICON_EXT_XPM],  [test "x$ICONEXT" = "xxpm"])
AM_CONDITIONAL([ICON_EXT_TIFF], [test "x$ICONEXT" = "xtiff"])


dnl ==============================================
dnl         End of Graphic Format Libraries
dnl ==============================================


dnl
dnl stdlib.h is checked here, because of conflict in jpeglib.h
AC_CHECK_HEADERS(stdlib.h)

# AC_PREFIX_PROGRAM(wmaker)

dnl Support for PIXMAPDIR option
dnl ============================
AC_ARG_WITH(pixmapdir, AS_HELP_STRING([--with-pixmapdir=PATH], [specify where pixmaps are located [DATADIR/pixmaps]]))

if test "x$with_pixmapdir" != "x"; then
	pixmapdir=$with_pixmapdir
else
	pixmapdir='${datadir}/pixmaps'
fi
AC_SUBST(pixmapdir)


dnl Support for GNUSTEP_LOCAL_ROOT, for WPrefs.app
dnl ==============================================

AC_ARG_WITH(gnustepdir, AS_HELP_STRING([--with-gnustepdir=PATH], [specify the directory for GNUstep applications]))

if test "x`echo $with_gnustepdir | grep ^/`" != "x"; then
    appspath=$with_gnustepdir
fi

if test "x$appspath$GNUSTEP_LOCAL_ROOT" = "x"; then
    wprefs_base_dir=${prefix}
    wprefs_datadir="${datadir}/WPrefs"
    wprefs_bindir="${bindir}"
else
    gnustepdir=$appspath

    if test "x$GNUSTEP_LOCAL_ROOT" != "x" ; then
	gnustepdir=`echo "$GNUSTEP_LOCAL_ROOT" | sed -e "s|^${prefix}|prefix|"`
	gnustepdir=`echo $gnustepdir | sed -e 's|^prefix|${prefix}|'`
    fi

    wprefs_base_dir=$gnustepdir/Applications
    wprefs_datadir=$wprefs_base_dir/WPrefs.app
    wprefs_bindir=$wprefs_base_dir/WPrefs.app
fi

AC_SUBST(wprefs_datadir)
AC_SUBST(wprefs_bindir)


dnl Enable User Defined Menu thing
dnl ==============================
AC_ARG_ENABLE(usermenu, AS_HELP_STRING([--enable-usermenu], [user defined menus for applications]),
if test "$enableval" = yes; then
	AC_DEFINE(USER_MENU, 1, [define if you want user defined menus for applications])
fi
)

gl_LD_VERSION_SCRIPT

AC_OUTPUT(Makefile po/Makefile util/Makefile util/po/Makefile test/Makefile \
	WINGs/Makefile WINGs/WINGs/Makefile WINGs/Documentation/Makefile \
	WINGs/Examples/Makefile WINGs/Resources/Makefile WINGs/Tests/Makefile \
	WINGs/Extras/Makefile WINGs/po/Makefile \
	wmlib/Makefile wrlib/Makefile wrlib/tests/Makefile \
	src/Makefile src/wconfig.h \
	doc/Makefile doc/sk/Makefile doc/cs/Makefile \
	doc/ru/Makefile \
	WindowMaker/Makefile WindowMaker/Backgrounds/Makefile \
	WindowMaker/Defaults/Makefile WindowMaker/IconSets/Makefile \
	WindowMaker/Icons/Makefile WindowMaker/Pixmaps/Makefile \
	WindowMaker/Styles/Makefile WindowMaker/Themes/Makefile \
	WPrefs.app/Makefile WPrefs.app/tiff/Makefile WPrefs.app/xpm/Makefile \
	WPrefs.app/po/Makefile )


dnl Output some helpful data for compiling wraster and WINGs/WUtil apps
dnl ===================================================================

dnl echo "WFLAGS=\"$LIBPL_INC_PATH -I$prefix/include\"" > WINGs-flags
dnl echo "WLIBS=\"-L$exec_prefix/lib -lWINGs -lwraster $LIBPL_LIBS $GFXLIBS -lm\""\
dnl 	| sed -e 's|\$(prefix)|'"$prefix|" >> WINGs-flags

dnl The #lp# and #rp# stuff below is a hack because [ and ] get lost when
dnl parsed by m4

AC_SUBST(lib_search_path)
AC_SUBST(inc_search_path)

dnl
dnl Spit out the configuration
dnl ==========================

if test "x$MOFILES" = "x"; then
	mof=None
else
	mof=`echo $MOFILES`
fi

if test "x$MOFILES" = "x"; then
	languages=None
else
	languages=`echo $MOFILES | sed 's/.mo//g'`
fi

echo
echo "Window Maker was configured as follows:"
echo
echo "Installation path prefix            : $prefix"
echo "Installation path for binaries      : $_bindir"
echo "Installation path for libraries     : $libdir"
echo "Installation path for WPrefs.app    : $wprefs_base_dir" | sed -e 's|\${prefix}|'"$prefix|"
echo "Supported X extensions:             :$supported_xext"
echo "Supported graphic format libraries  :$supported_gfx"
echo "Unsupported features                :$unsupported"
echo "Antialiased text support in WINGs   : $xft"
echo "Translated message files to install : $mof"
dnl echo "Supported languages beside English  : $languages"
if test "x$MOFILES" != "x"; then
	echo "Installation path for translations  : $NLSDIR" | sed -e 's|\$(prefix)|'"$prefix|"
fi
AS_IF([test "x$debug" = "xyes"],
    [AS_ECHO(["Debug enabled: CFLAGS = $CFLAGS"]) ])
echo

dnl WM_PRINT_REDCRAP_BUG_STATUS

AS_IF([test "x$enable_jpeg" = xno], [dnl
    AS_ECHO(["WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING"])
    AS_ECHO([])
    AS_ECHO(["JPEG support will not be included because the JPEG library is"])
    AS_ECHO(["not installed correctly or was not found. Background images"])
    AS_ECHO(["from themes will not display as they usually are JPEG files."])
    AS_ECHO([])
    AS_ECHO(["To fix, download and install the jpeg library and/or make sure you"])
    AS_ECHO(["installed all jpeg related packages, SPECIALLY the development packages"])
    AS_ECHO(["like jpeg-dev (if you use some prepackaged version of libjpeg)."])
    AS_ECHO([])
    AS_ECHO(["WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING"])dnl
])


dnl This is for Emacs.  I'm lazy, I know... (nicolai)
dnl ================================================
dnl Local Variables:
dnl compile-command: "autoconf"
dnl End:

