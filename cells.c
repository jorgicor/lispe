#include "config.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>

struct cell s_cells[NCELL];

#ifdef RANGE_CHECK
static void check_celli(int celli)
{
	assert(celli >= 0 && celli < NCELL);
}

SEXPR cell_car(int celli)
{
	check_celli(celli);
	return s_cells[celli].car;
}

SEXPR cell_cdr(int celli)
{
	check_celli(celli);
	return s_cells[celli].cdr;
}

void set_cell_car(int celli, SEXPR care)
{
	check_celli(celli);
	s_cells[celli].car = care;
}

void set_cell_cdr(int celli, SEXPR cdre)
{
	check_celli(celli);
	s_cells[celli].cdr = cdre;
}
#endif

void cells_init(void)
{
	printf("[ncells: %d, cell bytes: %u, cells array bytes: %u]\n",
		NCELL, sizeof(s_cells[0]), NCELL * sizeof(s_cells[0]));
}

