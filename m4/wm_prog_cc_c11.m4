# wm_prog_cc_c11.m4 - Macros to see if compiler may support STD C11
#
# Copyright (c) 2013 Christophe CURIS
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

# WM_PROG_CC_C11
# ---------------------
#
# Check if the compiler supports C11 standard natively, or if any
# option may help enabling the support
# This is (in concept) similar to AC_PROG_CC_C11, which is unfortunately
# not yet available in autotools; as a side effect we only check for
# compiler's acknowledgement and a few features instead of full support
AC_DEFUN_ONCE([WM_PROG_CC_C11],
[AC_CACHE_CHECK([for C11 standard support], [wm_cv_prog_cc_c11],
    [wm_cv_prog_cc_c11=no
     wm_save_CFLAGS="$CFLAGS"
     for wm_arg in dnl
"% native"   dnl
"-std=c11"
     do
         CFLAGS="$wm_save_CFLAGS `echo $wm_arg | sed -e 's,%.*,,' `"
         AC_COMPILE_IFELSE(
             [AC_LANG_PROGRAM([], [dnl
#if __STDC_VERSION__ < 201112L
fail_because_stdc_version_is_older_than_C11;
#endif
])],
             [wm_cv_prog_cc_c11="`echo $wm_arg | sed -e 's,.*% *,,' `" ; break])
     done
     CFLAGS="$wm_save_CFLAGS"])
AS_CASE([$wm_cv_prog_cc_c11],
    [no|native], [],
    [CFLAGS="$CFLAGS $wm_cv_prog_cc_c11"])
])
