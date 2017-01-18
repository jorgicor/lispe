#include "common.h"

int get_sexpr_type(SEXPR e)
{
	return e.type;
}

int get_sexpr_index(SEXPR e)
{
	return e.data.index;
}

LITERAL get_sexpr_literal(SEXPR e)
{
	return e.data.literal;
}

float get_sexpr_number(SEXPR e)
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

SEXPR make_builtin_special_form(int i)
{
	SEXPR e;

	e.type = SEXPR_BUILTIN_SPECIAL_FORM;
	e.data.index = i;
	return e;
}

SEXPR make_procedure(int args_n_body)
{
	SEXPR e;

	e.type = SEXPR_PROCEDURE;
	e.data.index = args_n_body;
	return e;
}
