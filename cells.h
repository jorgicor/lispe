#ifndef CELLS_H
#define CELLS_H

#ifndef CONFIG_H
#include "config.h"
#endif

#ifndef SEXPR_H
#include "sexpr.h"
#endif

#ifndef STDDEF_H
#define STDDEF_H
#include <stddef.h>
#endif

enum {
	NCELL = 3000,
};

/************************************************************/
/* private, don't use directly, only exposed for efficiency */
/************************************************************/

struct cell {
	SEXPR car;
	SEXPR cdr;
};

extern struct cell s_cells[];

/************************************************************/
/* end of private section                                   */
/************************************************************/

#ifdef RANGE_CHECK

size_t cell_size(void);
SEXPR cell_car(int celli);
SEXPR cell_cdr(int celli);
void set_cell_car(int celli, SEXPR care);
void set_cell_cdr(int celli, SEXPR cdre);

#else

#define cell_size() sizeof(struct cell)
#define cell_car(i) s_cells[i].car
#define cell_cdr(i) s_cells[i].cdr
#define set_cell_car(i, e) s_cells[i].car = e
#define set_cell_cdr(i, e) s_cells[i].cdr = e

#endif

#endif
