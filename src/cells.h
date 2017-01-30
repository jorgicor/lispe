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
