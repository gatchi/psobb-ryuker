ship_server.exe: $(SHIP_OBJS)

login_server.exe: $(LOGIN_OBJS)

patch_server.exe: $(PATCH_OBJS)

account_add: account_add.o
	gcc -o account_add account_add.o -L./mysql/lib-64 -llibmysql

account_add.o: account_add.c stdafx.cpp
	gcc -c account_add.c stdafx.cpp

clean:
	rm account_add *.o