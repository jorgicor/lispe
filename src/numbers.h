#ifndef NUMBERS_H
#define NUMBERS_H

/************************************************************
 * PRIVATE (exposed ony for efficiency)
 * Only numbers.c uses this representation directly.
 * The rest of the code use the functions and macros here
 * listed.
 ************************************************************/

struct number {
	int type;
	union {
		float real;
		int integer;
	} val;
};

/************************************************************/
/* end of private section                                   */
/************************************************************/

enum {
	NUM_INT, NUM_REAL,
       	N_NUM_TYPES,
};

enum {
	OP_ARITH_ADD, OP_ARITH_SUB, OP_ARITH_MUL, OP_ARITH_DIV,
       	N_NUM_ARITH_OPS,
};

enum {
	OP_LOGIC_EQUAL, OP_LOGIC_GT, OP_LOGIC_LT, OP_LOGIC_GE, OP_LOGIC_LE,
       	N_NUM_LOGIC_OPS,
};

void build_int_number(struct number *n, int i);
void build_real_number(struct number *n, float f);
int number_type(struct number *n);
void print_number(struct number *n);
void apply_arith_op(int op, struct number *a, struct number *b,
	       	    struct number *r);
int apply_logic_op(int op, struct number *a, struct number *b);
int numbers_eqv(struct number *a, struct number *b);
void copy_number(struct number *src, struct number *dst);
int exact_number(struct number *n);
int number_int(struct number *n);
int number_real(struct number *n);

int install_number(struct number *n);
struct number *get_number(int i);
void mark_number(int i);
void gc_numbers(void);
void init_numbers(void);

#endif
