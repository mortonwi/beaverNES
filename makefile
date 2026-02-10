CC = gcc
CFLAGS = -Wall -Werror -Iinclude

SRC = src
OBJS = $(SRC)/main.o $(SRC)/cpu.o $(SRC)/bus.o $(SRC)/memory.o $(SRC)/opcodes.o
TARGET = emulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(SRC)/*.o $(TARGET)

.PHONY: test
test:
	mkdir -p $(SRC)/tests/bin
	@for f in $(SRC)/tests/*.c; do \
		name=$$(basename $$f .c); \
		$(CC) $(CFLAGS) $$f $(SRC)/cpu.c $(SRC)/bus.c $(SRC)/memory.c $(SRC)/opcodes.c -o $(SRC)/tests/bin/$$name || exit 1; \
		$(SRC)/tests/bin/$$name || exit 1; \
	done

