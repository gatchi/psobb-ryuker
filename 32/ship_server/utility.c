#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For write_log
#include <stdarg.h> 	// for va_list and such
#include <time.h> 		// for time stuff
#include <windows.h>	// ...

#include "md5.h"
#include "utility.h"

unsigned char buff[ TCP_BUFFER_SIZE*4 ];


long CalculateChecksum(void* data,unsigned long size)
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
	This is almost like a debug function, really.
	
	Takes a string of unsigned chars and formats them all pretty.
	
	The format:
	Every sixteen nums (chars) it makes a line.
	At the left is the row annotation (shown as a hex num in the format XXXX),
	on the right is the nums converted to text (unicode i think... may depend on the OS/terminal),
	and "illegal characterrs" (chars less than 0x20 on ASCII/Unicode) are shown as periods.
	The right margin has a width of 16 characters.
	
	Looks to have been used originally to view the contents of PSO packets.
*/
void eight_bit_hex_converter ( unsigned char* array, int len )
{
	int a = 0;	// array placeholder
	int b = 0;	// buffer placeholder
	int t = 0;	// text margin placeholder

	for (a=0; a<len; a++)
	{
		// When at the end of a line of hexes
		if ( (a > 0) && !(a % 16) )
		{
			for (; t<a; t++)
				// Put readable characters into the buffer;
				if (array[t] >= ' ')
					buff[b++] = array[t];
				// otherwise replace them with periods
				else
					buff[b++] = '.';
			sprintf (&buff[b++], "\n" );
		}

		// Before a line of hexes
		if ((a == 0) || !(a % 16))
		{
			sprintf (&buff[b], "(%04X) ", a);	// The counter value cause index basically
			b += 7; // Text added is sevenn chars long, so catch up this counter by 7
		}

		// Heres where the numbers actually makes it into the buffer
		sprintf (&buff[b], "%02X ", array[a]);
		b += 3;	// Increment three because three chars are inserted
	}

	// Add padding to last line if array isnt multiple of 16
	if ( len % 16 )
	{
		int padc = 0;
		padc = len;
		while (padc % 16)
		{
			sprintf (&buff[b], "   ");
			b += 3;
			padc++;
		}
	}

	// Last line of margin text
	for (; t<a; t++)
		if (array[t] >= ' ') 
			buff[b++] = array[t];
		else
			buff[b++] = '.';

	buff[b] = 0;
}

/*
	Prints the packet to terminal but not without
	generating text from the packet first.
*/
void display_hex ( unsigned char* arr, int len )
{
	eight_bit_hex_converter ( arr, len );
	printf ( "%s\n\n", arr );
}

/*
	Computes the message digest for string inString.
	Prints out message digest, a space, the string (in quotes) and a
	carriage return.
*/
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

/*
	Since Unicode is a superset of ASCII this is more importantly a translator between
	two-byte characters (Unicode) and one-byte characters (ASCII).
	It also serves as an alphanum enforcer for the fields passed in.
	Do not use this if you need a strict converter.
	
	Because its converting two byte characters to one, it should take in a short array,
	not a char array.  That way increments go two bytes at a time.
*/
unsigned char* unicode_to_ascii (unsigned short* ucs, unsigned char* buf)
{
	char *s,c;

	s = buf;
	while (*ucs != '\0')
	// same as saying
	// for (int i=0; i<len(ucs); i++)
	// {
	//    c = ucs[i++] & 0xFF
	//    ...
	// }
	// only "len" isnt a standard function, which is why its implemented this way (probs)
	{
		// The following line probably puts the least significant byte of ucs into c.
		// Theoretically, casting ucs to unsigned char should have the same result, ie:
		// c = (unsigned char) *(ucs++);
		c = *(ucs++) & 0xFF;
		// The following construct ensures the output is alphanum only
		if ((c >= 0x20) && (c <= 0x80))
			*(s++) = c;
		else
			// Lets replace bad characters with stars so they at least get visible placeholders
			*(s++) = '*';
	}
	*(s++) = 0x00;
	return buf;
}

void write_log (char* log, char* fmt, ...)
{
	va_list args;
	char text[ 4096 ];
	SYSTEMTIME rawtime;

	FILE* fp;

	GetLocalTime (&rawtime);
	va_start (args, fmt);
	strcpy (text + vsprintf( text, fmt, args ), "\r\n"); 
	va_end (args);

	fp = fopen ( log, "a");
	if (!fp)
	{
		printf ("Unable to log to %s\n", log);
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}

void write_gm (char *fmt, ...)
{
	va_list args;
	char text[ 4096 ];
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
