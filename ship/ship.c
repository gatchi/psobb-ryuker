/************************************************************************
	Ryuker Ship Server

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
#include "cJSON.h"

#define SERVER_VERSION		"(alpha)"
#define CONFIG_FILE			"shipconfig.json"

int main( void )
{
	unsigned char dp[1] = {0};  // Dummy array for pressing enter in menus
	unsigned char c;  // Generic char for mostly key presses

#ifdef _WIN32
	// Starts off with giving the server installer this nice welcome message
	// dp is a temp char buffer, used throughout this whole file
	strcat (dp, "Ryuker Ship Server ");
	strcat (dp, SERVER_VERSION );
	
	// This looks to be a windows command (from windows.h maybe)
	SetConsoleTitle (dp);
	
	// Negotiate with winsock (windows networking) to get winsock data by providing version request
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(2,2), &winsock_data) )
	{
		printf ("Press [ENTER] to start setup...\n");
		gets (dp);
	}
	else
	{
		printf ("Could not negotiate with winsock.\n");
		printf ("Press [ENTER] to quit...");
		gets (dp);
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
	if (0)
	{
		// shit happens here
		printf ("Config file found, press [ENTER] to exit.");
		gets (dp);
		fclose (cf);
	}
	
	// If config file doesnt exist
	else
	{
		// Prompt to create config file
		printf ("\nThe configuration file %s appears to be missing.\n", CONFIG_FILE);
		printf ("Create file? [ENTER/q]: ");  // Capital letter means default
		c = 0;
		c = getchar();
		if (c != '\n')
		{
			exit (1);
		}
		
		// Generate config json
		
		cJSON * cj = cJSON_CreateObject();
		unsigned char mysqlhost[20];  // Max lengths for gets() safety reasons
		unsigned char mysqldbn[30];
		unsigned char mysqluname[30];
		unsigned char mysqlpass[30] = {0};
		unsigned int mysqlport;
		unsigned char tbuff[10] = {0};  // For string to int conversion
		
		//printf ("\nWelcome to the ship config creator.\n");
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
		
		
		// Print to file
		FILE * nf = fopen (CONFIG_FILE, "w");
		char * jbuff = cJSON_Print (cj);
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
	   Then it appears to get data from a packet (logon packet?) for setting up network
	   parameters for the ship and for connecting to the logon server.  It then tries to connect.
	
	   Then it allocates memory for ship blocks
	
	   Then, much later after other files have loaded, it loads ban data
	
	   Then it opens up the ship ports for connections
	
	   Then what looks like tray icon loading
	
	   Then a MASSIVE for loop at the end, im guessing for actual ship stuff
	 */
}
