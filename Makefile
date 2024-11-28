check: test
	./test

test: test.o other.o

test.o: test.c jtckdint.h

other.o: other.c jtckdint.h

clean:
	rm -f test test.o other.o
