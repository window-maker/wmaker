#!/bin/sh

dnl
dnl WM_CHECK_LIB(NAME, FUNCTION, EXTRALIBS)
dnl
AC_DEFUN(WM_CHECK_LIB,
[
LDFLAGS_old="$LDFLAGS"
LDFLAGS="$LDFLAGS $lib_search_path"
AC_CHECK_LIB([$1],[$2],yes=yes,no=no,[$3])
LDFLAGS="$LDFLAGS_old"
])


dnl
dnl WM_CHECK_HEADER(NAME)
dnl
AC_DEFUN(WM_CHECK_HEADER,
[
CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $inc_search_path"
AC_CHECK_HEADER([$1])
CPPFLAGS="$CPPFLAGS_old"
])


dnl
dnl WM_CHECK_REDCRAP_BUGS(prefix,bindir,libdir)
dnl
AC_DEFUN(WM_CHECK_REDCRAP_BUGS,
[
AC_MSG_CHECKING(for RedHat system)
wm_check_flag='no :)'
rh_is_redhat=no
if test -f /etc/redhat-release; then
    wm_check_flag=yes
    rh_is_redhat=yes
fi
AC_MSG_RESULT($wm_check_flag)

mins_found=no
bugs_found=no
if test "$wm_check_flag" = yes; then
echo
AC_MSG_WARN([Red Hat system; checking for Red-Hat-specific bugs.])
echo
#
# Check old wmaker from RedHat
#
if test "[$1]" != "/usr/X11R6" -a "$prefix" != "/usr/X11"; then
AC_MSG_CHECKING(for multiple installed wmaker versions)
if test -f /usr/X11R6/bin/wmaker; then
AC_MSG_RESULT(uh oh)
mins_found=yes
rh_old_wmaker=yes
else 
rh_old_wmaker=no
AC_MSG_RESULT(no apparent problems)
fi
fi
#
# Check for infamous en_RN bug 
# Wont work because autoconf will change LANG in the beginning of the
# script.

#
#AC_MSG_CHECKING(for silly en_RN joke that only causes headaches)
#echo $LANG
#if test "x$LANG" = xen_RN; then
#AC_MSG_RESULT(uh oh)
#AC_MSG_WARN([the LANG environment variable is set to the en_RN 
#locale. Please unset it or you will have mysterious problems when 
#using various software packages.])
#bugs_found=yes
#else
#AC_MSG_RESULT(no problem)
#fi
#
# If binary installation path is /usr/local/bin, check if it's in PATH
#
if test "[$2]" = "/usr/local/bin"; then
AC_MSG_CHECKING(if /usr/local/bin is in the search PATH)
wm_check_flag=no
rh_missing_usr_local_bin=yes
old_IFS="$IFS"
IFS=":"
for i in $PATH; do 
	if test "x$i" = "x/usr/local/bin"; then
		wm_check_flag=yes
		rh_missing_usr_local_bin=no
		break;
	fi
done
IFS="$old_IFS"
if test "$wm_check_flag" = no; then
AC_MSG_RESULT(uh oh)
bugs_found=yes
else
AC_MSG_RESULT(no problem)
fi
fi
#
# If library installation path is /usr/local/lib, 
# check if it's in /etc/ld.so.conf
#
if test "[$3]" = "/usr/local/lib"; then
wm_check_flag=yes
rh_missing_usr_local_lib=no
AC_MSG_CHECKING(if /usr/local/lib is in /etc/ld.so.conf)
test -z "`grep /usr/local/lib /etc/ld.so.conf`"
test "$?" -eq 0 && wm_check_flag=no
if test "$wm_check_flag" = no; then
AC_MSG_RESULT(uh oh)
rh_missing_usr_local_lib=yes
bugs_found=yes
else
AC_MSG_RESULT(no problem)
fi
fi
#
# Check for symbolic links
#
AC_MSG_CHECKING(for /usr/include/X11 symbolic link)
rh_missing_usr_include_x11=no
if test -d "/usr/include/X11"; then
AC_MSG_RESULT(found)
else
AC_MSG_RESULT(uh oh)
rh_missing_usr_include_x11=yes
mins_found=yes
fi

#
# Check for /lib/cpp
#
AC_MSG_CHECKING(for /lib/cpp)
rh_missing_lib_cpp=no
if test -f "/lib/cpp"; then
AC_MSG_RESULT(found)
else
AC_MSG_RESULT(uh oh)
rh_missing_lib_cpp=yes
bugs_found=yes
fi

echo
fi
])


dnl
dnl WM_PRINT_REDCRAP_BUG_STATUS()
dnl
AC_DEFUN(WM_PRINT_REDCRAP_BUG_STATUS,
[
if test "$rh_is_redhat" = yes; then
if test "$mins_found" = yes -o "$bugs_found" = yes; then
echo
AC_MSG_WARN([It seems you are using a system packaged by Red Hat.
I have done some checks for Red-Hat-specific bugs, and I found some
problems.  Please read the INSTALL file regarding Red Hat, resolve
the problems, and try to run configure again.

Here are the problems I found:
])
if test "x$rh_old_wmaker" = xyes; then
echo "Problem:     Old version of Window Maker in /usr/X11R6/bin."
echo "Description: You seem to have an old version of Window Maker"
echo "             installed in /usr/X11R6/bin. It is recommended"
echo "             that you uninstall any previously installed"
echo "             packages of WindowMaker before installing a new one."
echo
fi
if test "x$rh_missing_usr_local_bin" = xyes; then
echo "Problem:     PATH is missing /usr/local/bin."
echo "Description: Your PATH environment variable does not appear to"
echo "             contain the directory /usr/local/bin.  Please add it."
echo
fi
if test "x$rh_missing_usr_local_lib" = xyes; then
echo "Problem:     /etc/ld.so.conf missing /usr/local/lib"
echo "Description: Your /etc/ld.so.conf file does not appear to contain"
echo "             the directory /usr/local/lib.  Please add it."
echo
fi
if test "x$rh_missing_usr_x11" = xyes; then
echo "Problem:     Missing /usr/X11 symbolic link."
echo "Description: Your system is missing a symbolic link from"
echo "             /usr/X11R6 to /usr/X11.  Please create one."
echo
fi
if test "x$rh_missing_usr_include_x11" = xyes; then
echo "Problem:     Missing /usr/include/X11 symbolic link."
echo "Description: Your system is missing a symbolic link from"
echo "             /usr/X11R6/include/X11 to /usr/include/X11."
echo "             Please create one."
echo
fi
if test "x$rh_missing_lib_cpp" = xyes; then
echo "Problem:     Missing /lib/cpp symbolic link."
echo "Description: Your system is missing a symbolic link from the"
echo "             cpp (C preprocessor) program to /lib/cpp."
echo "             Please create one."
echo
fi
if test "x$bugs_found" = xyes; then
AC_MSG_ERROR([One or more of the problems above can potentially
cause Window Maker not to install or run properly.  Please resolve
the problems and try to run configure again.])
exit 1
elif test "x$mins_found" = xyes; then
AC_MSG_WARN([The problems above may or may not cause Window Maker
not to install or run properly.  If you have any problems during 
installation or execution, please resolve the problems and try to
install Window Maker again.])
echo
fi
else
echo
echo "You appear to have a system packaged by Red Hat, but I could"
echo "not find any Red-Hat-specific problems that I know about."
echo
fi
fi
])

