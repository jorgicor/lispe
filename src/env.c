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

	assert(!p_nullp(env));
	link = p_cdr(env);
	while (!p_nullp(link)) {
		bind = p_car(link);
		if (p_eqp(var, p_car(bind))) {
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

	while (!p_nullp(env)) {
		bind = lookup_local_variable(var, env);
		if (!p_nullp(bind)) {
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
	if (p_nullp(bind)) {
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

/* Sets a varable (must exist) in the environment env or parent environments.
 */
SEXPR set_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind;

	bind = lookup_variable(var, env);
	if (p_nullp(bind)) {
		throw_err();
	}

	p_setcdr(bind, val);
	return val;
}
		
void extend_environment(SEXPR env, SEXPR params, SEXPR args)
{
	if (p_nullp(params))
		return;

	if (p_symbolp(params)) {
		/* (lambda x x)
		 * The procedure takes n arguments, combined on a list in x.
		 */
		define_variable(params, args, env);
		return;
	}

	push3(params, args, env);
	define_variable(p_car(params), p_car(args), env);
	params = p_cdr(params);
	args = p_cdr(args);
	while (!p_nullp(params)) {
		if (p_pairp(params)) {
			/* lambda () OR lambda (a b ...) */
			define_variable(p_car(params), p_car(args), env);
			params = p_cdr(params);
			args = p_cdr(args);
		} else {
			/* lambda (a b . rest) */
			define_variable(params, args, env);
			break;
		}
	}
	popn(3);
}
