OBJS = lispe.o cells.o cellmark.o sexpr.o gcbase.o pred.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lispe.o: common.h config.h

cells.o cellmark.o sexpr.o: common.h config.h

gcbase.o pred.o: common.h config.h

clean:
	rm -f $(OBJS)
