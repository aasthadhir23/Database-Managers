
CC := gcc
SRC :=  dberror.c storage_mgr.c test_assign1_1.c
OBJ :=  dberror.o storage_mgr.o test_assign1_1.o

build: $(OBJ)
	$(CC) -o test_assign1 $?

%.o: %.c
	$(CC) -g -c $<

run: build
	./test_assign1

clean:
	rm -rf test_assign1 *.o


