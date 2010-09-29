

#ifndef WINGS_CONFIG_H_
#define WINGS_CONFIG_H_

#include "../config.h"

#if defined(HAVE_LIBINTL_H) && defined(I18N)
# include <libintl.h>
# define _(text) dgettext("WINGs", text)
#else
# define _(text) (text)
#endif

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define INLINE inline
#else
# define INLINE
#endif


#endif /* WINGS_CONFIG_H_ */

