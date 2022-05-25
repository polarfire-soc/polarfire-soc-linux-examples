# build an executable named gpiod-test and gpio-event
CC ?= gcc
LDFLAGS = -lgpiod

all: gpiod-test.c gpiod-event.c
	$(CC) -o gpiod-test gpiod-test.c $(LDFLAGS)
	$(CC) -o gpiod-event gpiod-event.c $(LDFLAGS)

clean: 
	$(RM) gpiod-test
	$(RM) gpiod-event

