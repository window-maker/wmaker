# windowmaker.m4 - General macros for Window Maker autoconf
#
# Copyright (c) 2004 Dan Pascu
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


dnl Tell m4 to not allow stuff starting with "WM_" in the generated file
dnl because this is likely a problem of a macro that was not expanded as
dnl expected (with an exception for an already used variable name)
m4_pattern_forbid([^_?WM_])
m4_pattern_allow([^WM_OSDEP(_[A-Z]*)?$])


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


dnl _WM_LIB_CHECK_FUNCTS
dnl -----------------------
dnl (internal shell functions)
dnl
dnl Create 2 shell functions:
dnl  wm_fn_imgfmt_try_link: try to link against library
dnl  wm_fn_imgfmt_try_compile: try to compile against header
dnl
AC_DEFUN_ONCE([_WM_LIB_CHECK_FUNCTS],
[@%:@ wm_fn_lib_try_link FUNCTION LFLAGS
@%:@ ----------------------------------
@%:@ Try linking against library in $LFLAGS using function named $FUNCTION
@%:@ Assumes that LIBS have been saved in 'wm_save_LIBS' by caller
wm_fn_lib_try_link ()
{
  LIBS="$wm_save_LIBS $[]2"
  AC_TRY_LINK_FUNC([$[]1],
    [wm_retval=0],
    [wm_retval=1])
  AS_SET_STATUS([$wm_retval])
}

@%:@ wm_fn_lib_try_compile HEADER GVARDEF FUNC_CALL CFLAGS
@%:@ -----------------------------------------------------
@%:@ Try to compile using header $HEADER and trying to call a function
@%:@ using the $FUNC_CALL expression and using extra $CFLAGS in the
@%:@ compiler's command line; GVARDEF can be used to include one line
@%:@ in the global context of the source.
@%:@ Assumes that CFLAGS have been saved in 'wm_save_CFLAGS' by caller
wm_fn_lib_try_compile ()
{
  CFLAGS="$wm_save_CFLAGS $[]4"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([@%:@include <$[]1>

$[]2], [  $[]3;])],
    [wm_retval=0],
    [wm_retval=1])
  AS_SET_STATUS([$wm_retval])
}
])
