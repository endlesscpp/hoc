hoc1:	hoc.o
	gcc hoc.o -o hoc1

.PHONY: clean
clean:
	rm *.o hoc1
