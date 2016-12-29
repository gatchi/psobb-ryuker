account_add: account_add.o utility.o
	gcc -o account_add account_add.o utility.o -L./mysql/lib-64 -llibmysql

account_add.o: account_add.c
	gcc -I./headers -c account_add.c

utility.o: utility.c
	gcc -I./headers -c utility.c

clean:
	rm account_add *.o