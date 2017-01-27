#include "config.h"
#include "common.h"

int p_null(SEXPR e)
{
	return sexpr_eq(s_nil_atom, e);
}

int p_symbolp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_LITERAL;
}

int p_numberp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_NUMBER;
}

int p_atom(SEXPR x)
{
	return p_numberp(x) || p_symbolp(x);
}

int p_consp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_CONS;
}

SEXPR p_car(SEXPR e)
{
	if (!p_consp(e))
		throw_err();

	return cell_car(sexpr_index(e));
}

SEXPR p_cdr(SEXPR e)
{
	if (!p_consp(e))
		throw_err();

	return cell_cdr(sexpr_index(e));
}

SEXPR p_add(SEXPR var, SEXPR val, SEXPR a)
{
	return p_cons(p_cons(var, val), a);
}
