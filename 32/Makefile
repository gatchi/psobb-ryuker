# 32-bit version

all: char_export account_add	
	
char_export: char_export.o utility.o md5.o
	gcc -o char_export char_export.o utility.o md5.o -L./mysql/lib -llibmysql

char_export.o: char_export.c
	gcc -I./headers -c char_export.c

account_add: account_add.o utility.o md5.o
	gcc -o account_add account_add.o utility.o md5.o -L./mysql/lib -llibmysql

account_add.o: account_add.c
	gcc -I./headers -c account_add.c

utility.o: utility.c
	gcc -I./headers -c utility.c

md5.o: md5.c
	gcc -I./headers -c md5.c

clean:
	rm account_add *.o *.exe