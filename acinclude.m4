#!/bin/sh

dnl
dnl WM_CHECK_LIB(NAME, FUNCTION, EXTRALIBS)
dnl
AC_DEFUN(WM_CHECK_LIB,
[
LDFLAGS_old="$LDFLAGS"
LDFLAGS="-DBEGIN $LDFLAGS -DEND $lib_search_path"
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
if test -f /etc/redhat-release; then
    wm_check_flag=yes
fi
AC_MSG_RESULT($wm_check_flag)

mins_found=no
bugs_found=no
if test "$wm_check_flag" = yes; then
echo
AC_MSG_WARN([it seems you are using a system packaged by RedHat. 
I will now do some checks for RedHat specific bugs. If some check
fail, please read the INSTALL file regarding RedHat, resolve the
problem and retry to configure.])
echo
#
# Check old wmaker from RedHat
#
if test "[$1]" != "/usr/X11R6" -a "$prefix" != "/usr/X11"; then
AC_MSG_CHECKING(for multiple installed wmaker versions)
if test -f /usr/X11R6/bin/wmaker; then
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([you seem to have an old version of Window Maker 
installed at /usr/X11R6/bin. It is recommended that you uninstall
any previously installed packages of WindowMaker before installing
a new one.])
mins_found=yes
else 
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
old_IFS="$IFS"
IFS=":"
for i in $PATH; do 
	if test "x$i" = "x/usr/local/bin"; then
		wm_check_flag=yes
		break;
	fi
done
IFS="$old_IFS"
if test "$wm_check_flag" = no; then
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([/usr/local/bin is not in the PATH environment variable. 
Please resolve the problem.])
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
AC_MSG_CHECKING(if /usr/local/lib is in /etc/ld.so.conf)
test -z "`grep /usr/local/lib /etc/ld.so.conf`"
test "$?" -eq 0 && wm_check_flag=no
if test "$wm_check_flag" = no; then
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([/usr/local/lib is not in the /etc/ld.so.conf file. 
Please add it there.])
bugs_found=yes
else
AC_MSG_RESULT(no problem)
fi
fi
#
# Check for symbolic links
#
AC_MSG_CHECKING(for /usr/X11 symbolic link)
if test -d "/usr/X11"; then
AC_MSG_RESULT(found)
else
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([Please create a symbolic link from /usr/X11R6 to /usr/X11.])
mins_found=yes
fi
AC_MSG_CHECKING(for /usr/include/X11 symbolic link)
if test -d "/usr/include/X11"; then
AC_MSG_RESULT(found)
else
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([Please create a symbolic link from /usr/X11R6/include/X11
to /usr/include/X11.])
mins_found=yes
fi

#
# Check for /lib/cpp
#
AC_MSG_CHECKING(for /lib/cpp)
if test -f "/lib/cpp"; then
AC_MSG_RESULT(found)
else
AC_MSG_RESULT(uh oh)
AC_MSG_WARN([Please create a symbolic link from the cpp (C preprocessor) 
program to /lib/cpp])
bugs_found=yes
fi

if test "x$bugs_found" = xyes; then
AC_MSG_ERROR([Some bugs that can potentially cause problems during
installation/execution were found. Please correct these problems
and retry later.])
exit 1
elif test "x$mins_found" = xyes; then
AC_MSG_WARN([Some minor problems that might or might not cause 
problems were found. If you have any problems during 
installation/execution, please resolve the pointed problems and try
to reinstall.])
echo "Press <Return> to continue."
read blabla
else
echo
echo "None of the RedHat problems known to this script were found."
echo
fi
fi
])

