CC=clang
CFLAGS=-Wall
TESTS_OBJ=test_exec_1.o test_exec_2.o

.INTERMEDIATE: main.o $(TESTS_OBJ) ../server.o ../program.o ../vm.o ../el.o ../utils.o

.PHONY: test

test: main.o ../server.o ../client.o ../program.o ../vm.o ../el.o ../utils.o $(TESTS_OBJ)
	$(CC) $(CFLAGS) -o main main.o ../server.o ../client.o ../program.o ../vm.o ../el.o ../utils.o $(TESTS_OBJ)
	./main

clean:
	rm -f main *.o
