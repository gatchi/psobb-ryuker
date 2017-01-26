# PSOBB Ryuker Server
# Ship Server Makefile, 32-bit version

CFLAGS = -I../headers
COMPILE = gcc $(CFLAGS) -c

all: ship_server
	
ship_server: ship_server.o
	gcc -o ship_server ship_server.o -lws2_32

ship_server.o: ship_server.c
	$(COMPILE) ship_server.c

clean:
	rm *.o ship_server.exe