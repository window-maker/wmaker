# wm_xext_check.m4 - Macros to check for X extensions support libraries
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

# WM_XEXT_CHECK_XSHAPE
# --------------------
#
# Check for the X Shaped Window extension
# The check depends on variable 'enable_xshape' being either:
#   yes  - detect, fail if not found
#   no   - do not detect, disable support
#   auto - detect, disable if not found
#
# When found, append appropriate stuff in XLIBS, and append info to
# the variable 'supported_xext'
# When not found, append info to variable 'unsupported'
AC_DEFUN_ONCE([WM_XEXT_CHECK_XSHAPE],
[WM_LIB_CHECK([XShape], [-lXext], [XShapeSelectInput], [$XLIBS],
    [wm_save_CFLAGS="$CFLAGS"
     AS_IF([wm_fn_lib_try_compile "X11/extensions/shape.h" "Window win;" "XShapeSelectInput(NULL, win, 0)" ""],
        [],
        [AC_MSG_ERROR([found $CACHEVAR but cannot compile using XShape header])])
     CFLAGS="$wm_save_CFLAGS"],
    [supported_xext], [XLIBS], [enable_shape], [-])dnl
]) dnl AC_DEFUN
