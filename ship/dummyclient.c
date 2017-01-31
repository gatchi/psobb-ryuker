#include <stdio.h>
#include <winsock2.h>

/*
 * Dummy client for dummy server.
 */
int main()
{
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
	
	// Make a socket
	SOCKET connectsock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectsock == INVALID_SOCKET)
	{
		printf ("Whoops, fucked up socket: %ld\n", WSAGetLastError());
	}
	else
		printf ("Socket created.\n");
	
	// Setup sockaddr
	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_addr.s_addr = inet_addr("127.0.0.1");
	ssa.sin_port = htons(5788);
	
	// Connect to socket
	int result = connect (connectsock, (SOCKADDR *) &ssa, sizeof(ssa));
	if (result == SOCKET_ERROR)
		printf ("Can't connect: %d\n", WSAGetLastError());
	else
		printf ("Connection made.\n");
	
	// Send data
	char * sendbuff = "Hey there, server.\n";
	result = send (connectsock, sendbuff, (int) strlen(sendbuff), 0);
	if (result == SOCKET_ERROR)
		printf ("Can't send: %d\n", WSAGetLastError());
	else
		printf ("Data sent\n");
	
	// Receive data
	char recvbuff[512] = {0};
	do
	{
		result = recv (connectsock, recvbuff, 512, 0);
		if (result > 0)
		{
			printf ("Data received. Message = %s", recvbuff);
			// Close connection
			result = shutdown (connectsock, SD_SEND);
			if (result == SOCKET_ERROR)
				printf ("Shutdown failure: %d\n", WSAGetLastError());
			else
				printf ("Connection closed.\n");
		}
		else if (result == 0)
			printf ("Connection closed.\n");
		else
			printf ("Connection error: %d\n", WSAGetLastError());
	} while (result > 0);
	
	closesocket (connectsock);
	WSACleanup();
	printf ("Winsock shutdown.\n");
	return(0);
}
