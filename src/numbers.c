#include "cfg.h"
#include "cbase.h"
#include "numbers.h"
#include "gc.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

enum { N_NUMBERS = NCELL };

union number_node {
	int next;
	struct number n;
};

static union number_node s_numbers[N_NUMBERS];
static union number_node s_free_nodes;

enum { N_NUM_MARKS = (N_NUMBERS / 32) + ((N_NUMBERS % 32) ? 1 : 0) };

static unsigned int s_num_marks[N_NUM_MARKS];

#ifdef DEBUG_NUMBERS
#define dprintf(...) printf(__VA_ARGS__) 
#else
#define dprintf(...)
#endif

static int pop_free_slot(void)
{
	int i;

	if (s_free_nodes.next == -1) {
		printf("[gc: need numbers]\n");
		p_gc();
		if (s_free_nodes.next == -1) {
			goto fatal;
		}
	}
	i = s_free_nodes.next;
	s_free_nodes.next = s_numbers[i].next;
	return i;

fatal:
	fprintf(stderr, "lispe: out of numbers\n");
	exit(EXIT_FAILURE);
	return 0;
}

int install_number(struct number *n)
{
	int i;

	// dprintf("install number %f\n", n);
	i = pop_free_slot();
	// dprintf("installed %f in %d\n", n, i);
	s_numbers[i].n = *n;
	return i;
}

void mark_number(int i)
{
	int w;

	dprintf("marked %d\n", i);
	chkrange(i, N_NUMBERS);
	w = i >> 5;
	chkrange(w, N_NUM_MARKS);
	i &= 31;
	s_num_marks[w] |= (1 << i);
}

static int number_marked(int i)
{
	int w;

	chkrange(i, N_NUMBERS);
	w = i >> 5;
	chkrange(w, N_NUM_MARKS);
	i &= 31;
	return s_num_marks[w] & (1 << i);
}

/* stop n copy */
void gc_numbers(void)
{
	int i;
	int nmarked;

	nmarked = 0;
	s_free_nodes.next = -1;
	for (i = 0; i < N_NUMBERS; i++) {
		if (!number_marked(i)) {
			s_numbers[i].next = s_free_nodes.next;
			s_free_nodes.next = i;
		} else {
			nmarked++;
		}
	}

	memset(s_num_marks, 0, sizeof(s_num_marks));
	printf("[gc: %d/%d numbers]\n", nmarked, N_NUMBERS);
}

int number_type(struct number *n)
{
	return n->type;
}

struct number *get_number(int i)
{
	chkrange(i, N_NUMBERS);
	return &s_numbers[i].n;
}

void init_numbers(void)
{
	int i;

	s_numbers[N_NUMBERS - 1].next = -1;
	for (i = 0; i < N_NUMBERS - 1; i++) {
		s_numbers[i].next = i + 1;
	}
	s_free_nodes.next = 0;

	printf("[numbers: %d, %u bytes, marks: %u bytes]\n", N_NUMBERS,
			sizeof(s_numbers), sizeof(s_num_marks));
}

void build_int_number(struct number *n, int i)
{
	n->type = NUM_INT;
	n->val.integer = i;
}

void build_real_number(struct number *n, float f)
{
	n->type = NUM_REAL;
	n->val.real = f;
}

void print_number(struct number *n)
{
	switch (n->type) {
	case NUM_INT:
		printf("%d", n->val.integer);
		break;
	case NUM_REAL:
		printf("%g", n->val.real);
		break;
	}
}

typedef void (*coer_fun)(struct number *, struct number *, struct number *);

static void coer_int_real(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_REAL;
	r->val.real = a->val.integer;
}

static const coer_fun coercion_table[N_NUM_TYPES][N_NUM_TYPES] = {
	{ NULL, coer_int_real },
	{ NULL, NULL },
};

static coer_fun get_coercion_fun(int typea, int typeb)
{
	return coercion_table[typea][typeb];
}

typedef void (*arith_fun)(struct number *, struct number *, struct number *);

static void add_int(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_INT;
	r->val.integer = a->val.integer + b->val.integer;
}

static void sub_int(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_INT;
	r->val.integer = a->val.integer - b->val.integer;
}

static void mul_int(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_INT;
	r->val.integer = a->val.integer * b->val.integer;
}

static void div_int(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_INT;
	r->val.integer = a->val.integer / b->val.integer;
}

static void add_real(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_REAL;
	r->val.real = a->val.real + b->val.real;
}

static void sub_real(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_REAL;
	r->val.real = a->val.real - b->val.real;
}

static void mul_real(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_REAL;
	r->val.real = a->val.real * b->val.real;
}

static void div_real(struct number *a, struct number *b, struct number *r)
{
	r->type = NUM_REAL;
	r->val.real = a->val.real / b->val.real;
}

static const arith_fun arith_table[N_NUM_TYPES][N_NUM_ARITH_OPS] = {
	{ add_int, sub_int, mul_int, div_int },
	{ add_real, sub_real, mul_real, div_real },
};

static arith_fun get_arith_fun(int type, int op)
{
	return arith_table[type][op];
}

void apply_arith_op(int op, struct number *a, struct number *b,
		    struct number *r)
{
	arith_fun opfn;
	coer_fun cfn;
	struct number cn;

	/* coerce: transform integers to reals if needed */
	if (a->type != b->type) {
		cfn = get_coercion_fun(a->type, b->type);
		if (cfn == NULL) {
			cfn = get_coercion_fun(b->type, a->type);
			assert(cfn != NULL);
			cfn(a, b, &cn);
			a = &cn;
		} else {
			cfn(b, a, &cn);
			b = &cn;
		}
	}

	opfn = get_arith_fun(a->type, op);
	assert(opfn);
	opfn(a, b, r);
}

typedef int (*logic_fun)(struct number *, struct number *);

static int equal_int(struct number *a, struct number *b)
{
	return a->val.integer == b->val.integer;
}

static int gt_int(struct number *a, struct number *b)
{
	return a->val.integer > b->val.integer;
}

static int lt_int(struct number *a, struct number *b)
{
	return a->val.integer < b->val.integer;
}

static int ge_int(struct number *a, struct number *b)
{
	return a->val.integer >= b->val.integer;
}

static int le_int(struct number *a, struct number *b)
{
	return a->val.integer <= b->val.integer;
}

static int equal_real(struct number *a, struct number *b)
{
	return a->val.real == b->val.real;
}

static int gt_real(struct number *a, struct number *b)
{
	return a->val.real > b->val.real;
}

static int lt_real(struct number *a, struct number *b)
{
	return a->val.real < b->val.real;
}

static int ge_real(struct number *a, struct number *b)
{
	return a->val.real >= b->val.real;
}

static int le_real(struct number *a, struct number *b)
{
	return a->val.real <= b->val.real;
}

static const logic_fun logic_table[N_NUM_TYPES][N_NUM_LOGIC_OPS] = {
	{ equal_int, gt_int, lt_int, ge_int, le_int },
	{ equal_real, gt_real, lt_real, ge_real, le_real },
};

static logic_fun get_logic_fun(int type, int op)
{
	return logic_table[type][op];
}

int apply_logic_op(int op, struct number *a, struct number *b)
{
	logic_fun opfn;
	coer_fun cfn;
	struct number cn;

	/* coerce: transform integers to reals if needed */
	if (a->type != b->type) {
		cfn = get_coercion_fun(a->type, b->type);
		if (cfn == NULL) {
			cfn = get_coercion_fun(b->type, a->type);
			assert(cfn != NULL);
			cfn(a, b, &cn);
			a = &cn;
		} else {
			cfn(b, a, &cn);
			b = &cn;
		}
	}

	opfn = get_logic_fun(a->type, op);
	assert(opfn);
	return opfn(a, b);
}

int numbers_eqv(struct number *a, struct number *b)
{
	if (number_type(a) == number_type(b)) {
		return apply_logic_op(OP_LOGIC_EQUAL, a, b);
	} else {
		return 0;
	}
}

void copy_number(struct number *src, struct number *dst)
{
	*dst = *src;
}
