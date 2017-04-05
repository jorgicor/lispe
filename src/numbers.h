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

#ifndef NUMBERS_H
#define NUMBERS_H

#ifndef CFG_H
#include "cfg.h"
#endif

/************************************************************
 * PRIVATE (exposed ony for efficiency)
 * Only numbers.c uses this representation directly.
 * The rest of the code use the functions and macros here
 * listed.
 ************************************************************/

struct number {
	int type;
	union {
		real_t vreal;
		complex_t vcomplex;
	} val;
};

/************************************************************/
/* end of private section                                   */
/************************************************************/

enum {
	OP_ARITH_ADD, OP_ARITH_SUB, OP_ARITH_MUL, OP_ARITH_DIV,
       	N_NUM_ARITH_OPS,
};

enum {
	OP_LOGIC_EQUAL, OP_LOGIC_GT, OP_LOGIC_LT, OP_LOGIC_GE, OP_LOGIC_LE,
       	N_NUM_LOGIC_OPS,
};

void build_real_number(struct number *n, real_t f);
void build_complex_number(struct number *n, complex_t d);
void print_number(struct number *n);
void apply_arith_op(int op, struct number *a, struct number *b,
	       	    struct number *r);
int apply_logic_op(int op, struct number *a, struct number *b);
int numbers_eqv(struct number *a, struct number *b);
void copy_number(struct number *src, struct number *dst);
int exact_number(struct number *n);
int number_integer(struct number *n);
int number_real(struct number *n);
int number_complex(struct number *n);

int install_number(struct number *n);
struct number *get_number(int i);
void mark_number(int i);
void gc_numbers(void);
void init_numbers(void);

#endif
