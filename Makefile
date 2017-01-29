OBJS = lex.o numbers.o symbols.o cells.o cellmark.o sexpr.o gcbase.o pred.o \
       parse.o lispe.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lex.o numbers.o symbols.o cells.o cellmark.o sexpr.o gcbase.o pred.o parse.o lispe.o: config.h 

cells.o sexpr.o gcbase.o pred.o parse.o lispe.o: common.h

cellmark.o sexpr.o gcbase.o lispe.o: cellmark.h

lex.o parse.o lispe.o: lex.h

gcbase.c numbers.c symbols.c lispe.c: gc.h

symbols.c lispe.c: cbase.h

clean:
	rm -f $(OBJS)
