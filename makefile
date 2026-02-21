CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 `sdl2-config --cflags`
LDFLAGS := `sdl2-config --libs`

TARGET  := beavernes

SRCS := main.c \
        rom_loader.c \
        cartridge.c \
        mapper.c \
        mapper_0.c \
        mapper_2.c \
        controller.c \
        input.c

OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run