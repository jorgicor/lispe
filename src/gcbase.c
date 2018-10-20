/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "cfg.h"
#include "gc.h"
#include "sexpr.h"
#include "symbols.h"
#include "numbers.h"
#include "common.h"
#include "cells.h"
#include "cellmark.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* List of free cells.  */
static SEXPR s_free_cells;

/* global environment */
SEXPR s_topenv;
SEXPR s_env;
SEXPR s_expr;
SEXPR s_val;
SEXPR s_proc;
SEXPR s_args;
SEXPR s_unev;

/* hidden environment, used to not gc quote, etc. */
static SEXPR s_hidenv;

/* current computation stack */
static SEXPR s_stack;

/* To protect from gc */
static SEXPR s_cons_car;
static SEXPR s_cons_cdr;
static SEXPR s_protect_a;
static SEXPR s_protect_b;
static SEXPR s_protect_c;

/* Other precreated atoms */
SEXPR s_quote_atom;

/* Makes an sexpr form two sexprs. */
SEXPR p_cons(SEXPR first, SEXPR rest)
{
	int i;
	
	/* protect */
	s_cons_car = first;
	s_cons_cdr = rest;

	i = pop_free_cell();
	set_cell_car(i, first);
	set_cell_cdr(i, rest);

	s_cons_car = SEXPR_NIL;
	s_cons_cdr = SEXPR_NIL;

	return make_cons(i);
}

int pop_free_cell(void)
{
	int celli;

	if (p_nullp(s_free_cells)) {
		printf("[gc: need cells]\n");
		p_gc();
		if (p_nullp(s_free_cells)) {
			fprintf(stderr, "lispe: out of cells\n");
			exit(EXIT_FAILURE);
		}
	}

	celli = sexpr_index(s_free_cells);
	s_free_cells = p_cdr(s_free_cells);
	return celli;
}

/*********************************************************
 * push and pop to stack to protect from gc.
 *********************************************************/

int stack_empty(void)
{
	return p_eqp(s_stack, SEXPR_NIL);
}

void clear_stack(void)
{
	s_stack = SEXPR_NIL;
	s_cons_car = SEXPR_NIL;
	s_cons_cdr = SEXPR_NIL;
	s_protect_a = SEXPR_NIL;
	s_protect_b = SEXPR_NIL;
	s_protect_c = SEXPR_NIL;
	s_env = SEXPR_NIL;
	s_expr = SEXPR_NIL;
	s_val = SEXPR_NIL;
	s_args = SEXPR_NIL;
	s_unev = SEXPR_NIL;
	s_proc = SEXPR_NIL;
}

/* Protect expression form gc by pushin it to s_stack. Return e. */
SEXPR push(SEXPR e)
{
	s_stack = p_cons(e, s_stack);
	return e;
}

void push2(SEXPR e1, SEXPR e2)
{
	s_protect_a = e1;
	s_protect_b = e2;
	push(e1);
	push(e2);
	s_protect_a = SEXPR_NIL;
	s_protect_b = SEXPR_NIL;
}

void push3(SEXPR e1, SEXPR e2, SEXPR e3)
{
	s_protect_a = e1;
	s_protect_b = e2;
	s_protect_c = e3;
	push(e1);
	push(e2);
	push(e3);
	s_protect_a = SEXPR_NIL;
	s_protect_b = SEXPR_NIL;
	s_protect_c = SEXPR_NIL;
}

/* Pop last expression from stack. */
SEXPR pop(void)
{
	SEXPR e;

	assert(!stack_empty());
	e = p_car(s_stack);
	s_stack = p_cdr(s_stack);
	return e;
}

void popn(int n)
{
	assert(n >= 0);
	while (n--) {
		pop();
	}
}

/* Marks an expression and subexpressions. */
static void gc_mark(SEXPR e)
{
	int celli;

	switch (sexpr_type(e)) {
	case SEXPR_NUMBER:
		mark_number(sexpr_index(e));
		break;
	case SEXPR_SYMBOL:
		mark_symbol(sexpr_index(e));
		break;
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
	case SEXPR_DYN_FUNCTION:
	case SEXPR_CONS:
		celli = sexpr_index(e);
		if (if_cell_mark(celli)) {
			gc_mark(cell_car(celli));
			gc_mark(cell_cdr(celli));
		}
		break;
	}
}

/* Collect garbage */
void p_gc(void)
{
	int i, used;

	/* Mark used. */
	gc_mark(s_topenv);
	gc_mark(s_env);
	gc_mark(s_expr);
	gc_mark(s_val);
	gc_mark(s_proc);
	gc_mark(s_args);
	gc_mark(s_unev);
	gc_mark(s_stack);
	gc_mark(s_hidenv);
	gc_mark(s_cons_car);
	gc_mark(s_cons_cdr);
	gc_mark(s_protect_a);
	gc_mark(s_protect_b);
	gc_mark(s_protect_c);

	gc_symbols();
	gc_numbers();

	used = 0;
	s_free_cells = SEXPR_NIL;
	for (i = 0; i < NCELL; i++) {
		if (if_cell_unmark(i)) {
			used++;
		} else {
			set_cell_cdr(i, s_free_cells);
			s_free_cells = make_cons(i);
		}
	}

	printf("[gc: %d/%d cells]\n", used, NCELL);

#if 0
	i = 0;
	e = s_free_cells;
	while (!p_null(e)) {
		i++;
		e = p_cdr(e);
	}

	printf("on free list %d\n", i);
#endif
}

static void install_symbols(void)
{
	s_quote_atom = make_symbol("quote", 5);
	s_expr = s_val = s_quote_atom;
	s_env = s_hidenv;
	define_variable();
}

/* Init this module, in particular the SEXPR_NIL atom and the free list of cells.
 * Must be done first, so that pop_free_cell() works.
 * Inits the environments and the internal stack.
 */
void gcbase_init(void)
{
	int i;

	/* link cells for the free cells list */
	set_cell_car(NCELL - 1, SEXPR_NIL);
	set_cell_cdr(NCELL - 1, SEXPR_NIL);
	for (i = 0; i < NCELL - 1; i++) {
		set_cell_car(i, SEXPR_NIL);
		set_cell_cdr(i, make_cons(i + 1));
	}
	s_free_cells = make_cons(0);

	s_topenv = make_environment(SEXPR_NIL);
	s_hidenv = make_environment(SEXPR_NIL);
	s_env = SEXPR_NIL;
	s_expr = SEXPR_NIL;
	s_val = SEXPR_NIL;
	s_proc = SEXPR_NIL;
	s_args = SEXPR_NIL;
	s_unev = SEXPR_NIL;
	s_stack = SEXPR_NIL;
	s_cons_car = SEXPR_NIL;
	s_cons_cdr = SEXPR_NIL;
	s_protect_a = SEXPR_NIL;
	s_protect_b = SEXPR_NIL;
	s_protect_c = SEXPR_NIL;

	install_symbols();
}

