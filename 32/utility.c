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

char* unicode_to_ascii (unsigned short* ucs)
{
	char *s,c;

	s = (char*) &chatBuf[0];
	while (*ucs != 0x0000)
	{
		c = *(ucs++) & 0xFF;
		if ((c >= 0x20) && (c <= 0x80))
			*(s++) = c;
		else
			*(s++) = 0x20;
	}
	*(s++) = 0x00;
	return (char*) &chatBuf[0];
}

void write_log (char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[ MAX_GM_MESG_LEN ];
	SYSTEMTIME rawtime;

	FILE *fp;

	GetLocalTime (&rawtime);
	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fp = fopen ( "ship.log", "a");
	if (!fp)
	{
		printf ("Unable to log to ship.log\n");
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}

void write_gm (char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[ MAX_GM_MESG_LEN ];
	SYSTEMTIME rawtime;

	FILE *fp;

	GetLocalTime (&rawtime);
	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fp = fopen ( "gm.log", "a");
	if (!fp)
	{
		printf ("Unable to log to gm.log\n");
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}

void debug_perror( char * msg )
{
	debug( "%s : %s\n" , msg , strerror(errno) );
}

void debug (char *fmt, ...)
{
#define MAX_MESG_LEN 1024

	va_list args;
	char text[ MAX_MESG_LEN ];

	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fprintf( stderr, "%s", text);
}


long calc_checksum (void* data, unsigned long size)
{
    long offset,y,cs = 0xFFFFFFFF;
    for (offset = 0; offset < (long)size; offset++)
    {
        cs ^= *(unsigned char*)((long)data + offset);
        for (y = 0; y < 8; y++)
        {
            if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
            else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
        }
    }
    return (cs ^ 0xFFFFFFFF);
}

/*
	Apparently this can be done with short strings using strol, stroll,
	and strtoimax (use 16 for base for that last one).
	If the input is longer than number-of-bits-in-the-longest-integer-type/4
	then this method is required for converting hex strings into byte arrays.
*/
unsigned char hex2byte ( char* hs )
{
	unsigned b;

	if ( hs[0] < 58 ) b = (hs[0] - 48); else b = (hs[0] - 55);
	b *= 16;
	if ( hs[1] < 58 ) b += (hs[1] - 48); else b += (hs[1] - 55);
	return (unsigned char) b;
}

/*
	A custom implementation of strlen for some reason.
*/
unsigned int wstrlen ( unsigned short* dest )
{
	unsigned int l = 0;
	while (*dest != 0x0000)
	{
		l+= 2;
		dest++;
	}
	return l;
}

/*
	Custom implementation of strcopy for some reason.
	But for short ints....
*/
void wstrcpy ( unsigned short* dest, const unsigned short* src )
{
	while (*src != 0x0000)
		*(dest++) = *(src++);
	*(dest++) = 0x0000;
}

/*
	Welp, saved the best for last apparently.
	I've never heard of "strcopy_char".  Probably because strings are
	NORMALLY a string of chars (i just realized that the above function
	takes in a short pointer, not a char pointer).
	Interestingly, this not only syncs the addresses of the source and the
	destination but it sets the next three addresses to 0... as if to pad.
	
	Something to note: chars are smaller than (and usually half the size of) shorts.
*/
void wstrcpy_char ( char* dest, const char* src )
{
	while (*src != 0x00)
	{
		*(dest++) = *(src++);
		*(dest++) = 0x00;
	}
	*(dest++) = 0x00;
	*(dest++) = 0x00;
}
