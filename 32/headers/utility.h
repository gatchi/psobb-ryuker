#include "stdafx.h"

// Not sure yet, something to do with hashing passwords i think
void MDString( char* inString, char* outString );

// For grabbing mySQL info like in account_add.c and char_export.c.
int init( char* config_data, char* mySQL_Host, char* mySQL_Username, char* mySQL_password, char* mySQL_Database, unsigned int mySQL_Port );