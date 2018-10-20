/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "cfg.h"
#include "cbase.h"
#include "cells.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "common.h"
#include <assert.h>
#include <stdio.h>

struct cell s_cells[NCELL];

#ifdef PP_RANGECHECKS
SEXPR cell_car(int celli)
{
	chkrange(celli, NCELL);
	return s_cells[celli].car;
}

SEXPR cell_cdr(int celli)
{
	chkrange(celli, NCELL);
	return s_cells[celli].cdr;
}

void set_cell_car(int celli, SEXPR care)
{
	chkrange(celli, NCELL);
	s_cells[celli].car = care;
}

void set_cell_cdr(int celli, SEXPR cdre)
{
	chkrange(celli, NCELL);
	s_cells[celli].cdr = cdre;
}
#endif

void cells_init(void)
{
	printf("[cells: %d, %zu bytes, 1 cell: %zu bytes]\n",
		NCELL, NCELL * sizeof(s_cells[0]), sizeof(s_cells[0]));
}

