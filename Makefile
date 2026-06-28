CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Iinclude -g

.PHONY: all test stress clean

all: bin/vmsim

bin/vmsim: src/main.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) src/main.c src/address.c -o bin/vmsim

test: bin/test_address
	./bin/test_address

bin/test_address: tests/test_address.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_address.c src/address.c -o bin/test_address

stress:
	@echo "Testes de estresse ainda não implementados."

clean:
	rm -rf bin build