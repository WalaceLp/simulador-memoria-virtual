CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Iinclude -g

COMMON_SOURCES = \
	src/address.c \
	src/page_table.c \
	src/process.c \
	src/physical_memory.c \
	src/virtual_memory.c

.PHONY: all test stress clean valgrind

all: bin/vmsim

bin/vmsim: src/main.c $(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		src/main.c \
		$(COMMON_SOURCES) \
		-o bin/vmsim

test: \
	bin/test_address \
	bin/test_page_table \
	bin/test_process \
	bin/test_physical_memory \
	bin/test_virtual_memory
	./bin/test_address
	./bin/test_page_table
	./bin/test_process
	./bin/test_physical_memory
	./bin/test_virtual_memory

bin/test_address: tests/test_address.c src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_address.c \
		src/address.c \
		-o bin/test_address

bin/test_page_table: \
	tests/test_page_table.c \
	src/page_table.c \
	src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_page_table.c \
		src/page_table.c \
		src/address.c \
		-o bin/test_page_table

bin/test_process: \
	tests/test_process.c \
	src/process.c \
	src/page_table.c \
	src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_process.c \
		src/process.c \
		src/page_table.c \
		src/address.c \
		-o bin/test_process

bin/test_physical_memory: \
	tests/test_physical_memory.c \
	src/physical_memory.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_physical_memory.c \
		src/physical_memory.c \
		-o bin/test_physical_memory

bin/test_virtual_memory: \
	tests/test_virtual_memory.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_virtual_memory.c \
		$(COMMON_SOURCES) \
		-o bin/test_virtual_memory

stress:
	@echo "Testes de estresse ainda não implementados."

valgrind: test
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_page_table
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_process
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_physical_memory
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_virtual_memory

clean:
	rm -rf bin build