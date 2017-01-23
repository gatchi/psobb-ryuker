#include "stdafx.h"
#include "ship_server.h"
#include "items.h"
#include "mag.h"
#include "def_tables.h"

void FixItem (ITEM* i, unsigned char* armor_dfpvar_table, unsigned char* armor_evpvar_table, unsigned char* barrier_dfpvar_table, unsigned char* barrier_evpvar_table )
{
	unsigned ch3;

	if (i->data[0] == 2) // Mag
	{
		MAG* m;
		short mDefense, mPower, mDex, mMind;
		int total_levels;

		m = (MAG*) &i->data[0];

		if ( m->synchro > 120 )
			m->synchro = 120;

		if ( m->synchro < 0 )
			m->synchro = 0;

		if ( m->IQ > 200 )
			m->IQ = 200;

		if ( ( m->defense < 0 ) || ( m->power < 0 ) || ( m->dex < 0 ) || ( m->mind < 0 ) )
			total_levels = 201; // Auto fail if any stat is under 0...
		else
		{
			mDefense = m->defense / 100;
			mPower = m->power / 100;
			mDex = m->dex / 100;
			mMind = m->mind / 100;
			total_levels = mDefense + mPower + mDex + mMind;
		}

		if ( ( total_levels > 200 ) || ( m->level > 200 ) )
		{
			// Mag fails IRL, initialize it
			m->defense = 500;
			m->power = 0;
			m->dex = 0;
			m->mind = 0;
			m->level = 5;
			m->blasts = 0;
			m->IQ = 0;
			m->synchro = 20;
			m->mtype = 0;
			m->PBflags = 0;
		}
	}

	if (i->data[0] == 1) // Normalize Armor & Barriers
	{
		switch (i->data[1])
		{
		case 0x01:
			if (i->data[6] > armor_dfpvar_table[ i->data[2] ])
				i->data[6] = armor_dfpvar_table[ i->data[2] ];
			if (i->data[8] > armor_evpvar_table[ i->data[2] ])
				i->data[8] = armor_evpvar_table[ i->data[2] ];
			break;
		case 0x02:
			if (i->data[6] > barrier_dfpvar_table[ i->data[2] ])
				i->data[6] = barrier_dfpvar_table[ i->data[2] ];
			if (i->data[8] > barrier_evpvar_table[ i->data[2] ])
				i->data[8] = barrier_evpvar_table[ i->data[2] ];
			break;
		}
	}

	if (i->data[0] == 0) // Weapon
	{
		signed char percent_table[6];
		signed char percent;
		unsigned max_percents, num_percents;
		int srank;

		if ( ( i->data[1] == 0x33 ) ||  // SJS & Lame max 2 percents
			 ( i->data[1] == 0xAB ) )
			max_percents = 2;
		else
			max_percents = 3;

		srank = 0;
		memset (&percent_table[0], 0, 6);
		num_percents = 0;

		for (ch3=6;ch3<=4+(max_percents*2);ch3+=2)
		{
			if ( i->data[ch3] & 128 )
			{
				srank = 1; // S-Rank
				break; 
			}

			if ( ( i->data[ch3] ) &&
				( i->data[ch3] < 0x06 ) )
			{
				// Percents over 100 or under -100 get set to 0
				percent = (char) i->data[ch3+1];
				if ( ( percent > 100 ) || ( percent < -100 ) )
					percent = 0;
				// Save percent
				percent_table[i->data[ch3]] = 
					percent;
			}
		}

		if (!srank)
		{
			for (ch3=6;ch3<=4+(max_percents*2);ch3+=2)
			{
				// Reset all %s
				i->data[ch3]   = 0;
				i->data[ch3+1] = 0;
			}

			for (ch3=1;ch3<=5;ch3++)
			{
				// Rebuild %s
				if ( percent_table[ch3] )
				{
					i->data[6 + ( num_percents * 2 )] = ch3;
					i->data[7 + ( num_percents * 2 )] = (unsigned char) percent_table[ch3];
					num_percents ++;
					if ( num_percents == max_percents )
						break;
				}
			}
		}
	}
}

unsigned long ExpandDropRate(unsigned char pc)
{
    long shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    return ((2 << (unsigned long) shift) * ((pc & 7) + 7));
}

void LoadShopData2( unsigned int* equip_prices )
{
	char buff[1];
	FILE *fp;
	fp = fopen ("shop\\shop2.dat", "rb");
	if (!fp)
	{
		printf ("shop\\shop2.dat is missing.");
		printf ("Press [ENTER] to quit...");
		gets (buff);
		exit (1);
	}
	fread (equip_prices, 1, sizeof (equip_prices), fp);
	fclose (fp);
}

unsigned int GetShopPrice(INVENTORY_ITEM* ci, unsigned short** weapon_atpmax_table, unsigned char* armor_dfpvar_table, unsigned char* armor_evpvar_table, unsigned int**** equip_prices, unsigned char* barrier_dfpvar_table, unsigned char* barrier_evpvar_table )
{
	unsigned compare_item, ch;
	int percent_add, price;
	unsigned char variation;
	float percent_calc;
	float price_calc;

	price = 10;

/*	printf ("Raw item data for this item is:\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n", 
		ci->item.data[0], ci->item.data[1], ci->item.data[2], ci->item.data[3], 
		ci->item.data[4], ci->item.data[5], ci->item.data[6], ci->item.data[7], 
		ci->item.data[8], ci->item.data[9], ci->item.data[10], ci->item.data[11], 
		ci->item.data[12], ci->item.data[13], ci->item.data[14], ci->item.data[15] ); */

	switch (ci->item.data[0])
	{
	case 0x00: // Weapons
		if (ci->item.data[4] & 0x80)
			price  = 1; // Untekked = 1 meseta
		else
		{
			if ((ci->item.data[1] < 0x0D) && (ci->item.data[2] < 0x05))
			{
				if ((ci->item.data[1] > 0x09) && (ci->item.data[2] > 0x03)) // Canes, Rods, Wands become rare faster
					break;
				price = weapon_atpmax_table[ci->item.data[1]][ci->item.data[2]] + ci->item.data[3];
				price *= price;
				price_calc = (float) price;
				switch (ci->item.data[1])
				{
				case 0x01:
					price_calc /= 5.0;
					break;
				case 0x02:
					price_calc /= 4.0;
					break;
				case 0x03:
				case 0x04:
					price_calc *= 2.0;
					price_calc /= 3.0;
					break;
				case 0x05:
					price_calc *= 4.0;
					price_calc /= 5.0;
					break;
				case 0x06:
					price_calc *= 10.0;
					price_calc /= 21.0;
					break;
				case 0x07:
					price_calc /= 3.0;
					break;
				case 0x08:
					price_calc *= 25.0;
					break;
				case 0x09:
					price_calc *= 10.0;
					price_calc /= 9.0;
					break;
				case 0x0A:
					price_calc /= 2.0;
					break;
				case 0x0B:
					price_calc *= 2.0;
					price_calc /= 5.0;
					break;
				case 0x0C:
					price_calc *= 4.0;
					price_calc /= 3.0;
					break;
				}

				percent_add = 0;
				if (ci->item.data[6])
					percent_add += (char) ci->item.data[7];
				if (ci->item.data[8])
					percent_add += (char) ci->item.data[9];
				if (ci->item.data[10])
					percent_add += (char) ci->item.data[11];

				if ( percent_add != 0 )
				{
					percent_calc = price_calc;
					percent_calc /= 300.0;
					percent_calc *= percent_add;
					price_calc += percent_calc;
				}
				price_calc /= 8.0;
				price = (int) ( price_calc );
				price += attrib[ci->item.data[4]];
			}
		}
		break;
	case 0x01:
		switch (ci->item.data[1])
		{
		case 0x01: // Armor
			if (ci->item.data[2] < 0x18)
			{
				// Calculate the amount to boost because of slots...
				if (ci->item.data[5] > 4)
					price = armor_prices[(ci->item.data[2] * 5) + 4];
				else
					price = armor_prices[(ci->item.data[2] * 5) + ci->item.data[5]];
				price -= armor_prices[(ci->item.data[2] * 5)];
				if (ci->item.data[6] > armor_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= armor_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price += equip_prices[1][1][ci->item.data[2]][variation];
			}
			break;
		case 0x02: // Shield
			if (ci->item.data[2] < 0x15)
			{
				if (ci->item.data[6] > barrier_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= barrier_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price = equip_prices[1][2][ci->item.data[2]][variation];
			}
			break;
		case 0x03: // Units
			if (ci->item.data[2] < 0x40)
				price = unit_prices [ci->item.data[2]];
			break;
		}
		break;
	case 0x03:
		// Tool
		if (ci->item.data[1] == 0x02) // Technique
		{
			if (ci->item.data[4] < 0x13)
				price = ((int) (ci->item.data[2] + 1) * tech_prices[ci->item.data[4]]) / 100L;
		}
		else
		{
			compare_item = 0;
			memcpy (&compare_item, &ci->item.data[0], 3);
			for (ch=0;ch<(sizeof(tool_prices)/4);ch+=2)
				if (compare_item == tool_prices[ch])
				{
					price = tool_prices[ch+1];
					break;
				}		
		}
		break;
	}
	if ( price < 0 )
		price = 0;
	//printf ("GetShopPrice = %u\n", price);
	return (unsigned) price;
}

void CheckMaxGrind (INVENTORY_ITEM* i, unsigned char** grind_table)
{
	if (i->item.data[3] > grind_table[i->item.data[1]][i->item.data[2]])
		i->item.data[3] = grind_table[i->item.data[1]][i->item.data[2]];
}
