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


