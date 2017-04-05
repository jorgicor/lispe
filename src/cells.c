/*
Copyright (c) 2017 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
	printf("[cells: %d, %u bytes, 1 cell: %u bytes]\n",
		NCELL, NCELL * sizeof(s_cells[0]), sizeof(s_cells[0]));
}

