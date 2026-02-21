CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

TARGET_ROM  := beavernes
SRCS_ROM    := main.c rom_loader.c cartridge.c mapper.c mapper_0.c mapper_2.c
OBJS_ROM    := $(SRCS_ROM:.c=.o)

TARGET_INPUT := input_test
SRCS_INPUT   := input_test.c controller.c input.c
OBJS_INPUT   := $(SRCS_INPUT:.c=.o)

all: $(TARGET_ROM) $(TARGET_INPUT)

$(TARGET_ROM): $(OBJS_ROM)
	$(CC) $(OBJS_ROM) -o $@ $(LDFLAGS)

# IMPORTANT: explicit input_test rule
$(TARGET_INPUT): CFLAGS += $(SDL_CFLAGS)
$(TARGET_INPUT): $(OBJS_INPUT)
	$(CC) $(OBJS_INPUT) -o $@ $(SDL_LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET_ROM) $(TARGET_INPUT) $(OBJS_ROM) $(OBJS_INPUT)

run-input: $(TARGET_INPUT)
	./$(TARGET_INPUT)

.PHONY: all clean run-input