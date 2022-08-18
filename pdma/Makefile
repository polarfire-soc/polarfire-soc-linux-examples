CC ?= gcc
INCLUDE = .
CFLAGS = -I$(INCLUDE) -Wall -Wpedantic
LIBS = -lm


DEPS = pdma-ex.c
OBJS = pdma-ex.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pdma-ex: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -rf *.o *~ pdma-ex

