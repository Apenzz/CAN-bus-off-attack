CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = can_sim.out

SRCS = main.c bus.c ecu.c frame.c test_bus.c test_ecu.c
OBJS = $(SRCS:.c=.o)
HEADERS = bus.h ecu.h frame.h test_bus.h test_ecu.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
