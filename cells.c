#include "cells.h"

#ifndef ASSERT_H
#include <assert.h>
#endif

struct cell s_cells[NCELL];

#ifdef RANGE_CHECK
size_t cell_size(void)
{
	return sizeof(struct cell);
}

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
