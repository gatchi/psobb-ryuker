#include <stdio.h>
#include <string.h>
#include "headers/md5.h"

void MDString (char * inString, char * outString)
{
	unsigned char c;
	MD5_CTX mdContext;
	unsigned int len = strlen (inString);

	MD5Init (&mdContext);
	MD5Update (&mdContext, inString, len);
	MD5Final (&mdContext);
	for (c=0;c<16;c++)
	{
		*outString = mdContext.digest[c];
		outString++;
	}
}