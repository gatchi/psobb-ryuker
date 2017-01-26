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

#define SERVER_VERSION "(alpha)"

int main( void )
{
	unsigned char dp[1000] = {0};

#ifdef _WIN32
	// Starts off with giving the server installer this nice welcome message
	// dp is a temp char buffer, used throughout this whole file
	strcat (dp, "Ryuker Ship Server ");
	strcat (dp, SERVER_VERSION );
	
	// This looks to be a windows command (from windows.h maybe)
	SetConsoleTitle (dp);
	
	// Negotiate with winsock (windows networking) to get winsock data by providing version request
	WSADATA winsock_data;
	if ( !WSAStartup(MAKEWORD(1,0), &winsock_data) )
	{
		printf ("Recieved winsock data.\n");
		printf ("Winsock version %x.%x\n", winsock_data.wVersion, winsock_data.wHighVersion);
	}
	else
		printf ("Could not negotiate with winsock.\n");
	
	printf ("Press [ENTER] to quit...");
	gets(dp);
	
	// In the original file, this is the part where configuration files are loaded:
	//  ship.ini
	//  language file
	//  ship_key.bin
	//  weapon parameter file
	//  barrier parameter file
	//  technique parameter file
	//  battle parameter files
	//  param\ItemPT.gsl
	//  "EP1 data"
	//  "EP2 data"
	//  param\PlyLevelTbl.bin
	//  quests
	//  shop.dat
	//  configure shop settings for each level (shop indices)
	
	// Then it appears to get data from a packet (logon packet?) for setting up network
	// parameters for the ship and for connecting to the logon server.  It then tries to connect.
	
	// Then it allocates memory for ship blocks
	
	// Then, much later after other files have loaded, it loads ban data
	
	// Then it opens up the ship ports for connections
	
	// Then what looks like tray icon loading
	
	// Then a MASSIVE for loop at the end, im guessing for actual ship stuff
#endif
}
