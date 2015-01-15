include ../benchmarks.mk

all: main.o testcase.o testcase1.o backup_main.o

main.o: my_queue.h main.c my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ main.c my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

testcase.o: my_queue.h testcase.c my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ testcase.c my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

testcase1.o: my_queue.h testcase1.c my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ testcase1.c my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

backup_main.o: backup_my_queue.h backup_main.c backup_my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ backup_main.c backup_my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
