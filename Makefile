OBJS = lispe.o
CFLAGS = -g

lispe: $(OBJS)
	cc -Wall $(OBJS) -o lispe

lispe.o: common.h

clean:
	rm -f $(OBJS)
