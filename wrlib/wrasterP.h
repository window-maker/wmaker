

#ifndef WRASTERP_H_
#define WRASTERP_H_

#include <config.h>


#include "wraster.h"


#ifdef HAVE_HERMES

# include <Hermes/Hermes.h>

typedef struct RHermesData {
    HermesHandle palette;
    HermesHandle converter;
} RHermesData;

#endif


#endif
