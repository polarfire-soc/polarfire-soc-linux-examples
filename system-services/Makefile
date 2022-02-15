# build an executable named system-services-example from system-services-example.c
CC ?= gcc
CFLAGS = -g -Wall -Wpedantic
LDFLAGS = -lcrypto

all: system-services-example.c signature-verification-demo.c
	$(CC) -o system-services-example system-services-example.c
	$(CC) -o signature-verification-demo signature-verification-demo.c -lcrypto

clean: 
	$(RM) system-services-example
	$(RM) signature-verification-demo

