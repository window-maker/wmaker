/* src/config.h.  Generated automatically by configure.  */
/* src/config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if `sys_siglist' is declared by <signal.h>.  */
#define SYS_SIGLIST_DECLARED 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if the X Window System is missing or not being used.  */
/* #undef X_DISPLAY_MISSING */

/* define to the path to cpp */
#define CPP_PATH "/usr/bin/cpp"

/* define if you want GNOME stuff support */
/* #undef GNOME_STUFF */

/* define if you want KDE hint support */
/* #undef KWM_HINTS */

/* define if you want OPEN LOOK(tm) hint support */
/* #undef OLWM_HINTS */

/* define if XPM libraries are available
 * set by configure */
#define USE_XPM 1

/* define if PNG libraries are available
 * set by configure */
#define USE_PNG 1

/* define if JPEG libraries are available
 * set by configure */
#define USE_JPEG 1

/* define if GIF libraries are available
 * set by configure */
#define USE_GIF 1

/* define if TIFF libraries are available
 * set by configure */
#define USE_TIFF 1

/* define if X's shared memory extension is available
 * set by configure */
#define XSHM 1

/* define an extra path for pixmaps
 * set by configure */
#define PIXMAPDIR "/usr/local/share/pixmaps"

/*
 * define REDUCE_APPICONS if you want apps with the same WM_INSTANCE &&
 * WM_CLASS to share an appicon
 */
/* #undef REDUCE_APPICONS */

/* Internationalization (I18N) support 
 * set by configure */
/* #undef I18N */

/* Multi-byte (japanese, korean, chinese etc.) character support */
/* #undef I18N_MB */

/* define if you want sound support */
#define WMSOUND 1

/* define if you want the 'lite' version */
/* #undef LITE */

/* define if you want support for shaped windows
 * set by configure */
#define SHAPE 1

/* define if you want support for X window's X_LOCALE
 * set by configure */
#define X_LOCALE 1

/* the place where shared data is stored
 * defined by configure */
#define PKGDATADIR "/usr/local/share/WindowMaker"

/* the place where the configuration is stored
 * defined by configure */
#define SYSCONFDIR "/usr/local/etc/WindowMaker"

/* Define if you have the atexit function.  */
#define HAVE_ATEXIT 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the poll function.  */
#define HAVE_POLL 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setpgid function.  */
#define HAVE_SETPGID 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strncasecmp function.  */
#define HAVE_STRNCASECMP 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <libintl.h> header file.  */
#define HAVE_LIBINTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <poll.h> header file.  */
/* #undef HAVE_POLL_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/select.h> header file.  */
/* #undef HAVE_SYS_SELECT_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Name of package */
#define PACKAGE "WindowMaker"

/* Version number of package */
#define VERSION "0.51.2"

