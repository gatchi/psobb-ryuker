/****************************************************************/
/*	Author:	gatchi
/*	Date:	01/06/2017
/*	char_export.c :  Tool for exporting character data to a .DAT
/*	                   file (exported.dat) from the database.
/*
/*	History:
/*		07/22/2008  TC  First version... (by Sodaboy)
/*		01/06/2017		Start of Ryuker fork dev (by gatchi)
/*		01/07/2017		Cleaned up
/****************************************************************/


#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>

#include	"mysql/include/mysql.h"

int main( int argc, char * argv[] )
{
	char inputstr[255] = {0};
	MYSQL * myData;
	char myQuery[255] = {0};
	MYSQL_ROW myRow ;
	MYSQL_RES * myResult;
	int num_rows;
	unsigned gc_num, slot;

	char mySQL_Host[255] = {0};
	char mySQL_Username[255] = {0};
	char mySQL_Password[255] = {0};
	char mySQL_Database[255] = {0};
	unsigned int mySQL_Port;
	int config_index = 0;
	char config_data[255];
	unsigned ch;

	FILE* fp;
	FILE* cf;

	// The following reads a config file and sets some stuff up i guess?
	if ( ( fp = fopen ("tethealla.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file tethealla.ini appears to be missing.\n");
		return 1;
	}
	else
	{
		unsigned char len;
		while (fgets (config_data, 255, fp) != NULL) // This reads a full line
		{
			if (config_data[0] != '#')  // Symbol for comments, at beginning of line
			{
				// Strip the line feed from the string
				len = strlen (config_data);
				if (config_data[len-1] == '\n')
					config_data[--len]  = 0;
				
				switch (config_index)
				{
					// Use memccpy instead of strcpy cause len is known,
					// len+1 because null terminator
					case 0:
						memcpy (mySQL_Host, config_data, len+1);
						break;
					case 1:
						memcpy (mySQL_Username, config_data, len+1);
						break;
					case 2:
						memcpy (mySQL_Password, config_data, len+1);
						break;
					case 3:
						memcpy (mySQL_Database, config_data, len+1);
						break;
					case 4:
						mySQL_Port = strtol (config_data, NULL, 10);
						break;
					default:
						break;
				}
				if (config_index > 4)
					break;
				config_index++;
			}
		}
		fclose (fp);
	}
	
	if (config_index < 5)
	{
		printf ("tethealla.ini is missing info or corrupted.\n");
		return 1;
	}
	
	if ( (myData = mysql_init((MYSQL*) 0) ) && 
		mysql_real_connect( myData, mySQL_Host, mySQL_Username, mySQL_Password, NULL, mySQL_Port,
		NULL, 0 ) )
	{
		if ( mysql_select_db( myData, mySQL_Database ) < 0 ) {
			printf( "Can't select the %s database !\n", mySQL_Database ) ;
			mysql_close( myData ) ;
			return 2 ;
		}
	}
	else
	{
		printf( "Can't connect to the mysql server (%s) on port %d !\nmysql_error = %s\n",
			mySQL_Host, mySQL_Port, mysql_error(myData) ) ;

		mysql_close( myData ) ;
		return 1 ;
	}

	printf ("Tethealla Server Character Export Tool\n");
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
	printf ("Guild card #: ");
	scanf ("%s", inputstr );
	gc_num = atoi ( inputstr );
	printf ("Slot #: ");
	scanf ("%s", inputstr );
	slot = atoi ( inputstr );
	sprintf (myQuery, "SELECT * from character_data WHERE guildcard='%u' AND slot='%u'", gc_num, slot );
	
	// Check to see if that character exists.
	if ( ! mysql_query ( myData, myQuery ) )
	{
		myResult = mysql_store_result ( myData );
		num_rows = (int) mysql_num_rows ( myResult );
		if (num_rows)
		{
			myRow = mysql_fetch_row ( myResult );
			cf = fopen ("exported.dat", "wb");
			fwrite (myRow[2], 1, 14752, cf);
			fclose ( cf );
			printf ("Character exported!\n");
		}
		else
			printf ("Character not found.\n");
		mysql_free_result ( myResult );
	}
	else
	{
		printf ("Couldn't query the MySQL server.\n");
		return 1;
	}
	mysql_close( myData ) ;
	return 0;
}
