CC ?= gcc
CFLAGS = -Wall
OBJ = japll-pi.o print.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

japll-pi: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) japll-pi
