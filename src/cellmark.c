/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "cfg.h"
#include "cbase.h"
#include "cellmark.h"
#include <assert.h>
#include <stdio.h>

enum { NCELLMARK = (NCELL / 32) + ((NCELL % 32) ? 1 : 0) };

/* We use 1 bit for each cell. */
static unsigned int s_cellmarks[NCELLMARK];

static void calc_w_i(int celli, int *w, int *i)
{
	int wt;

	chkrange(celli, NCELL);
	wt = celli >> 5;
	chkrange(wt, NCELLMARK);
	celli &= 31;
	*w = wt;
	*i = celli;
}

void mark_cell(int celli)
{
	int w;

	calc_w_i(celli, &w, &celli);
	s_cellmarks[w] |= (1 << celli);
}

int cell_marked(int celli)
{
	int w;

	calc_w_i(celli, &w, &celli);
	return s_cellmarks[w] & (1 << celli);
}

/* If a cell is unmarked, mark it and return 1; else 0. */
int if_cell_mark(int celli)
{
	int w;
	unsigned int mask;

	calc_w_i(celli, &w, &celli);
	mask = 1 << celli;
	if (!(s_cellmarks[w] & mask)) {
		s_cellmarks[w] |= mask;
		return 1;
	}

	return 0;
}

/* If a cell is marked, unmark it and return 1; else 0. */
int if_cell_unmark(int celli)
{
	int w;
	unsigned int mask;

	calc_w_i(celli, &w, &celli);
	mask = 1 << celli;
	if (s_cellmarks[w] & mask) {
		s_cellmarks[w] &= ~mask;
		return 1;
	}

	return 0;
}

void cellmark_init(void)
{
	printf("[cells: marks: %d, %zu bytes]\n",
		NCELLMARK, NCELLMARK * sizeof(s_cellmarks[0]));
}
