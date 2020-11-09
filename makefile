YFLAGS = -d	# force creation of y.tab.h
CFLAGS = -g -O0
OBJS = hoc.o code.o init.o math.o symbol.o

hoc6:	$(OBJS)
	gcc $(OBJS) -lm -o hoc6

hoc.o:	hoc.h

code.o init.o symbol.o:	hoc.h y.tab.h


.PHONY: clean
clean:
	rm -f $(OBJS) hoc6 y.tab.[ch]
