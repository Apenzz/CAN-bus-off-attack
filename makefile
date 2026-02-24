CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g -I include -I test
TARGET  = can_sim.out
BUILD   = build

SRCS    = main.c \
          src/bus.c \
          src/ecu.c \
          src/frame.c \
          test/test_bus.c \
          test/test_ecu.c

OBJS    = $(patsubst %.c, $(BUILD)/%.o, $(SRCS))
DEPS    = $(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)
