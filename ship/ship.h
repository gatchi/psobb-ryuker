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

#include "cJSON.h"

const int debug = 1;

/*
 * Requests text from the user (stdin) and then puts it into the
 * the supplied json.
 *  
 * j      - the json to add the number to
 * name   - name of the json item
 * s      - string that will be used if the user only presses [ENTER].
 *          If less than 0, it assumes no default value.
 */
void get_json_string (cJSON * j, unsigned char * name, unsigned short size, unsigned char * s);

/*
 * Requests a num from the user (stdin) and then puts it into the
 * the supplied json.
 *  
 * j      - the json to add the number to
 * name   - name of the json item
 * dnum   - number that will be used if the user only presses [ENTER].
 */
void get_json_num (cJSON * j, unsigned char * name, signed int dnum);

/*
 * Starts the ship server.
 * 
 * returns  0 if shutdown normally
 *          1 if accept error
 */
int serve ();

/*
 * Function called when a new client socket is accepted.
 *
 * socket   - socket to process
 *
 * returns  0 if normal close
 *          1 if error
 */
unsigned int __stdcall startClientSesh (void * socket);
