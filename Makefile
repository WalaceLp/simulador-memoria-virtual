CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Iinclude -g

REPLACEMENT_SOURCES = \
	src/replacement/replacement.c \
	src/replacement/fifo.c \
	src/replacement/lru.c \
	src/replacement/clock.c \
	src/replacement/aging.c

CORE_SOURCES = \
	src/address.c \
	src/page_table.c \
	src/process.c

COMMON_SOURCES = \
	$(CORE_SOURCES) \
	src/physical_memory.c \
	src/virtual_memory.c \
	src/tlb.c \
	$(REPLACEMENT_SOURCES)

.PHONY: all test stress clean valgrind

all: bin/vmsim

bin/vmsim: src/main.c $(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		src/main.c \
		$(COMMON_SOURCES) \
		-o bin/vmsim

test: \
	bin/test_address \
	bin/test_page_table \
	bin/test_process \
	bin/test_physical_memory \
	bin/test_replacement \
	bin/test_tlb \
	bin/test_virtual_memory
	./bin/test_address
	./bin/test_page_table
	./bin/test_process
	./bin/test_physical_memory
	./bin/test_replacement
	./bin/test_tlb
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
	$(CORE_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_process.c \
		$(CORE_SOURCES) \
		-o bin/test_process

bin/test_physical_memory: \
	tests/test_physical_memory.c \
	src/physical_memory.c \
	$(CORE_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_physical_memory.c \
		src/physical_memory.c \
		$(CORE_SOURCES) \
		-o bin/test_physical_memory

bin/test_replacement: \
	tests/test_replacement.c \
	$(REPLACEMENT_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_replacement.c \
		$(REPLACEMENT_SOURCES) \
		-o bin/test_replacement

bin/test_tlb: \
	tests/test_tlb.c \
	src/tlb.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_tlb.c \
		src/tlb.c \
		-o bin/test_tlb

bin/test_virtual_memory: \
	tests/test_virtual_memory.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
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
		./bin/test_replacement
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_tlb
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--error-exitcode=1 \
		./bin/test_virtual_memory

clean:
	rm -rf bin build