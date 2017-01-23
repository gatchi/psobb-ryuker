# PSOBB Ryuker Server
# Ship Server Makefile, 32-bit version

CFLAGS = -I../headers
COMPILE = gcc $(CFLAGS) -c

all: ship_server
	
ship_server: ship_server.o prs.o utility.o network.o items.o character.o params.o account.o/
	           mag.o quest.o fileio.o pso_crypt.o packet.o commands.o mtwist.o
	gcc -o ship_server ship_server.o prs.o mtwist.o utility.o network.o items.o character.o/
	         mag.o quest.o fileio.o pso_crypt.o packet.o commands.o

ship_server.o: ship_server.c
	$(COMPILE) ship_server.c
	
prs.o: prs.cpp
	$(COMPILE) prs.cpp
	
mtwist.o: mtwist.c
	$(COMPILE) mtwist.c

network.o: network.c
	$(COMPILE) network.c

items.o: items.c
	$(COMPILE) items.c

character.o: character.c
	$(COMPILE) character.c

params.o: params.c
	$(COMPILE) params.c

account.o: account.c
	$(COMPILE) account.c

mag.o: mag.c
	$(COMPILE) mag.c

quest.o: quest.c
	$(COMPILE) quest.c

fileio.o: fileio.c
	$(COMPILE) fileio.c

pso_crypt.o: pso_crypt.c
	$(COMPILE) pso_crypt.c

packets.o: packet.c
	$(COMPILE) packet.c

commands.o: commands.c
	$(COMPILE) commands.c

utility.o: utility.c
	$(COMPILE) utility.c

md5.o: md5.c
	$(COMPILE) md5.c

clean:
	rm *.o ship_server.exe