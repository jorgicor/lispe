#ifndef ERR_H
#define ERR_H

#ifndef SETJMP_H
#define SETJMP_H
#include <setjmp.h>
#endif

extern jmp_buf s_err_buf; 

void throw_err(const char *s);

#endif
