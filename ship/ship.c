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

#ifdef _WIN32
	// Starts off with setting the server installer window title
	unsigned char ctitle[30] = {0};
	sprintf (ctitle, "Ryuker Ship Server %s", SERVER_VERSION);
	SetConsoleTitle (ctitle);
	
	// Negotiate with winsock (windows networking) to get winsock data by providing version request
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(2,2), &winsock_data) )
	{
		printf ("Press [ENTER] to start setup...");
		fgets (dp, 2, stdin);
	}
	else
	{
		printf ("Could not negotiate with winsock.\n");
		printf ("Press [ENTER] to quit...");
		fgets (dp, 2, stdin);
		exit (1);
	}
#else
	printf ("\n");
	printf ("Welcome to Ryuker PSOBB server program.\n");
#endif
	
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
	
	// If config file exists
	FILE * cf = fopen (CONFIG_FILE, "r+");
	if (cf != NULL)
	{
		// shit happens here
		printf ("Config file found, press [ENTER] to continue.");
		gets (dp);
	}
	
	// If config file doesnt exist
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
	
	// Now lets load ship.json
	
	fclose (cf);
	
	/*
	   Then it appears to get data from a packet (logon packet?) for setting up network
	   parameters for the ship and for connecting to the logon server.  It then tries to connect.
	
	   Then it allocates memory for ship blocks
	
	   Then, much later after other files have loaded, it loads ban data
	
	   Then it opens up the ship ports for connections
	
	   Then what looks like tray icon loading
	
	   Then a MASSIVE for loop at the end, im guessing for actual ship stuff
	 */
	
	// A loop would go here with server things, but for now, lets just exit
	return 0;
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
