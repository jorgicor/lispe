/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#ifndef SYMBOLS_H
#define SYMBOLS_H

int install_symbol(const char *s, int len);
const char *get_symbol(int i);
void mark_symbol(int i);
void gc_symbols(void);
void init_symbols(void);

#endif
