# ============================================================
# Agile Modbus - Build System
# ============================================================
# Targets:
#   make lib          - Build static library (default)
#   make test         - Build and run unit tests
#   make size-report  - Show ROM/RAM resource usage
#   make clean        - Remove build artifacts
#   make format       - Format code with clang-format
#   make format-check - Check code format
# ============================================================

CC       ?= gcc
AR       ?= ar
SIZE     ?= size
# Strict flags for upstream source (no -Wconversion: upstream style)
CFLAGS   := -std=c99 -Wall -Wextra -Wpedantic -Werror \
            -Wno-unused-parameter \
            -Iinc -Iutil
# Strict flags for our own code
CFLAGS_STRICT := $(CFLAGS) -Wconversion -Wshadow -Wdouble-promotion
LDFLAGS  :=

BUILD    := build
OBJDIR   := $(BUILD)/obj
LIBDIR   := $(BUILD)/lib
TESTDIR  := $(BUILD)/tests

SRC      := src/agile_modbus.c src/agile_modbus_rtu.c src/agile_modbus_tcp.c \
            util/agile_modbus_slave_util.c
OBJ      := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRC)))
LIB      := $(LIBDIR)/libagile_modbus.a

TEST_SRC := $(wildcard tests/*.c)
TEST_BIN := $(patsubst tests/%.c,$(TESTDIR)/%,$(TEST_SRC))

CHECK_CFLAGS := $(shell pkg-config --cflags check 2>/dev/null)
CHECK_LIBS   := $(shell pkg-config --libs check 2>/dev/null || echo -lcheck -lm -lpthread -lrt -lsubunit)

.PHONY: all lib test clean size-report format format-check

all: lib

# ---- Library ----
lib: $(LIB)

$(LIB): $(OBJ) | $(LIBDIR)
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -c $< -o $@

$(OBJDIR)/%.o: util/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

$(LIBDIR):
	mkdir -p $@

# ---- Tests ----
test: $(TEST_BIN)
	@echo "=== Running unit tests ==="
	@pass=0; fail=0; total=0; \
	for t in $(TEST_BIN); do \
		echo "--- Running: $$t ---"; \
		$$t || { fail=$$((fail+1)); }; \
		total=$$((total+1)); \
	done; \
	echo ""; \
	echo "=== Results: $$total test binaries executed ==="

$(TESTDIR)/%: tests/%.c $(LIB) | $(TESTDIR)
	$(CC) $(CFLAGS_STRICT) $(CFLAGS_EXTRA) $(CHECK_CFLAGS) $< -L$(LIBDIR) -lagile_modbus $(CHECK_LIBS) -o $@

$(TESTDIR):
	mkdir -p $@

# ---- Size Report ----
size-report: $(OBJ) $(LIB)
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║            Agile Modbus Resource Usage Report               ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "┌─────────────────────────────────────────────────────────────┐"
	@echo "│  Object Files (.o)                                         │"
	@echo "├──────────────────┬──────────┬──────────┬──────────┬─────────┤"
	@echo "│ Module           │   Text   │   Data   │    BSS   │  Total  │"
	@echo "├──────────────────┼──────────┼──────────┼──────────┼─────────┤"
	@total_text=0; total_data=0; total_bss=0; total_all=0; \
	$(SIZE) $(OBJ) | tail -n +2 | while read text data bss dec hex name; do \
		name=$$(basename "$$name" .o); \
		total_text=$$((total_text + text)); \
		total_data=$$((total_data + data)); \
		total_bss=$$((total_bss + bss)); \
		total_all=$$((total_all + dec)); \
		printf "│ %-16s │ %7d  │ %7d  │ %7d  │ %7d │\n" "$$name" "$$text" "$$data" "$$bss" "$$dec"; \
		echo "$$total_text $$total_data $$total_bss $$total_all" > /tmp/size_totals; \
	done; \
	if [ -f /tmp/size_totals ]; then \
		read total_text total_data total_bss total_all < /tmp/size_totals; \
		rm -f /tmp/size_totals; \
	fi; \
	echo "├──────────────────┼──────────┼──────────┼──────────┼─────────┤"; \
	printf "│ %-16s │ %7d  │ %7d  │ %7d  │ %7d │\n" "TOTAL" "$$total_text" "$$total_data" "$$total_bss" "$$total_all"; \
	echo "└──────────────────┴──────────┴──────────┴──────────┴─────────┘"
	@echo ""
	@echo "┌─────────────────────────────────────────────────────────────┐"
	@echo "│  Summary                                                   │"
	@echo "├─────────────────────────────┬───────────────────────────────┤"
	@eval "$$( $(SIZE) $(OBJ) | tail -n +2 | awk '{t+=$$1; d+=$$2; b+=$$3} END{printf "tt=%d; td=%d; tb=%d", t, d, b}' )"; \
	rom=$$((tt + td)); ram=$$((td + tb)); \
	printf "│  ROM (.text + .data)         │  %6d bytes                  │\n" "$$rom"; \
	echo "├─────────────────────────────┼───────────────────────────────┤"; \
	printf "│  RAM (.data + .bss)          │  %6d bytes                  │\n" "$$ram"; \
	echo "├─────────────────────────────┼───────────────────────────────┤"; \
	printf "│  Code (.text)                │  %6d bytes                  │\n" "$$tt"; \
	echo "├─────────────────────────────┼───────────────────────────────┤"; \
	printf "│  Init data (.data)           │  %6d bytes                  │\n" "$$td"; \
	echo "├─────────────────────────────┼───────────────────────────────┤"; \
	printf "│  Zero-init (.bss)            │  %6d bytes                  │\n" "$$tb"; \
	echo "└─────────────────────────────┴───────────────────────────────┘"
	@echo ""
	@echo "Notes:"
	@echo "  ROM = .text (code) + .data (init values, copied to RAM at startup)"
	@echo "  RAM = .data (init values) + .bss (zero-init values)"
	@echo ""

# ---- Clean ----
clean:
	rm -rf $(BUILD)
	rm -f *.gcda *.gcno *.gcov coverage.info

# ---- Format ----
format:
	@echo "Formatting code..."
	clang-format -i $(SRC) $(wildcard util/*.c) $(wildcard inc/*.h) $(wildcard util/*.h) $(wildcard tests/*.c)
	@echo "Done"

format-check:
	@echo "Checking format..."
	@clang-format --dry-run -Werror $(SRC) $(wildcard util/*.c) $(wildcard inc/*.h) $(wildcard util/*.h) $(wildcard tests/*.c) && echo "OK" || echo "Run 'make format' to fix"

help:
	@echo "Targets: lib | test | size-report | format | format-check | clean | help"
