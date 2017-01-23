#include "stdafx.h"

void load_battle_param (BATTLEPARAM* dest, const char* filename, unsigned num_records, long expected_checksum)
{
	FILE* fp;
	long battle_checksum;

	printf ("Loading %s ... ", filename);
	fp = fopen ( filename, "rb");
	if (!fp)
	{
		printf ("%s is missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	if ( ( fread ( dest, 1, sizeof (BATTLEPARAM) * num_records, fp ) != sizeof (BATTLEPARAM) * num_records ) )
	{
		printf ("%s is corrupted.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fclose ( fp );

	printf ("OK!\n");

	battle_checksum = CalculateChecksum (dest, sizeof (BATTLEPARAM) * num_records);

	if ( battle_checksum != expected_checksum )
	{
		printf ("Checksum of file: %08x\n", battle_checksum );
		printf ("WARNING: Battle parameter file has been modified.\n");
	}
}

void load_armor_param ()
{
	unsigned ch,wi1;

	LoadCSV ("param\\armorpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][6]);
		armor_dfpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][17]);
		armor_evpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][18]);
		armor_equip_table[wi1] = (unsigned char) atoi (csv_params[ch][10]);
		armor_level_table[wi1] = (unsigned char) atoi (csv_params[ch][11]);
		//printf ("armor index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, armor_dfpvar_table[wi1], armor_evpvar_table[wi1], armor_equip_table[wi1], armor_level_table[wi1]);
	}
	FreeCSV ();
	LoadCSV ("param\\shieldpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][6]);
		barrier_dfpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][17]);
		barrier_evpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][18]);
		barrier_equip_table[wi1] = (unsigned char) atoi (csv_params[ch][10]);
		barrier_level_table[wi1] = (unsigned char) atoi (csv_params[ch][11]);
		//printf ("barrier index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, barrier_dfpvar_table[wi1], barrier_evpvar_table[wi1], barrier_equip_table[wi1], barrier_level_table[wi1]);
	}
	FreeCSV ();
	// Set up the stack table too.
	for (ch=0;ch<0x09;ch++)
	{
		if (ch != 0x02)
			stackable_table[ch] = 10;
	}
	stackable_table[0x10] = 99;

}

void load_weapon_param ()
{
	unsigned ch,wi1,wi2;

	LoadCSV ("param\\weaponpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][4]);
		wi2 = hexToByte (&csv_params[ch][0][6]);
		weapon_equip_table[wi1][wi2] = (unsigned) atoi (csv_params[ch][6]);
		*(unsigned short*) &weapon_atpmax_table[wi1][wi2] = (unsigned) atoi (csv_params[ch][8]);
		grind_table[wi1][wi2] = (unsigned char) atoi (csv_params[ch][14]);
		if (( ((wi1 >= 0x70) && (wi1 <= 0x88)) ||
			  ((wi1 >= 0xA5) && (wi1 <= 0xA9)) ) &&
			  (wi2 == 0x10))
			special_table[wi1][wi2] = 0x0B; // Fix-up S-Rank King's special
		else
			special_table[wi1][wi2] = (unsigned char) atoi (csv_params[ch][16]);
		//printf ("weapon index %02x%02x, eq: %u, grind: %u, atpmax: %u, special: %u \n", wi1, wi2, weapon_equip_table[wi1][wi2], grind_table[wi1][wi2], weapon_atpmax_table[wi1][wi2], special_table[wi1][wi2] );
	}
	FreeCSV ();
}

void load_tech_param ()
{
	unsigned ch,ch2;

	LoadCSV ("param\\tech.ini");
	if (csv_lines != 19)
	{
		printf ("Technique CSV file is corrupt.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	for (ch=0;ch<19;ch++) // For technique
	{
		for (ch2=0;ch2<12;ch2++) // For class
		{
			if (csv_params[ch][ch2+1])
				max_tech_level[ch][ch2] = ((char) atoi (csv_params[ch][ch2+1])) - 1;
			else
			{
				printf ("Technique CSV file is corrupt.\n");
				printf ("Press [ENTER] to quit...");
				gets(&dp[0]);
				exit (1);
			}
		}
	}
	FreeCSV ();
}

void GenerateCommonItem (int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client)
{
	unsigned ch, num_percents, item_set, meseta, do_area, r, eq_type;
	unsigned short ch2;
	PTDATA* ptd;
	unsigned area,fl;
	unsigned did_area[6] = {0};
	char percent;

	if ((!l) || (!i))
		return;

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];
	else
		ptd = &pt_tables_ep1[sid][l->difficulty];

	area = 0;

	switch ( l->episode )
	{
	case 0x01:
		switch ( l->floor[client->clientID ] )
		{
		case 11:
			// dragon
			area = 3;
			break;
		case 12:
			// de rol
			area = 6;
			break;
		case 13:
			// vol opt
			area = 8;
			break;
		case 14:
			// falz
			area = 10;
			break;
		default:
			area = l->floor[client->clientID ];
			break;
		}	
		break;
	case 0x02:
		switch ( l->floor[client->clientID ] )
		{
		case 14:
			// barba ray
			area = 3;
			break;
		case 15:
			// gol dragon
			area = 6;
			break;
		case 12:
			// gal gryphon
			area = 9;
			break;
		case 13:
			// olga flow
			area = 10;
			break;
		default:
			// could be tower
			if ( l->floor[client->clientID] <= 11 )
				area = ep2_rtremap[(l->floor[client->clientID] * 2)+1];
			else
				area = 0x0A;
			break;
		}	
		break;
	case 0x03:
		area = l->floor[client->clientID ] + 1;
		break;
	}

	if ( !area )
		return;

	if ( ( l->battle ) && ( l->quest_loaded ) )
	{
		if ( ( !l->battle_level ) || ( l->battle_level > 5 ) )
			area = 6; // Use mines equipment for rule #1, #5 and #8
		else
			area = 3; // Use caves 1 equipment for all other rules...
	}

	if ( area > 10 )
		area = 10;

	fl = area;
	area--; // Our tables are zero based.

	if (is_enemy)
	{
		if ( ( mt_lrand() % 100 ) > 40)
			item_set = 3;
		else
		{
			switch (item_type)
			{
			case 0x00:
				item_set = 0;
				break;
			case 0x01:
				item_set = 1;
				break;
			case 0x02:
				item_set = 1;
				break;
			case 0x03:
				item_set = 1;
				break;
			default:
				item_set = 3;
				break;
			}
		}
	}
	else
	{
		if ( ( l->meseta_boost ) && ( ( mt_lrand() % 100 ) > 25 ) )
			item_set = 4; // Boost amount of meseta dropped during rules #4 and #5
		else
		{
			if ( item_type == 0xFF )
				item_set = common_table[mt_lrand() % 100000];
			else
				item_set = item_type;
		}
	}

	memset (&i->item.data[0], 0, 12);
	memset (&i->item.data2[0], 0, 4);

	switch (item_set)
	{
	case 0x00:
		// Weapon

		if ( l->episode == 0x02 )
			ch2 = weapon_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
		else
			ch2 = weapon_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

		i->item.data[1] = ch2 & 0xFF;
		i->item.data[2] = ch2 >> 8;

		if (i->item.data[1] > 0x09)
		{
			if (i->item.data[2] > 0x03)
				i->item.data[2] = 0x03;
		}
		else
		{
			if (i->item.data[2] > 0x04)
				i->item.data[2] = 0x04;
		}
		
		r = 100 - ( mt_lrand() % 100 );

		if ( ( r > ptd->element_probability[area] ) && ( ptd->element_ranking[area] ) )
		{
			i->item.data[4] = 0xFF;
			while (i->item.data[4] == 0xFF) // give a special
				i->item.data[4] = elemental_table[(12 * ( ptd->element_ranking[area] - 1 ) ) + ( mt_lrand() % 12 )];
		}
		else
			i->item.data[4] = 0;

		if ( i->item.data[4] )
			i->item.data[4] |= 0x80; // untekked

		num_percents = 0;

		for (ch=0;ch<3;ch++)
		{
			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			if ( ( do_area ) && ( !did_area[do_area] ) )
			{
				did_area[do_area] = 1;
				i->item.data[6+(num_percents*2)] = (unsigned char) do_area;
				if ( l->episode == 0x02 )
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				else
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				percent -= 2;
				percent *= 5;
				(char) i->item.data[6+(num_percents*2)+1] = percent;
				num_percents++;
			}
		}

		// Add a grind

		if ( l->episode == 0x02 )
			ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
		else
			ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		i->item.data[3] = (unsigned char) ch;

		break;		
	case 0x01:
		r = mt_lrand() % 100;
		if (!is_enemy)
		{
			// Probabilities (Box): Armor 41%, Shields 41%, Units 18%
			if ( r > 82 )
				eq_type = 3;
			else
				if ( r > 59 )
					eq_type = 2;
				else
					eq_type = 1;
		}
		else
			eq_type = (unsigned) item_type;

		switch ( eq_type )
		{
		case 0x01:
			// Armor
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x01;
			i->item.data[2] = (unsigned char) ( fl / 3L ) + ( 5 * l->difficulty ) + ( mt_lrand() % ( ( (unsigned char) fl / 2L ) + 2 ) );
			if ( i->item.data[2] > 0x17 )
				i->item.data[2] = 0x17;
			r = mt_lrand() % 11;
			if (r < 7)
			{
				if (armor_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (armor_dfpvar_table[i->item.data[2]] + 1));
				if (armor_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (armor_evpvar_table[i->item.data[2]] + 1));
			}

			// Slots
			if ( l->episode == 0x02 )
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096]; 
			else
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];

			break;
		case 0x02:
			// Shield
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x02;
			i->item.data[2] = (unsigned char) ( fl / 3L ) + ( 4 * l->difficulty ) + ( mt_lrand() % ( ( (unsigned char) fl / 2L ) + 2 ) );
			if ( i->item.data[2] > 0x14 )
				i->item.data[2] = 0x14;
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		case 0x03:
			// unit
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x03;
			if ( ( ptd->unit_level[area] >= 2 ) && ( ptd->unit_level[area] <= 8 ) )
			{
				i->item.data[2] = 0xFF;
				while (i->item.data[2] == 0xFF)
					i->item.data[2] = unit_drop [mt_lrand() % ((ptd->unit_level[area] - 1) * 10)];
			}
			else
			{
				i->item.data[0] = 0x03;
				i->item.data[1] = 0x00;
				i->item.data[2] = 0x00; // Give a monomate when failed to look up unit.
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data [0]  = 0x02;
		i->item.data [2]  = 0x05;
		i->item.data [4]  = 0xF4;
		i->item.data [5]  = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
		// Tool
		if ( l->episode == 0x02 )
			*(unsigned *) &i->item.data[0] = tool_remap[tool_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096]];
		else
			*(unsigned *) &i->item.data[0] = tool_remap[tool_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096]];
		if ( i->item.data[1] == 0x02 ) // Technique
		{
			if ( l->episode == 0x02 )
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			i->item.data[2] = (unsigned char) ptd->tech_levels[i->item.data[4]][area*2];
			if ( ptd->tech_levels[i->item.data[4]][(area*2)+1] > ptd->tech_levels[i->item.data[4]][area*2] )
				i->item.data[2] += (unsigned char) mt_lrand() % ( ( ptd->tech_levels[i->item.data[4]][(area*2)+1] - ptd->tech_levels[i->item.data[4]][(area*2)] ) + 1 );
		}
		if (stackable_table[i->item.data[1]])
			i->item.data[5] = 0x01;
		break;
	case 0x04:
		// Meseta
		i->item.data[0] = 0x04;
		meseta  = ptd->box_meseta[area][0];
		if ( ptd->box_meseta[area][1] > ptd->box_meseta[area][0] )
			meseta += mt_lrand() % ( ( ptd->box_meseta[area][1] - ptd->box_meseta[area][0] ) + 1 );
		*(unsigned *) &i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
	i->item.itemid = l->itemID++;
}
