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

MEMORY_SOURCES = \
	src/physical_memory.c \
	src/virtual_memory.c \
	src/tlb.c \
	src/swap.c

COMMON_SOURCES = \
	$(CORE_SOURCES) \
	$(MEMORY_SOURCES) \
	$(REPLACEMENT_SOURCES)

.PHONY: all test stress clean valgrind belady validate

all: bin/vmsim bin/tracegen

bin/vmsim: \
	src/main.c \
	src/cli.c \
	src/csv_export.c \
	src/trace.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		src/main.c \
		src/cli.c \
		src/csv_export.c \
		src/trace.c \
		$(COMMON_SOURCES) \
		-o bin/vmsim

bin/tracegen: \
	src/tracegen_main.c \
	src/trace_generator.c \
	src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		src/tracegen_main.c \
		src/trace_generator.c \
		src/address.c \
		-o bin/tracegen

test: \
	bin/test_address \
	bin/test_page_table \
	bin/test_process \
	bin/test_physical_memory \
	bin/test_replacement \
	bin/test_tlb \
	bin/test_swap \
	bin/test_virtual_memory \
	bin/test_trace \
	bin/test_swap_integration \
	bin/test_cli \
	bin/test_csv_export \
	bin/test_trace_generator \
	bin/test_stress
	./bin/test_address
	./bin/test_page_table
	./bin/test_process
	./bin/test_physical_memory
	./bin/test_replacement
	./bin/test_tlb
	./bin/test_swap
	./bin/test_virtual_memory
	./bin/test_trace
	./bin/test_swap_integration
	./bin/test_cli
	./bin/test_csv_export
	./bin/test_trace_generator
	./bin/test_stress

bin/test_address: \
	tests/test_address.c \
	src/address.c
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

bin/test_swap: \
	tests/test_swap.c \
	src/swap.c \
	src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_swap.c \
		src/swap.c \
		src/address.c \
		-o bin/test_swap

bin/test_virtual_memory: \
	tests/test_virtual_memory.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_virtual_memory.c \
		$(COMMON_SOURCES) \
		-o bin/test_virtual_memory

bin/test_trace: \
	tests/test_trace.c \
	src/trace.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_trace.c \
		src/trace.c \
		$(COMMON_SOURCES) \
		-o bin/test_trace

bin/test_swap_integration: \
	tests/test_swap_integration.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_swap_integration.c \
		$(COMMON_SOURCES) \
		-o bin/test_swap_integration

bin/test_cli: \
	tests/test_cli.c \
	src/cli.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_cli.c \
		src/cli.c \
		-o bin/test_cli

bin/test_csv_export: \
	tests/test_csv_export.c \
	src/csv_export.c \
	src/cli.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_csv_export.c \
		src/csv_export.c \
		src/cli.c \
		$(COMMON_SOURCES) \
		-o bin/test_csv_export

bin/test_trace_generator: \
	tests/test_trace_generator.c \
	src/trace_generator.c \
	src/address.c
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		tests/test_trace_generator.c \
		src/trace_generator.c \
		src/address.c \
		-o bin/test_trace_generator

bin/test_stress: \
	tests/test_stress.c \
	$(COMMON_SOURCES)
	@mkdir -p bin
	$(CC) $(CFLAGS) \
		-Isrc/replacement \
		tests/test_stress.c \
		$(COMMON_SOURCES) \
		-o bin/test_stress

belady: all
	./scripts/run_belady.sh

stress: all bin/test_stress
	./bin/test_stress
	./scripts/run_stress.sh

valgrind: test
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_address
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_page_table
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_process
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_physical_memory
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_replacement
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_tlb
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_swap
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_virtual_memory
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_trace
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_swap_integration
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_cli
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_csv_export
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_trace_generator
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		./bin/test_stress

validate:
	./scripts/run_final_validation.sh

clean:
	rm -rf bin build