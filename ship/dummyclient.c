/*******************************
Dummy client for dummy server.
*******************************/
#include <stdio.h>

#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#define SOCKET int
#endif

void shut_down (SOCKET socket)
{
#ifdef _WIN32
	closesocket (socket);
	WSACleanup();
	printf ("Winsock shutdown.\n");
#else
	close (socket);
#endif
}

int main()
{
#ifdef _WIN32
	// Negotiate with winsock (windows networking) to get winsock data by providing version request
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(2,2), &winsock_data) )
	{
		printf ("WSAStartup a success.\n");
	}
	else
	{
		printf ("Could not negotiate with winsock.\n");
	}
#endif
	
	// Make a socket
	SOCKET connectsock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
	if (connectsock == INVALID_SOCKET)
#else
	if (connectsock == -1)
#endif
	{
#ifdef _WIN32
		printf ("Whoops, fucked up socket: %ld\n", WSAGetLastError());
#else
		perror ("Whoops, fucked up socket: ");
#endif
	}
	else
		printf ("Socket created.\n");
	
	// Setup ship sockaddr
	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_addr.s_addr = inet_addr("127.0.0.1");
	ssa.sin_port = htons(5788);
	
	// Connect to ship socket
	int result = connect (connectsock, (struct sockaddr *) &ssa, sizeof(ssa));
	if (result < 0)
#ifdef _WIN32
		printf ("Can't connect: %d\n", WSAGetLastError());
#else
		perror ("Can't connect: ");
#endif
	else
		printf ("Connection made.\n");
	
	// Send data
	char * sendbuff = "Hey there, server.\n";
	char dbuff[5];
	do
	{
		// Wait for input
		printf ("Send data?\n");
		gets (dbuff);
		
		// Send data
		result = send (connectsock, sendbuff, (int) strlen(sendbuff), 0);
		if (result < 0)
#ifdef _WIN32
			printf ("Can't send: %d\n", WSAGetLastError());
#else
			perror ("Can't send: ");
#endif
		else
			printf ("Data sent\n");
	} while (1);
	
	// Receive data
	char recvbuff[512] = {0};
	do
	{
		result = recv (connectsock, recvbuff, 512, 0);
		if (result > 0)
		{
			printf ("Data received. Message = %s", recvbuff);
			
			/* // Close connection
			result = shutdown (connectsock, SD_SEND);
			if (result == SOCKET_ERROR)
				printf ("Shutdown failure: %d\n", WSAGetLastError());
			else
				printf ("Connection closed.\n"); */
		}
		else if (result == 0)
			printf ("Connection closed.\n");
		else
		{
#ifdef _WIN32
			printf ("Connection error: %d\n", WSAGetLastError());
#else
			perror ("Connectionn error: ");
#endif
			shut_down (connectsock);
			return(1);
		}
	} while (result > 0);
	
	shut_down (connectsock);
	return(0);
}
