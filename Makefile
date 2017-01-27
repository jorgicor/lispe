OBJS = lispe.o cells.o cellmark.o sexpr.o gcbase.o pred.o parse.o lex.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lispe.o: lex.h common.h config.h

cells.o cellmark.o sexpr.o: common.h config.h

gcbase.o pred.o: common.h config.h

parse.o: lex.h common.h config.h

lex.o: lex.h

clean:
	rm -f $(OBJS)
