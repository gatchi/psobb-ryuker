# 32-bit version

CFLAGS = -I./headers

all: char_export account_add	
	
ship_server: ship_server.o prs.o
	gcc -o ship_server ship_server.o prs.o

ship_server.o: ./ship_server/ship_server.c
	gcc $(CFLAGS) ./ship_server/ship_server.c
	
prs.o: prs.c
	gcc $(CFLAGS) prs.cpp
	
char_export: char_export.o utility.o md5.o
	gcc -o char_export char_export.o utility.o md5.o -L./mysql/lib -llibmysql

char_export.o: char_export.c
	gcc $(CFLAGS) -c char_export.c

account_add: account_add.o utility.o md5.o
	gcc -o account_add account_add.o utility.o md5.o -L./mysql/lib -llibmysql

account_add.o: account_add.c
	gcc $(CFLAGS) -c account_add.c

utility.o: utility.c
	gcc $(CFLAGS) -c utility.c

md5.o: md5.c
	gcc $(CFLAGS) -c md5.c

clean:
	rm account_add *.o *.exe