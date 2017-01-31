#include "cfg.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "cells.h"
#include "common.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif

void p_println_env(SEXPR env);

/*
 * In lispe.c we have the functions called from the interpreter, for example
 * atom(). These in turn call these in this file, for example p_atom().
 * All the internal functions start with p_ .
 */

static int s_debug = 1;

int p_null(SEXPR e)
{
	return sexpr_eq(SEXPR_NIL, e);
}

int p_symbolp(SEXPR e)
{
	return p_null(e) || sexpr_type(e) == SEXPR_SYMBOL;
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
	if (p_null(e))
		return SEXPR_NIL;

	if (!p_consp(e))
		throw_err();

	return cell_car(sexpr_index(e));
}

SEXPR p_cdr(SEXPR e)
{
	if (p_null(e))
		return SEXPR_NIL;

	if (!p_consp(e))
		throw_err();

	return cell_cdr(sexpr_index(e));
}

SEXPR p_add(SEXPR var, SEXPR val, SEXPR a)
{
	return p_cons(p_cons(var, val), a);
}

int p_eq(SEXPR x, SEXPR y)
{
	if (p_null(x) && p_null(y)) {
		return 1;
	} else if (p_symbolp(x) && p_symbolp(y)) {
		return sexpr_eq(x, y);
	} else if (p_numberp(x) && p_numberp(y)) {
		return sexpr_number(x) == sexpr_number(y);
	} else {
		throw_err();
		return SEXPR_NIL;
	}
}

int p_equal(SEXPR x, SEXPR y)
{
	for (;;) {
		if (p_atom(x) && p_atom(y)) {
			return p_eq(x, y);
		} else if (p_consp(x) && p_consp(y) &&
			   p_equal(p_car(x), p_car(y)))
	       	{
			x = p_cdr(x);
			y = p_cdr(y);
		} else {
			return 0;
		}
	}
}

SEXPR p_setcar(SEXPR e, SEXPR val)
{
	if (!p_consp(e))
		throw_err();

	set_cell_car(sexpr_index(e), val);
	return val;
}

SEXPR p_setcdr(SEXPR e, SEXPR val)
{
	if (!p_consp(e))
		throw_err();

	set_cell_cdr(sexpr_index(e), val);
	return val;
}

/* 'a is an association list ((A . B) (C . D) ... ).
 * Return the first pair whose 'car is 'x.
 */
SEXPR p_assoc(SEXPR x, SEXPR a)
{
	for (;;) {
		if (p_null(a)) {
			return SEXPR_NIL;
		} else if (p_equal(p_car(p_car(a)), x)) {
			return p_car(a);
		} else {
			a = p_cdr(a);
		}
	}
}

/* Given two lists, 'x and 'y, returns a list with pairs of each list, i.e.:
 * (A B) (C D) -> ((A C) (B D))
 * and adds it to the start of the list 'a.
 * The list 'x must contain symbols.
 * The list 'x can be shorter than 'y; in that case, if the last element of 'x
 * is the symbol &rest, it will be paired with a list containing the remaining
 * elements of 'y.
 */
static SEXPR p_pairargs(SEXPR x, SEXPR y, SEXPR a)
{
	SEXPR head, node, node2;

	if (p_null(x))
		return a;

	push3(x, y, a);

	/* Handle the case of only one parameter called &rest */
	if (p_null(p_cdr(x)) && p_eq(p_car(x), s_rest_atom)) {
		node = p_cons(p_cons(p_car(x), y), SEXPR_NIL);
		p_setcdr(node, a);
		popn(3);
		return node;
	}

	head = push(p_cons(p_cons(p_car(x), p_car(y)), SEXPR_NIL));
	node = head;
	x = p_cdr(x);
	y = p_cdr(y);
	while (!p_null(x)) {
		if (p_null(p_cdr(x)) && p_eq(p_car(x), s_rest_atom)) {
			node2 = p_cons(p_cons(p_car(x), y), SEXPR_NIL);
			p_setcdr(node, node2);
			node = node2;
		} else {
			node2 = p_cons(p_cons(p_car(x), p_car(y)), SEXPR_NIL);
			p_setcdr(node, node2);
			node = node2;
			y = p_cdr(y);
		}
		x = p_cdr(x);
	}
	p_setcdr(node, a);
	popn(4);

	return head;
}

/* eval each list member.  */
SEXPR p_evlis(SEXPR m, SEXPR a)
{
	SEXPR head, node, node2;
	
	if (p_null(m))
		return m;

	push2(m, a);
	head = push(p_cons(p_eval(p_car(m), a), SEXPR_NIL));
	node = head;
	m = p_cdr(m);
	while (!p_null(m)) {
		node2 = p_cons(p_eval(p_car(m), a), SEXPR_NIL);
		p_setcdr(node, node2);
		node = node2;
		m = p_cdr(m);
	}
	popn(3);
	return head;
}

/* eval COND */
SEXPR p_evcon(SEXPR c, SEXPR a)
{
	for (;;) {
		if (p_eq(p_eval(p_car(p_car(c)), a), s_true_atom)) {
			return p_eval(p_car(p_cdr(p_car(c))), a);
		} else {
			c = p_cdr(c);
		}
	}
}

/* If tailrec != NULL, we will act tail recursive, that is, the last
 * expression in the function to apply will be returned unevaluated.
 * In that case, *tailrec will be set to 1 or 0.
 * If 1, it means that the SEXPR returned is the last expression of a
 * sequence and has not been evaluated. Eval can evaluate it and 'env2
 * will be the environment in which to evaluate.
 * This is to implement tail recursion, until a better idea.
 * If 0, the return value is already the final one, eval does not need to
 * evaluate anything.
 */
static SEXPR p_apply(SEXPR fn, SEXPR x, SEXPR env, int *tailrec, SEXPR *env2)
{
	SEXPR e1, r, env_new;
	int celli;

#if 0
	printf("apply fn: ");
	println(fn);
	printf("x: ");
	println(x);
	printf("a: ");
	println(a);
#endif

	r = SEXPR_NIL;
	if (tailrec) {
		assert(env2);
		*tailrec = 0;
	}

	push3(x, fn, env);
	switch (sexpr_type(fn)) {
	case SEXPR_BUILTIN_FUNCTION:
		*tailrec = builtin_function_tailrec(sexpr_index(fn));
		r = apply_builtin_function(sexpr_index(fn), x, env);
		popn(3);
		if (*tailrec) {
			*env2 = env;
		}
		break;
	case SEXPR_BUILTIN_SPECIAL:
		*tailrec = builtin_special_tailrec(sexpr_index(fn));
		r = apply_builtin_special(sexpr_index(fn), x, env);
		popn(3);
		if (*tailrec) {
			*env2 = env;
		}
		break;
	case SEXPR_FUNCTION:
		/* a function (lambda) creates a new environment with
		 * its saved environment as parent.
		 */
		celli = sexpr_index(fn);
		fn = cell_car(celli);
		env = make_environment(cell_cdr(celli));
		push2(fn, env);
		printf("new env: ");
		p_println_env(env);
		celli = sexpr_index(fn);
		/* pair parameters with their arguments and extend the
		 * environment. */
		extend_environment(env, cell_car(celli), x);
		/* eval sequence except the last expression */
		e1 = cell_cdr(celli);
		while (!p_null(e1)) {
			/* last expr */
			if (p_null(p_cdr(e1))) {
				break;
			}
			r = p_eval(p_car(e1), env);
			e1 = p_cdr(e1);
		}
		if (!p_null(e1)) {
			r = p_car(e1);
		} else {
			r = SEXPR_NIL;
		}
		popn(5);
		if (!p_null(r)) {
			if (tailrec == NULL) {
				r = p_eval(r, env);
			} else {
				*tailrec = 1;
				*env2 = env;
			}
		}
		break;
	case SEXPR_SPECIAL:
		celli = sexpr_index(fn);
		fn = cell_car(celli);
		env_new = make_environment(cell_cdr(celli));
		push2(fn, env_new);
		printf("new env: ");
		p_println_env(env_new);
		celli = sexpr_index(fn);
		/* pair parameters with their arguments and append 'a. */
		extend_environment(env_new, cell_car(celli), x);
		/* eval sequence */
		e1 = cell_cdr(celli);
		while (!p_null(e1)) {
			r = p_eval(p_car(e1), env_new);
			e1 = p_cdr(e1);
		}
		popn(5);
		/* The returned expression must be evaluated in the previous
		 * environment. */
		*tailrec = 1;
		*env2 = env;
		printf("return expression to environment:\n");
		p_println(r); 
		p_println_env(env);
		break;
	default:
		throw_err();
	}

	return r;
}

static int s_evalc = 0;

static int listlen(SEXPR e)
{
	int n;

	if (!p_consp(e))
		return 0;

	n = 0;
	while (!p_null(e)) {
		n++;
		e = p_cdr(e);
	}
	return n;
}

SEXPR p_eval(SEXPR e, SEXPR env)
{
	int tailrec, t;
	SEXPR c, e1, env2;

	s_evalc++;
#if 1
	printf("eval stack %d\n", s_evalc);
	// p_println(a);
#endif

again:  switch (sexpr_type(e)) {
	case SEXPR_NIL:
	case SEXPR_NUMBER:
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
	case SEXPR_DYN_FUNCTION:
		s_evalc--;
		return e;
	case SEXPR_SYMBOL:
		c = lookup_variable(e, env);
		if (p_null(c)) {
			throw_err(); /* unbound symbol */
		}
		c = p_cdr(c);
		s_evalc--;
		return c;
	case SEXPR_CONS:
		if (s_debug) {
			printf("eval: ");
			p_println(e);
			printf("env: ");
			p_println_env(env);
		}
		/* evaluate the operator */
		push2(env, e);
		c = p_eval(p_car(e), env);
		t = sexpr_type(c);
		switch (t) {
		case SEXPR_BUILTIN_SPECIAL:
		case SEXPR_SPECIAL:
			/* special forms evaluate the arguments themselves */
			popn(2);
			c = p_apply(c, p_cdr(e), env, &tailrec, &env2);
			break;
		default:
			/* evaluate the arguments */
			push(c);
			e1 = p_evlis(p_cdr(e), env);
			popn(3);
			c = p_apply(c, e1, env, &tailrec, &env2);
		}
		if (tailrec) {
			e = c;
			env = env2;
			goto again;
		}
		if(s_debug) {
			printf("r: ");
			p_println(c);
		}
		s_evalc--;
		return c;

	default:
		throw_err();
		return SEXPR_NIL;
	}
}

void p_print(SEXPR sexpr)
{
	int i;

	switch (sexpr_type(sexpr)) {
	case SEXPR_NIL:
		printf("nil");
		break;
	case SEXPR_CONS:
		/* TODO: avoid infinite recursion */
		i = sexpr_index(sexpr);
		printf("(");
		p_print(cell_car(i));
		while (!p_null(cell_cdr(i))) {
			sexpr = cell_cdr(i);
			if (sexpr_type(sexpr) == SEXPR_CONS) {
				i = sexpr_index(sexpr);
				printf(" ");
				p_print(cell_car(i));
			} else {
				printf(" . ");
				p_print(sexpr);
				break;
			}
		}
		printf(")");
		break;
	case SEXPR_SYMBOL:
		printf("%s", sexpr_name(sexpr));
		break;
	case SEXPR_NUMBER:
		printf("%g", sexpr_number(sexpr));
		break;
	case SEXPR_BUILTIN_FUNCTION:
		printf("{builtin function %s}",
			builtin_function_name(sexpr_index(sexpr)));
		break;
	case SEXPR_BUILTIN_SPECIAL:
		printf("{builtin special %s}",
			builtin_special_name(sexpr_index(sexpr)));
		break;
	case SEXPR_FUNCTION:
		printf("{lambda}");
#if 0
		printf("(lambda ");
		p_print(cell_car(sexpr_index(sexpr)));
		p_print(cell_cdr(sexpr_index(sexpr))));
		printf(")");
#endif
		break;
	case SEXPR_SPECIAL:
		printf("{special}");
		break;
	case SEXPR_DYN_FUNCTION:
		printf("{d-lambda}");
		break;
	}
}

void p_println_env(SEXPR env)
{
	int any;

	any = 0;
	while (!p_null(env)) {
		if (p_null(p_car(env)))
			break;
		any = 1;
		p_println(p_cdr(env));
		env = p_car(env);
	}
	if (!any)
		printf("\n");
}

void p_println(SEXPR sexpr)
{
	p_print(sexpr);
	printf("\n");
}
