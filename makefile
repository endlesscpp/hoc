YFLAGS = -d	# force creation of y.tab.h
OBJS = hoc.o init.o math.o symbol.o

hoc3:	$(OBJS)
	gcc $(OBJS) -lm -o hoc3

hoc.o:	hoc.h

init.o symbol.o:	hoc.h y.tab.h


.PHONY: clean
clean:
	rm -f $(OBJS) hoc3 y.tab.[ch]
