# PSOBB Ryuker Server
# Top-level Makefile, 32-bit version

CFLAGS = -I./headers
COMPILE = gcc $(CFLAGS) -c

utility.o: utility.c
	$(COMPILE) utility.c

test: test.o
	gcc -o test test.o

network.o: ship_server/network.c
	$(COMPILE) ship_server/network.c

test.o: test.c
	$(COMPILE) test.c

clean:
	rm *.o test.exe
