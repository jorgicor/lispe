OBJS = lispe.o sexpr.o symtab.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lispe.o sexpr.o symtab.o: common.h

clean:
	rm -f $(OBJS)
