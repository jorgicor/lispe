#include "cfg.h"
#include "sexpr.h"
#include "common.h"
#include <assert.h>

SEXPR make_environment(SEXPR parent)
{
	return p_cons(parent, SEXPR_NIL);
}

/* Looks up a variable only in the environment env (not parents).
 * Returns the cons(variable, value).
 */
static SEXPR lookup_local_variable(SEXPR var, SEXPR env)
{
	SEXPR link, bind;

	assert(!p_null(env));
	link = p_cdr(env);
	while (!p_null(link)) {
		bind = p_car(link);
		if (p_eq(var, p_car(bind))) {
			return bind;
		}
		link = p_cdr(link);
	}

	return SEXPR_NIL;
}

/* Looks up a variable in this environment and parents.
 * Returns the cons(variable, value).
 */
SEXPR lookup_variable(SEXPR var, SEXPR env)
{
	SEXPR bind;

	while (!p_null(env)) {
		bind = lookup_local_variable(var, env);
		if (!p_null(bind)) {
			return bind;
		}
		env = p_car(env);
	}

	return SEXPR_NIL;
}

/* Sets a varable (and creates it if needed) in the environment env. */
SEXPR define_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind, link;

	bind = lookup_local_variable(var, env);
	if (p_null(bind)) {
		push(env);
		bind = p_cons(var, val);
		link = p_cons(bind, p_cdr(env));
		pop();
		p_setcdr(env, link);
	} else {
		p_setcdr(bind, val);
	}

	return val;
}

/* Sets a varable (must exist) in the environment env or parent environments.
 */
SEXPR set_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind;

	bind = lookup_variable(var, env);
	if (p_null(bind)) {
		throw_err();
	}

	p_setcdr(bind, val);
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
