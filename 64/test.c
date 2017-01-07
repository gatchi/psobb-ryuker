#include <stdio.h>
#include <string.h>

int main( void )
{
	char str[2] = { '\n', '\n' };
	char str2[10] = "string";
	int len = strlen( str );
	int len2 = strlen( str2 );
	
	printf( "Length of '\n\n' is %d\n", str, len );
	printf( "Length of '%s' is %d\n", str2, len2 );
	printf( str2 );
	char c = str2[6];
	printf( "Value at end of string2 is %d\n", c );
	str2[6] = 0x01;
	printf( "Length of '%s' is %d\n", str2, strlen( str2 ) );
	c = str2[6];
	printf( "Value at end of string2 is %d\n", c );
	printf( "Null is %d\n", NULL );
}
