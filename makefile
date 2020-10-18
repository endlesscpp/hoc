hoc1:	hoc.o
	gcc hoc.o -o hoc2

.PHONY: clean
clean:
	rm *.o hoc2
