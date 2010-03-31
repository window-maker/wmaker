dnl
dnl WM_CHECK_LIB(NAME, FUNCTION, EXTRALIBS)
dnl
AC_DEFUN([WM_CHECK_LIB],
[
LDFLAGS_old="$LDFLAGS"
LDFLAGS="$LDFLAGS $lib_search_path"
AC_CHECK_LIB([$1],[$2],yes=yes,no=no,[$3])
LDFLAGS="$LDFLAGS_old"
])

dnl
dnl WM_CHECK_HEADER(NAME)
dnl
AC_DEFUN([WM_CHECK_HEADER],
[
CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $inc_search_path"
AC_CHECK_HEADER([$1])
CPPFLAGS="$CPPFLAGS_old"
])


dnl
dnl WM_CHECK_XFT_VERSION(MIN_VERSION, [ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
dnl
dnl # $XFTFLAGS should be defined before calling this macro,
dnl # else it will not be able to find Xft.h
dnl
AC_DEFUN([WM_CHECK_XFT_VERSION],
[
CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $XFTFLAGS $inc_search_path"
xft_major_version=`echo $1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
xft_minor_version=`echo $1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
xft_micro_version=`echo $1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
AC_MSG_CHECKING([whether libXft is at least version $1])
AC_CACHE_VAL(ac_cv_lib_xft_version_ok,
[AC_TRY_LINK(
[/* Test version of libXft we have */
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#if !defined(XFT_VERSION) || XFT_VERSION < $xft_major_version*10000 + $xft_minor_version*100 + $xft_micro_version
#error libXft on this system is too old. Consider upgrading to at least $1
#endif
], [],
eval "ac_cv_lib_xft_version_ok=yes",
eval "ac_cv_lib_xft_version_ok=no")])
if eval "test \"`echo '$ac_cv_lib_xft_version_ok'`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
  ifelse([$3], , , [$3])
fi
CPPFLAGS="$CPPFLAGS_old"
])
