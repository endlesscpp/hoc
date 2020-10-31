YFLAGS = -d	# force creation of y.tab.h
OBJS = hoc.o code.o init.o math.o symbol.o

hoc5:	$(OBJS)
	gcc $(OBJS) -lm -o hoc5

hoc.o:	hoc.h

code.o init.o symbol.o:	hoc.h y.tab.h


.PHONY: clean
clean:
	rm -f $(OBJS) hoc5 y.tab.[ch]
