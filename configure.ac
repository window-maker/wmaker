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


AC_INIT(src/WindowMaker.h)



AM_INIT_AUTOMAKE(WindowMaker, 0.81.0)

AC_PROG_LIBTOOL

# by Marcelo Magallon <mmagallo@efis.ucr.ac.cr>
# Turn around -rpath problem with libtool 1.0c
# This define should be improbable enough to not conflict with anything
case ${host} in
  *-pc-linux-gnu)
    AC_MSG_RESULT([Fixing libtool for -rpath problems.])
    sed < libtool > libtool-2 \
    's/^hardcode_libdir_flag_spec.*$/hardcode_libdir_flag_spec=" -D__LIBTOOL_IS_A_FOOL__ "/'
    mv libtool-2 libtool
    chmod 755 libtool
  ;;
esac


AM_CONFIG_HEADER(src/config.h)


dnl Checks for host/os name
dnl =======================
dnl AC_CANONICAL_HOST -- already done by AC_PROG_LIBTOOL


dnl Checks for programs.
dnl ===================
AC_ISC_POSIX
AC_PROG_CC
dnl AC_PROG_MAKE_SET -- already done by AM_INIT_AUTOMAKE
#AC_PROG_RANLIB
dnl AC_PROG_INSTALL -- already done by AM_INIT_AUTOMAKE
AC_PROG_LN_S
AC_PROG_GCC_TRADITIONAL


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

_libdir=`eval echo $libdir`
_libdir=`eval echo $_libdir`
lib_search_path="-L$_libdir"

inc_search_path=`eval echo $includedir`
inc_search_path="-I`eval echo $inc_search_path`"


dnl
dnl Check for RedHat bugs
dnl =====================

dnl WM_CHECK_REDCRAP_BUGS($prefix,$_bindir,$_libdir)


dnl
dnl Specify paths to look for libraries and headers
dnl ===============================================
AC_ARG_WITH(libs-from,
[  --with-libs-from        pass compiler flags to look for libraries],
	[lib_search_path="$withval $lib_search_path"])

AC_ARG_WITH(incs-from,
[  --with-incs-from        pass compiler flags to look for header files],
	[inc_search_path="$withval $inc_search_path"])



dnl Checks for library functions.
dnl ============================
dnl not used anywhere
dnl AC_FUNC_MEMCMP  
AC_FUNC_VPRINTF
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(gethostname select poll strerror strcasecmp strncasecmp setsid atexit mallinfo mkstemp snprintf vsnprintf asprintf vasprintf)


dnl ripped from samba
dnl 
AC_CACHE_CHECK([for C99 vsnprintf],_cv_HAVE_C99_VSNPRINTF,[
AC_TRY_RUN([
#include <sys/types.h>
#include <stdarg.h>
void foo(const char *format, ...) { 
       va_list ap;
       int len;
       char buf[5];

       va_start(ap, format);
       len = vsnprintf(0, 0, format, ap);
       va_end(ap);
       if (len != 5) exit(1);

       if (snprintf(buf, 3, "hello") != 5 || strcmp(buf, "he") != 0) exit(1);

       exit(0);
}
main() { foo("hello"); }
],
_cv_HAVE_C99_VSNPRINTF=yes,_cv_HAVE_C99_VSNPRINTF=no,_cv_HAVE_C99_VSNPRINTF=cross)])
if test x"$_cv_HAVE_C99_VSNPRINTF" = x"yes"; then
    AC_DEFINE(HAVE_C99_VSNPRINTF, 1, [define if you have vsnprintf with C99 semantics (set by configure)])
fi

dnl Loading of dynamic libraries at runtime
dnl =======================================
DLLIBS=""

AC_CHECK_FUNC(dlopen, [HAVEDL="yes"], 
	AC_CHECK_LIB(dl, dlopen, [DLLIBS="-ldl" HAVEDL="yes"],
			 DLLIBS="" ))

if test "x$HAVEDL" = xyes; then
    AC_CHECK_HEADERS(dlfcn.h)
fi



dnl Check CPP
dnl =========
if test "x$CPP_PATH" = x; then
    AC_PATH_PROG(CPP_PATH, cpp, notfound, 
		$PATH:/lib:/usr/bin:/bin:/usr/lib:/usr/ccs/lib)
fi

dnl
dnl Tell stupid Solaris cpp that the files it will process have C++ like syntax
dnl RedHat 5.x is broken too, so it won't make a symlink from cpp to the
dnl standard locations
dnl
if test "$CPP_PATH" = "/usr/ccs/lib/cpp" -o "$CPP_PATH" = "notfound" ; then
    if test "$GCC" = "yes"; then
	CPP_PATH="gcc -E -x c"
    else 
	if test "$CPP_PATH" = "/usr/ccs/lib/cpp"; then
	    CPP_PATH="$CPP_PATH -B"
	else
	    echo "cpp, the C preprocessor was not found in your system."
  	    echo "Create a symbolic link from it to /lib/cpp and rerun configure"
  	    exit
	fi
    fi
fi
AC_DEFINE_UNQUOTED(CPP_PATH, "$CPP_PATH", [define to the path to cpp])



dnl Checks for header files.
dnl =======================
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/time.h sys/types.h libintl.h sys/select.h poll.h malloc.h ctype.h string.h strings.h)



dnl Checks for typedefs, structures, and compiler characteristics.
dnl ==============================================================
AC_DECL_SYS_SIGLIST
AC_C_CONST
#AC_TYPE_SIZE_T
#AC_TYPE_PID_T
AC_TYPE_SIGNAL



dnl Compiler/architecture specific optimizations
dnl ============================================


dnl GCC/as with MMX support
dnl -----------------------

# until we fix it, leave it disabled
asm_support=no
mmx_support=no
if test "$ac_cv_prog_gcc" = yes; then
    x86=0
    changequote(,)dnl
    case "$host_cpu" in
       i[3456]86) x86=1
       ;;
    esac
    changequote([,])dnl

    if test "$host_os" != "linux" -a "$host_os" != "linux-gnu" ; then
       x86=0
    fi

    if test $x86 = 1; then
       AC_DEFINE(ASM_X86, 1, [define if processor is x86 (normally detected by configure)])
       asm_support=yes

       AC_CACHE_CHECK(whether gcc supports MMX(tm) inline asm,
                      ac_cv_c_inline_mmx,
                      [AC_TRY_LINK(,[asm ("movq %mm0, %mm1");],
                      ac_cv_c_inline_mmx=yes,
                      ac_cv_c_inline_mmx=no)])

       if test "x$ac_cv_c_inline_mmx" = xyes; then
          AC_DEFINE(ASM_X86_MMX, 1, [define if processor is x86 with MMX(tm) support (normally autodetected by configure)])
          mmx_support=yes
       fi
    fi
fi



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
#       if test "$LINGUAS" = ""; then
#   	    ling=` (cd src/po; /bin/ls *.po) `
#    	    for l in $ling; do
#	        lcode=`basename $l .po`
#	        LINGUAS="$LINGUAS $lcode"
#           done
#       fi
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
	PO=""
fi


dnl The Tower of Babel
dnl ==================

dnl List of supported locales
dnl -------------------------
supported_locales="be bg cs da de el es et fi fr gl hr hu it ja ko ms nl no pl pt ro ru sk sv tr zh_CN zh_TW.Big5 ca"
supported_wprefs_locales="bg cs de es et fi fr hr hu it ja ko pt ru sk zh_CN zh_TW.Big5 ca"
supported_wings_locales="cs de fr sk ca"

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


dnl Kanji Characters support
dnl ========================

case $host_os in
	freebsd*)
		AC_CHECK_LIB(xpg4, setlocale, [LIBS="$LIBS -lxpg4"]);;
	*)
	;;
esac



dnl Added by Oliver - Support for NLSDIR option,   Hi Oliver!
dnl ===========================================
AC_ARG_WITH(nlsdir, 
	[  --with-nlsdir=PATH      specify where the locale stuff should go ])


if test "x$NLSDIR" = "x"; then
	if test "x$with_nlsdir" != "x"; then
		NLSDIR=$with_nlsdir
	else
		NLSDIR='$(prefix)/lib/locale'
	fi
fi



AC_SUBST(DLLIBS)
AC_SUBST(INTLIBS)
AC_SUBST(NLSDIR)
AC_SUBST(MOFILES)
AC_SUBST(WPMOFILES)
AC_SUBST(WINGSMOFILES)
AC_SUBST(supported_locales)



dnl Support for various hint things
dnl ===============================


AC_ARG_ENABLE(gnome,
  [  --enable-gnome          enable stuff needed for GNOME ],
  [if test x$enableval = xyes; then
    AC_DEFINE(GNOME_STUFF, 1, [define if you want GNOME stuff support])
    gnome_on=yes
  fi])


AC_ARG_ENABLE(kde,
  [  --enable-kde            enable support for KDE window manager (kwm) hints ],
  [if test x$enableval = xyes; then
    AC_DEFINE(KWM_HINTS, 1, [define if you want KDE hint support])
    kde_on=yes
  fi])


AC_ARG_ENABLE(openlook,
  [  --enable-openlook       enable support for OPEN LOOK(tm) (olwm) hints ],
  [if test x$enableval = xyes; then
    AC_DEFINE(OLWM_HINTS, 1, [define if you want OPEN LOOK(tm) hint support])
    openlook_on=yes
  fi])


dnl
dnl Disable some stuff that are duplicated in kde
dnl ---------------------------------------------
AC_ARG_ENABLE(lite,
  [  --enable-lite           disable some stuff (dont use it) ],
  [if test x$enableval = xyes; then
        LITE=yes
	AC_DEFINE(LITE, 1, [define if you want the 'lite' version])
        AC_SUBST(LITE)
  fi])



dnl
dnl Networking stuff
dnl

NETLIBS=""

AC_CHECK_FUNC(connect,,
	AC_CHECK_LIB(socket, connect, NETLIBS="$NETLIBS -lsocket"))

AC_CHECK_FUNC(gethostbyname,,
	AC_CHECK_LIB(nsl, gethostbyname, NETLIBS="$NETLIBS -lnsl"))

AC_CHECK_FUNC(inet_aton, AC_DEFINE(HAVE_INET_ATON, 1, [define if you have then inet_aton function (set by configure)]),
     for lib in resolv socket inet bsd; do
	AC_CHECK_LIB($lib, inet_aton, [AC_DEFINE(HAVE_INET_ATON)
	             NETLIBS="$NETLIBS -l$lib"; break],, $NETLIBS)
     done)

AC_SUBST(NETLIBS)


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
AC_ARG_ENABLE(locale, 
[  --disable-locale        disable use of X locale support],
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
AC_ARG_ENABLE(modelock,
[  --enable-modelock       XKB keyboard language status support],
		AC_DEFINE(XKB_MODELOCK, 1, [whether XKB language MODELOCK should be enabled]))



dnl Shape support
dnl =============
shape=yes
AC_ARG_ENABLE(shape, 
[  --disable-shape         disable shaped window extension support],
		shape=$enableval, shape=yes)

added_xext=no

if test "$shape" = yes; then
	AC_CHECK_LIB(Xext, XShapeSelectInput, [XLIBS="-lXext $XLIBS"
		added_xext=yes
		AC_DEFINE(SHAPE, 1, [define if you want support for shaped windows (set by configure)])], 
		shape=no, $XLFLAGS $XLIBS)
fi


dnl Xft antialiased font support
dnl ============================
xft=yes
XFTLIBS=""
AC_ARG_ENABLE(xft, 
[  --disable-xft           disable Xft antialiased font support],
		xft=$enableval, xft=yes)

if test "$xft" = yes; then
	AC_CHECK_LIB(Xft, XftDrawCreate, [XFTLIBS="-lXft"
		AC_SUBST(XFTLIBS)
		AC_DEFINE(XFT, 1, [define if you want support for antialiased fonts (set by configure)])],
		xft=no, $XLFLAGS $XLIBS)
fi


dnl XINERAMA support
dnl ================
xinerama=no
AC_ARG_ENABLE(xinerama,
[  --enable-xinerama       enable Xinerama extension support],
		xinerama=$enableval, xinerama=no)

if test "$xinerama" = yes; then
	AC_CHECK_LIB(Xinerama, XineramaQueryScreens, [XLIBS="-lXinerama $XLIBS"
		AC_DEFINE(XINERAMA, 1, [define if you want support for the XINERAMA extension (set by configure)])],
		xinerama=no, $XLFLAGS $XLIBS)
fi



dnl MIT-SHM support
dnl ===============
shm=yes
AC_ARG_ENABLE(shm,
[  --disable-shm           disable usage of MIT-SHM extension],
		shm=$enableval, shm=yes)

if test "$shm" = yes; then
	AC_CHECK_LIB(Xext, XShmAttach, ok=yes, ok=no, $XLFLAGS $XLIBS)

	if test "$ok" = yes; then
	AC_CHECK_FUNC(shmget, ok=yes, ok=no)
	fi

	if test "$ok" = yes; then
		if test "$added_xext" = no; then
			XLIBS="-lXext $XLIBS"
		fi
                AC_DEFINE(XSHM, 1, [define if X's shared memory extension is available (set by configure)])
	fi
fi


dnl R6 Style Session Management Support
dnl ===================================

#
#
#AC_DEFINE(R6SM)
#AC_SUBST(XSMPLIBS)
#


dnl ==============================================
dnl         Graphic Format Libraries
dnl ==============================================





dnl XPM Support
dnl ===========
xpm=yes
AC_ARG_ENABLE(xpm, 
[  --disable-xpm           disable use of XPM pixmaps through libXpm],
	xpm=$enableval, xpm=yes)

if test "$xpm" = yes; then
    WM_CHECK_LIB(Xpm, XpmCreatePixmapFromData, [$XLFLAGS $XLIBS])

    if test "x$ac_cv_lib_Xpm_XpmCreatePixmapFromData" = xyes; then
        WM_CHECK_HEADER(X11/xpm.h)
	if test "x$ac_cv_header_X11_xpm_h" = xyes; then
		GFXLIBS="$GFXLIBS -lXpm"
		supported_gfx="XPM"
		AC_DEFINE(USE_XPM, 1, [define if XPM libraries are available (set by configure)])
	else
		supported_gfx="builtin-XPM"
	fi
    fi
fi

# for wmlib
AC_SUBST(XCFLAGS)
# for test
AC_SUBST(XLFLAGS)
AC_SUBST(XLIBS)

AC_SUBST(X_EXTRA_LIBS)

dnl ===============================================
dnl 		End of stuff that uses X
dnl ===============================================



dnl PNG Support
dnl ===========
png=yes
AC_ARG_ENABLE(png, 
[  --disable-png           disable PNG support through libpng],
	png=$enableval, png=yes, png=no)


if test "$png" = yes ; then
    WM_CHECK_LIB(png, png_get_valid, [-lz -lm])

    if test "x$ac_cv_lib_png_png_get_valid" = xyes; then
    	WM_CHECK_HEADER(png.h)
	if test "x$ac_cv_header_png_h" = xyes; then
	    GFXLIBS="$GFXLIBS -lpng -lz" 
	    supported_gfx="$supported_gfx PNG"
            AC_DEFINE(USE_PNG, 1, [define if PNG libraries are available (set by configure)])
	fi
    fi
fi


dnl JPEG Support
dnl ============
jpeg=yes
ljpeg=""
AC_ARG_ENABLE(jpeg,
[  --disable-jpeg          disable JPEG support through libjpeg],
	jpeg=$enableval, jpeg=yes, jpeg=no)

if test "$jpeg" = yes; then
    WM_CHECK_LIB(jpeg, jpeg_destroy_compress)

    if test "x$ac_cv_lib_jpeg_jpeg_destroy_compress" = xyes; then

    	ljpeg="-ljpeg"

	WM_CHECK_HEADER(jpeglib.h)
	if test "x$ac_cv_header_jpeglib_h" = xyes; then
	    GFXLIBS="$GFXLIBS -ljpeg"
	    supported_gfx="$supported_gfx JPEG"
            AC_DEFINE(USE_JPEG, 1, [define if JPEG libraries are available (set by configure)])
	fi
    fi
fi


dnl GIF Support
dnl ============
gif=yes
AC_ARG_ENABLE(gif,
[  --disable-gif           disable GIF support through libgif or libungif],
	gif=$enableval, gif=yes, gif=no)

if test "$gif" = yes; then
    my_libname=""
    WM_CHECK_LIB(ungif, DGifOpenFileName, [$XLFLAGS $XLIBS])
    if test "x$ac_cv_lib_ungif_DGifOpenFileName" = xyes; then
        my_libname=-lungif
    fi
dnl
dnl libungif is the same thing as libgif for all practical purposes.
dnl
    if test "x$my_libname" = x; then
        WM_CHECK_LIB(gif, DGifOpenFileName, [$XLFLAGS $XLIBS])
        if test "x$ac_cv_lib_gif_DGifOpenFileName" = xyes; then
            my_libname=-lgif
        fi
    fi

    if test "$my_libname" != x; then
	WM_CHECK_HEADER(gif_lib.h)
	if test "x$ac_cv_header_gif_lib_h" = xyes; then
	    GFXLIBS="$GFXLIBS $my_libname"
	    supported_gfx="$supported_gfx GIF"
            AC_DEFINE(USE_GIF, 1, [define if GIF libraries are available (set by configure)])
	fi
    fi
fi



dnl TIFF Support
dnl ============
AC_ARG_ENABLE(tiff, 
[  --disable-tiff          disable use of TIFF images through libtiff],
	tif=$enableval, tif=yes, tif=no)

#
# TIFF can optionally have JPEG and/or zlib support. Must find out
# when they are supported so that correct library flags are passed during
# detection and linkage
#
#
# By default use xpm icons if tiff is not found.
ICONEXT="xpm"
#

if test "$tif" = yes; then
    my_libname=""
    WM_CHECK_LIB(tiff, TIFFGetVersion, [-lm])
    if test "x$ac_cv_lib_tiff_TIFFGetVersion" = xyes; then
    	my_libname="-ltiff"
    fi
dnl
dnl Retry with zlib
dnl
    unset ac_cv_lib_tiff_TIFFGetVersion
    if test "x$my_libname" = x; then
    	WM_CHECK_LIB(tiff, TIFFGetVersion, [$ljpeg -lz -lm])
	if test "x$ac_cv_lib_tiff_TIFFGetVersion" = xyes; then
	    my_libname="-ltiff -lz"
	fi
    fi

    if test "x$my_libname" = x; then
	WM_CHECK_LIB(tiff34, TIFFGetVersion, [$ljpeg -lm])
	if test "x$ac_cv_lib_tiff34_TIFFGetVersion" = xyes; then
	    my_libname="-ltiff34"
	fi
    fi


    if test "x$my_libname" != x; then
	WM_CHECK_HEADER(tiffio.h)
	if test "x$ac_cv_header_tiffio_h" = xyes; then
	    GFXLIBS="$my_libname $GFXLIBS"
	    ICONEXT="tiff"
            supported_gfx="$supported_gfx TIFF"
            AC_DEFINE(USE_TIFF, 1, [define if TIFF libraries are available (set by configure)])	    
	fi
    fi
fi

LIBRARY_SEARCH_PATH="$lib_search_path"
HEADER_SEARCH_PATH="$inc_search_path"

AC_SUBST(LIBRARY_SEARCH_PATH)
AC_SUBST(HEADER_SEARCH_PATH)


AC_SUBST(GFXLIBS)
AC_SUBST(ICONEXT)


dnl ==============================================
dnl         End of Graphic Format Libraries
dnl ==============================================


dnl
dnl stdlib.h is checked here, because of conflict in jpeglib.h
AC_CHECK_HEADERS(stdlib.h)

# AC_PREFIX_PROGRAM(wmaker)

dnl Support for PIXMAPDIR option
dnl ============================
AC_ARG_WITH(pixmapdir,
[  --with-pixmapdir=PATH   specify where pixmaps are located [DATADIR/pixmaps]])

if test "x$with_pixmapdir" != "x"; then
	pixmapdir=$with_pixmapdir
else
	pixmapdir=`eval echo ${datadir}/pixmaps`
fi

AC_DEFINE_UNQUOTED(PIXMAPDIR, "$pixmapdir", [define an extra path for pixmaps (set by configure)])

pkgdatadir=`eval echo $datadir`
AC_DEFINE_UNQUOTED(PKGDATADIR, "$pkgdatadir/WindowMaker", [where shared data is stored (defined by configure)])

_sysconfdir=`eval echo $sysconfdir`
AC_DEFINE_UNQUOTED(SYSCONFDIR, "$_sysconfdir", [where the configuration is stored (defined by configure)])


dnl Support for GNUSTEP_LOCAL_ROOT, for WPrefs.app
dnl ==============================================

appspath=""

AC_ARG_WITH(appspath,
[  --with-appspath=PATH    specify the directory for GNUstep applications], appspath=$withval )

if test "x$appspath" = "x"; then
    gnustepdir='$(prefix)/GNUstep'

    if test "x$GNUSTEP_LOCAL_ROOT" != "x" ; then
	gnustepdir=`echo "$GNUSTEP_LOCAL_ROOT" | sed -e "s|^${prefix}|prefix|"`
	gnustepdir=`echo $gnustepdir | sed -e 's|^prefix|${prefix}|'`
    fi

    with_appspath=$gnustepdir/Apps
fi

wprefsdir=$with_appspath/WPrefs.app

AC_SUBST(wprefsdir)


dnl Enable User Defined Menu thing
dnl ==================================
AC_ARG_ENABLE(usermenu,
[  --enable-usermenu       user defined menus for applications
],
if test "$enableval" = yes; then
	AC_DEFINE(USER_MENU, 1, [define if you want user defined menus for applications])
fi
)


dnl
dnl Nicolai:  Program tests for Documentation Section
dnl =================================================
dnl DOCTYPES=""
dnl AC_CHECK_PROG(MAKEINFO, makeinfo, makeinfo, ,$PATH)
dnl if test "x$MAKEINFO" != "x" ; then
dnl       DOCTYPES="$DOCTYPES info_doc"
dnl fi
dnl AC_CHECK_PROG(TEX, tex, tex, ,$PATH)
dnl if test "x$TEX" != "x" ; then
dnl       DOCTYPES="$DOCTYPES dvi_doc"
dnl fi
dnl AC_CHECK_PROG(DVIPS, dvips, dvips, ,$PATH)
dnl if test "x$DVIPS" != "x" ; then
dnl       DOCTYPES="$DOCTYPES ps_doc"
dnl fi
dnl AC_CHECK_PROG(PERL, perl, perl, ,$PATH)
dnl if test "x$PERL" != "x" ; then
dnl       DOCTYPES="$DOCTYPES html_doc"
dnl fi
dnl AC_PATH_PROG(PERL_PATH, perl, perl, ,$PATH)
dnl AC_CHECK_PROG(TEXI2HTML, texi2html, texi2html, ,$PATH)
  
dnl AC_SUBST(DOCTYPES)

AC_OUTPUT(Makefile po/Makefile util/Makefile test/Makefile \
	WINGs/Makefile WINGs/WINGs/Makefile WINGs/Documentation/Makefile \
	WINGs/Examples/Makefile WINGs/Resources/Makefile WINGs/Tests/Makefile \
	WINGs/Extras/Makefile WINGs/po/Makefile \
	wmlib/Makefile wrlib/Makefile src/Makefile src/wconfig.h \
	doc/Makefile doc/sk/Makefile \
	WindowMaker/Makefile WindowMaker/Backgrounds/Makefile \
	WindowMaker/Defaults/Makefile WindowMaker/IconSets/Makefile \
	WindowMaker/Icons/Makefile WindowMaker/Pixmaps/Makefile \
	WindowMaker/Styles/Makefile WindowMaker/Themes/Makefile \
	WPrefs.app/Makefile WPrefs.app/tiff/Makefile WPrefs.app/xpm/Makefile \
	WPrefs.app/po/Makefile \
	contrib/Makefile contrib/WindowMaker.spec )
#	plugins/Makefile plugins/libwmfun/Makefile)



dnl
dnl Output some helpful data for compiling wraster and WINGs/WUtil apps
dnl ===================================================================
dnl

dnl echo "WFLAGS=\"$LIBPL_INC_PATH -I$prefix/include\"" > WINGs-flags
dnl echo "WLIBS=\"-L$exec_prefix/lib -lWINGs -lwraster $LIBPL_LIBS $GFXLIBS -lm\""\
dnl 	| sed -e 's|\$(prefix)|'"$prefix|" >> WINGs-flags

dnl The #lp# and #rp# stuff below is a hack because [ and ] get lost when
dnl parsed by m4

cat <<EOF >get-wraster-flags
#!/bin/sh

WCFLAGS="$inc_search_path"
WLFLAGS="$lib_search_path"
WLIBS="-lwraster $GFXLIBS $XLIBS -lm"

usage="Usage: get-wraster-flags #lp#--cflags#rp# #lp#--ldflags#rp# #lp#--libs#rp#"

if test \$# -eq 0; then
      echo "\${usage}" 1>&2
      exit 1
fi

while test \$# -gt 0; do
  case \$1 in
    --cflags)
	echo \$WCFLAGS
        ;;
    --ldflags|--lflags)
	echo \$WLFLAGS
        ;;
    --libs)
	echo \$WLIBS
        ;;
    *)
    	echo "\${usage}" 1>&2
    	exit 1
      	;;
  esac
  shift
done


EOF

cat <<EOF >get-wings-flags
#!/bin/sh

WCFLAGS="$inc_search_path"
WLFLAGS="$lib_search_path"
WLIBS="-lWINGs -lwraster $GFXLIBS $XFTLIBS $XLIBS -lm $NETLIBS $INTLIBS"

usage="Usage: get-wings-flags #lp#--cflags#rp# #lp#--ldflags#rp# #lp#--libs#rp#"

if test \$# -eq 0; then
      echo "\${usage}" 1>&2
      exit 1
fi

while test \$# -gt 0; do
  case \$1 in
    --cflags)
	echo \$WCFLAGS
        ;;
    --ldflags|--lflags)
	echo \$WLFLAGS
        ;;
    --libs)
	echo \$WLIBS
        ;;
    *)
    	echo "\${usage}" 1>&2
    	exit 1
      	;;
  esac
  shift
done


EOF

cat <<EOF >get-wutil-flags
#!/bin/sh

WCFLAGS="-I`eval echo ${includedir}`"
WLFLAGS="-L${_libdir}"
WLIBS="-lWUtil $NETLIBS $INTLIBS"

usage="Usage: get-wutil-flags #lp#--cflags#rp# #lp#--ldflags#rp# #lp#--libs#rp#"

if test \$# -eq 0; then
      echo "\${usage}" 1>&2
      exit 1
fi

while test \$# -gt 0; do
  case \$1 in
    --cflags)
	echo \$WCFLAGS
        ;;
    --ldflags|--lflags)
	echo \$WLFLAGS
        ;;
    --libs)
	echo \$WLIBS
        ;;
    *)
    	echo "\${usage}" 1>&2
    	exit 1
      	;;
  esac
  shift
done


EOF

sed 's/#lp#/[/g' get-wraster-flags | sed 's/#rp#/]/g' > wrlib/get-wraster-flags
sed 's/#lp#/[/g' get-wings-flags | sed 's/#rp#/]/g' > WINGs/get-wings-flags
sed 's/#lp#/[/g' get-wutil-flags | sed 's/#rp#/]/g' > WINGs/get-wutil-flags

chmod 755 wrlib/get-wraster-flags WINGs/get-wings-flags WINGs/get-wutil-flags

rm -f get-wraster-flags get-wings-flags get-wutil-flags


dnl
dnl Spit out the configuration
dnl ==========================

supported_gfx="$supported_gfx builtin-PPM"

if test "x$MOFILES" = "x"; then
	mof=none
else
	mof=`echo $MOFILES`
fi

if test "x$MOFILES" = "x"; then
	languages=none
else
	languages=`echo $MOFILES | sed 's/.mo//g'`
fi


echo
echo "Window Maker was configured as follows:"
echo
echo "Installation path prefix            : $prefix"
echo "Installation path for binaries      : $_bindir"
echo "Installation path for WPrefs.app    : $wprefsdir" | sed -e 's|\$(prefix)|'"$prefix|"
echo "Supported graphic format libraries  : $supported_gfx"
echo "Antialiased font support for WINGs  : $xft"
echo "Xinerama extension support          : $xinerama"
echo "Use assembly routines for wrlib     : $asm_support"
echo "Use inline MMX(tm) x86 assembly     : $mmx_support"
echo "Translated message files to install : $mof"
dnl echo "Supported languages beside English  : $languages"
if test "x$MOFILES" != "x"; then
	echo "Installation path for translations  : $NLSDIR" | sed -e 's|\$(prefix)|'"$prefix|"
fi
echo

dnl WM_PRINT_REDCRAP_BUG_STATUS

if test "x$ac_cv_header_jpeglib_h" != xyes; then
echo "WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING"
echo
echo "JPEG support will not be included because the JPEG library is"
echo "not installed correctly or was not found. Background images"
echo "from themes will not display as they usually are JPEG files."
echo
echo "To fix, download and install the jpeg library and/or make sure you"
echo "installed all jpeg related packages, SPECIALLY the development packages"
echo "like jpeg-devel (if you use some prepackaged version of libjpeg)."
echo
echo "WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING"
fi


dnl This is for Emacs.  I'm lazy, I know... (nicolai)
dnl ================================================
dnl Local Variables:
dnl compile-command: "autoconf"
dnl End:

