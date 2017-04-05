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

#ifndef CELLS_H
#define CELLS_H

#ifndef CFG_H
#include "cfg.h"
#endif

#ifndef SEXPR_H
#include "sexpr.h"
#endif

/************************************************************
 * PRIVATE (exposed ony for efficiency)
 * Only cells.c uses this representation directly.
 * The rest of the code use the functions and macros here
 * listed.
 ************************************************************/

struct cell {
	SEXPR car;
	SEXPR cdr;
};

extern struct cell s_cells[];

/************************************************************/
/* end of private section                                   */
/************************************************************/

#ifdef PP_RANGECHECKS

SEXPR cell_car(int celli);
SEXPR cell_cdr(int celli);
void set_cell_car(int celli, SEXPR care);
void set_cell_cdr(int celli, SEXPR cdre);

#else

#define cell_car(i) s_cells[i].car
#define cell_cdr(i) s_cells[i].cdr
#define set_cell_car(i, e) s_cells[i].car = e
#define set_cell_cdr(i, e) s_cells[i].cdr = e

#endif

void cells_init(void);

#endif
