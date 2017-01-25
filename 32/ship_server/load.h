void load_object_data (LOBBY* l, int unused, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records, ch, ch2;
	char new_file[256];

	if (!l) 
		return;

	memcpy (&new_file[0], filename, strlen (filename) + 1);

	if ( filename [ strlen ( filename ) - 5 ] == 101 )
		new_file [ strlen ( filename ) - 5 ] = 111; // change e to o

	//debug ("Loading object %s... current index: %u", new_file, l->objIndex);

	fp = fopen ( &new_file[0], "rb");
	if (!fp)
		WriteLog ("Could not load object data from %s\n", new_file);
	else
	{
		fseek  ( fp, 0, SEEK_END );
		num_records = ftell ( fp ) / 68;
		fseek  ( fp, 0, SEEK_SET );
		fread  ( &dp[0], 1, 68 * num_records, fp );
		fclose ( fp );
		oldIndex = l->objIndex;
		ch2 = 0;
		for (ch=0;ch<num_records;ch++)
		{
			if ( l->objIndex < 0xB50 )
			{
				memcpy (&l->objData[l->objIndex], &dp[ch2+0x28], 12);
				l->objData[l->objIndex].drop[3] = 0;
				l->objData[l->objIndex].drop[2] = dp[ch2+0x35];
				l->objData[l->objIndex].drop[1] = dp[ch2+0x36];
				l->objData[l->objIndex++].drop[0] = dp[ch2+0x37];
				ch2 += 68;
			}
			else
				break;
		}
		//debug ("Added %u objects, total: %u", l->objIndex - oldIndex, l->objIndex );
	}
}

void load_map_data (LOBBY* l, int aMob, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records;

	if (!l) 
		return;

	//debug ("Loading map %s... current index: %u", filename, l->mapIndex);

	fp = fopen ( filename, "rb");
	if (!fp)
		WriteLog ("Could not load map data from %s\n", filename);
	else
	{
		fseek  ( fp, 0, SEEK_END );
		num_records = ftell ( fp ) / 72;
		fseek  ( fp, 0, SEEK_SET );
		fread  ( &dp[0], 1, sizeof ( MAP_MONSTER ) * num_records, fp );
		fclose ( fp );
		oldIndex = l->mapIndex;
		parse_map_data ( l, (MAP_MONSTER*) &dp[0], aMob, num_records );
		//debug ("Added %u mids, total: %u", l->mapIndex - oldIndex, l->mapIndex );
	}
};

void load_ship_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch = 0;

	FILE* fp;

	experience_rate = 1; // Default to 100% EXP

	if ( ( fp = fopen ("ship.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file ship.ini appears to be missing.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	else
		while (fgets (&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					ch = strlen (&config_data[0]);
					if (config_data[ch-1] == 0x0A)
						config_data[ch--]  = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case 0x00:
					// Server IP address
					{
						if ((config_data[0] == 0x41) || (config_data[0] == 0x61))
						{
							autoIP = 1;
						}
						else
						{
							convertIPString (&config_data[0], ch+1, 1, &serverIP[0] );
						}
					}
					break;
				case 0x01:
					// Server Listen Port
					serverPort = atoi (&config_data[0]);
					break;
				case 0x02:
					// Number of blocks
					serverBlocks = atoi (&config_data[0]);
					if (serverBlocks > 10) 
					{
						printf ("You cannot host more than 10 blocks... Adjusted.\n");
						serverBlocks = 10;
					}
					if (serverBlocks == 0)
					{
						printf ("You have to host at least ONE block... Adjusted.\n");
						serverBlocks = 1;
					}
					break;
				case 0x03:
					// Max Client Connections
					serverMaxConnections = atoi (&config_data[0]);
					if ( serverMaxConnections > ( serverBlocks * 180 ) )
					{
						printf ("\nYou're attempting to server more connections than the amount of blocks\nyou're hosting allows.\nAdjusted...\n");
						serverMaxConnections = serverBlocks * 180;
					}
					if ( serverMaxConnections > SHIP_COMPILED_MAX_CONNECTIONS )
					{
						printf ("This copy of the ship serving software has not been compiled to accept\nmore than %u connections.\nAdjusted...\n", SHIP_COMPILED_MAX_CONNECTIONS);
						serverMaxConnections = SHIP_COMPILED_MAX_CONNECTIONS;
					}
					break;
				case 0x04:
					// Login server host name or IP
					{
						unsigned p;
						unsigned alpha;
						alpha = 0;
						for (p=0;p<ch;p++)
							if (((config_data[p] >= 65 ) && (config_data[p] <= 90)) ||
								((config_data[p] >= 97 ) && (config_data[p] <= 122)))
							{
								alpha = 1;
								break;
							}
						if (alpha)
						{
							struct hostent *IP_host;

							config_data[strlen(&config_data[0])-1] = 0x00;
							printf ("Resolving %s ...\n", (char*) &config_data[0] );
							IP_host = gethostbyname (&config_data[0]);
							if (!IP_host)
							{
								printf ("Could not resolve host name.");
								printf ("Press [ENTER] to quit...");
								gets(&dp[0]);
								exit (1);
							}
							*(unsigned *) &loginIP[0] = *(unsigned *) IP_host->h_addr;
						}
						else
							convertIPString (&config_data[0], ch+1, 1, &loginIP[0] );
					}
					break;
				case 0x05:
					// Ship Name
					memset (&Ship_Name[0], 0, 255 );
					memcpy (&Ship_Name[0], &config_data[0], ch+1 );
					Ship_Name[12] = 0x00;
					break;
				case 0x06:
					// Event
					shipEvent = (unsigned char) atoi (&config_data[0]);
					PacketDA[0x04] = shipEvent;
					break;
				case 0x07:
					weapon_drop_rate = atoi (&config_data[0]);
					break;
				case 0x08:
					armor_drop_rate = atoi (&config_data[0]);
					break;
				case 0x09:
					mag_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0A:
					tool_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0B:
					meseta_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0C:
					experience_rate = atoi (&config_data[0]);
					if ( experience_rate > 99 )
					{
						printf ("\nWARNING: You have your experience rate set to a very high number.\n");
						printf ("As of ship_server.exe version 0.038, you now just use single digits\n");
						printf ("to represent 100%% increments.  (ex. 1 for 100%, 2 for 200%)\n\n");
						 ("If you've set the high value of %u%% experience on purpose,\n", experience_rate * 100 );
						printf ("press [ENTER] to continue, otherwise press CTRL+C to abort.\n");
						printf (":");
						gets   (&dp[0]);
						printf ("\n\n");
					}
					break;
				case 0x0D:
					ship_support_extnpc = atoi (&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
		fclose (fp);

		if (config_index < 0x0D)
		{
			printf ("ship.ini seems to be corrupted.\n");
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		common_rates[0] = 100000 / weapon_drop_rate;
		common_rates[1] = 100000 / armor_drop_rate;
		common_rates[2] = 100000 / mag_drop_rate;
		common_rates[3] = 100000 / tool_drop_rate;
		common_rates[4] = 100000 / meseta_drop_rate;
		load_mask_file(num_masks, ship_banmasks);
}

void load_quests (const char* filename, unsigned int category)
{
	/*unsigned oldIndex;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	LOBBY *l;*/
	FILE* fp;
	FILE* qf;
	FILE* qd;
	unsigned qs;
	char qfile[256];
	char qfile2[256];
	char qfile3[256];
	char qfile4[256];
	char qname[256];
	unsigned qnl = 0;
	QUEST* q;
	unsigned ch, ch2, ch3, ch4, ch5, qf2l;
	unsigned short qps, qpc;
	unsigned qps2;
	QUEST_MENU* qm;
	unsigned* ed;
	unsigned ed_size, ed_ofs;
	unsigned num_records, num_objects, qm_ofs = 0, qb_ofs = 0;
	char true_filename[16];
	QDETAILS* ql;
	int extf;
	unsigned char qpd_buffer  [PRS_BUFFER];
	unsigned char qpdc_buffer [PRS_BUFFER];

	qm = &quest_menus[category];
	printf ("Loading quest list from %s ... \n", filename);
	fp = fopen ( filename, "r");
	if (!fp)
	{
		printf ("%s is missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	while (fgets (&qfile[0], 255, fp) != NULL)
	{
		for (ch=0;ch<strlen(&qfile[0]);ch++)
			if ((qfile[ch] == 10) || (qfile[ch] == 13))
				qfile[ch] = 0; // Reserved
		qfile3[0] = 0;
		strcat ( &qfile3[0], "quest\\");
		strcat ( &qfile3[0], &qfile[0] );
		memcpy ( &qfile[0], &qfile3[0], strlen ( &qfile3[0] ) + 1 );
		strcat ( &qfile3[0], "quest.lst");
		qf = fopen ( &qfile3[0], "r");
		if (!qf)
		{
			printf ("%s is missing.\n", qfile3);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		if (fgets (&qname[0], 64, fp) == NULL)
		{
			printf ("%s is corrupted.\n", filename);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		for (ch=0;ch<64;ch++)
		{
			if (qname[ch] != 0x00)
			{
				qm->c_names[qm->num_categories][ch*2] = qname[ch];
				qm->c_names[qm->num_categories][(ch*2)+1] = 0x00;
			}
			else
			{
				qm->c_names[qm->num_categories][ch*2] = 0x00;
				qm->c_names[qm->num_categories][(ch*2)+1] = 0x00;
				break;
			}
		}
		if (fgets (&qname[0], 120, fp) == NULL)
		{
			printf ("%s is corrupted.\n", filename);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		for (ch=0;ch<120;ch++)
		{
			if (qname[ch] != 0x00)
			{
				if (qname[ch] == 0x24)
					qm->c_desc[qm->num_categories][ch*2] = 0x0A;
				else
					qm->c_desc[qm->num_categories][ch*2] = qname[ch];
				qm->c_desc[qm->num_categories][(ch*2)+1] = 0x00;
			}
			else
			{
				qm->c_desc[qm->num_categories][ch*2] = 0x00;
				qm->c_desc[qm->num_categories][(ch*2)+1] = 0x00;
				break;
			}
		}
		memcpy (&qfile2[0], &qfile[0], strlen (&qfile[0])+1);
		qf2l = strlen (&qfile2[0]);
		while (fgets (&qfile2[qf2l], 255, qf) != NULL)
		{
			for (ch=0;ch<strlen(&qfile2[0]);ch++)
				if ((qfile2[ch] == 10) || (qfile2[ch] == 13))
					qfile2[ch] = 0; // Reserved

			for (ch4=0;ch4<numLanguages;ch4++)
			{
				memcpy (&qfile4[0], &qfile2[0], strlen (&qfile2[0])+1);

				// Add extension to .qst and .raw for languages

				extf = 0;

				if (strlen(languageExts[ch4]))
				{
					if ( (strlen(&qfile4[0]) - qf2l) > 3)
						for (ch5=qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5+1]) == 113) &&
								(tolower(qfile4[ch5+2]) == 115) &&
								(tolower(qfile4[ch5+3]) == 116))
							{
								qfile4[ch5] = 0;
								strcat (&qfile4[ch5], "_");
								strcat (&qfile4[ch5], languageExts[ch4]);
								strcat (&qfile4[ch5], ".qst");
								extf = 1;
								break;
							}
						}

						if (( (strlen(&qfile4[0]) - qf2l) > 3) && (!extf))
							for (ch5=qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
							{
								if ((qfile4[ch5] == 46) &&
									(tolower(qfile4[ch5+1]) == 114) &&
									(tolower(qfile4[ch5+2]) == 97) &&
									(tolower(qfile4[ch5+3]) == 119))
								{
									qfile4[ch5] = 0;
									strcat (&qfile4[ch5], "_");
									strcat (&qfile4[ch5], languageExts[ch4]);
									strcat (&qfile4[ch5], ".raw");
									break;
								}
							}
				}

				qd = fopen ( &qfile4[0], "rb" );
				if (qd != NULL)
				{
					if (ch4 == 0)
					{
						q = &quests[numQuests];
						memset (q, 0, sizeof (QUEST));
					}
					ql = q->ql[ch4] = malloc ( sizeof ( QDETAILS ));
					memset (ql, 0, sizeof ( QDETAILS ));
					fseek ( qd, 0, SEEK_END );
					ql->qsize = qs = ftell ( qd );
					fseek ( qd, 0, SEEK_SET );
					ql->qdata = malloc ( qs );
					questsMemory += qs;
					fread ( ql->qdata, 1, qs, qd );
					ch = 0;
					ch2 = 0;
					while (ch < qs)
					{
						qpc = *(unsigned short*) &ql->qdata[ch+2];
						if ( (qpc == 0x13) && (strstr(&ql->qdata[ch+8], ".bin")) && (ch2 < PRS_BUFFER))
						{
							memcpy ( &true_filename[0], &ql->qdata[ch+8], 16 );
							qps2 = *(unsigned*) &ql->qdata[ch+0x418];
							memcpy (&qpd_buffer[ch2], &ql->qdata[ch+0x18], qps2);
							ch2 += qps2;
						}
						else
							if (ch2 >= PRS_BUFFER)
							{
								printf ("PRS buffer too small...\n");
								printf ("Press [ENTER] to quit...");
								gets (&dp[0]);
								exit (1);
							}
							qps = *(unsigned short*) &ql->qdata[ch];
							if (qps % 8) 
								qps += ( 8 - ( qps % 8 ) );
							ch += qps;
					}
					ed_size = prs_decompress (&qpd_buffer[0], &qpdc_buffer[0]);
					if (ed_size > PRS_BUFFER)
					{
						printf ("Memory corrupted!\n", ed_size );
						printf ("Press [ENTER] to quit...");
						gets (&dp[0]);
						exit (1);
					}
					fclose (qd);

					if (ch4 == 0)
						qm->quest_indexes[qm->num_categories][qm->quest_counts[qm->num_categories]++] = numQuests++;

					qnl = 0;
					for (ch2=0x18;ch2<0x48;ch2+=2)
					{
						if ( *(unsigned short *) &qpdc_buffer[ch2] != 0x0000 )
						{
							qname[qnl] = qpdc_buffer[ch2];
							if (qname[qnl] < 32)
								qname[qnl] = 32;
							qnl++;
						}
						else
							break;
					}

					qname[qnl] = 0;
					memcpy (&ql->qname[0], &qpdc_buffer[0x18], 0x40);
					ql->qname[qnl] = 0x0000;
					memcpy (&ql->qsummary[0], &qpdc_buffer[0x58], 0x100);
					memcpy (&ql->qdetails[0], &qpdc_buffer[0x158], 0x200);

					if (ch4 == 0)
					{
						// Load enemy data

						ch = 0;
						ch2 = 0;

						while (ch < qs)
						{
							qpc = *(unsigned short*) &ql->qdata[ch+2];
							if ( (qpc == 0x13) && (strstr(&ql->qdata[ch+8], ".dat")) && (ch2 < PRS_BUFFER))
							{
								qps2 = *(unsigned *) &ql->qdata[ch+0x418];
								memcpy (&qpd_buffer[ch2], &ql->qdata[ch+0x18], qps2);
								ch2 += qps2;
							}
							else
								if (ch2 >= PRS_BUFFER)
								{
									printf ("PRS buffer too small...\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit (1);
								}

								qps = *(unsigned short*) &ql->qdata[ch];
								if (qps % 8) 
									qps += ( 8 - ( qps % 8 ) );
								ch += qps;
						}
						ed_size = prs_decompress (&qpd_buffer[0], &qpdc_buffer[0]);
						if (ed_size > PRS_BUFFER)
						{
							printf ("Memory corrupted!\n", ed_size );
							printf ("Press [ENTER] to quit...");
							gets (&dp[0]);
							exit (1);
						}
						ed_ofs = 0;
						ed = (unsigned*) &qpdc_buffer[0];
						qm_ofs = 0;
						qb_ofs = 0;
						num_objects = 0;
						while ( ed_ofs < ed_size )
						{
							switch ( *ed )
							{
							case 0x01:
								if (ed[2] > 17)
								{
									printf ("Area out of range in quest!\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit(1);
								}
								num_records = ed[3] / 68L;
								num_objects += num_records;
								*(unsigned *) &qpd_buffer[qb_ofs] = *(unsigned *) &ed[2];
								qb_ofs += 4;
								//printf ("area: %u, object count: %u\n", ed[2], num_records);
								*(unsigned *) &qpd_buffer[qb_ofs] = num_records;
								qb_ofs += 4;
								memcpy ( &qpd_buffer[qb_ofs], &ed[4], ed[3] );
								qb_ofs += num_records * 68L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x03:
								//printf ("data type: %u\n", *ed );
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x02:
								num_records = ed[3] / 72L;
								*(unsigned *) &dp[qm_ofs] = *(unsigned *) &ed[2];
								//printf ("area: %u, mid count: %u\n", ed[2], num_records);
								if (ed[2] > 17)
								{
									printf ("Area out of range in quest!\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit(1);
								}
								qm_ofs += 4;
								*(unsigned *) &dp[qm_ofs] = num_records;
								qm_ofs += 4;
								memcpy ( &dp[qm_ofs], &ed[4], ed[3] );
								qm_ofs += num_records * 72L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							default:
								// Probably done loading...
								ed_ofs = ed_size;
								break;
							}
						}

						// Do objects
						q->max_objects = num_objects;
						questsMemory += qb_ofs;
						q->objectdata = malloc ( qb_ofs );
						// Need to sort first...
						ch3 = 0;
						for (ch=0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < qb_ofs)
							{
								unsigned qa;

								qa = *(unsigned *) &qpd_buffer[ch2];
								num_records = *(unsigned *) &qpd_buffer[ch2+4];
								if (qa == ch)
								{
									memcpy (&q->objectdata[ch3], &qpd_buffer[ch2+8], ( num_records * 68 ) );
									ch3 += ( num_records * 68 );
								}
								ch2 += ( num_records * 68 ) + 8;
							}
						}

						// Do enemies

						qm_ofs += 4;
						questsMemory += qm_ofs;
						q->mapdata = malloc ( qm_ofs );
						*(unsigned *) q->mapdata = qm_ofs;
						// Need to sort first...
						ch3 = 4;
						for (ch=0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < ( qm_ofs - 4 ))
							{
								unsigned qa;

								qa = *(unsigned *) &dp[ch2];
								num_records = *(unsigned *) &dp[ch2+4];
								if (qa == ch)
								{
									memcpy (&q->mapdata[ch3], &dp[ch2], ( num_records * 72 ) + 8 );
									ch3 += ( num_records * 72 ) + 8;
								}
								ch2 += ( num_records * 72 ) + 8;
							}
						}
						for (ch=0;ch<num_objects;ch++)
						{
							// Swap fields in advance
							dp[0] = q->objectdata[(ch*68)+0x37];
							dp[1] = q->objectdata[(ch*68)+0x36];
							dp[2] = q->objectdata[(ch*68)+0x35];
							dp[3] = q->objectdata[(ch*68)+0x34];
							*(unsigned *) &q->objectdata[(ch*68)+0x34] = *(unsigned *) &dp[0];
						}
						printf ("Loaded quest %s (%s),\nObject count: %u, Enemy count: %u\n", qname, true_filename, num_objects, ( qm_ofs - 4 ) / 72L );
					}
					/*
					// Time to load the map data...
					l = &fakelobby;
					memset ( l, 0, sizeof (LOBBY) );
					l->bptable = &ep2battle[0];
					memset ( &l->mapData[0], 0, 0xB50 * sizeof (MAP_MONSTER) ); // Erase!
					l->mapIndex = 0;
					l->rareIndex = 0;
					for (ch=0;ch<0x20;ch++)
					l->rareData[ch] = 0xFF;

					qmap = q->mapdata;
					qm_length = *(unsigned*) qmap;
					qmap += 4;
					ch = 4;
					while ( ( qm_length - ch ) >= 80 )
					{
					oldIndex = l->mapIndex;
					qa = *(unsigned*) qmap; // Area
					qmap += 4;
					nr = *(unsigned*) qmap; // Number of monsters
					qmap += 4;
					if ( ( l->episode == 0x03 ) && ( qa > 5 ) )
					parse_map_data ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					if ( ( l->episode == 0x02 ) && ( qa > 15 ) )
					parse_map_data ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					parse_map_data ( l, (MAP_MONSTER*) qmap, 0, nr );
					qmap += ( nr * 72 );
					ch += ( ( nr * 72 ) + 8 );
					debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
					}
					exit (1);
					*/
				}
				else
				{
					if (ch4 == 0)
					{
						printf ("Quest file %s is missing!  Could not load the quest.\n", qfile4);
						printf ("Press [ENTER] to quit...");
						gets (&dp[0]);
						exit(1);
					}
					else
					{
						printf ("Non-fatal: Alternate quest language file %s is missing.\n", qfile4);
					}
				}
			}
		}
		fclose (qf);
		qm->num_categories++;
	}
	fclose (fp);
}

void LoadShopData2()
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

void load_tech_param ()
{
	unsigned int ch,ch2;
	char buff[1];

	LoadCSV ("param\\tech.ini");
	if (csv_lines != 19)
	{
		printf ("Technique CSV file is corrupt.\n");
		printf ("Press [ENTER] to quit...");
		gets(buff);
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
				gets(buff);
				exit (1);
			}
		}
	}
	FreeCSV ();
}

void load_armor_param ()
{
	unsigned ch,wi1;

	LoadCSV ("param\\armorpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hex2byte (&csv_params[ch][0][6]);
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
		wi1 = hex2byte (&csv_params[ch][0][6]);
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
		wi1 = hex2byte (&csv_params[ch][0][4]);
		wi2 = hex2byte (&csv_params[ch][0][6]);
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

void load_language_file()
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

void load_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch = 0;

	FILE* fp;

	experience_rate = 1; // Default to 100% EXP

	if ( ( fp = fopen ("ship.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file ship.ini appears to be missing.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	else
		while (fgets (&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					ch = strlen (&config_data[0]);
					if (config_data[ch-1] == 0x0A)
						config_data[ch--]  = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case 0x00:
					// Server IP address
					{
						if ((config_data[0] == 0x41) || (config_data[0] == 0x61))
						{
							autoIP = 1;
						}
						else
						{
							convertIPString (&config_data[0], ch+1, 1, &serverIP[0] );
						}
					}
					break;
				case 0x01:
					// Server Listen Port
					serverPort = atoi (&config_data[0]);
					break;
				case 0x02:
					// Number of blocks
					serverBlocks = atoi (&config_data[0]);
					if (serverBlocks > 10) 
					{
						printf ("You cannot host more than 10 blocks... Adjusted.\n");
						serverBlocks = 10;
					}
					if (serverBlocks == 0)
					{
						printf ("You have to host at least ONE block... Adjusted.\n");
						serverBlocks = 1;
					}
					break;
				case 0x03:
					// Max Client Connections
					serverMaxConnections = atoi (&config_data[0]);
					if ( serverMaxConnections > ( serverBlocks * 180 ) )
					{
						printf ("\nYou're attempting to server more connections than the amount of blocks\nyou're hosting allows.\nAdjusted...\n");
						serverMaxConnections = serverBlocks * 180;
					}
					if ( serverMaxConnections > SHIP_COMPILED_MAX_CONNECTIONS )
					{
						printf ("This copy of the ship serving software has not been compiled to accept\nmore than %u connections.\nAdjusted...\n", SHIP_COMPILED_MAX_CONNECTIONS);
						serverMaxConnections = SHIP_COMPILED_MAX_CONNECTIONS;
					}
					break;
				case 0x04:
					// Login server host name or IP
					{
						unsigned p;
						unsigned alpha;
						alpha = 0;
						for (p=0;p<ch;p++)
							if (((config_data[p] >= 65 ) && (config_data[p] <= 90)) ||
								((config_data[p] >= 97 ) && (config_data[p] <= 122)))
							{
								alpha = 1;
								break;
							}
						if (alpha)
						{
							struct hostent *IP_host;

							config_data[strlen(&config_data[0])-1] = 0x00;
							printf ("Resolving %s ...\n", (char*) &config_data[0] );
							IP_host = gethostbyname (&config_data[0]);
							if (!IP_host)
							{
								printf ("Could not resolve host name.");
								printf ("Press [ENTER] to quit...");
								gets(&dp[0]);
								exit (1);
							}
							*(unsigned *) &loginIP[0] = *(unsigned *) IP_host->h_addr;
						}
						else
							convertIPString (&config_data[0], ch+1, 1, &loginIP[0] );
					}
					break;
				case 0x05:
					// Ship Name
					memset (&Ship_Name[0], 0, 255 );
					memcpy (&Ship_Name[0], &config_data[0], ch+1 );
					Ship_Name[12] = 0x00;
					break;
				case 0x06:
					// Event
					shipEvent = (unsigned char) atoi (&config_data[0]);
					PacketDA[0x04] = shipEvent;
					break;
				case 0x07:
					weapon_drop_rate = atoi (&config_data[0]);
					break;
				case 0x08:
					armor_drop_rate = atoi (&config_data[0]);
					break;
				case 0x09:
					mag_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0A:
					tool_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0B:
					meseta_drop_rate = atoi (&config_data[0]);
					break;
				case 0x0C:
					experience_rate = atoi (&config_data[0]);
					if ( experience_rate > 99 )
					{
						printf ("\nWARNING: You have your experience rate set to a very high number.\n");
						printf ("As of ship_server.exe version 0.038, you now just use single digits\n");
						printf ("to represent 100%% increments.  (ex. 1 for 100%, 2 for 200%)\n\n");
						 ("If you've set the high value of %u%% experience on purpose,\n", experience_rate * 100 );
						printf ("press [ENTER] to continue, otherwise press CTRL+C to abort.\n");
						printf (":");
						gets   (&dp[0]);
						printf ("\n\n");
					}
					break;
				case 0x0D:
					ship_support_extnpc = atoi (&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
		fclose (fp);

		if (config_index < 0x0D)
		{
			printf ("ship.ini seems to be corrupted.\n");
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		common_rates[0] = 100000 / weapon_drop_rate;
		common_rates[1] = 100000 / armor_drop_rate;
		common_rates[2] = 100000 / mag_drop_rate;
		common_rates[3] = 100000 / tool_drop_rate;
		common_rates[4] = 100000 / meseta_drop_rate;
		load_mask_file();
}

void load_mask_file()
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
