# PSOBB Ryuker Server
# Ship Server Makefile

CFLAGS = -I../shared
COMPILE = gcc $(CFLAGS) -c

ifeq ($(OS), Windows_NT)
	NETLIB = -lws2_32
else
	MATHLIB = -lm
endif


all: ship
	
ship: ship.o cJSON.o
	gcc -o ship ship.o $(NETLIB) cJSON.o $(MATHLIB)

ship.o: ship.c
	$(COMPILE) ship.c

cJSON.o: ../shared/cJSON.c
	$(COMPILE) ../shared/cJSON.c

test: test.c
	gcc -o test test.c

clean:
	rm *.o

cleanest:
	rm *.o ship_server.exe
