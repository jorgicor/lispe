#include "common.h"
#include <stddef.h>

int sexpr_type(SEXPR e)
{
	return e.type;
}

int sexpr_index(SEXPR e)
{
	return e.data.index;
}

LITERAL sexpr_literal(SEXPR e)
{
	return e.data.literal;
}

float sexpr_number(SEXPR e)
{
	return e.data.number;
}

SEXPR make_cons(int i)
{
	SEXPR e;

	e.type = SEXPR_CONS;
	e.data.index = i;
	return e;
}

SEXPR make_literal(LITERAL lit)
{
	SEXPR e;

	e.type = SEXPR_LITERAL;
	e.data.literal = lit;
	return e;
}

SEXPR make_number(float n)
{
	SEXPR e;

	e.type = SEXPR_NUMBER;
	e.data.number = n;
	return e;
}

SEXPR make_builtin_function(int i)
{
	SEXPR e;

	e.type = SEXPR_BUILTIN_FUNCTION;
	e.data.index = i;
	return e;
}

SEXPR make_builtin_special(int i)
{
	SEXPR e;

	e.type = SEXPR_BUILTIN_SPECIAL;
	e.data.index = i;
	return e;
}

SEXPR make_closure(int args_n_body)
{
	SEXPR e;

	e.type = SEXPR_CLOSURE;
	e.data.index = args_n_body;
	return e;
}

SEXPR make_function(int args_n_body)
{
	SEXPR e;

	e.type = SEXPR_FUNCTION;
	e.data.index = args_n_body;
	return e;
}

SEXPR make_special(int args_n_body)
{
	SEXPR e;

	e.type = SEXPR_SPECIAL;
	e.data.index = args_n_body;
	return e;
}
