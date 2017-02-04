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

static void delay(void);
static void cons_stream(void);
static void pairp(void);
static void numberp(void);
static void symbolp(void);
static void eqp(void);
static void eqvp(void);
static void equalp(void);
static void cons(void);
static void car(void);
static void cdr(void);
static void setcar(void);
static void setcdr(void);
static void quote(void);
static void cond(void);
static void lambda(void);
static void special(void);
static void body(void);
static void define(void);
static void set(void);
static void plus(void);
static void difference(void);
static void times(void);
static void divide(void);
static void lessp(void);
static void greaterp(void);
static void greater_eqp(void);
static void less_eqp(void);
static void equal_numbersp(void);
static void eval(void);
static void apply(void);
static void gc(void);
static void quit(void);

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
	void (*fun)(void);
};

static struct builtin builtin_functions[] = {
	{ "car",  &car },
	{ "cdr", &cdr },
	{ "cons", &cons },
	{ "pair?", &pairp },
	{ "-", &difference },
	{ "=", &equal_numbersp },
	{ "eq?", &eqp },
	{ "eqv?", &eqvp },
	{ "equal?", &equalp },
	{ ">", &greaterp },
	{ ">=", &greater_eqp },
	{ "eval", &eval },
	{ "apply", &apply },
	{ "gc", &gc },
	{ "<", &lessp },
	{ "<=", &less_eqp },
	{ "number?", &numberp },
	{ "+", &plus },
	{ "set-car!", &setcar },
	{ "set-cdr!", &setcdr },
	{ "symbol?", &symbolp },
	{ "*", &times },
	{ "quit", &quit },
	{ "/", &divide },
	/* modulo and remainder */
};

static struct builtin builtin_specials[] = {
	{ "body", &body },
	{ "cond", &cond },
	{ "lambda", &lambda },
	{ "quote", &quote },
	{ "set!", &set },
	{ "define", &define },
	{ "special", &special },
	// { "delay", &delay },
	// { "cons-stream", &cons_stream },
};

static void install_builtin(const char *name, SEXPR val)
{
	s_val = val;
	s_expr = make_symbol(name, strlen(name));
	s_env = s_topenv;
	define_variable();
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

static void apply_builtin(struct builtin *pbin)
{
	pbin->fun();
}

void apply_builtin_function(int i)
{
	chkrange(i, NELEMS(builtin_functions));
	apply_builtin(&builtin_functions[i]);
}

void apply_builtin_special(int i)
{
	chkrange(i, NELEMS(builtin_specials));
	apply_builtin(&builtin_specials[i]);
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
		s_expr = get_sexpr(parse(&t, &p), &errorc);
		if (errorc == ERRORC_OK) {
			s_env = s_topenv;
			p_eval(); 
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

	return get_sexpr(parse(tokenize(read_console(&ic), &t), &p), errorc);
}

/****************************************************/

static void quote(void)
{
	s_val = p_car(s_args);
}

static void cond(void)
{
	p_evcon();
	s_tailrec = 1;
}

static void quit(void)
{
	exit(EXIT_SUCCESS);
}

static void set(void)
{
	SEXPR var;

	if (!p_nullp(s_args)) {
		var = p_car(s_args);
		if (!p_symbolp(var)) {
			throw_err("set! on something that is not a symbol");
		}

		s_expr = p_car(p_cdr(s_args));
		push(var);
		push(s_env);
		p_eval();
		s_env = pop();
		var = pop();
		set_variable(var, s_val, s_env);
	}
}

static void define(void)
{
	SEXPR var, args;
	int is_fn;

	is_fn = 0;
	var = p_car(s_args);
	if (p_pairp(var)) {
		args = p_cdr(var);
		var = p_car(var);
		is_fn = 1;
	}

	if (!p_symbolp(var)) {
		throw_err("define requires a variable name to define");
	}

	s_expr = var;
	if (is_fn) {
		s_args = p_cons(args, p_cdr(s_args));
		lambda();
	} else {
		push(s_env);
		push(s_expr);
		s_expr = p_car(p_cdr(s_args));
		p_eval();
		s_expr = pop();
		s_env = pop();
	}

	define_variable();
}

static void cons(void)
{
	s_val = p_cons(p_car(s_args), p_car(p_cdr(s_args)));
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

static void arith(float n0, float (*fun)(float, float))
{
	float n;

	/* count arguments */
	if (!at_leastn(s_args, 1)) {
		throw_err("too few arguments for arithmetic procedure");
	}

	/* check that they are numbers */
	if (!all_numbers(s_args)) {
		throw_err("bad argument for arithmetic procedure:"
			  "not a number");
	}

	/* calculate */
	n = sexpr_number(p_car(s_args));
	s_args = p_cdr(s_args);
	if (!p_pairp(s_args)) {
		n = fun(n0, n);
	} else {
		while (p_pairp(s_args)) {
			n = fun(n, sexpr_number(p_car(s_args)));
			s_args = p_cdr(s_args);
		}
	}

	s_val = make_number(n);
}

/* used for =, <, >, <=, >= */
static void logic(int (*fun)(float, float))
{
	float n, n2;

	/* count arguments */
	if (!at_leastn(s_args, 2)) {
		throw_err("too few arguments for logic procedure");
	}

	/* check that they are numbers */
	if (!all_numbers(s_args)) {
		throw_err("bad argument for logic procedure: not a number");
	}

	/* calculate */
	n = sexpr_number(p_car(s_args));
	s_args = p_cdr(s_args);
	while (!p_nullp(s_args)) {
		n2 = sexpr_number(p_car(s_args));
		if (!fun(n, n2)) {
			s_val = SEXPR_FALSE;
			return;
		}
		n = n2;
		s_args = p_cdr(s_args);
	}

	s_val = SEXPR_TRUE;
}

static int lessp_fun(float a, float b)
{
	return a < b;
}

static void lessp(void)
{
	logic(lessp_fun);
}

static int greaterp_fun(float a, float b)
{
	return a > b;
}

static void greaterp(void)
{
	logic(greaterp_fun);
}

static int greater_eqp_fun(float a, float b)
{
	return a >= b;
}

static void greater_eqp(void)
{
	logic(greater_eqp_fun);
}

static int less_eqp_fun(float a, float b)
{
	return a <= b;
}

static void less_eqp(void)
{
	logic(less_eqp_fun);
}

static int equal_numbersp_fun(float a, float b)
{
	return a == b;
}

static void equal_numbersp(void)
{
	logic(equal_numbersp_fun);
}

static float plus_fun(float a, float b)
{
	return a + b;
}

static void plus(void)
{
	arith(0, plus_fun);
}

static float difference_fun(float a, float b)
{
	return a - b;
}

static void difference(void) 
{
	arith(0, difference_fun);
}

static float times_fun(float a, float b)
{
	return a * b;
}

static void times(void)
{
	arith(1, times_fun);
}

static float divide_fun(float a, float b)
{
	return a / b;
}

static void divide(void)
{
	arith(1, divide_fun);
}

static void car(void)
{
	s_val = p_car(p_car(s_args));
}

static void cdr(void)
{
	s_val = p_cdr(p_car(s_args));
}

static void setcar(void)
{
	SEXPR p, q;

	p = p_car(s_args);
	q = p_car(p_cdr(s_args));
	s_val = p_setcar(p, q); 
}

static void setcdr(void)
{
	SEXPR p, q;

	p = p_car(s_args);
	q = p_car(p_cdr(s_args));
	s_val = p_setcdr(p, q); 
}

static void symbolp(void)
{
	s_val = p_symbolp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void pairp(void)
{
	s_val = p_pairp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void numberp(void)
{
	s_val = p_numberp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void eqp(void)
{
	s_val = p_eqp(p_car(s_args), p_car(p_cdr(s_args))) ? SEXPR_TRUE
							   : SEXPR_FALSE;
}

static void eqvp(void)
{
	s_val = p_eqvp(p_car(s_args), p_car(p_cdr(s_args))) ? SEXPR_TRUE
							    : SEXPR_FALSE;
}

static void equalp(void)
{
	s_val = p_equalp(p_car(s_args), p_car(p_cdr(s_args))) ? SEXPR_TRUE
	                                                      : SEXPR_FALSE;
}

static void special(void)
{
	s_val = make_special(sexpr_index(p_cons(s_args, s_env)));
}

static void lambda(void)
{
	s_val = make_function(sexpr_index(p_cons(s_args, s_env)));
}

static void body(void)
{
	int celli;
	SEXPR e;

	p_evlis();
	e = p_car(s_val);
	switch (sexpr_type(e)) {
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		s_val = cell_car(sexpr_index(e));
		break;
	case SEXPR_DYN_FUNCTION:
		celli = sexpr_index(e);
		s_val = p_cons(cell_car(celli), cell_cdr(celli));
		break;
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
		s_val = SEXPR_NIL;
		break;
	default:
		throw_err("no body for object");
		s_val = SEXPR_NIL;
	}
}

static void delay(void)
{
	s_args = p_cons(SEXPR_NIL, s_args);
	lambda();
}

static void cons_stream(void)
{
	SEXPR thecdr;

	push(s_args);
	push(s_env);
	s_expr = p_car(s_args);
	p_eval();
	s_env = pop();
	s_args = pop();
	push(s_val);
	s_args = p_cdr(s_args);
	delay();
	thecdr = s_val;
	s_val = pop();
	s_val = p_cons(s_val, thecdr); 
}

static void eval(void)
{
	s_val = p_car(s_args);
	s_tailrec = 1;
}

static void apply(void)
{
	s_proc = p_car(s_args);
	s_args = p_car(p_cdr(s_args));
	p_apply(); 
}

static void gc(void)
{
	p_gc();
	s_val = SEXPR_NIL;
}

int main(int argc, char* argv[])
{
	int errorc;

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
			s_expr = read(&errorc);
			if (errorc == ERRORC_EOF) {
				break;
			} else if (errorc == ERRORC_SYNTAX) {
				printf("lispe: syntax error\n");
			} else {
				s_env = s_topenv;
				p_eval();
				p_println(s_val);
			}
			assert(stack_empty());
		} else {
			printf("lispe: ** stop **\n");
		}
		clear_stack();
	}

	return 0;
}
