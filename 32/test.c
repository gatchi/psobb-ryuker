#include <stdio.h>
#include <string.h>

#define PACKET_LEN 34
#define ALL_VALID_PACKET_LEN 32
#define TEXT_PACKET_LEN 15

void text_to_packet( unsigned char* text );
void eight_bit_hex_converter( unsigned char* array, int len );

unsigned char buff[1000];

int main()
{
	int plen = PACKET_LEN;
	int panlen = ALL_VALID_PACKET_LEN;
	int ptlen = TEXT_PACKET_LEN;
	
	char* s = "This is a sample message.\nConvert to packet.";	
	int len = strlen(s);
	
	// packet, random values from 0 to 255
	unsigned char p[PACKET_LEN] = { 0x8f, 0x2c, 0x09, 0x93, 0x5b, 0x0b, 0x76, 0x1a, 0xf7, 0x5a, 0xbe, 0x0a, 0x89, 0x7c, 0x45, 0x59, 0x15, 0xdf, 0x09, 0x1f, 0xcb, 0x03, 0x45, 0xd6, 0x90, 0x03, 0xd8, 0xe4, 0x89, 0xdd, 0x40, 0xc3, 0x3e, 0x70 };
	
	// packet, mix of random alphanumeric and symbols
	unsigned char pan[ALL_VALID_PACKET_LEN] = { 0x5e, 0x5f, 0x52, 0x7b, 0x4d, 0x2d, 0x7b, 0x43, 0x6c, 0x23, 0x27, 0x50, 0x39, 0x2e, 0x5b, 0x50, 0x38, 0x2e, 0x79, 0x63, 0x2b, 0x76, 0x45, 0x4c, 0x7a, 0x68, 0x7d, 0x41, 0x49, 0x3a, 0x76, 0x60 };
	
	// text "packet" (its just a string really)
	unsigned char pt[TEXT_PACKET_LEN] = { 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x2e, '\n', '\0' };
	
	unsigned char s2[8] = { 'r', 'e', 'a', 'l', 'l', 'y', '?', '\0' };
	
	printf( ":%s\n\n", p );
	printf( ":%s\n\n", pan );
	printf( ":%s\n\n", pt );
	
	for( int i=0; i<ptlen; i++ )
	{
		printf("%02x ", pt[i]);
	}
	
	printf( "\n\nSTART\n\n" );
	
	eight_bit_hex_converter( p, plen );
	printf( "%s\n\n", buff );
	eight_bit_hex_converter( pan, panlen );
	printf( "%s\n\n", buff );
	eight_bit_hex_converter( pt, ptlen-1 );
	printf( "%s\n\n", buff );
	eight_bit_hex_converter( s, len );
	printf( "%s\n\n", buff );
	
	printf( "END\n" );
	
	return(0);
}

void text_to_packet( unsigned char* text )
{
	
}

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
