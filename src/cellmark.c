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
#include "cellmark.h"
#include <assert.h>
#include <stdio.h>

enum { NCELLMARK = (NCELL / 32) + ((NCELL % 32) ? 1 : 0) };

/* We use 1 bit for each cell. */
static unsigned int s_cellmarks[NCELLMARK];

static void calc_w_i(int celli, int *w, int *i)
{
	int wt;

	chkrange(celli, NCELL);
	wt = celli >> 5;
	chkrange(wt, NCELLMARK);
	celli &= 31;
	*w = wt;
	*i = celli;
}

void mark_cell(int celli)
{
	int w;

	calc_w_i(celli, &w, &celli);
	s_cellmarks[w] |= (1 << celli);
}

int cell_marked(int celli)
{
	int w;

	calc_w_i(celli, &w, &celli);
	return s_cellmarks[w] & (1 << celli);
}

/* If a cell is unmarked, mark it and return 1; else 0. */
int if_cell_mark(int celli)
{
	int w;
	unsigned int mask;

	calc_w_i(celli, &w, &celli);
	mask = 1 << celli;
	if (!(s_cellmarks[w] & mask)) {
		s_cellmarks[w] |= mask;
		return 1;
	}

	return 0;
}

/* If a cell is marked, unmark it and return 1; else 0. */
int if_cell_unmark(int celli)
{
	int w;
	unsigned int mask;

	calc_w_i(celli, &w, &celli);
	mask = 1 << celli;
	if (s_cellmarks[w] & mask) {
		s_cellmarks[w] &= ~mask;
		return 1;
	}

	return 0;
}

void cellmark_init(void)
{
	printf("[cells: marks: %d, %u bytes]\n",
		NCELLMARK, NCELLMARK * sizeof(s_cellmarks[0]));
}
