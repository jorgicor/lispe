#ifndef CELLMARK_H
#define CELLMARK_H

enum {
	CELL_FREE,
	CELL_CREATED,
	CELL_USED
};

void mark_cell(int celli, int mark);
int cell_mark(int celli);
int if_cell_mark(int celli, int ifmark, int thenmark);
void cellmark_init(void);

#endif
