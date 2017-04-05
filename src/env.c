/*
Copyright (c) 2017 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "cfg.h"
#include "sexpr.h"
#include "common.h"
#include "err.h"
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

// s_expr (var), s_val (val), s_env
void define_variable(void)
{
	SEXPR bind, link;

	bind = lookup_local_variable(s_expr, s_env);
	if (p_nullp(bind)) {
		bind = p_cons(s_expr, s_val);
		link = p_cons(bind, p_cdr(s_env));
		p_setcdr(s_env, link);
	} else {
		p_setcdr(bind, s_val);
	}
}

#if 0
/* Sets a varable (and creates it if needed) in the environment env.  */
SEXPR define_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind, link;

	bind = lookup_local_variable(var, env);
	if (p_nullp(bind)) {
		push3(env, var, val);
		bind = p_cons(var, val);
		popn(2);
		link = p_cons(bind, p_cdr(env));
		p_setcdr(env, link);
		pop();
	} else {
		p_setcdr(bind, val);
	}

	return val;
}
#endif

/* Sets a varable (must exist) in the environment env or parent environments.
 */
SEXPR set_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR bind;

	bind = lookup_variable(var, env);
	if (p_nullp(bind)) {
		throw_err("set! on an undefined variable");
	}

	p_setcdr(bind, val);
	return val;
}

#if 0
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

	push3(env, params, args);
	define_variable(p_car(params), p_car(args), env);
	popn(2);
	params = p_cdr(params);
	args = p_cdr(args);
	while (!p_nullp(params)) {
		if (p_pairp(params)) {
			/* lambda () or lambda (a b ...) */
			push2(params, args);
			define_variable(p_car(params), p_car(args), env);
			popn(2);
			params = p_cdr(params);
			args = p_cdr(args);
		} else {
			/* lambda (a b . rest) */
			define_variable(params, args, env);
			break;
		}
	}
	pop(); // env
}
#endif

// s_env, s_unev (params), s_args
void extend_environment(void)
{
	if (p_nullp(s_unev))
		return;

	if (p_symbolp(s_unev)) {
		/* (lambda x x)
		 * The procedure takes n arguments, combined on a list in x.
		 */
		s_expr = s_unev;
		s_val = s_args;
		define_variable();
		return;
	}

	s_expr = p_car(s_unev);
	s_val = p_car(s_args);
	define_variable();
	s_unev = p_cdr(s_unev);
	s_args = p_cdr(s_args);
	while (!p_nullp(s_unev)) {
		if (p_pairp(s_unev)) {
			/* lambda () or lambda (a b ...) */
			s_expr = p_car(s_unev);
			s_val = p_car(s_args);
			define_variable();
			s_unev = p_cdr(s_unev);
			s_args = p_cdr(s_args);
		} else {
			/* lambda (a b . rest) */
			s_expr = s_unev;
			s_val = s_args;
			define_variable();
			break;
		}
	}
}
