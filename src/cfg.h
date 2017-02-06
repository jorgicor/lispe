#ifndef CFG_H
#define CFG_H

#ifndef CONFIG_H
#define CONFIG_H
#include "config.h"
#endif

#define REAL double

#if REAL==double

#define r_modf modf
#define r_strtod strtod
#define REAL_MAX_INT 9007199254740991

#elif REAL==float

#define r_modf modff
#define r_strtod strtof
#define REAL_MAX_INT 16777215

#endif

enum { NCELL = 3000 };

#endif
