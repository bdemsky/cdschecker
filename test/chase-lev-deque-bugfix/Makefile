include ../benchmarks.mk

all: main.o testcase.o

deque.o: deque.c $(SPEC_OBJ)
	$(CC) -c -o deque.o deque.c $(CFLAGS)

main.o: main.c deque.o $(SPEC_OBJ)
	$(CC) -o $@ main.c deque.o $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

testcase.o: testcase.o deque.o $(SPEC_OBJ)
	$(CC) -o $@ testcase.c deque.o $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

#%.o: %.c $(SPEC_OBJ)
	#$(CC) -c -o $@ $< $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
