#include "cfg.h"
#include "cells.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "common.h"
#include <assert.h>
#include <stdio.h>

struct cell s_cells[NCELL];

#ifdef PP_RANGECHECKS
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
	printf("[cells: %d, %u bytes, 1 cell: %u bytes]\n",
		NCELL, NCELL * sizeof(s_cells[0]), sizeof(s_cells[0]));
}
