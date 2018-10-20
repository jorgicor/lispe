/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#ifndef CFG_H
#define CFG_H

#ifndef CONFIG_H
#define CONFIG_H
#include "config.h"
#endif

#ifndef COMPLEX_H
#define COMPLEX_H
#include <complex.h>
#endif

#define USE_DOUBLE

#ifdef USE_DOUBLE

typedef double real_t;
typedef double complex complex_t;

#define r_modf modf
#define r_strtod strtod
#define REAL_MAX_INT 9007199254740991

#else

typedef float real_t;
typedef float complex complex_t;

#define r_modf modff
#define r_strtod strtof
#define REAL_MAX_INT 16777215

#endif

enum { NCELL = 5000 };

#endif
