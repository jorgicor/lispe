#ifndef COMMON_H
#define COMMON_H

/* sexpr.c */

/*
 * We look at the 4 left bits of type.
 * This gives SEXPR_CONS, SEXPR_SYMBOL, etc.
 * For:
 * SEXPR_NIL: 0.
 * SEXPR_CONS: bits(28..0) is index into cells.
 * SEXPR_BUILTIN_FUNCTION: bits(28..0) is index into table of builtin
 *                         functions.
 * SEXPR_BUILTIN_SPECIAL: bits(28..0) is index into table of builtin
 *                             special forms.
 * SEXPR_FUNCTION and
 * SEXPR_SPECIAL and
 * SEXPR_CLOSURE: bits(28..0) is index into cells.
 * SEXPR_NUMBER: bits(28..0) is index to a cell whose car is the floating
 *               number (the cdr we don't care).
 * SEXPR_SYMBOL: bits(28..0) is index to a cell whose car is the pointer
 *                to the struct literal (the cdr we don't care).
 */

typedef int SEXPR;

enum { SHIFT_SEXPR = 28 };

enum {
	SEXPR_NIL = 0,
	SEXPR_CONS = 1 << SHIFT_SEXPR,
	SEXPR_SYMBOL = 2 << SHIFT_SEXPR,
	SEXPR_NUMBER = 3 << SHIFT_SEXPR,
	SEXPR_BUILTIN_FUNCTION = 4 << SHIFT_SEXPR,
	SEXPR_BUILTIN_SPECIAL = 5 << SHIFT_SEXPR,
	SEXPR_FUNCTION = 6 << SHIFT_SEXPR,
	SEXPR_SPECIAL = 7 << SHIFT_SEXPR,
	SEXPR_CLOSURE = 8 << SHIFT_SEXPR,
};

enum {
	TYPE_MASK_SEXPR = ((unsigned int) -1) << SHIFT_SEXPR,
	INDEX_MASK_SEXPR = ~TYPE_MASK_SEXPR,
};

#define sexpr_type(e) ((e) & TYPE_MASK_SEXPR)
#define sexpr_index(e) ((e) & INDEX_MASK_SEXPR)
#define make_cons(celli) (SEXPR_CONS | (celli))
#define sexpr_eq(e1, e2) ((e1) == (e2))
#define make_builtin_function(table_index) \
	(SEXPR_BUILTIN_FUNCTION | (table_index))
#define make_builtin_special(table_index) \
	(SEXPR_BUILTIN_SPECIAL | (table_index))
#define make_closure(lambda_n_alist_celli) \
	(SEXPR_CLOSURE | (lambda_n_alist_celli))
#define make_function(args_n_body_celli) \
	(SEXPR_FUNCTION | (args_n_body_celli))
#define make_special(args_n_body_celli) \
	(SEXPR_SPECIAL | (args_n_body_celli))

int sexpr_equal(SEXPR e1, SEXPR e2);
float sexpr_number(SEXPR e);
const char* sexpr_name(SEXPR e);

SEXPR make_symbol(const char *s, int len);
SEXPR make_number(float n);

#if 0
int sexpr_type(SEXPR e);
int sexpr_index(SEXPR e);
int sexpr_eq(SEXPR e1, SEXPR e2);
SEXPR make_cons(int celli);
SEXPR make_builtin_function(int table_index);
SEXPR make_builtin_special(int table_index);
SEXPR make_function(int args_n_body_celli);
SEXPR make_special(int args_n_body_celli);
SEXPR make_closure(int lambda_n_alist_celli);
#endif

/* numbers.c */

int install_number(float n);
float get_number(int i);
void mark_number(int i);
void gc_numbers(void);
void init_numbers(void);

/* symbols.c */

int install_symbol(const char *s, int len);
const char *get_symbol(int i);
void mark_symbol(int i);
void gc_symbols(void);
void init_symbols(void);

/* cells.c */

/************************************************************/
/* private, only cells.c uses this directly                 */
/* exposed for efficiendy                                   */
/************************************************************/

struct cell {
	SEXPR car;
	SEXPR cdr;
};

extern struct cell s_cells[];

/************************************************************/
/* end of private section                                   */
/************************************************************/

#ifdef RANGE_CHECK

SEXPR cell_car(int celli);
SEXPR cell_cdr(int celli);
void set_cell_car(int celli, SEXPR care);
void set_cell_cdr(int celli, SEXPR cdre);

#else

#define cell_car(i) s_cells[i].car
#define cell_cdr(i) s_cells[i].cdr
#define set_cell_car(i, e) s_cells[i].car = e
#define set_cell_cdr(i, e) s_cells[i].cdr = e

#endif

void cells_init(void);

/* pred.c */

int p_null(SEXPR e);
int p_atom(SEXPR x);
int p_consp(SEXPR e);
int p_symbolp(SEXPR e);
int p_numberp(SEXPR e);
SEXPR p_car(SEXPR e);
SEXPR p_cdr(SEXPR e);
int p_eq(SEXPR x, SEXPR y);
int p_equal(SEXPR x, SEXPR y);
SEXPR p_setcar(SEXPR e, SEXPR val);
SEXPR p_setcdr(SEXPR e, SEXPR val);
SEXPR p_assoc(SEXPR x, SEXPR a);
SEXPR p_evcon(SEXPR c, SEXPR a);
SEXPR p_evlis(SEXPR m, SEXPR a);
SEXPR p_eval(SEXPR e, SEXPR a);
SEXPR p_add(SEXPR var, SEXPR val, SEXPR a);

void p_print(SEXPR sexpr);
void p_println(SEXPR sexpr);

/* gcbase.c */

extern SEXPR s_env;
extern SEXPR s_true_atom;
extern SEXPR s_quote_atom;
extern SEXPR s_rest_atom;

int pop_free_cell(void);
SEXPR p_cons(SEXPR first, SEXPR rest);

void clear_stack(void);
SEXPR push(SEXPR e);
void push2(SEXPR e1, SEXPR e2);
void push3(SEXPR e1, SEXPR e2, SEXPR e3);
void pop(void);
void popn(int n);
int stack_empty(void);

void gcbase_init(void);

/* parse.c */

struct parser {
	struct tokenizer *tokenizer;
	int sp;
};

struct parser *parse(struct tokenizer *t, struct parser *p);

enum {
	ERRORC_OK,
	ERRORC_EOF,
	ERRORC_SYNTAX,
};

SEXPR get_sexpr(struct parser *p, int *errorc);

/* lispe.c */

void throw_err(void);
SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a);
SEXPR apply_builtin_special(int i, SEXPR args, SEXPR a);
const char *builtin_function_name(int i);
const char *builtin_special_name(int i);
SEXPR lambda(SEXPR e, SEXPR a);

#endif
