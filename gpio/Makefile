# build an executable named gpiod-test and gpio-event
CC ?= gcc

all:  gpiod-test.c gpio-event-test.c
	$(CC) -lgpiod -o gpiod-test  gpiod-test.c
	$(CC) -o gpio-event  gpio-event-test.c

clean:
	rm -rf gpiod-test gpio-event .*.swp .*.un* *~
