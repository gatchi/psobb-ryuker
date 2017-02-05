/************************************************************************
	Ryuker Ship Server
	
	Based off of sodayboy's Tethealla ship server.
	(github.com/justnoxx/psobb-tethealla)
	
	Official Ryuker codebase host:
	(github.com/gatchi/psobb-ryuker)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3 as
	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

// OS specific includes
#ifdef _WIN32  // Windows
#include <windows.h>
#include <winsock2.h>
#include <process.h>
#elif __gnu_linux__  // Desktop Linux
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>   // What is this even for....
#include <string.h>  // These because gcc on my linux machine W H I N E S
#include <stdlib.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include "ship.h"

#define DEBUG 1

#define SERVER_VERSION      "(alpha)"
#define CONFIG_FILE         "ship.json"

#ifdef __gnu_linux__
#define SOCKET int
#define SOCKADDR struct sockaddr
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
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

int main (void)
{
	unsigned char dp[5] = {0};  // Dummy array for pressing enter in menus
	unsigned char c;            // Generic char for mostly key presses

#ifndef _WIN32
	printf ("\n");
#else
	// Starts off with setting the server installer window title
	unsigned char ctitle[30] = {0};
	sprintf (ctitle, "Ryuker Ship Server %s", SERVER_VERSION);
	SetConsoleTitle (ctitle);
#endif
	printf ("Welcome to Ryuker PSOBB server program.\n");
	
	/*
	   In the original file, this is the part where configuration files are loaded:
	   - ship.ini
	   - language file
	   - ship_key.bin
	   - weapon parameter file
	   - barrier parameter file
	   - technique parameter file
	   - battle parameter files
	   - param\ItemPT.gsl (common drop tables, created by SEGA, archive format)
	   - "EP1 data"
	   - "EP2 data"
	   - param\PlyLevelTbl.bin (level up stats and such, made by SEGA)
	      (uncompressed version found in login_server folder)
	   - quests
	   - shop.dat
	   - configure shop settings for each level (shop indices)
	
	   It also generates a mtwist best seed right before loading the config file (ship.ini), not sure y.
	   Its not actually used in the load_config function, nor in load_mask.
	 */
	
	// If config file exists, proceed
	FILE * cf = fopen (CONFIG_FILE, "r+");
	if (cf != NULL)
	{
		printf ("Config file found.\n");
		fclose (cf);
	}
	else // If config file doesnt exist, create config file as its required
	{
		// Prompt to create config file
		printf ("\nThe configuration file %s appears to be missing.\n", CONFIG_FILE);
		printf ("Create file? [Y/n]: ");  // Capital letter means default
		do
		{
			fgets (dp, 5, stdin);
			if (dp[0] == 'n')
			{
				exit (1);
			}
			if ((tolower(dp[0]) != 'y') && (dp[0] != '\n'))
				printf ("Create file? [Y/n]: ");
			
		} while ((tolower(dp[0]) != 'y') && (dp[0] != '\n'));
		
		// Generate ship config json //
		
		cJSON * shipj = cJSON_CreateObject();
		
		// Ship Name
		printf ("Ship name?: ");
		get_json_string (shipj, "shipname", 30, NULL);
		
		// Ship IP
		printf ("Ship IP? [AUTO]: ");
		get_json_string (shipj, "shipip", 15, "auto");
		
		// Number of Blocks
		printf ("Num of ship blocks? (1-10): ");
		get_json_num (shipj, "numblks", 5);
		
		// Ship Port
		printf ("Ship inital port? [5278]: ");
		get_json_num (shipj, "shiport", 5278);
		
		// Max Connections
		printf ("Max connections? (100-180000) (see INSTALL) [300]: ");
		get_json_num (shipj, "maxcon", 300);
		
		// Login Server IP
		printf ("Login server IP?: ");
		get_json_string (shipj, "lgnip", 15, NULL);
		
		// Event?
		//printf ("Any events happening? [NONE]:");
		
		// Bunch of others stuff im not gonna add in yet
		
		
		// Print to file
		FILE * nf = fopen (CONFIG_FILE, "w");
		char * jbuff = cJSON_Print (shipj);
		if (jbuff != NULL)
		{
			int len = strlen(jbuff);
			fwrite (jbuff, 1, len, nf);
			printf ("\nCreated %s.\n", CONFIG_FILE);
		}
		else
			printf ("Error: couldnt print json.\n");
		fclose (nf);
		printf ("Press [ENTER] to quit.");
		gets (dp);
	}
	
	/*
	 * Ok, lets connect to logon server.
	 *
	 * First we gotta setup the the ship server.
	 * In Windows, this is
	 * - make socket
	 * - bind socket
	 * - start listening
	 * - accept when client connects
	 *
	 * We will fill parameters using info from the ship json.
	 * This should always be the last section of code in main,
	 * as listening is a loop that shouldnt break unless
	 * - the server is shutdown by the admin (so almost never)
	 * - the server errors (and ideally immediately restarts)
	 *
	 * Im not sure yet how this works in non-Windows machines like
	 * Linux or Unix so as of now, this server only works on Windows.
	 */

	serve();  // Right now this is just making ship a very simple server

	return 1;  // If we get here we fucked up
}

void account_creation (void)
{
	unsigned char dp[1] = {0};
	
	// Create MySQL config json
	
	cJSON * cj = cJSON_CreateObject();
	unsigned char mysqlhost[20];  // Max lengths for gets() safety reasons
	unsigned char mysqldbn[30];
	unsigned char mysqluname[30];
	unsigned char mysqlpass[30] = {0};
	unsigned int mysqlport;
	unsigned char tbuff[10] = {0};  // For string to int conversion
	
	printf ("\nPlease enter the following values (you can change these later):\n");
	
	// MySQL Host
	printf ("MySQL host? [localhost]: ");
	gets (mysqlhost);
	if (mysqlhost[0] == '\0')
		cJSON_AddStringToObject (cj, "mysqlhost", "localhost");
	else
		cJSON_AddStringToObject (cj, "mysqlhost", mysqlhost);
	
	// MySQL Database Name
	printf ("MySQL database name? [pso_db]: ");
	gets (mysqldbn);
	if (mysqldbn[0] == '\0')
		cJSON_AddStringToObject (cj, "mysqldbname", "pso_db");
	else
		cJSON_AddStringToObject (cj, "mysqldbname", mysqldbn);
	
	// MySQL Username
	printf ("MySQL username? [pso]: ");
	gets (mysqluname);
	if (mysqluname[0] == '\0')
		cJSON_AddStringToObject (cj, "mysqluname", "pso");
	else
		cJSON_AddStringToObject (cj, "mysqluname", mysqlhost);
	
	// MySQL Password
	printf ("MySQL password?: ");
	gets (mysqlpass);
	cJSON_AddStringToObject (cj, "mysqlpass", mysqlpass);
	
	// MySQL Port
	printf ("MySQL port? [3306]: ");
	gets (tbuff);
	int filled = sscanf (tbuff, "%u", &mysqlport);
	if (filled < 1)
		cJSON_AddNumberToObject (cj, "mysqlport", 3306);
	else
		cJSON_AddNumberToObject (cj, "mysqlport", mysqlport);
}

void get_json_num (cJSON * j, unsigned char * name, signed int dnum)
{
	signed int x;
	unsigned char s[30];
	fgets (s, 30, stdin);
	int filled = sscanf (s, "%d", &x);
	if ((filled < 1) && (dnum > 0))
		cJSON_AddNumberToObject (j, name, dnum);
	else if ((filled < 1) && (dnum < 0))
		cJSON_AddNumberToObject (j, name, 1);
	else
		cJSON_AddNumberToObject (j, name, x);
}

void get_json_string (cJSON * j, unsigned char * name, unsigned short size, unsigned char * dname)
{
	unsigned char s[size];
	*s = 0;
	gets (s);
	if ((dname != NULL) && (s[0] == '\0'))
		cJSON_AddStringToObject (j, name, dname);
	else
		cJSON_AddStringToObject (j, name, s);
}

void networkerror (int error, unsigned char * text)
{
#ifdef _WIN32
	strcat (text, "%d\n");
	if (DEBUG) printf (text, WSAGetLastError());
#elif __gnu_linux__
	perror (text);
#endif
}

int serve ()
{	
#ifdef _WIN32  // Negotiate with winsock (windows sockets)
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(2,2), &winsock_data) )  // Send version number
		if (DEBUG) printf ("WSAStartup a success.\n");
	else
		if (DEBUG) printf ("Could not negotiate with winsock.\n");
#endif
	
	// Make a ship socket
	SOCKET ssock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int error = INVALID_SOCKET;
	if (ssock == error)
		networkerror (error, "Whoops, fucked up socket");
	else
		if (DEBUG) printf ("Socket created.\n");
	
	// Open ship config file cause it has network info
	unsigned char * js = (unsigned char *) calloc (144, sizeof (char));
	signed int i;
	FILE * fp;
	
	// Convert file to json string
	fp = fopen (CONFIG_FILE, "r");
	do
	{
		i = fgetc (fp);
		if (i != EOF) strcat (js, (char*)&i);
	} while (i != EOF);
	fclose (fp);
	
	// Grab ip address and port from json
	cJSON * j;
	j = cJSON_Parse (js);
	unsigned char *  ipaddr_s = cJSON_GetObjectItem (j, "shipip")->valuestring;
	int port = cJSON_GetObjectItem (j, "shiport") -> valueint;
	int max_clients = cJSON_GetObjectItem (j, "maxcon") -> valueint;
	
	// Setup ship sock address
	struct sockaddr_in ssa;
	ssa . sin_family = AF_INET;
	ssa . sin_addr.s_addr = inet_addr (ipaddr_s);
	ssa . sin_port = htons (port);
	
	// Bind socket
	int result = bind (ssock, (SOCKADDR*)&ssa, sizeof(ssa));
	error = SOCKET_ERROR;
	if (result == error)
		networkerror (error, "Binding fucked up: ");
	else
		printf ("Socket bound.\n");
	
	// Listen to socket
	result = listen (ssock, SOMAXCONN);
	if (result == error)
		networkerror (error, "Can't listen for some reason: ");
	else
		printf ("Listening.\n");
	
	// Accept connections
#ifdef _WIN32         // MULTITHREADED WINDOWS VERS.
	SOCKET csock;
	struct sockaddr_in csa;
	int csa_len = sizeof(csa);
	while (1)
	{
		csock = accept (ssock, (SOCKADDR*)&csa, &csa_len);
		if (csock == INVALID_SOCKET)
		{
			printf ("Accept failed: %d\n", WSAGetLastError());
			closesocket (csock);
			WSACleanup();
			printf ("Winsock shutdown.\n");
			return 1;
		}
		else
		{
			printf ("Socket accepted (%s).\n", inet_ntoa (csa.sin_addr));
			unsigned int threadid;
			HANDLE thandle = (HANDLE) _beginthreadex (NULL, 0, &startClientSesh, (void *) csock, 0, &threadid);
		}
	}
#endif

#ifdef __gnu_linux__  // LINUX SELECT VERS.
	fd_set readlist;           // List of sockets ("file descriptiors" cause linux lol)
	//int * csock = (int *) calloc (max_clients, sizeof(int));
	int csock[max_clients];
	int new_sock;
	int activity;
	struct sockaddr_in csa;
	int csa_len = sizeof(csa);
	unsigned char buffer[1025];
	int valread;  // What is this for...
	
	memset (csock, 0, sizeof (int) * max_clients);  // Zero out the array
	
	int b = 0;
	while (b == 0)
	{
		// Guess what.
		// Cause select() fucks up the flags you have to re-zero and re-add shit
		// at the beginning of the while loop.
		// Whats even the point of having a remove function...
		
		FD_ZERO (&readlist);         // Zero out the list
		FD_SET (ssock, &readlist);   // Add socket to list
		int maxrank = ssock;         // Used in select()
		
		for (i=0; i<max_clients; i++) // Add them back to the set 
		{
			if (csock[i] > 0) // If valid socket
			{
				FD_SET (csock[i], &readlist);
				
				if (csock[i] > maxrank)
					maxrank = csock[i];
			}
		}
		
		
		// Check for activity (loop)
		activity = select (maxrank + 1, &readlist, NULL, NULL, NULL);
		if ((activity < 0) && (errno!=EINTR)) 
			printf ("Select error: ");
		
		// Check on client connections
		for (i=0; i < max_clients; i++)
		{
			if (FD_ISSET (csock[i], &readlist))
			{
				// Here is where real client work happens
				// TODO
				
				valread = recv (csock[i], buffer, 1024, 0);
				
				// But also to check for disconnects (TODO: This isnt triggering!!)
				if (valread == 0)
				{
					getpeername (csock[i], (SOCKADDR*)&csa, (socklen_t*)&csa_len);
					printf ("Client left (%s)\n", inet_ntoa(csa.sin_addr));
					FD_CLR (csock[i], &readlist);
					close (csock[i]);
					csock[i] = 0;  // Free up the dead sock's spot
				}
				else if (valread < 0)
					printf ("Error reading from socket %d.\n", i);
				else  // Print the message
				{
					buffer[valread] = '\0';
					printf ("Message: %s\n", buffer);
				}
			}
		}
		
		printf ("Passed out client loop.\n");
		
		// Check ship socket for incoming connections
		if (FD_ISSET (ssock, &readlist));
		{
			new_sock = accept (ssock, (SOCKADDR*)&csa, (socklen_t*)&csa_len);
			error = INVALID_SOCKET;
			if (new_sock == error)
			{
				networkerror (error, "Accept failed: ");
				close (new_sock);
			}
			else // Preliminary acception, check if room
			{
				unsigned char * client_ip = inet_ntoa (csa.sin_addr);
				int cport = ntohs (csa.sin_port);
				if (DEBUG) printf ("Request to connect from (%s)....\n", client_ip);
				for (i=0; i < max_clients; i++)  // Find spot for client
				{
					if (csock[i] == 0) // If spot is empty, accept
					{
						csock[i] = new_sock;
						if (DEBUG) printf ("Accepted client socket %d (%s).\n", i, client_ip);
						FD_SET (new_sock, &readlist);
						if (new_sock > maxrank)
							maxrank = new_sock;
						break;
					}
				}
				if (i == max_clients) // If no empty spots found, reject
				{
					printf ("Blocked client (%s) from connecting: block full\n", client_ip);
					unsigned char * mesg = "Block is full, try again later.\n";
					if (send(new_sock, mesg, strlen(mesg), 0) != strlen(mesg) )
						perror ("send error: ");
					close (new_sock);
				}
			}
		}
		
		printf ("Passed by ship socket loop.\n");
	}
#endif
	
	// Cleanup
	shut_down (ssock);
	return 0;
}

#ifdef _WIN32
unsigned int __stdcall startClientSesh (void * socket)
{
	SOCKET csock = (SOCKET) socket;
	
	/*
	 * Ok, down here lets do some real things.
	 * 
	 * Its odd, im not exactly sure what the relationship with the ship
	 * and logon server is.
	 * It almost feels like the logon_server is acting on behalf of the client.
	 * But even then, why would the ship connect to the logon server, instead
	 * of the other way around....
	 *
	 * So something notable in the server loop, is that it waits for a new
	 * packet from the logon server (reconnects first if its connection has
	 * timed out).
	 */
	
	// Handle received data
	char recvbuff[512] = {0};
	int result;
	do
	{
		result = recv (csock, recvbuff, 512, 0);
		if (result > 0)
		{
			printf ("Got some stuff.\n");
		}
		else if (result == 0)
			printf ("Connection closed.\n");
		else
		{
			printf ("Connection error: %d\n", WSAGetLastError());
			return 1;
		}
	} while (result > 0);
	
	return 0;
}
#endif
