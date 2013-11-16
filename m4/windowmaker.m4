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
