include ../benchmarks.mk

all: main.o backup_main.o

main.o: my_queue.h main.c my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ main.c my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

backup_main.o: backup_my_queue.h backup_main.c backup_my_queue.c $(SPEC_OBJ)
	$(CC) -o $@ backup_main.c backup_my_queue.c $(SPEC_OBJ) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
