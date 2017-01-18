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
