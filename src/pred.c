#include "cfg.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "cells.h"
#include "numbers.h"
#include "common.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif

void p_println_env(SEXPR env);

/*
 * In lispe.c we have the functions called from the interpreter, for example
 * null(). These in turn call these in this file, for example p_null().
 * All the internal functions start with p_ .
 */

static int s_debug = 0;

int p_nullp(SEXPR e)
{
	return sexpr_eq(SEXPR_NIL, e);
}

int p_symbolp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_SYMBOL;
}

int p_numberp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_NUMBER;
}

int p_realp(SEXPR e)
{
	return p_numberp(e) && number_type(sexpr_number(e)) == NUM_REAL;
}

int p_integerp(SEXPR e)
{
	return p_numberp(e) && number_type(sexpr_number(e)) == NUM_INT;
}

int p_pairp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_CONS;
}

SEXPR p_car(SEXPR e)
{
	if (!p_pairp(e)) {
		throw_err("car used with something that is not a pair");
	}

	return cell_car(sexpr_index(e));
}

SEXPR p_cdr(SEXPR e)
{
	if (!p_pairp(e)) {
		throw_err("cdr used with something that is not a pair");
	}

	return cell_cdr(sexpr_index(e));
}

int p_eqp(SEXPR x, SEXPR y)
{
	return sexpr_eq(x, y);
}

int p_eqvp(SEXPR x, SEXPR y)
{
	if (p_numberp(x) && p_numberp(y))
		return numbers_eqv(sexpr_number(x), sexpr_number(y));
	else
		return sexpr_eq(x, y);
}

int p_equalp(SEXPR x, SEXPR y)
{
	for (;;) {
		if (p_pairp(x) && p_pairp(y)) {
			if (p_equalp(p_car(x), p_car(y))) {
				x = p_cdr(x);
				y = p_cdr(y);
			} else {
				return 0;
			}
		} else if (!p_pairp(x) && !p_pairp(y)) {
			return p_eqvp(x, y);
		} else {
			return 0;
		}
	}
}

SEXPR p_setcar(SEXPR e, SEXPR val)
{
	if (!p_pairp(e)) {
		throw_err("set-car! used on something that is not a pair");
	}

	set_cell_car(sexpr_index(e), val);
	return val;
}

SEXPR p_setcdr(SEXPR e, SEXPR val)
{
	if (!p_pairp(e)) {
		throw_err("set-cdr! used on something that is not a pair");
	}

	set_cell_cdr(sexpr_index(e), val);
	return val;
}

/*
 * Evaluate every expression in s_args.
 * s_val will be the value of the last evaluated expression.
 *
 * in: args, env
 * out: val
 */
void p_evlis(void)
{
	SEXPR node, node2;
	
	if (p_nullp(s_args)) {
		s_val = s_args;
		return;
	}

	push(s_env);
	push(s_args);
	s_expr = p_car(s_args);
	p_eval();
	s_args = pop();
	s_env = pop();

	s_val = push(p_cons(s_val, SEXPR_NIL));
	node = s_val;
	s_args = p_cdr(s_args);
	while (!p_nullp(s_args)) {
		push(s_env);
		push(s_args);
		s_expr = p_car(s_args);
		p_eval();
		s_args = pop();
		s_env = pop();

		node2 = p_cons(s_val, SEXPR_NIL);
		p_setcdr(node, node2);
		node = node2;
		s_args = p_cdr(s_args);
	}
	s_val = pop();
}

/* Evaluates a list of expressions which are in s_unev. If !eval_last, the last
 * expression is returned in s_val but not evaluated. If eval_last, all
 * expressions are evaluated and the result of the last is s_val.
 *
 * in: unev, env.
 * out: val.
 */
void p_evseq(int eval_last)
{
	while (!p_nullp(s_unev)) {
		s_expr = p_car(s_unev);
		if (!eval_last && p_nullp(p_cdr(s_unev))) {
			/* Don't eval the last expression: tail recursion. */
			s_val = s_expr;
			return;
		}
		push(s_unev);
		push(s_env);
		p_eval();
		s_env = pop();
		s_unev = pop();
		s_unev = p_cdr(s_unev);
	}
}

/* in: proc, args.
 * Exits: val.
 * Will set s_tailrec if tail recursion should be made.
 *
 * TODO: we don't have s_env now probably, so we cannot implement dynamic
 * binding right now... (?)
 */
void p_apply(void)
{
	SEXPR params, body, params_n_body;
	int celli;

	if (s_debug) {
		printf("apply fn: ");
		p_println(s_proc);
		printf("args: ");
		p_println(s_args);
		// printf("a: ");
		// println(s_env);
	}

	switch (sexpr_type(s_proc)) {
	case SEXPR_BUILTIN_FUNCTION:
		s_tailrec = 0;
		apply_builtin_function(sexpr_index(s_proc));
		return;

	case SEXPR_BUILTIN_SPECIAL:
		s_tailrec = 0;
		apply_builtin_special(sexpr_index(s_proc));
		return;

	case SEXPR_FUNCTION:
		/* 
		 * A lambda creates a new environment with its saved
		 * environment as parent.
		 */
		celli = sexpr_index(s_proc);
		params_n_body = cell_car(celli);
		s_env = make_environment(cell_cdr(celli));
		/* 
		 * Pair parameters with their arguments and extend the
		 * environment.
		 */
		celli = sexpr_index(params_n_body);
		params = cell_car(celli);
		s_unev = params;
		extend_environment();
		body = cell_cdr(celli);
		s_unev = body;
		p_evseq(0);
		s_tailrec = 1;
		return;

	case SEXPR_SPECIAL:
		/*
		 * A special creates a new environment with its saved
		 * environment as parent but will return the expression to the
		 * previous environment.
		 */
		celli = sexpr_index(s_proc);
		params_n_body = cell_car(celli);
		push(s_env);
		s_env = make_environment(cell_cdr(celli));
		/*
		 * Pair parameters with their arguments and extend the
		 * environment.
		 */
		celli = sexpr_index(params_n_body);
		params = cell_car(celli);
		s_unev = params;
		extend_environment();
		body = cell_cdr(celli);
		s_unev = body;
		p_evseq(1);
		/* get the last environment */
		s_env = pop();
		s_tailrec = 1;
		return;

	default:
		throw_err("applying to a unknown object type");
	}
}

static int s_evalc = 0;

static int listlen(SEXPR e)
{
	int n;

	if (!p_pairp(e))
		return 0;

	n = 0;
	while (!p_nullp(e)) {
		n++;
		e = p_cdr(e);
	}
	return n;
}

/* Given the list p (possibly empty) and node pointing to the last pair
 * of the list (undefined if p is empty), returns a new list with a new
 * pair added to the end and node will point to that pair.
 */
static SEXPR p_adjoin(SEXPR p, SEXPR *node, SEXPR e)
{
	SEXPR node2;

	if (p_nullp(p)) {
		*node = p_cons(e, SEXPR_NIL);
		return *node;
	} else {
		node2 = p_cons(e, SEXPR_NIL);
		p_setcdr(*node, node2);
		*node = node2;
		return p;
	}
}
			
/* Evaluates the elements of s_unev and builds the list s_args.
 * in: unev, env, args (NIL).
 * out: args.
 */
void p_evargs(void)
{
	SEXPR node;

	assert(p_nullp(s_args));

	if (p_nullp(s_unev)) {
		s_args = s_unev;
	}

	node = s_args;
	for (;;) {
		push(s_args);
		s_expr = p_car(s_unev);
		if (p_nullp(p_cdr(s_unev))) {
			/* last operand */
			p_eval();
			s_args = pop();
			s_args = p_adjoin(s_args, &node, s_val);
			break;
		} else {
			push(s_env);
			push(s_unev);
			p_eval();
			s_unev = pop();
			s_env = pop();
			s_args = pop();
			s_args = p_adjoin(s_args, &node, s_val);
			s_unev = p_cdr(s_unev);
		}
	}
}

/* in: expr, env.
 * out: val
 */
void p_eval(void)
{
	int t;
	SEXPR bind;

	s_evalc++;

	if (s_debug) {
		printf("eval stack %d\n", s_evalc);
		// p_println(a);
	}

again:  switch (sexpr_type(s_expr)) {
	/* () does not evaluate to itself in Scheme
	 * case SEXPR_NIL:
	 */
	case SEXPR_TRUE:
	case SEXPR_FALSE:
	case SEXPR_NUMBER:
		s_evalc--;
		s_val = s_expr;
		return;

	case SEXPR_SYMBOL:
		// printf("lookup: ");
		// p_println(e);
		bind = lookup_variable(s_expr, s_env);
		if (p_nullp(bind)) {
			throw_err("variable not bound");
		}
		s_evalc--;
		s_val = p_cdr(bind);
		return;

	case SEXPR_CONS:
		/* application */
		if (s_debug) {
			printf("eval: ");
			p_println(s_expr);
			printf("env: ");
			p_println_env(s_env);
		}

		/* evaluate the operator */
		push(s_env);
		s_unev = p_cdr(s_expr);
		push(s_unev);
		s_expr = p_car(s_expr);
		p_eval();
		s_proc = s_val;

		/* evaluate arguments if needed and apply */
		s_unev = pop();
		s_env = pop();
		s_args = SEXPR_NIL;
		t = sexpr_type(s_proc);
		if (t == SEXPR_BUILTIN_SPECIAL || t == SEXPR_SPECIAL) {
			s_args = s_unev;
		} else if (!p_nullp(s_unev)) {
			/* 
			 * For non special forms we evaluate the arguments
			 * only if there is any.
			 */
			push(s_proc);
			p_evargs();
			s_proc = pop();
		}
		p_apply();
		if (s_tailrec) {
			s_expr = s_val;
			goto again;
		}
		if (s_debug) {
			printf("r: ");
			p_println(s_val);
		}
		s_evalc--;
		return;

	default:
		throw_err("unknown object to eval");
	}
}

void p_print(SEXPR sexpr)
{
	int i;

	switch (sexpr_type(sexpr)) {
	case SEXPR_NIL:
		printf("()");
		break;
	case SEXPR_TRUE:
		printf("#t");
		break;
	case SEXPR_FALSE:
		printf("#f");
		break;
	case SEXPR_CONS:
		/* TODO: avoid infinite recursion */
		i = sexpr_index(sexpr);
		printf("(");
		p_print(cell_car(i));
		while (!p_nullp(cell_cdr(i))) {
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
		print_number(sexpr_number(sexpr));
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
	while (!p_nullp(env)) {
		if (p_nullp(p_car(env))) {
			break;
		}
		any = 1;
		p_println(p_cdr(env));
		env = p_car(env);
	}
	if (!any) {
		printf("\n");
	}
}

void p_println(SEXPR sexpr)
{
	p_print(sexpr);
	printf("\n");
}
