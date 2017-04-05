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

#ifndef LEX_H
#define LEX_H

#ifndef CFG_H
#include "cfg.h"
#endif

#ifndef STDIO_H
#define STDIO_H
#include <stdio.h>
#endif

struct input_channel {
	FILE *file;
};

struct input_channel *read_console(struct input_channel *ic);
struct input_channel *read_file(struct input_channel *ic, FILE *fp);
int getc_from_channel(struct input_channel *ic);

enum {
	T_ATOM = 1024,
	T_INTEGER,
	T_REAL,
	T_COMPLEX,
	T_TRUE,
	T_FALSE
};

enum {
	MAX_NAME = 32
};

struct token {
	int type;
	union {
		struct {
			char name[MAX_NAME];
			int len;
		} atom;
		real_t vreal;
		complex_t vcomplex;
	} value;
};

struct tokenizer {
	struct input_channel *in;
	struct token tok;
	int peekc;
};

struct tokenizer *tokenize(struct input_channel *ic, struct tokenizer *t);
struct token *pop_token(struct tokenizer *t);
struct token *peek_token(struct tokenizer *t);

#endif
