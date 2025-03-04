CC=clang
CFLAGS=-Wall
INSTALL_PATH=/usr/bin
SERVER_NAME=netvm
CLIENT_NAME=netvm_repl
TESTS_DIR=tests

.INTERMEDIATE: netvm.o server.o client.o program.o vm.o el.o repl.o utils.o

.PHONY: test

all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): netvm.o server.o program.o vm.o el.o utils.o
	$(CC) $(CFLAGS) -o $(SERVER_NAME) netvm.o server.o program.o vm.o el.o utils.o

$(CLIENT_NAME): repl.o client.o program.o utils.o
	$(CC) $(CFLAGS) -o $(CLIENT_NAME) repl.o client.o program.o utils.o

test:
	make -C $(TESTS_DIR) test

clean:
	rm -f $(SERVER_NAME) $(CLIENT_NAME) *.o
	make -C $(TESTS_DIR) clean

install: $(SERVER_NAME)
	echo Installing executable to ${INSTALL_PATH}
	mv $(SERVER_NAME) ${INSTALL_PATH}

uninstall:
	echo Removing executable from ${INSTALL_PATH}
	rm ${INSTALL_PATH}/${SERVER_NAME}
