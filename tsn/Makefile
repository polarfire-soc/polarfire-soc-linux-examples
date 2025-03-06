# build an executable named microchip-tsn-cli and microchip-tsn-replace-tsnbasetime
# build a library named libmicrochiptsn.so
CC ?= gcc
LDFLAGS_APP = -lcjson -lm
LDFLAGS_LIB = -fPIC -shared 

all: microchip-tsn-cli.c microchip-tsn-lib.c
	$(CC) -o microchip-tsn-cli microchip-tsn-cli.c microchip-tsn-lib.c $(LDFLAGS_APP)
	$(CC) -o microchip-tsn-replace-tsnbasetime microchip-tsn-replace-tsnbasetime.c $(LDFLAGS_APP)
	$(CC) -o libmicrochiptsn.so microchip-tsn-cli.c $(LDFLAGS_LIB)

clean: 
	$(RM) microchip-tsn-cli
	$(RM) microchip-tsn-replace-tsnbasetime
	$(RM) libmicrochiptsn.so

