CC=clang
CFLAGS=-Wall
TESTS_OBJ=test_exec_1.o test_exec_2.o test_exec_3.o test_exec_4.o

.INTERMEDIATE: tests.o $(TESTS_OBJ) ../server.o ../client.o ../program.o ../vm.o ../el.o ../utils.o

.PHONY: test

test: tests
	./tests

tests: tests.o ../server.o ../client.o ../program.o ../vm.o ../el.o ../utils.o $(TESTS_OBJ)
	$(CC) $(CFLAGS) -o tests tests.o ../server.o ../client.o ../program.o ../vm.o ../el.o ../utils.o $(TESTS_OBJ)

clean:
	rm -f tests *.o
