#ifndef COMMON_H
#define COMMON_H

#ifndef CONFIG_H
#include "config.h"
#endif

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

/* sexpr.c */

/************************************************************/
/* private, don't use directly, only exposed for efficiency */
/************************************************************/

/*
 * We look at the 3 left bits of type.
 * This gives SEXPR_CONS, SEXPR_LITERAL, etc.
 * For:
 * SEXPR_CONS: bits(28..0) is index into cells.
 * SEXPR_BUILTIN_FUNCTION: bits(28..0) is index into table of builtin
 *                         functions.
 * SEXPR_BUILTIN_SPECIAL: bits(28..0) is index into table of builtin
 *                             special forms.
 * SEXPR_FUNCTION and
 * SEXPR_SPECIAL and
 * SEXPR_CLOSURE: bits(28..0) is index into cells.
 * SEXPR_NUMBER: bits(28..0) is index to a cell whose car is the floating
 *               number (the cdr we don't care).
 * SEXPR_LITERAL: bits(28..0) is index to a cell whose car is the pointer
 *                to the struct literal (the cdr we don't care).
 */

union sexpr {
	int type;
	float number;
	const char *name;
};

/************************************************************/
/* end of private section                                   */
/************************************************************/

typedef union sexpr SEXPR;

enum {
	SEXPR_CONS = 0 << 29,
	SEXPR_LITERAL = 1 << 29,
	SEXPR_NUMBER = 2 << 29,
	SEXPR_BUILTIN_FUNCTION = 3 << 29,
	SEXPR_BUILTIN_SPECIAL = 4 << 29,
	SEXPR_FUNCTION = 5 << 29,
	SEXPR_SPECIAL = 6 << 29,
	SEXPR_CLOSURE = 7 << 29,
};

int sexpr_eq(SEXPR e1, SEXPR e2);
int sexpr_equal(SEXPR e1, SEXPR e2);
int sexpr_type(SEXPR e);
int sexpr_index(SEXPR e);
float sexpr_number(SEXPR e);
const char* sexpr_name(SEXPR e);

SEXPR make_cons(int celli);
SEXPR make_literal(const char *s, int len);
SEXPR make_number(float n);
SEXPR make_builtin_function(int table_index);
SEXPR make_builtin_special(int table_index);
SEXPR make_function(int args_n_body_celli);
SEXPR make_special(int args_n_body_celli);
SEXPR make_closure(int lambda_n_alist_celli);

SEXPR make_literal_in_cell(const char *s, int len, int celli);

void gc_literals(void);


/* cells.c */

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

/* cellmark.h */

enum {
	CELL_FREE,
	CELL_CREATED,
	CELL_USED
};

void mark_cell(int celli, int mark);
int cell_mark(int celli);
int if_cell_mark(int celli, int ifmark, int thenmark);
void cellmark_init(void);

/* pred.c */

int p_null(SEXPR e);
int p_atom(SEXPR x);
int p_consp(SEXPR e);
int p_symbolp(SEXPR e);
int p_numberp(SEXPR e);
SEXPR p_car(SEXPR e);
SEXPR p_cdr(SEXPR e);

/* gcbase.c */

extern SEXPR s_nil_atom;
extern SEXPR s_env;
extern SEXPR s_hidenv;

int pop_free_cell(void);
SEXPR p_cons(SEXPR first, SEXPR rest);

void clear_stack(void);
SEXPR push(SEXPR e);
void push2(SEXPR e1, SEXPR e2);
void push3(SEXPR e1, SEXPR e2, SEXPR e3);
void pop(void);
void popn(int n);

void p_gc(void);

void gcbase_init(void);

/* lispe.c */

void throw_err(void);

#endif
