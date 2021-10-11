CC ?= gcc

all: gpio-test gpio-event

gpio-test:
	$(CC) -lgpiod -o gpiod-test  gpiod-test.c
	
gpio-event:
	$(CC) -o gpio-event  gpio-event-test.c
clean:
	rm -rf gpiod-test gpio-event .*.swp .*.un* *~
