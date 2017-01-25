#include "common.h"
#include <stddef.h>
#include <limits.h>

static int spexpr_is_pointer_to_cons_cell(SEXPR e)
{
	return e.index < 0;
}

int sexpr_type(SEXPR e)
{
	if (spexpr_is_pointer_to_cons_cell(e)) {
		return SEXPR_CONS;
	} else {
		cellp(sexpr_index(e), pcell);
		return pcell->car.type;
	}
}

int sexpr_index(SEXPR e)
{
	return e.index & ~INT_MIN;
}

LITERAL sexpr_literal(SEXPR e)
{
	struct cell *pcell;

	cellp(sexpr_index(e), pcell);
	assert(pcell->car.type == SEXPR_LITERAL);
	return pcell->cdr.literal;
}

float sexpr_number(SEXPR e)
{
	struct cell *pcell;

	cellp(sexpr_index(e), pcell);
	assert(pcell->car.type == SEXPR_NUMBER);
	return pcell->cdr.number;
}

SEXPR make_cons(int i)
{
	SEXPR e;

	e.pointer = INT_MIN & i;
	return e;
}

static SEXPR make_non_cons(int i)
{
	SEXPR e;

	e.pointer = i;
	return e;
}

SEXPR make_literal(LITERAL lit)
{
	int celli;

	// TODO: protect_literal(lit)
	celli = pop_free_cell();
	// TODO: unprotect_literal(lit)
	cells[celli].car.type = SEXPR_LITERAL;
	cells[celli].cdr.literal = lit;
	return make_non_cons(celli);
}

SEXPR make_number(float n)
{
	int celli;

	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_NUMBER;
	cells[celli].cdr.number = n;
	return make_non_cons(celli);
}

SEXPR make_builtin_function(int i)
{
	int celli;

	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_BUILTIN_FUNCTION;
	cells[celli].cdr.index = i;
	return make_non_cons(celli);
}

SEXPR make_builtin_special(int i)
{
	int celli;

	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_BUILTIN_SPECIAL;
	cells[celli].cdr.index = i;
	return make_non_cons(celli);
}

SEXPR make_closure(SEXPR args_n_body)
{
	int celli;

	s_cons_cdr = args_n_body;
	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_CLOSURE;
	cells[celli].cdr = args_n_body;
	s_cons_cdr = s_nil_atom;
	return make_non_cons(celli);
}

SEXPR make_function(SEXPR args_n_body)
{
	int celli;

	s_cons_cdr = args_n_body;
	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_FUNCTION;
	cells[celli].cdr = args_n_body;
	s_cons_cdr = s_nil_atom;
	return make_non_cons(celli);
}

SEXPR make_special(SEXPR args_n_body)
{
	int celli;

	s_cons_cdr = args_n_body;
	celli = pop_free_cell();
	cells[celli].car.type = SEXPR_SPECIAL;
	cells[celli].cdr = args_n_body;
	s_cons_cdr = s_nil_atom;
	return make_non_cons(celli);
}
