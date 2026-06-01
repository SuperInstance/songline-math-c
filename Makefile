CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRCS = songline.c navigation.c corroboree.c tradition.c songline_api.c
OBJS = $(SRCS:.c=.o)
TEST  = test_runner

.PHONY: all test clean

all: libsongline.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libsongline.a: $(OBJS)
	ar rcs $@ $^

$(TEST): test.c libsongline.a
	$(CC) $(CFLAGS) -o $@ test.c -L. -lsongline $(LDFLAGS)

test: $(TEST)
	./$(TEST)

clean:
	rm -f *.o *.a $(TEST)
