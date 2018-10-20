/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "err.h"
#ifndef STDIO_H
#include <stdio.h>
#endif

jmp_buf s_err_buf; 

void throw_err(const char *s)
{
	printf("lispe: ** error **\n");
	if (s) {
		printf("lispe: %s\n", s);
	}
	longjmp(s_err_buf, 1);
}
