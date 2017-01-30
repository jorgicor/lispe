#ifndef CBASE_H
#define CBASE_H

#ifndef CFG_H
#define CFG_H
#include "cfg.h"
#endif

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

#ifdef PP_RANGECHECKS

#define chkrange(i, n) \
	do { \
		int k; \
		k = i; \
		assert(k >= 0 && k < n); \
	} while(0);

#else

#define chkrange(i, n)

#endif

#endif
