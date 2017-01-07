/***************************************************************
/*	Author:	gatchi												
/*	Date:	12/26/16										
/*	accountadd.c :  Adds an account to the Ryuker PSO			
/*			server...											
/*															
/*	History:													
/*		07/22/2008  TC  First version...
/*		12/26/2016		Start of Ryuker fork dev				
/****************************************************************/

#include <windows.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include	"mysql/include/mysql.h"
#include	"md5.h"
#include	"utility.h"

int main( int argc, char * argv[] )
{
	// A bunch of stuff for account creation
	char inputstr[255] = {0};
	char username[17];
	char password[34];
	char password_check[17];
	char md5password[34] = {0};
	char email[255];
	char email_check[255];
	unsigned char ch;
	time_t regtime;
	unsigned reg_seconds;
	unsigned char max_fields;
	MYSQL * myData;
	char myQuery[255] = {0};
	MYSQL_ROW myRow ;
	MYSQL_RES * myResult;
	int num_rows, pw_ok, pw_same;
	unsigned guildcard_number;
	char mySQL_Host[255] = {0};
	char mySQL_Username[255] = {0};
	char mySQL_Password[255] = {0};
	char mySQL_Database[255] = {0};
	unsigned int mySQL_Port;
	int config_index = 0;  // Place in the config file, i think
	char config_data[255];
	
	unsigned char MDBuffer[17] = {0};

	FILE* fp;

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
	
	if ( (myData = mysql_init((MYSQL*) 0)) && 
		mysql_real_connect( myData, &mySQL_Host[0], &mySQL_Username[0], &mySQL_Password[0], NULL, mySQL_Port,
		NULL, 0 ) )
	{
		if ( mysql_select_db( myData, &mySQL_Database[0] ) < 0 ) {
			printf( "Can't select the %s database !\n", mySQL_Database ) ;
			mysql_close( myData ) ;
			return 2 ;
		}
	}
	else {
		printf( "Can't connect to the mysql server (%s) on port %d !\nmysql_error = %s\n",
			mySQL_Host, mySQL_Port, mysql_error(myData) ) ;

		mysql_close( myData ) ;
		return 1 ;
	}

	printf ("Tethealla Server Account Addition\n");
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	pw_ok = 0;
	while (!pw_ok)
	{
		printf ("New account's username: ");
		scanf ("%s", inputstr );
		if (strlen(inputstr) < 17)
		{
			sprintf (&myQuery[0], "SELECT * from account_data WHERE username='%s'", inputstr );
			// Check to see if that account already exists.
			//printf ("Executing MySQL query: %s\n", myQuery );
			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{
				myResult = mysql_store_result ( myData );
				num_rows = (int) mysql_num_rows ( myResult );
				if (num_rows)
				{
					printf ("There is already an account by that name on the server.\n");
					myRow = mysql_fetch_row ( myResult );
					max_fields = mysql_num_fields ( myResult );
					printf ("Row data:\n");
					printf ("-=-=-=-=-\n");
					for (ch=0;ch<max_fields;ch++)
						printf ("column %u = %s\n", ch, myRow[ch] );
				}
				else
					pw_ok = 1;
				mysql_free_result ( myResult );
			}
			else
			{
				printf ("Couldn't query the MySQL server.\n");
				return 1;
			}

		}
		else
			printf ("Desired account name length should be 16 characters or less.\n");
	}
	memcpy (&username[0], &inputstr[0], strlen (inputstr)+1);
	// Gunna use this to salt it up
	regtime = time(NULL);
	pw_ok = 0;
	while (!pw_ok)
	{
		printf ("New account's password: ");
		scanf ("%s", inputstr );
		if ( ( strlen (inputstr ) < 17 ) || ( strlen (inputstr) < 8 ) )
		{
			memcpy (&password[0], &inputstr[0], 17 );
			printf ("Verify password: ");
			scanf ("%s", inputstr );
			memcpy (&password_check[0], &inputstr[0], 17 );
			pw_same = 1;
			for (ch=0;ch<16;ch++)
			{
				if (password[ch] != password_check[ch])
					pw_same = 0;
			}
			if (pw_same)
				pw_ok = 1;
			else
				printf ("The input passwords did not match.\n");
		}
		else
			printf ("Desired password length should be 16 characters or less.\n");
	}
	pw_ok = 0;
	while (!pw_ok)
	{
		printf ("New account's e-mail address: ");
		scanf ("%s", inputstr );
		memcpy (&email[0], &inputstr[0], strlen (inputstr)+1 );
		// Check to see if the e-mail address has already been registered to an account.

		sprintf (&myQuery[0], "SELECT * from account_data WHERE email='%s'", email );
		//printf ("Executing MySQL query: %s\n", myQuery );
		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( myData );
			num_rows = (int) mysql_num_rows ( myResult );
			mysql_free_result ( myResult );
			if (num_rows)
				printf ("That e-mail address has already been registered to an account.\n");
		}
		else
		{
			printf ("Couldn't query the MySQL server.\n");
			return 1;
		}

		if (!num_rows)
		{
			printf ("Verify e-mail address: ");
			scanf ("%s", inputstr );
			memcpy (&email_check[0], &inputstr[0], strlen (inputstr)+1 );
			pw_same = 1;
			for (ch=0;ch<strlen(email);ch++)
			{
				if (email[ch] != email_check[ch])
					pw_same = 0;
			}
			if (pw_same)
				pw_ok = 1;
			else
				printf ("The input e-mail addresses did not match.\n");
		}
	}

	// Check to see if any accounts already registered in the database at all.
	sprintf (&myQuery[0], "SELECT * from account_data" );
	//printf ("Executing MySQL query: %s\n", myQuery );
	// Check to see if the e-mail address has already been registered to an account.
	if ( ! mysql_query ( myData, &myQuery[0] ) )
	{
		myResult = mysql_store_result ( myData );
		num_rows = (int) mysql_num_rows ( myResult );
		mysql_free_result ( myResult );
		printf ("There are %i accounts already registered on the server.\n", num_rows );
	}
	else
	{
		printf ("Couldn't query the MySQL server.\n");
		return 1;
	}

	reg_seconds = (unsigned) regtime / 3600L;
	ch = strlen (&password[0]);
	//_itoa (reg_seconds, &config_data[0], 10 );
	sprintf ( &config_data[0], "%d", reg_seconds );
	//Throw some salt in the game ;)
	sprintf (&password[ch], "_%s_salt", &config_data[0] );
	//printf ("New password = %s\n", password );
	MDString (&password[0], &MDBuffer[0] );
	for (ch=0;ch<16;ch++)
		sprintf (&md5password[ch*2], "%02x", (unsigned char) MDBuffer[ch]);
	md5password[32] = 0;
	if (!num_rows)
	{
		/* First account created is always GM. */
		guildcard_number = 42000001;

		sprintf (&myQuery[0], "INSERT into account_data (username,password,email,regtime,guildcard,isgm,isactive) VALUES ('%s','%s','%s','%u','%u','1','1')", username, md5password, email, reg_seconds, guildcard_number );
	}
	else
	{
		sprintf (&myQuery[0], "INSERT into account_data (username,password,email,regtime,isactive) VALUES ('%s','%s','%s','%u','1')", username, md5password, email, reg_seconds );
	}
	// Insert into table.
	//printf ("Executing MySQL query: %s\n", myQuery );

	if ( ! mysql_query ( myData, &myQuery[0] ) )
		printf ("Account successfully added to the database!");
	else
	{
		printf ("Couldn't query the MySQL server.\n");
		return 1;
	}
	mysql_close( myData ) ;

	return 0;
}
