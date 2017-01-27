#include "config.h"
#include "cellmark.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>

enum { NCELLMARK = (NCELL / 16) + ((NCELL % 16) ? 1 : 0) };

/* We use 2 bits for each cell, with the values:
 * CELL_FREE, CELL_CREATED and CELL_USED.
 */
static unsigned int s_cellmarks[NCELLMARK];

#ifdef RANGE_CHECK
#define check_celli(i) assert(i >= 0 && i < NCELL)
#define check_marki(i) assert(i >= 0 && i < NCELLMARK)
#else
#define check_celli(i)
#define check_marki(i)
#endif

/* 'mark must be CELL_FREE, CELL_CREATED or CELL_USED. */
void mark_cell(int celli, int mark)
{
	int w;

	check_celli(celli);
	w = celli >> 4;
	check_marki(w);
	celli = (celli & 15) << 1;
	s_cellmarks[w] = (s_cellmarks[w] & ~(3 << celli)) | (mark << celli);
}

/* Returns CELL_FREE, CELL_CREATED or CELL_USED. */
int cell_mark(int celli)
{
	int w;

	check_celli(celli);
	w = celli >> 4;
	check_marki(w);
	celli = (celli & 15) << 1;
	return (s_cellmarks[w] & (3 << celli)) >> celli;
}

int if_cell_mark(int celli, int ifmark, int thenmark)
{
	int w;
	unsigned int mask;

	check_celli(celli);
	w = celli >> 4;
	check_marki(w);
	celli = (celli & 15) << 1;
	mask = 3 << celli;
	if ((s_cellmarks[w] & mask) == (ifmark << celli)) {
		s_cellmarks[w] = (s_cellmarks[w] & ~mask) |
		       	         (thenmark << celli);
		return 1;
	}

	return 0;
}

void cellmark_init(void)
{
	printf("[ncellmarks: %d, cellmarks array bytes: %u]\n",
		NCELLMARK, NCELLMARK * sizeof(s_cellmarks[0]));
}
