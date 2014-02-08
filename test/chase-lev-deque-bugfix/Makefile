include ../benchmarks.mk

all: main.o

main.o: deque.h main.c deque.c $(SPEC_OBJ)
	$(CC) -c -o deque.o deque.c $(CFLAGS)
	$(CC) -o $@ main.c deque.o $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

#%.o: %.c $(SPEC_OBJ)
	#$(CC) -c -o $@ $< $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
