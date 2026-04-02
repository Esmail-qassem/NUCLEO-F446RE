# ─────────────────────────────────────────────────────────────
# tests/Makefile  —  builds and runs Unity tests on host x86
#
# HOW TO USE:
#   make all    — compile all test runners
#   make run    — run all test runners, report results
#   make clean  — remove build artifacts
#
# Add new test files by appending to TEST_SRCS below.
# ─────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g \
           -I unity \
           -I mocks \
           -I ../APP/Inc \
           -I ../BTLD/Inc \
           -I ../BM/Inc \
           -DUNIT_TEST \
           -DSTM32F446xx

BUILD_DIR = build

# ── Add your test source files here ──────────────────────────
TEST_SRCS = \
    test_crc32.c \
    test_ota_header.c \
    test_adc_average.c

UNITY_SRC = unity/unity.c

RUNNERS = $(TEST_SRCS:%.c=$(BUILD_DIR)/%)

.PHONY: all run clean

all: $(BUILD_DIR) $(RUNNERS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Each test file compiles with unity + the mocks
$(BUILD_DIR)/%: %.c $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC)

run: all
	@PASS=0; FAIL=0; \
	for runner in $(RUNNERS); do \
	    echo "── $$runner ──────────────────────────"; \
	    $$runner; \
	    RC=$$?; \
	    if [ $$RC -eq 0 ]; then PASS=$$((PASS+1)); else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; \
	echo "══════════════════════════════════════"; \
	echo "  Results: $$PASS passed, $$FAIL failed"; \
	echo "══════════════════════════════════════"; \
	[ $$FAIL -eq 0 ]

clean:
	rm -rf $(BUILD_DIR)
