#include "cfg.h"
#include "cbase.h"
#include "gc.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "cells.h"
#include "cellmark.h"
#include "numbers.h"
#include "symbols.h"
#include "common.h"
#include "lex.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <tgmath.h>

static SEXPR pairp(SEXPR e, SEXPR a);
static SEXPR numberp(SEXPR e, SEXPR a);
static SEXPR symbolp(SEXPR e, SEXPR a);
static SEXPR eqp(SEXPR e, SEXPR a);
static SEXPR eqvp(SEXPR e, SEXPR a);
static SEXPR equalp(SEXPR e, SEXPR a);
static SEXPR cons(SEXPR e, SEXPR a);
static SEXPR car(SEXPR e, SEXPR a);
static SEXPR cdr(SEXPR e, SEXPR a);
static SEXPR setcar(SEXPR e, SEXPR a);
static SEXPR setcdr(SEXPR e, SEXPR a);
static SEXPR quote(SEXPR e, SEXPR a);
static SEXPR cond(SEXPR e, SEXPR a);
static SEXPR lambda(SEXPR e, SEXPR a);
static SEXPR dynfn(SEXPR e, SEXPR a);
static SEXPR special(SEXPR e, SEXPR a);
static SEXPR body(SEXPR e, SEXPR a);
static SEXPR define(SEXPR sexpr, SEXPR a);
static SEXPR set(SEXPR sexpr, SEXPR a);
static SEXPR plus(SEXPR e, SEXPR a);
static SEXPR difference(SEXPR e, SEXPR a);
static SEXPR times(SEXPR e, SEXPR a);
static SEXPR divide(SEXPR e, SEXPR a);
static SEXPR quotient(SEXPR e, SEXPR a);
static SEXPR lessp(SEXPR sexpr, SEXPR a);
static SEXPR greaterp(SEXPR sexpr, SEXPR a);
static SEXPR greater_eqp(SEXPR sexpr, SEXPR a);
static SEXPR less_eqp(SEXPR sexpr, SEXPR a);
static SEXPR equal_numbersp(SEXPR sexpr, SEXPR a);
static SEXPR list(SEXPR e, SEXPR a);
static SEXPR assoc(SEXPR e, SEXPR a);
static SEXPR eval(SEXPR e, SEXPR a);
static SEXPR apply(SEXPR e, SEXPR a);
static SEXPR gc(SEXPR e, SEXPR a);
static SEXPR quit(SEXPR e, SEXPR a);

/*********************************************************
 * Exceptions.
 *********************************************************/

static jmp_buf buf; 

void throw_err(const char *s)
{
	printf("lispe: ** error **\n");
	if (s) {
		printf("lispe: %s\n", s);
	}
	longjmp(buf, 1);
}

/*********************************************************/

struct builtin {
	const char* id;
	SEXPR (*fun)(SEXPR, SEXPR);
	int tailrec;
};

static struct builtin builtin_functions[] = {
	{ "apply", &apply, 0 },
	{ "assoc", &assoc, 0 },
	{ "car",  &car, 0 },
	{ "cdr", &cdr, 0 },
	{ "cons", &cons, 0 },
	{ "pair?", &pairp, 0 },
	{ "-", &difference, 0 },
	{ "=", &equal_numbersp, 0 },
	{ "eq?", &eqp, 0 },
	{ "eqv?", &eqvp, 0 },
	{ "equal?", &equalp, 0 },
	{ ">", &greaterp, 0 },
	{ ">=", &greater_eqp, 0 },
	{ "eval", &eval, 1 },
	{ "gc", &gc, 0 },
	{ "<", &lessp, 0 },
	{ "<=", &less_eqp, 0 },
	{ "number?", &numberp, 0 },
	{ "+", &plus, 0 },
	{ "quotient", &quotient },
	{ "set-car!", &setcar, 0 },
	{ "set-cdr!", &setcdr, 0 },
	{ "symbol?", &symbolp, 0 },
	{ "*", &times, 0 },
	{ "quit", &quit, 0 },
	{ "/", &divide, 0 },
	/* modulo and remainder */
};

static struct builtin builtin_specials[] = {
	{ "body", &body, 0 },
	{ "cond", &cond, 1 },
	{ "lambda", &lambda, 0 },
	{ "dynfn", &dynfn, 0 },
	{ "list", &list, 0 },
	{ "quote", &quote, 0 },
	{ "set!", &set, 0 },
	{ "define", &define, 0 },
	{ "special", &special, 0 },
};

static void install_builtin(const char *name, SEXPR e)
{
	SEXPR var;

	var = make_symbol(name, strlen(name));
	define_variable(var, e, s_env);
}

static void install_builtin_functions(void)
{
	int i;

	for (i = 0; i < NELEMS(builtin_functions); i++) {
		install_builtin(builtin_functions[i].id,
			make_builtin_function(i));
	}
}

static void install_builtin_specials(void)
{
	int i;

	for (i = 0; i < NELEMS(builtin_specials); i++) {
		install_builtin(builtin_specials[i].id,
			make_builtin_special(i));
	}
}

static SEXPR apply_builtin(struct builtin *pbin, SEXPR args, SEXPR a)
{
	return pbin->fun(args, a);
}

SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a)
{
	chkrange(i, NELEMS(builtin_functions));
	return apply_builtin(&builtin_functions[i], args, a);
}

SEXPR apply_builtin_special(int i, SEXPR args, SEXPR a)
{
	chkrange(i, NELEMS(builtin_specials));
	return apply_builtin(&builtin_specials[i], args, a);
}

int builtin_function_tailrec(int i)
{
	chkrange(i, NELEMS(builtin_functions));
	return builtin_functions[i].tailrec;
}

int builtin_special_tailrec(int i)
{
	chkrange(i, NELEMS(builtin_specials));
	return builtin_specials[i].tailrec;
}

static const char *builtin_name(struct builtin *pbin)
{
	return pbin->id;
}

const char *builtin_function_name(int i)
{
	assert(i >= 0 && i < NELEMS(builtin_functions));
	return builtin_name(&builtin_functions[i]);
}

const char *builtin_special_name(int i)
{
	assert(i >= 0 && i < NELEMS(builtin_specials));
	return builtin_name(&builtin_specials[i]);
}

/*********************************************************/

static void load_init_file(void)
{
	struct input_channel ic;
	struct tokenizer t;
	struct parser p;
	SEXPR sexpr;
	int errorc;
	FILE *fp;

	if (setjmp(buf)) {
		printf("lispe: error loading init.lisp\n");
		clear_stack();
		goto end;
	}

	fp = fopen("init.scm", "r");
	if (!fp)
		return;

	tokenize(read_file(&ic, fp), &t);
	do {
		sexpr = get_sexpr(parse(&t, &p), &errorc);
		if (errorc == ERRORC_OK) {
			p_eval(sexpr, s_env); 
		}
	} while (errorc == ERRORC_OK);

	if (errorc == ERRORC_SYNTAX) {
		printf("lispe: syntax error on init.lisp\n");
	} else {
		printf("[init.lisp loaded ok]\n");
	}

end:	fclose(fp);
}

static SEXPR read(int *errorc)
{
	struct input_channel ic;
	struct tokenizer t;
	struct parser p;

	return get_sexpr(parse(tokenize(read_console(&ic), &t), &p),
			 errorc);
}

/****************************************************/

static SEXPR quote(SEXPR e, SEXPR a)
{
	return p_car(e);
}

static SEXPR cond(SEXPR e, SEXPR a)
{
	SEXPR tmp;

	for (;;) {
		tmp = p_car(e);
		if (p_eqp(p_eval(p_car(tmp), a), SEXPR_FALSE)) {
			e = p_cdr(e);
		} else  {
			return p_car(p_cdr(tmp));
		}
	}
}

/* TODO: make builtin function so the arguments eval automatically? */
static SEXPR list(SEXPR e, SEXPR a)
{
	return p_evlis(e, a);
}

static SEXPR assoc(SEXPR e, SEXPR a)
{
	return p_assoc(p_car(e), p_car(p_cdr(e)));
}

static SEXPR quit(SEXPR e, SEXPR a)
{
	exit(EXIT_SUCCESS);
}

static SEXPR set(SEXPR sexpr, SEXPR a)
{
	SEXPR var;
	SEXPR val;

	val = SEXPR_NIL;
	push2(sexpr, a);
	if (!p_nullp(sexpr)) {
		var = p_car(sexpr);
		if (!p_symbolp(var)) {
			throw_err("set! on something that is not a symbol");
		}

		sexpr = p_cdr(sexpr);
		val = p_eval(p_car(sexpr), a);
		set_variable(var, val, a);
	}
	popn(2);

	return val;
}

static SEXPR define(SEXPR sexpr, SEXPR a)
{
	SEXPR var, val, args;
	int is_fn;

	push2(sexpr, a);
	is_fn = 0;
	var = p_car(sexpr);
	if (p_pairp(var)) {
		args = p_cdr(var);
		var = p_car(var);
		is_fn = 1;
	}

	if (!p_symbolp(var)) {
		throw_err("define requires a variable name to define");
	}

	if (is_fn) {
		val = lambda(p_cons(args, p_cdr(sexpr)), a);
	} else {
		val = p_eval(p_car(p_cdr(sexpr)), a);
	}

	define_variable(var, val, a);
	popn(2);

	return val;
}

static SEXPR cons(SEXPR e, SEXPR a)
{
	return p_cons(p_car(e), p_car(p_cdr(e)));
}

/* Checks that there are at least n elements on the list p. */
static int at_leastn(SEXPR p, int n)
{
	while (n > 0 && p_pairp(p)) {
		n--;
		p = p_cdr(p);
	}

	return n == 0;
}

/* Check that all the elements of p are numbers.  */
static int all_numbers(SEXPR p)
{
	while (p_pairp(p)) {
		if (!p_numberp(p_car(p))) {
			return 0;
		} else {
			p = p_cdr(p);
		}
	}

	return 1;
}

static SEXPR arith(SEXPR e, SEXPR a, float n0, float (*fun)(float, float))
{
	float n;

	/* count arguments */
	if (!at_leastn(e, 1)) {
		throw_err("too few arguments for arithmetic procedure");
		return SEXPR_FALSE;
	}

	/* check that they are numbers */
	if (!all_numbers(e)) {
		throw_err("bad argument for arithmetic procedure:"
			  "not a number");
		return SEXPR_FALSE;
	}

	/* calculate */
	n = sexpr_number(p_car(e));
	e = p_cdr(e);
	if (!p_pairp(e)) {
		n = fun(n0, n);
	} else {
		while (p_pairp(e)) {
			n = fun(n, sexpr_number(p_car(e)));
			e = p_cdr(e);
		}
	}

	return make_number(n);
}

/* used for =, <, >, <=, >= */
static SEXPR logic(SEXPR e, SEXPR a, int (*fun)(float, float))
{
	float n, n2;

	/* count arguments */
	if (!at_leastn(e, 2)) {
		throw_err("too few arguments for logic procedure");
		return SEXPR_FALSE;
	}

	/* check that they are numbers */
	if (!all_numbers(e)) {
		throw_err("bad argument for logic procedure: not a number");
		return SEXPR_FALSE;
	}

	/* calculate */
	n = sexpr_number(p_car(e));
	e = p_cdr(e);
	while (!p_nullp(e)) {
		n2 = sexpr_number(p_car(e));
		if (!fun(n, n2))
			return SEXPR_FALSE;
		n = n2;
		e = p_cdr(e);
	}

	return SEXPR_TRUE;
}

static int lessp_fun(float a, float b)
{
	return a < b;
}

static SEXPR lessp(SEXPR e, SEXPR a)
{
	return logic(e, a, lessp_fun);
}

static int greaterp_fun(float a, float b)
{
	return a > b;
}

static SEXPR greaterp(SEXPR e, SEXPR a)
{
	return logic(e, a, greaterp_fun);
}

static int greater_eqp_fun(float a, float b)
{
	return a >= b;
}

static SEXPR greater_eqp(SEXPR e, SEXPR a)
{
	return logic(e, a, greater_eqp_fun);
}

static int less_eqp_fun(float a, float b)
{
	return a <= b;
}

static SEXPR less_eqp(SEXPR e, SEXPR a)
{
	return logic(e, a, less_eqp_fun);
}

static int equal_numbersp_fun(float a, float b)
{
	return a == b;
}

static SEXPR equal_numbersp(SEXPR e, SEXPR a)
{
	return logic(e, a, equal_numbersp_fun);
}

static float plus_fun(float a, float b)
{
	return a + b;
}

static SEXPR plus(SEXPR e, SEXPR a)
{
	return arith(e, a, 0, plus_fun);
}

static float difference_fun(float a, float b)
{
	return a - b;
}

static SEXPR difference(SEXPR e, SEXPR a) 
{
	return arith(e, a, 0, difference_fun);
}

static float times_fun(float a, float b)
{
	return a * b;
}

static SEXPR times(SEXPR e, SEXPR a)
{
	return arith(e, a, 1, times_fun);
}

static float divide_fun(float a, float b)
{
	return a / b;
}

static SEXPR divide(SEXPR e, SEXPR a)
{
	return arith(e, a, 1, divide_fun);
}

static SEXPR quotient(SEXPR e, SEXPR a)
{
	SEXPR e1, e2;
	float n1, n2;

	e = p_evlis(e, a);
	if (!p_pairp(e))
	       goto eargs;
	e1 = p_car(e);
	if (!p_numberp(e1))
		goto eargs;
	e2 = p_car(p_cdr(e));
	if (!p_numberp(e2))
		goto eargs;
	n1 = sexpr_number(e1);
	n2 = sexpr_number(e2);
	if (n2 == 0)
		throw_err("quotient by zero");
	return make_number(trunc(n1 / n2));

eargs:	throw_err("wrong arguments for quotient");
	return SEXPR_NIL;
}

static SEXPR car(SEXPR e, SEXPR a)
{
	return p_car(p_car(e));
}

static SEXPR cdr(SEXPR e, SEXPR a)
{
	return p_cdr(p_car(e));
}

static SEXPR setcar(SEXPR e, SEXPR a)
{
	SEXPR p, q;

	p = p_car(e);
	q = p_car(p_cdr(e));
	return p_setcar(p, q); 
}

static SEXPR setcdr(SEXPR e, SEXPR a)
{
	SEXPR p, q;

	p = p_car(e);
	q = p_car(p_cdr(e));
	return p_setcdr(p, q); 
}

static SEXPR symbolp(SEXPR e, SEXPR a)
{
	return p_symbolp(p_car(e)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static SEXPR pairp(SEXPR e, SEXPR a)
{
	return p_pairp(p_car(e)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static SEXPR numberp(SEXPR e, SEXPR a)
{
	return p_numberp(p_car(e)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static SEXPR eqp(SEXPR e, SEXPR a)
{
	return p_eqp(p_car(e), p_car(p_cdr(e))) ? SEXPR_TRUE : SEXPR_FALSE;
}

static SEXPR eqvp(SEXPR e, SEXPR a)
{
	return p_eqvp(p_car(e), p_car(p_cdr(e))) ? SEXPR_TRUE : SEXPR_FALSE;
}

static SEXPR equalp(SEXPR e, SEXPR a)
{
	return p_equalp(p_car(e), p_car(p_cdr(e))) ? SEXPR_TRUE
	                                           : SEXPR_FALSE;
}

static SEXPR special(SEXPR e, SEXPR a)
{
	/* e is parameters and body */
	// return make_special(sexpr_index(e));
	return make_special(sexpr_index(p_cons(e, a)));
}

static SEXPR lambda(SEXPR e, SEXPR a)
{
	/* e is parameter list and body */
	// printf("made lambda with env: ");
	// p_println_env(a);
	return make_function(sexpr_index(p_cons(e, a)));
}

static SEXPR dynfn(SEXPR e, SEXPR a)
{
	return make_dyn_function(sexpr_index(e));
}

/* TODO: change for symbol-function and print like (lambda (x) (nc ...)) */
static SEXPR body(SEXPR e, SEXPR a)
{
	int celli;

	/* TODO: admit any number of arguments */
	e = p_evlis(e, a);
	e = p_car(e);
	switch (sexpr_type(e)) {
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		return cell_car(sexpr_index(e));
	case SEXPR_DYN_FUNCTION:
		celli = sexpr_index(e);
		return p_cons(cell_car(celli), cell_cdr(celli));
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
		return SEXPR_NIL;
	default:
		throw_err("no body for object");
		return SEXPR_NIL;
	}
}

static SEXPR eval(SEXPR e, SEXPR a)
{
	return p_car(e);
}

static SEXPR apply(SEXPR e, SEXPR a)
{
	int tailrec;
	SEXPR env2, r;

	r = p_apply(p_car(e), p_car(p_cdr(e)), a, &tailrec, &env2); 
	if (tailrec) {
		r = p_eval(r, env2);
	}
	return r;
}

static SEXPR gc(SEXPR e, SEXPR a)
{
	p_gc();
	return SEXPR_NIL;
}

int main(int argc, char* argv[])
{
	int errorc;
	SEXPR e;

	printf("lispe minimal lisp 1.0\n\n");

	cells_init();
	cellmark_init();
	init_numbers();
	init_symbols();
	gcbase_init();

	install_builtin_functions();
	install_builtin_specials();

	load_init_file();

	/* REPL */
	for (;;) {
		printf("lispe> ");
		if (!setjmp(buf)) {
			e = read(&errorc);
			if (errorc == ERRORC_EOF) {
				break;
			} else if (errorc == ERRORC_SYNTAX) {
				printf("lispe: syntax error\n");
			} else {
				p_println(p_eval(e, s_env));
			}
		} else {
			printf("lispe: ** stop **\n");
			clear_stack();
		}
		assert(stack_empty());
	}

	return 0;
}
