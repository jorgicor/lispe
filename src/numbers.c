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
	float number;
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

int install_number(float n)
{
	int i;

	dprintf("install number %f\n", n);
	if (s_free_nodes.next == -1) {
		printf("[gc: need numbers]\n");
		p_gc();
		if (s_free_nodes.next == -1) {
			goto fatal;
		}
	}
	i = s_free_nodes.next;
	s_free_nodes.next = s_numbers[i].next;
	dprintf("installed %f in %d\n", n, i);
	s_numbers[i].number = n;
	return i;

fatal:
	fprintf(stderr, "lispe: out of numbers\n");
	exit(EXIT_FAILURE);
	return 0;
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


float get_number(int i)
{
	chkrange(i, N_NUMBERS);
	return s_numbers[i].number;
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
