OBJS = lispe.o cells.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lispe.o cells.o: cells.h sexpr.h config.h

clean:
	rm -f $(OBJS)
