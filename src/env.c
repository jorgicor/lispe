#include "cfg.h"
#include "sexpr.h"
#include "common.h"

#include <stdio.h>

SEXPR make_environment(SEXPR parent)
{
	return p_cons(parent, SEXPR_NIL);
}

SEXPR lookup_variable(SEXPR var, SEXPR env)
{
	SEXPR link, bind;

	while (!p_null(env)) {
		link = p_cdr(env);
		while (!p_null(link)) {
			bind = p_car(link);
			if (p_eq(var, p_car(bind))) {
				return bind;
			}
			link = p_cdr(link);
		}
		env = p_car(env);
	}

	return SEXPR_NIL;
}

SEXPR define_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind, link;

	bind = lookup_variable(var, env);
	if (p_null(bind)) {
		push3(var, val, env);
		bind = p_cons(var, val);
		link = p_cons(bind, p_cdr(env));
		popn(3);
		p_setcdr(env, link);
	} else {
		p_setcdr(bind, val);
	}

	return val;
}
		
void extend_environment(SEXPR env, SEXPR params, SEXPR args)
{
	if (p_null(params))
		return;

	/* Handle the case of only one parameter called &rest */
	if (p_null(p_cdr(params)) && p_eq(p_car(params), s_rest_atom)) {
		push(params);
		define_variable(p_car(params), args, env);
		pop();
		return;
	}

	push3(params, args, env);
	define_variable(p_car(params), p_car(args), env);
	params = p_cdr(params);
	args = p_cdr(args);
	while (!p_null(params)) {
		if (p_null(p_cdr(params)) && p_eq(p_car(params), s_rest_atom))
	       	{
			define_variable(p_car(params), args, env);
			break;
		}

		define_variable(p_car(params), p_car(args), env);
		args = p_cdr(args);
		params = p_cdr(params);
	}
	popn(3);
}
