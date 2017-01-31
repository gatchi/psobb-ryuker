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

#ifdef _WIN32  // Windows specific includes
#include <windows.h>
#include <winsock2.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include "ship.h"

#define SERVER_VERSION		"(alpha)"
#define CONFIG_FILE			"ship.json"

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
	
	// If config file doesnt exist, create config file as its required
	else
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
		printf ("Max connections? (100-180000) (see INSTALL)");
		get_json_num (shipj, "maxcon", 300);
		
		// Login Server IP
		printf ("Login server IP?:");
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
#ifdef _WIN32
	serve();
#endif

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
	strcat (text, ": %d\n");
	if (debug) printf (text, WSAGetLastError());
}

int serve ()
{	
	// Negotiate with winsock (windows networking) to get winsock data by providing version request
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(2,2), &winsock_data) )
		if (debug) printf ("WSAStartup a success.\n");
	else
		if (debug) printf ("Could not negotiate with winsock.\n");
	
	// Make a socket
	SOCKET listensock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int error = INVALID_SOCKET;
	if (listensock == error)
		networkerror (error, "Whoops, fucked up socket");
	else
		if (debug) printf ("Socket created.\n");
	
	// Open ship config file
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
	//printf (js);
	j = cJSON_Parse (js);
	unsigned char *  ipaddr_s = cJSON_GetObjectItem (j, "shipip")->valuestring;
	int port = cJSON_GetObjectItem (j, "shiport") -> valueint;
	
	// Setup sockaddr
	struct sockaddr_in ssa;
	ssa . sin_family = AF_INET;
	ssa . sin_addr.s_addr = inet_addr (ipaddr_s);
	ssa . sin_port = htons (port);
	
	// Bind socket
	int result = bind (listensock, (SOCKADDR *) &ssa, sizeof(ssa));
	error = SOCKET_ERROR;
	if (result == error)
		networkerror (error, "Binding fucked up");
	else
		printf ("Socket bound.\n");
	
	// Listen to socket
	result = listen (listensock, SOMAXCONN);
	if (result == error)
		networkerror (error, "Can't listen for some reason: %d\n");
	else
		printf ("Listening.\n");
	
	// Accept connections
	SOCKET thesock = accept (listensock, NULL, NULL);
	if (thesock == INVALID_SOCKET)
		printf ("Accept failed: %d\n", WSAGetLastError());
	else
		printf ("Socket accepted.\n");
	
	// Receive data
	char recvbuff[512] = {0};
	do
	{
		result = recv (thesock, recvbuff, 512, 0);
		if (result > 0)
		{
			printf ("Bytes received: %d\n", result);
			printf ("Message: %s", recvbuff);
			// Send back a message
			char * mesg = "I gotchu\n";
			int mresult = send (thesock, mesg, (int) strlen(mesg), 0);
			if (mresult == SOCKET_ERROR)
				printf ("Couldnt reply: %d\n", WSAGetLastError());
			else
				printf ("Sent reply.\n");
		}
		else if (result == 0)
			printf ("Connection closed.\n");
		else
			printf ("Connection error: %d\n", WSAGetLastError());
	} while (result > 0);
	
	closesocket (thesock);
	WSACleanup();
	printf ("Winsock shutdown.\n");	
	return 0;
}
