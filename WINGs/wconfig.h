

#ifndef WINGS_CONFIG_H_
#define WINGS_CONFIG_H_


#include "../src/config.h"

#if defined(HAVE_LIBINTL_H) && defined(I18N)
# include <libintl.h>
# define _(text) dgettext("WINGs", text)
#else
# define _(text) (text)
#endif


#endif /* WINGS_CONFIG_H_ */

