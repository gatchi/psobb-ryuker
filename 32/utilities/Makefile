# 32-bit version

CFLAGS = -I./headers
COMPILE = gcc $(CFLAGS) -c

all: char_export account_add	
	
char_export: char_export.o utility.o md5.o
	gcc -o char_export char_export.o utility.o md5.o -L./mysql/lib -llibmysql

char_export.o: char_export.c
	$(COMPILE) char_export.c

account_add: account_add.o utility.o md5.o
	gcc -o account_add account_add.o utility.o md5.o -L./mysql/lib -llibmysql

account_add.o: account_add.c
	$(COMPILE) account_add.c

utility.o: utility.c
	$(COMPILE) utility.c

md5.o: md5.c
	$(COMPILE) md5.c

clean:
	rm *.o account_add.exe char_export.exe