/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "lex.h"
#include <string.h>
#include <ctype.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

/************************************************************/
/* input channels                                           */
/************************************************************/

struct input_channel *read_console(struct input_channel *ic)
{
	ic->file = stdin;
	return ic;
}

struct input_channel *read_file(struct input_channel *ic, FILE *fp)
{
	ic->file = fp;
	return ic;
}

int getc_from_channel(struct input_channel *ic)
{
	return fgetc(ic->file);
}

/************************************************************/
/* tokenizer for s-expressions                              */
/************************************************************/

struct tokenizer *tokenize(struct input_channel *ic, struct tokenizer *t)
{
	t->in = ic;
	t->tok.type = EOF;
	t->peekc = EOF;
	return t;
}

static int is_id_start(int c)
{
	return isalpha(c) ||
	       	(strchr("!$%&*+-./:<=>?@^_~", c) != NULL);
}

static int is_id_next(int c)
{
	return isdigit(c) || isalpha(c) ||
	       	(strchr("!$%&*+-./:<=>?@^_~", c) != NULL);
}

static int separator(int c)
{
	return c == EOF || isspace(c) || strchr("()[]", c);
}

/* Returns true if the conversion was performed */
static int convert_to_real(const char *p, real_t *result)
{
	char *ep;
	real_t nreal;

	errno = 0;
	nreal = r_strtod(p, &ep);
	if (*ep != '\0')
		return 0;
	*result = nreal;
	return 1;
}

static int convert_to_complex(const char *p, complex_t *result)
{
	char *ep;
	real_t realp;
	real_t imagp;

	errno = 0;
	imagp = 0;
	realp = r_strtod(p, &ep);
	if (p == ep) {
		realp = 0;
		if (*p == '+' && p[1] == 'i') {
			imagp = 1;
			ep += 2;
		} else if (*p == '-' && p[1] == 'i') {
			imagp = -1;
			ep += 2;
		}
	} else if (*ep == 'i') {
		imagp = realp;
		realp = 0;
		ep++;
	} else if (*ep == '+' || *ep == '-') {
		p = ep;
		imagp = r_strtod(p, &ep);
		if (p == ep) {
			imagp = (*p == '+') ? 1 : -1;
			ep++;
		}
		if (*ep != 'i') {
			return 0;
		}
		ep++;
	}

	if (*ep != '\0') {
		return 0;
	}

	*result = realp + imagp * I;
	return 1;
}

struct token *pop_token(struct tokenizer *t)
{
	int c, i;
	complex_t vcomplex;
	const char *p;

again:	
	if (t->peekc >= 0) {
		c = t->peekc;
		t->peekc = EOF;
	} else {
		c = getc_from_channel(t->in);
	}

	if (c == EOF) {
		t->tok.type = EOF;
	} else if (isspace(c)) {
		while (isspace(c)) {
			c = getc_from_channel(t->in);
		}
		t->peekc = c;
		goto again;
	} else if (c == ';') {
		while (c != '\n' && c != EOF) {
			c = getc_from_channel(t->in);
		}
		if (c == EOF) {
			t->tok.type = EOF;
		} else {
			goto again;
		}
	} else if (strchr("()[]'", c) != NULL) {
		t->tok.type = c;
	} else {
		i = 0;
		while (!separator(c) && i < MAX_NAME - 1) {
			t->tok.value.atom.name[i++] = c;
			c = getc_from_channel(t->in);
		}
		t->tok.value.atom.name[i] = '\0';
		t->tok.value.atom.len = i;
		if (!separator(c)) {
			/* TODO: issue warning: identifier truncated */
			while (!separator(c)) {
				c = getc_from_channel(t->in);
			}
		}
		t->peekc = c;
		p = t->tok.value.atom.name;
		/* TODO: make these checks more efficient: for numbers
		 * particulary...
		 */
		if (p[0] == '.' && p[1] == '\0' && separator(c)) {
			t->tok.type = '.';
		} else if (p[0] == '#' && p[1] == 't' && p[2] == '\0') {
			t->tok.type = T_TRUE;
		} else if (p[0] == '#' && p[1] == 'f' && p[2] == '\0') {
			t->tok.type = T_FALSE;
		} else if (convert_to_complex(p, &vcomplex)) {
			if (cimag(vcomplex) != 0) {
				t->tok.type = T_COMPLEX;
				t->tok.value.vcomplex = vcomplex;
			} else {
				t->tok.type = T_REAL;
				t->tok.value.vreal = creal(vcomplex);
			}
		} else {
			t->tok.type = T_ATOM;
		}
	}

	return &t->tok;
}

struct token *peek_token(struct tokenizer *t)
{
	return &t->tok;
}
