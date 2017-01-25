#include "stdafx.h"
#include "params.h"
#include "utility.h"

unsigned int csv_lines = 0;
char* csv_params[1024][64]; // 1024 lines which can carry 64 parameters each

void load_battle_param (BATTLEPARAM* dest, const char* filename, unsigned num_records, long expected_checksum)
{
	char buff[1];
	FILE* fp;
	long battle_checksum;

	printf ("Loading %s ... ", filename);
	fp = fopen ( filename, "rb");
	if (!fp)
	{
		printf ("%s is missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(buff);
		exit (1);
	}
	if ( ( fread ( dest, 1, sizeof (BATTLEPARAM) * num_records, fp ) != sizeof (BATTLEPARAM) * num_records ) )
	{
		printf ("%s is corrupted.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(buff);
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




/* Load CSV into RAM */
void LoadCSV(const char* filename)
{
	char buff[1];
	FILE* fp;
	char csv_data[1024];
	unsigned ch, ch2, ch3 = 0;
	//unsigned ch4;
	int open_quote = 0;
	char* csv_param;
	
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));

	//printf ("Loading CSV file %s ...\n", filename );

	if ( ( fp = fopen (filename, "r" ) ) == NULL )
	{
		printf ("The parameter file %s appears to be missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets (buff);	// why are you capturing the ENTER.... into dp..... probably no reason, just need buffer
		exit (1);
	}

	while (fgets (&csv_data[0], 1023, fp) != NULL)
	{
		// ch2 = current parameter we're on
		// ch3 = current index into the parameter string
		ch2 = ch3 = 0;
		open_quote = 0;
		csv_param = csv_params[csv_lines][0] = malloc (256); // allocate memory for parameter
		for (ch=0;ch<strlen(&csv_data[0]);ch++)
		{
			if ((csv_data[ch] == 44) && (!open_quote)) // comma not surrounded by quotations
			{
				csv_param[ch3] = 0; // null terminate current parameter
				ch3 = 0;
				ch2++; // new parameter
				csv_param = csv_params[csv_lines][ch2] = malloc (256); // allocate memory for parameter
			}
			else
			{
				if (csv_data[ch] == 34) // quotation mark
					open_quote = !open_quote;
				else
					if (csv_data[ch] > 31) // no loading low ascii
						csv_param[ch3++] = csv_data[ch];
			}
		}
		if (ch3)
		{
			ch2++;
			csv_param[ch3] = 0;
		}
		/*
		for (ch4=0;ch4<ch2;ch4++)
			printf ("%s,", csv_params[csv_lines][ch4]);
		printf ("\n");
		*/
		csv_lines++;
		if (csv_lines > 1023)
		{
			printf ("CSV file has too many entries.\n");
			printf ("Press [ENTER] to quit...");
			gets (buff);	// here it is again... starting to think dp is a throwaway buffer
			exit (1);
		}
	}
	printf ("Loaded %u lines...\r\n", csv_lines);
	fclose (fp);
}


/* Release RAM from loaded CSV */
void FreeCSV()
{
	unsigned ch, ch2;

	for (ch=0;ch<csv_lines;ch++)
	{
		for (ch2=0;ch2<64;ch2++)
			if (csv_params[ch][ch2] != NULL) free (csv_params[ch][ch2]);
	}
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));
}
