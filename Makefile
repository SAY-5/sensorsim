CC ?= clang
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS ?= -lm

BUILD := build
BIN := $(BUILD)/sensorsim

SRCS := src/sensorsim.c src/sensor.c src/fault.c src/golden.c
OBJS := $(SRCS:src/%.c=$(BUILD)/%.o)

TESTS := tests/test_sensor.c tests/test_fault.c tests/test_golden.c
TEST_BINS := $(TESTS:tests/%.c=$(BUILD)/%)

.PHONY: all test clean
all: $(BIN)

$(BIN): $(OBJS) | $(BUILD)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/test_sensor: tests/test_sensor.c src/sensor.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_fault: tests/test_fault.c src/fault.c src/sensor.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_golden: tests/test_golden.c src/golden.c src/sensor.c src/fault.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "=== $$t ==="; $$t || exit $$?; done

clean:
	rm -rf $(BUILD)
