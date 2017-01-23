#include "account.h"

unsigned ship_gcsend_count = 0;
unsigned ship_gcsend_list[MAX_GCSEND*3] = {0};

void prep_guildcard ( unsigned int from, unsigned int to, time_t servertime )
{
	int gc_present = 0;
	unsigned hightime = 0xFFFFFFFF;
	unsigned highidx = 0;
	unsigned ch;

	if (ship_gcsend_count < MAX_GCSEND)
	{
		for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
		{
			if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch+1] == to))
			{
				gc_present = 1;
				break;
			}
		}

		if (!gc_present)
		{
			highidx = ship_gcsend_count * 3;
			ship_gcsend_count++;
		}
	}
	else
	{
		// Erase oldest sent card
		for (ch=0;ch<(MAX_GCSEND*3);ch+=3)
		{
			if (ship_gcsend_list[ch+2] < hightime)
			{
				hightime = ship_gcsend_list[ch+2];
				highidx  = ch;
			}
		}
	}
	ship_gcsend_list[highidx]	= from;
	ship_gcsend_list[highidx+1]	= to;
	ship_gcsend_list[highidx+2]	= (unsigned) servertime;
}

int prepped_guildcard ( unsigned int from, unsigned int to )
{
	int gc_present = 0;
	unsigned ch, ch2;

	for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
	{
		if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch+1] == to))
		{
			ship_gcsend_list[ch]   = 0;
			ship_gcsend_list[ch+1] = 0;
			ship_gcsend_list[ch+2] = 0;
			gc_present = 1;
			break;
		}
	}

	if (gc_present)
	{
		// Clean up the list
		ch2 = 0;
		for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
		{
			if ((ship_gcsend_list[ch] != 0) && (ch != ch2))
			{
				ship_gcsend_list[ch2]   = ship_gcsend_list[ch];
				ship_gcsend_list[ch2+1] = ship_gcsend_list[ch+1];
				ship_gcsend_list[ch2+2] = ship_gcsend_list[ch+2];
				ch2 += 3;
			}
		}
		ship_gcsend_count = ch2 / 3;
	}

	return gc_present;
}
