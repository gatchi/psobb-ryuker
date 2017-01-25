#include <ctype.h>
#include "quest.h"
#include "prs.cpp"

char dp[1];

// "Quest flag"?
int qflag (unsigned char* flag_data, unsigned int flag, unsigned int difficulty)
{
	if (flag_data[(difficulty * 0x80) + ( flag >> 3 )] & (1 << (7 - ( flag & 0x07 ))))
		return 1;
	else
		return 0;
}

int qflag_ep1solo (unsigned char* flag_data, unsigned int difficulty)
{
	int i;
	unsigned int quest_flag;

	for(i=1;i<=25;i++)
	{
		quest_flag = 0x63 + (i << 1);
		if(!qflag (flag_data, quest_flag, difficulty)) return 0;
	}
	return 1;
}