#include <wtheme.h>

#define NEW_THEME
#ifdef NEW_THEME
#include <newtheme.c>
#else
#include <oldtheme.c>
#endif
