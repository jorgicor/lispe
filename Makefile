OBJS = lex.o numbers.o symbols.o cells.o cellmark.o sexpr.o gcbase.o pred.o \
       parse.o lispe.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lex.o numbers.o symbols.o cells.o cellmark.o sexpr.o gcbase.o pred.o parse.o lispe.o: config.h 

sexpr.o cells.o gcbase.o parse.o pred.o lispe.o: sexpr.h

cells.o gcbase.o pred.o parse.o lispe.o: common.h

cellmark.o gcbase.o lispe.o: cellmark.h

lex.o parse.o lispe.o: lex.h

gcbase.c numbers.c symbols.c lispe.c: gc.h

symbols.c lispe.c: cbase.h

symbols.c sexpr.c gcbase.c lispe.c: symbols.h numbers.h

cells.c pred.c gcbase.c lispe.c: cells.h

clean:
	rm -f $(OBJS)
