CC = gcc
CFLAGS = -Wall -Werror -Iinclude

SRC = src
OBJS = $(SRC)/main.o $(SRC)/cpu.o $(SRC)/bus.o $(SRC)/memory.o $(SRC)/opcodes.o
TARGET = emulator

# ROM loader + cartridge + mapper objects
ROM_OBJS = \
    beaverNES-anjelica-dev/rom_loader.o \
    beaverNES-anjelica-dev/cartridge.o \
    beaverNES-anjelica-dev/mapper.o \
    beaverNES-anjelica-dev/mapper_0.o \
    beaverNES-anjelica-dev/mapper_2.o

all: $(TARGET)

$(TARGET): $(OBJS) $(ROM_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(ROM_OBJS) -o $(TARGET)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build ROM loader + cartridge + mapper objects
beaverNES-anjelica-dev/%.o: beaverNES-anjelica-dev/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(SRC)/*.o $(TARGET) cpu_test \
		beaverNES-anjelica-dev/*.o

.PHONY: test
test:
	mkdir -p $(SRC)/tests/bin
	@for f in $(SRC)/tests/*.c; do \
        name=$$(basename $$f .c); \
        $(CC) $(CFLAGS) $$f \
            $(SRC)/cpu.c $(SRC)/bus.c $(SRC)/memory.c $(SRC)/opcodes.c \
            $(ROM_OBJS) \
            -o $(SRC)/tests/bin/$$name || exit 1; \
        $(SRC)/tests/bin/$$name || exit 1; \
    done

# Standalone CPU test target
cpu_test: cpu_test.c $(SRC)/cpu.o $(SRC)/bus.o $(SRC)/memory.o $(SRC)/opcodes.o $(ROM_OBJS)
	$(CC) $(CFLAGS) cpu_test.c \
    	$(SRC)/cpu.o $(SRC)/bus.o $(SRC)/memory.o $(SRC)/opcodes.o \
        $(ROM_OBJS) \
        -o cpu_test
