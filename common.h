#ifndef COMMON_H
#define COMMON_H

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
