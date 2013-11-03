# wm_imgfmt_check.m4 - Macros to check for image file format support libraries
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

# WM_IMGFMT_CHECK_GIF
# -------------------
#
# Check for GIF file support through 'libgif' or 'libungif'
# The check depends on variable 'enable_gif' being either:
#   yes  - detect, fail if not found
#   no   - do not detect, disable support
#   auto - detect, disable if not found
#
# When found, append appropriate stuff in GFXLIBS, and append info to
# the variable 'supported_gfx'
# When not found, append info to variable 'unsupported'
AC_DEFUN_ONCE([WM_IMGFMT_CHECK_GIF],
[AC_REQUIRE([_WM_IMGFMT_CHECK_FUNCTS])
AS_IF([test "x$enable_gif" = "xno"],
    [unsupported="$unsupported GIF"],
    [AC_CACHE_CHECK([for GIF support library], [wm_cv_imgfmt_gif],
        [wm_cv_imgfmt_gif=no
         wm_save_LIBS="$LIBS"
         dnl
         dnl We check first if one of the known libraries is available
         for wm_arg in "-lgif" "-lungif" ; do
           AS_IF([wm_fn_imgfmt_try_link "DGifOpenFileName" "$XLFLAGS $XLIBS $wm_arg"],
             [wm_cv_imgfmt_gif="$wm_arg" ; break])
         done
         LIBS="$wm_save_LIBS"
         AS_IF([test "x$enable_gif$wm_cv_imgfmt_gif" = "xyesno"],
           [AC_MSG_ERROR([explicit GIF support requested but no library found])])
         AS_IF([test "x$wm_cv_imgfmt_gif" != "xno"],
           [dnl
            dnl A library was found, now check for the appropriate header
            wm_save_CFLAGS="$CFLAGS"
            AS_IF([wm_fn_imgfmt_try_compile "gif_lib.h" "return 0" ""],
              [],
              [AC_MSG_ERROR([found $wm_cv_imgfmt_gif but could not find appropriate header - are you missing libgif-dev package?])])
            AS_IF([wm_fn_imgfmt_try_compile "gif_lib.h" "DGifOpenFileName(filename)" ""],
              [wm_cv_imgfmt_gif="$wm_cv_imgfmt_gif version:4"],
              [AC_MSG_ERROR([found $wm_cv_imgfmt_gif and header, but cannot compile - unsupported version?])])
            CFLAGS="$wm_save_CFLAGS"])
         ])
    AS_IF([test "x$wm_cv_imgfmt_gif" = "xno"],
        [unsupported="$unsupported GIF"
         enable_gif="no"],
        [supported_gfx="$supported_gfx GIF"
         GFXLIBS="$GFXLIBS `echo "$wm_cv_imgfmt_gif" | sed -e 's, *version:.*,,' `"
         AC_DEFINE_UNQUOTED([USE_GIF],
           [1],
           [defined when valid GIF library with header was found])])
    ])
    AM_CONDITIONAL([USE_GIF], [test "x$enable_gif" != "xno"])dnl
]) dnl AC_DEFUN


# _WM_IMGFMT_CHECK_FUNCTS
# -----------------------
# (internal shell functions)
#
# Create 2 shell functions:
#  wm_fn_imgfmt_try_link: try to link against library
#  wm_fn_imgfmt_try_compile: try to compile against header
#
AC_DEFUN_ONCE([_WM_IMGFMT_CHECK_FUNCTS],
[@%:@ wm_fn_imgfmt_try_link FUNCTION LFLAGS
@%:@ -------------------------------------
@%:@ Try linking aginst library in $LFLAGS using function named $FUNCTION
@%:@ Assumes that LIBS have been saved in 'wm_save_LIBS' by caller
wm_fn_imgfmt_try_link ()
{
  LIBS="$wm_save_LIBS $[]2"
  AC_TRY_LINK_FUNC([$[]1],
    [wm_retval=0],
    [wm_retval=1])
  AS_SET_STATUS([$wm_retval])
}

@%:@ wm_fn_imgfmt_try_compile HEADER FUNC_CALL CFLAGS
@%:@ -----------------------------------------
@%:@ Try to compile using header $HEADER and trying to call a function
@%:@ using the $FUNC_CALL expression and using extra $CFLAGS in the
@%:@ compiler's command line
@%:@ Assumes that CFLAGS have been saved in 'wm_save_CFLAGS' by caller
wm_fn_imgfmt_try_compile ()
{
  CFLAGS="$wm_save_CFLAGS $[]3"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([@%:@include <$[]1>

const char *filename = "dummy";], [  $[]2;])],
    [wm_retval=0],
    [wm_retval=1])
  AS_SET_STATUS([$wm_retval])
}
])
