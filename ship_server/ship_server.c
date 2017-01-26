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

#include <windows.h>
#include <stdio.h>

#define SERVER_VERSION "(alpha)"

int main( void )
{
	unsigned char dp[1000] = {0};

	// Starts off with giving the server installer this nice welcome message
	// dp is a temp char buffer, used throughout this whole file
	strcat (dp, "Ryuker Ship Server ");
	strcat (dp, SERVER_VERSION );
	
	// This looks to be a windows command (from windows.h maybe)
	SetConsoleTitle (dp);
	
	printf ("Press [ENTER] to quit...");
	gets(dp);
}
