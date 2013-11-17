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


# WM_XEXT_CHECK_XINERAMA
# ----------------------
#
# Check for the Xinerama extension for multiscreen-as-one support
# The check depends on variable 'enable_xinerama' being either:
#   yes  - detect, fail if not found
#   no   - do not detect, disable support
#   auto - detect, disable if not found
#
# When found, append appropriate stuff in LIBXINERAMA, and append info to
# the variable 'supported_xext'
# When not found, append info to variable 'unsupported'
AC_DEFUN_ONCE([WM_XEXT_CHECK_XINERAMA],
[LIBXINERAMA=""
AS_IF([test "x$enable_xinerama" = "xno"],
    [unsupported="$unsupported Xinerama"],
    [AC_CACHE_CHECK([for Xinerama support library], [wm_cv_xext_xinerama],
        [wm_cv_xext_xinerama=no
         dnl
         dnl We check that the library is available
         wm_save_LIBS="$LIBS"
         for wm_arg in dnl
dnl           Lib flag   % Function name        % info
             "-lXinerama % XineramaQueryScreens" dnl
             "-lXext     % XineramaGetInfo      % solaris" ; do
           AS_IF([wm_fn_lib_try_link "`echo "$wm_arg" | dnl
sed -e 's,^[[^%]]*% *,,' | sed -e 's, *%.*$,,' `" dnl
"$XLFLAGS $XLIBS `echo "$wm_arg" | sed -e 's, *%.*$,,' `"],
             [wm_cv_xext_xinerama="`echo "$wm_arg" | sed -e 's, *%[[^%]]*, ,' `"
              break])
         done
         LIBS="$wm_save_LIBS"
         AS_IF([test "x$enable_xinerama$wm_cv_xext_xinerama" = "xyesno"],
            [AC_MSG_ERROR([explicit Xinerama support requested but no library found])])
         dnl
         dnl A library was found, check if header is available and compile
         wm_save_CFLAGS="$CFLAGS"
         AS_CASE([`echo "$wm_cv_xext_xinerama" | sed -e 's,^[[^%]]*,,' `],
             [*solaris*], [wm_header="X11/extensions/xinerama.h" ; wm_fct="XineramaGetInfo(NULL, 0, NULL, NULL, &intval)"],
             [wm_header="X11/extensions/Xinerama.h" ; wm_fct="XineramaQueryScreens(NULL, &intval)"])
         AS_IF([wm_fn_lib_try_compile "$wm_header" "int intval;" "$wm_fct" ""],
             [],
             [AC_MSG_ERROR([found $wm_cv_xext_xinerama but cannot compile with the header])])
         AS_UNSET([wm_header])
         AS_UNSET([wm_fct])
         CFLAGS="$wm_save_CFLAGS"])
     AS_IF([test "x$wm_cv_xext_xinerama" = "xno"],
        [unsupported="$unsupported Xinerama"
         enable_xinerama="no"],
        [LIBXINERAMA="`echo "$wm_cv_xext_xinerama" | sed -e 's, *%.*$,,' `"
         AC_DEFINE([USE_XINERAMA], [1],
             [defined when usable Xinerama library with header was found])
         AS_CASE([`echo "$wm_cv_xext_xinerama" | sed -e 's,^[[^%]]*,,' `],
             [*solaris*], [AC_DEFINE([SOLARIS_XINERAMA], [1],
                 [defined when the Solaris Xinerama extension was detected])])
         supported_xext="$supported_xext Xinerama"])
    ])
AC_SUBST(LIBXINERAMA)dnl
])
