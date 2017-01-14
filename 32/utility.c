#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "utility.h"

void MDString (char * inString, char * outString)
{
	unsigned char c;
	MD5_CTX mdContext;
	unsigned int len = strlen (inString);

	MD5Init( &mdContext );
	MD5Update( &mdContext, inString, len );
	MD5Final( &mdContext );
	for (c=0;c<16;c++)
	{
		*outString = mdContext.digest[c];
		outString++;
	}
}

int init( char* config_data, char* mySQL_Host, char* mySQL_Username, char* mySQL_Password, char* mySQL_Database, unsigned int mySQL_Port )
{
	FILE* fp;
	int config_index = 0;  // Position in the config file
	
	// The following reads a config file to get MySQL params
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
	
	return 0;
}
