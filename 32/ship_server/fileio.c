#include "fileio.h"
#include "network.h"

char dp[1];

/*
	Yet another settings file.
	Loads a file that stores the IP ban masks.
	(These should all either use json or sql instead.)
*/
void load_mask_file(unsigned int num_masks, unsigned short ** ship_banmasks)
{
	char mask_data[255];
	unsigned ch = 0;

	FILE* fp;

	// Load masks.txt for IP ban masks

	num_masks = 0;

	if ( ( fp = fopen ("masks.txt", "r" ) ) )
	{
		while (fgets (&mask_data[0], 255, fp) != NULL)
		{
			if (mask_data[0] != 0x23)
			{
				ch = strlen (&mask_data[0]);
				if (mask_data[ch-1] == 0x0A)
					mask_data[ch--]  = 0x00;
				mask_data[ch] = 0;
			}
			convertMask (&mask_data[0], ch+1, &ship_banmasks[num_masks++][0]);
		}
	}
}

void load_language_file(char * languageNames, char * languageExts, unsigned int numLanguages)
{
	FILE* fp;
	char lang_data[256];
	int langExt = 0;
	unsigned int ch;

	for (ch=0;ch<10;ch++)
	{
		languageNames[ch] = (char) malloc ( 256 );
		memset (languageNames[ch], 0, 256);
		languageExts[ch] = malloc ( 256 );
		memset (languageExts[ch], 0, 256);
	}

	if ( ( fp = fopen ("lang.ini", "r" ) ) == NULL )
	{
		printf ("Language file does not exist...\nWill use English only...\n\n");
		numLanguages = 1;
		strcat (languageNames[0], "English");	
	}
	else
	{
		while ((fgets (lang_data, 255, fp) != NULL) && (numLanguages < 10))
		{
			if (!langExt)
			{
				memcpy (languageNames[numLanguages], lang_data, strlen (lang_data)+1);
				for (ch=0; ch<strlen(languageNames[numLanguages]); ch++)
					if ((languageNames[ch] == 10) || (languageNames[ch] == 13))
						languageNames[ch] = 0;
				langExt = 1;
			}
			else
			{
				memcpy (languageExts[numLanguages], lang_data, strlen (lang_data)+1);
				for (ch=0; ch<strlen(languageExts[numLanguages]); ch++)
					if ((languageExts[ch] == 10) || (languageExts[ch] == 13))
						languageExts[ch] = 0;
				numLanguages++;
				printf ("Language %u (%s:%s)\n", numLanguages, languageNames[numLanguages-1], languageExts[numLanguages-1]);
				langExt = 0;
			}
		}
		fclose (fp);
		if (numLanguages < 1)
		{
			numLanguages = 1;
			strcat (languageNames[0], "English");
		}
	}
}

