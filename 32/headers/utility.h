#include "stdafx.h"

// Not sure yet, something to do with hashing passwords i think */
void MDString( char* inString, char* outString );

/* For grabbing mySQL info like in account_add.c and char_export.c. */
int init( char* config_data, char* mySQL_Host, char* mySQL_Username, char* mySQL_password, char* mySQL_Database, unsigned int mySQL_Port );

/*
	Apparently this can be done with short strings using strol, stroll,
	and strtoimax (use 16 for base for that last one).
	If the input is longer than number-of-bits-in-the-longest-integer-type/4
	then this method is required for converting hex strings into byte arrays.
*/
unsigned char hex2byte ( char* hs );

/*
	A custom implementation of strlen for some reason.
*/
unsigned int wstrlen ( unsigned short* dest );

/*
	Custom implementation of strcopy for some reason.
	But for short ints....
*/
void wstrcpy ( unsigned short* dest, const unsigned short* src );

/*
	Welp, saved the best for last apparently.
	I've never heard of "strcopy_char".  Probably because strings are
	NORMALLY a string of chars (i just realized that the above function
	takes in a short pointer, not a char pointer).
	Interestingly, this not only syncs the addresses of the source and the
	destination but it sets the next three addresses to 0... as if to pad.
	
	Something to note: chars are smaller than (and usually half the size of) shorts.
*/
void wstrcpy_char ( char* dest, const char* src );

char* unicode_to_ascii (unsigned short* ucs);
void write_log (char *fmt, ...);
void write_gm (char *fmt, ...);
long calc_checksum (void* data, unsigned long size);
void debug_perror( char * msg );
void debug (char *fmt, ...);