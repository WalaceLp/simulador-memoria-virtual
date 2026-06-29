CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Iinclude -g

.PHONY: all test stress clean

all: bin/vmsim

bin/vmsim: src/main.c src/address.c src/page_table.c src/process.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		src/main.c \
		src/address.c \
		src/page_table.c \
		src/process.c \
		-o bin/vmsim

test: bin/test_address bin/test_page_table bin/test_process
	./bin/test_address
	./bin/test_page_table
	./bin/test_process

bin/test_address: tests/test_address.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_address.c \
		src/address.c \
		-o bin/test_address

bin/test_page_table: tests/test_page_table.c src/page_table.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_page_table.c \
		src/page_table.c \
		src/address.c \
		-o bin/test_page_table

bin/test_process: tests/test_process.c src/process.c src/page_table.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_process.c \
		src/process.c \
		src/page_table.c \
		src/address.c \
		-o bin/test_process

stress:
	@echo "Testes de estresse ainda não implementados."

clean:
	rm -rf bin build