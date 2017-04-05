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
#include "err.h"
#include "lex.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <complex.h>

static void delay(void);
static void cons_stream(void);
static void pairp(void);
static void numberp(void);
static void complexp(void);
static void realp(void);
static void integerp(void);
static void exactp(void);
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
static void iff(void);
static void or(void);
static void and(void);
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

/*********************************************************/

struct builtin {
	const char* id;
	void (*fun)(void);
};

 struct builtin builtin_functions[] = {
	{ "apply", &apply },
	{ "car",  &car },
	{ "cdr", &cdr },
	{ "complex?", &complexp },
	{ "cons", &cons },
	{ "-", &difference },
	{ "=", &equal_numbersp },
	{ "eq?", &eqp },
	{ "eqv?", &eqvp },
	{ "equal?", &equalp },
	{ "eval", &eval },
	{ "exact?", &exactp },
	{ ">", &greaterp },
	{ ">=", &greater_eqp },
	{ "gc", &gc },
	{ "integer?", &integerp },
	{ "<", &lessp },
	{ "<=", &less_eqp },
	{ "number?", &numberp },
	{ "pair?", &pairp },
	{ "+", &plus },
	{ "real?", &realp },
	{ "set-car!", &setcar },
	{ "set-cdr!", &setcdr },
	{ "symbol?", &symbolp },
	{ "*", &times },
	{ "quit", &quit },
	{ "/", &divide },
	/* modulo and remainder */
};

static struct builtin builtin_specials[] = {
	{ "and", &and },
	{ "body", &body },
	{ "cond", &cond },
	{ "define", &define },
	{ "if", &iff },
	{ "lambda", &lambda },
	{ "or", &or },
	{ "quote", &quote },
	{ "set!", &set },
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
	static const char *fpath = "init.scm";

	if (setjmp(s_err_buf)) {
		printf("lispe: error loading %s\n", fpath);
		clear_stack();
		goto end;
	}

	fp = fopen(fpath, "r");
	if (!fp) {
		printf("[%s not found]\n", fpath);
		fpath = PP_DATADIR "/init.scm";
		fp = fopen(fpath, "r");
		if (!fp) {
			printf("[%s not found]\n", fpath);
			return;
		}
	}

	tokenize(read_file(&ic, fp), &t);
	do {
		s_expr = get_sexpr(parse(&t, &p), &errorc);
		if (errorc == ERRORC_OK) {
			s_env = s_topenv;
			p_eval(); 
		}
	} while (errorc == ERRORC_OK);

	if (errorc == ERRORC_SYNTAX) {
		printf("lispe: syntax error on %s\n", fpath);
	} else {
		printf("[%s loaded ok]\n", fpath);
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

static void and(void)
{
	s_val = SEXPR_TRUE;
	if (p_nullp(s_args))
		return;

	while (!p_eqp(s_val, SEXPR_FALSE)) {
		s_val = p_car(s_args);
		s_args = p_cdr(s_args);
		if (p_nullp(s_args)) {
			s_tailrec = 1;
			break;
		}
		s_expr = s_val;
		push(s_args);
		push(s_env);
		p_eval();
		s_env = pop();
		s_args = pop();
	}
}

static void or(void)
{
	s_val = SEXPR_FALSE;
	if (p_nullp(s_args))
		return;

	while (p_eqp(s_val, SEXPR_FALSE)) {
		s_val = p_car(s_args);
		s_args = p_cdr(s_args);
		if (p_nullp(s_args)) {
			s_tailrec = 1;
			break;
		}
		s_expr = s_val;
		push(s_args);
		push(s_env);
		p_eval();
		s_env = pop();
		s_args = pop();
	}
}

static void cond(void)
{
	int more;
	SEXPR c, tmp;

	c = s_args;
	more = 1;
	while (more && !p_nullp(c)) {
		tmp = p_car(c);
		s_expr = p_car(tmp);
		push(s_args);
		push(s_env);
		p_eval();
		s_env = pop();
		s_args = pop();
		if (p_eqp(s_val, SEXPR_FALSE)) {
			c = p_cdr(c);
		} else {
			s_unev = p_cdr(tmp);
			p_evseq(0);
			s_tailrec = 1;
			more = 0;
		}
	}

	if (more) {
		throw_err("cond: no condition was true");
	}
}

static void iff(void)
{
	s_expr = p_car(s_args);
	s_args = p_cdr(s_args);
	push(s_args);
	push(s_env);
	p_eval();
	s_env = pop();
	s_args = pop();
	if (p_eqp(s_val, SEXPR_FALSE)) {
		s_val = p_car(p_cdr(s_args));
	} else {
		s_val = p_car(s_args);
	}
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

static void arith(int n0, int op)
{
	struct number m, n;
	struct number *pn;

	build_real_number(&m, n0);	

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
	copy_number(sexpr_number(p_car(s_args)), &n);
	s_args = p_cdr(s_args);
	if (!p_pairp(s_args)) {
		apply_arith_op(op, &m, &n, &n);
	} else {
		while (p_pairp(s_args)) {
			pn = sexpr_number(p_car(s_args));
			apply_arith_op(op, &n, pn, &n);
			s_args = p_cdr(s_args);
		}
	}

	s_val = make_number(&n);
}

/* used for =, <, >, <=, >= */
static void logic(int op)
{
	struct number n;
	struct number *pn;

	/* count arguments */
	if (!at_leastn(s_args, 2)) {
		throw_err("too few arguments for logic procedure");
	}

	/* check that they are numbers */
	if (!all_numbers(s_args)) {
		throw_err("bad argument for logic procedure: not a number");
	}

	/* calculate */
	copy_number(sexpr_number(p_car(s_args)), &n);
	s_args = p_cdr(s_args);
	while (!p_nullp(s_args)) {
		pn = sexpr_number(p_car(s_args));
		if (!apply_logic_op(op, &n, pn)) {
			s_val = SEXPR_FALSE;
			return;
		}
		copy_number(pn, &n);
		s_args = p_cdr(s_args);
	}

	s_val = SEXPR_TRUE;
}

static void lessp(void)
{
	logic(OP_LOGIC_LT);
}

static void greaterp(void)
{
	logic(OP_LOGIC_GT);
}

static void greater_eqp(void)
{
	logic(OP_LOGIC_GE);
}

static void less_eqp(void)
{
	logic(OP_LOGIC_LE);
}

static void equal_numbersp(void)
{
	logic(OP_LOGIC_EQUAL);
}

static void plus(void)
{
	arith(0, OP_ARITH_ADD);
}

static void difference(void) 
{
	arith(0, OP_ARITH_SUB);
}

static void times(void)
{
	arith(1, OP_ARITH_MUL);
}

static void divide(void)
{
	arith(1, OP_ARITH_DIV);
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

static void complexp(void)
{
	s_val = p_complexp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void realp(void)
{
	s_val = p_realp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void integerp(void)
{
	s_val = p_integerp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
}

static void exactp(void)
{
	s_val = p_exactp(p_car(s_args)) ? SEXPR_TRUE : SEXPR_FALSE;
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

static void check_params(SEXPR params)
{
	SEXPR pars, pars2, p, p2;

	/* check that all are symbols */
	pars = params;
	while (!p_nullp(pars)) {
		if (p_pairp(pars) && p_symbolp(p_car(pars)))
		{
			pars = p_cdr(pars);
		} else if (p_symbolp(pars)) {
			pars = SEXPR_NIL;
		} else {
			throw_err("bad syntax on procedure parameters");
		}
	}

	/* check that they don't repeat */
	for (pars = params; p_pairp(pars); pars = p_cdr(pars)) {
		p = p_car(pars);
		pars2 = p_cdr(pars);
		while (!p_nullp(pars2)) {
			if (p_pairp(pars2)) {
				p2 = p_car(pars2);
				pars2 = p_cdr(pars2);
			} else {
				p2 = pars2;
				pars2 = SEXPR_NIL;
			}
			if (p_eqp(p, p2)) {
				throw_err("parameter repeated on procedure");
			}
		}
	}
}

static void special(void)
{
	check_params(p_car(s_args));
	s_val = make_special(sexpr_index(p_cons(s_args, s_env)));
}

static void lambda(void)
{
	check_params(p_car(s_args));
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
		if (!setjmp(s_err_buf)) {
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
