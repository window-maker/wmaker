# wm_library_constructors.m4 - Macros to see if compiler supports attributes "constructors" for libraries
#
# Copyright (c) 2021 Christophe CURIS
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


# WM_LIBRARY_CONSTRUCTORS
# -----------------------
#
# GCC introduced the attribute 'constructor' which says that a function must be
# called when a library is loaded (needed for C++ support). However this may not
# be totally portable, so we may use the old method "_init" as a fall-back
# solution, which is considered Obsolete/Dangerous.
#
# This is explained here:
#   https://tldp.org/HOWTO/Program-Library-HOWTO/miscellaneous.html#INIT-AND-CLEANUP
#
# This macro is making sure that the compiler supports the attribute, because we
# need it for the WRaster library (see RStartup in wrlib/misc.c to see why).
AC_DEFUN_ONCE([WM_LIBRARY_CONSTRUCTORS],
[AC_CACHE_CHECK([how to declare a library constructor], [wm_cv_library_constructors],
    [save_CFLAGS="$CFLAGS"
     dnl We need this to cause a detectable failure in case of unsupported attribute name
     dnl if we don't do this, we just get a warning and AC_COMPILE suppose it was ok.
     CFLAGS="$CFLAGS -Werror"
     AC_COMPILE_IFELSE(
        [AC_LANG_SOURCE([[
static int is_initialised = 0;

void __attribute__ ((constructor)) special_function(void)
{
	is_initialised = 1;
}

int main(int argc, char **argv)
{
	/* To avoid warning on unused parameters, they could cause a false failure */
	(void) argc;
	(void) argv;
	return (is_initialised)?0:1;
}
]]) ],
        [wm_cv_library_constructors=attribute],
        [wm_cv_library_constructors=legacy])
     CFLAGS="$save_CFLAGS"])
 dnl In a perfect world we should also make sure that the feature works, but that implies
 dnl to be able to actually build and run a program, which is not compatible with a
 dnl cross-compiling user setup
 AS_IF([test "X$wm_cv_library_constructors" = "Xattribute"],
    [AC_DEFINE([WLIB_CONSTRUCTOR(func_name)], [__attribute__ ((constructor)) func_name],
        [Method for defining a library initialisation function])],
    [AC_DEFINE([WLIB_CONSTRUCTOR(func_name)], [_init],
        [Method for defining a library initialisation function])] )
])
