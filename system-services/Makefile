# build an executable named system-services-example from system-services-example.c
CC ?= riscv64-unknown-linux-gnu-gcc
all: system-services-example.c 
	$(CC) -g -Wall -o system-services-example system-services-example.c

clean: 
	$(RM) system-services-example