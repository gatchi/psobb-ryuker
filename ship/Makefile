# PSOBB Ryuker Server
# Ship Server Makefile, 32-bit version

CFLAGS = -I../shared
COMPILE = gcc $(CFLAGS) -c

all: ship
	
ship: ship.o cJSON.o
	gcc -o ship ship.o -lws2_32 cJSON.o

ship.o: ship.c
	$(COMPILE) ship.c

cJSON.o: cJSON.c
	$(COMPILE) cJSON.c

test: test.c
	gcc -o test test.c

clean:
	rm *.o

cleanest:
	rm *.o ship_server.exe
