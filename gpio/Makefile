# build an executable named gpiod-test and gpio-event
CC ?= gcc

gpiod-test: gpiod-test.c
	$(CC) -lgpiod -o gpiod-test gpiod-test.c

gpio-event: gpio-event-test.c
	$(CC) -o gpio-event  gpio-event-test.c

all:  gpiod-test gpio-event
clean:
	rm -rf gpiod-test gpio-event .*.swp .*.un* *~
