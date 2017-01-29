#include "config.h"
#include "cellmark.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>

enum { NCELLMARK = (NCELL / 32) + ((NCELL % 32) ? 1 : 0) };

static unsigned int s_cellmarks[NCELLMARK];

#ifdef RANGE_CHECK
#define check_celli(i) assert(i >= 0 && i < NCELL)
#define check_marki(i) assert(i >= 0 && i < NCELLMARK)
#else
#define check_celli(i)
#define check_marki(i)
#endif

void mark_cell(int celli)
{
	int w;

	check_celli(celli);
	w = celli >> 5;
	check_marki(w);
	celli &= 31;
	s_cellmarks[w] |= (1 << celli);
}

int cell_mark(int celli)
{
	int w;

	check_celli(celli);
	w = celli >> 5;
	check_marki(w);
	celli &= 31;
	return s_cellmarks[w] & (1 << celli);
}

int if_cell_mark(int celli)
{
	int w;
	unsigned int mask;

	check_celli(celli);
	w = celli >> 5;
	check_marki(w);
	celli &= 31;
	mask = 1 << celli;
	if (!(s_cellmarks[w] & mask)) {
		s_cellmarks[w] |= mask;
		return 1;
	}

	return 0;
}

int if_cell_unmark(int celli)
{
	int w;
	unsigned int mask;

	check_celli(celli);
	w = celli >> 5;
	check_marki(w);
	celli &= 31;
	mask = 1 << celli;
	if (s_cellmarks[w] & mask) {
		s_cellmarks[w] &= ~mask;
		return 1;
	}

	return 0;
}

void cellmark_init(void)
{
	printf("[ncellmarks: %d, cellmarks array bytes: %u]\n",
		NCELLMARK, NCELLMARK * sizeof(s_cellmarks[0]));
}
