void parse_map_data (LOBBY* l, MAP_MONSTER* mapData, int aMob, unsigned num_records)
{
	MAP_MONSTER* mm;
	unsigned ch, ch2;
	unsigned num_recons;
	int r;
	
	for (ch2=0;ch2<num_records;ch2++)
	{
		if ( l->mapIndex >= 0xB50 )
			break;
		memcpy (&l->mapData[l->mapIndex], mapData, 72);
		mapData++;
		mm = &l->mapData[l->mapIndex];
		mm->exp = 0;
		switch ( mm->base )
		{
		case 64:
			// Hildebear and Hildetorr
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < hildebear_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 0x02;
				mm->exp = l->bptable[0x4A].XP;
			}
			else
			{
				mm->rt_index = 0x01;
				mm->exp = l->bptable[0x49].XP;
			}
			break;
		case 65:
			// Rappies
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < rappy_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( l->episode == 0x03 )
			{
				// Del Rappy and Sand Rappy
				if ( aMob )
				{
					if ( r )
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x18].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x17].XP;
					}
				}
				else
				{
					if ( r )
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x06].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x05].XP;
					}
				}
			}
			else
			{
				// Rag Rappy, Al Rappy, Love Rappy and Seasonal Rappies
				if ( r )
				{
					if ( l->episode == 0x01 )
						mm->rt_index = 6; // Al Rappy
					else
					{
						switch ( shipEvent )
						{
						case 0x01:
							mm->rt_index = 79; // St. Rappy
							break;
						case 0x04:
							mm->rt_index = 81; // Easter Rappy
							break;
						case 0x05:
							mm->rt_index = 80; // Halo Rappy
							break;
						default:
							mm->rt_index = 51; // Love Rappy
							break;
						}
					}
					mm->exp = l->bptable[0x19].XP;
				}
				else
				{
					mm->rt_index = 5;
					mm->exp = l->bptable[0x18].XP;
				}
			}
			break;
		case 66:
			// Monest + 30 Mothmants
			mm->exp = l->bptable[0x01].XP;
			mm->rt_index = 4;

			for (ch=0;ch<30;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 3;
				mm->exp = l->bptable[0x00].XP;
			}
			break;
		case 67:
			// Savage Wolf and Barbarous Wolf
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				mm->rt_index = 8;
				mm->exp = l->bptable[0x03].XP;
			}
			else
			{
				mm->rt_index = 7;
				mm->exp = l->bptable[0x02].XP;
			}
			break;
		case 68:
			// Booma family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 11;
				mm->exp = l->bptable[0x4D].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 10;
					mm->exp = l->bptable[0x4C].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable[0x4B].XP;
				}
			break;
		case 96:
			// Grass Assassin
			mm->rt_index = 12;
			mm->exp = l->bptable[0x4E].XP;
			break;
		case 97:
			// Del Lily, Poison Lily, Nar Lily
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < lily_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( ( l->episode == 0x02 ) && ( aMob ) )
			{
				mm->rt_index = 83;
				mm->exp = l->bptable[0x25].XP;
			}
			else
				if ( r )
				{
					mm->rt_index = 14;
					mm->exp = l->bptable[0x05].XP;
				}
				else
				{
					mm->rt_index = 13;
					mm->exp = l->bptable[0x04].XP;
				}
			break;
		case 98:
			// Nano Dragon
			mm->rt_index = 15;
			mm->exp = l->bptable[0x1A].XP;
			break;
		case 99:
			// Shark family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 18;
				mm->exp = l->bptable[0x51].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 17;
					mm->exp = l->bptable[0x50].XP;
				}
				else
				{
					mm->rt_index = 16;
					mm->exp = l->bptable[0x4F].XP;
				}
			break;
		case 100:
			// Slime + 4 clones
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < slime_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 20;
				mm->exp = l->bptable[0x2F].XP;
			}
			else
			{
				mm->rt_index = 19;
				mm->exp = l->bptable[0x30].XP;
			}
			for (ch=0;ch<4;ch++)
			{
				l->mapIndex++;
				mm++;
				r = 0;
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < slime_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
				if ( r )
				{
					mm->rt_index = 20;
					mm->exp = l->bptable[0x2F].XP;
				}
				else
				{
					mm->rt_index = 19;
					mm->exp = l->bptable[0x30].XP;
				}
			}
			break;
		case 101:
			// Pan Arms, Migium, Hidoom
			mm->rt_index = 21;
			mm->exp = l->bptable[0x31].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 22;
			mm->exp = l->bptable[0x32].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 23;
			mm->exp = l->bptable[0x33].XP;
			break;
		case 128:
			// Dubchic and Gilchic
			if (mm->skin & 0x01)
			{
				mm->exp = l->bptable[0x1C].XP;
				mm->rt_index = 50;
			}
			else
			{
				mm->exp = l->bptable[0x1B].XP;
				mm->rt_index = 24;
			}
			break;
		case 129:
			// Garanz
			mm->rt_index = 25;
			mm->exp = l->bptable[0x1D].XP;
			break;
		case 130:
			// Sinow Beat and Gold
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				mm->rt_index = 27;
				mm->exp = l->bptable[0x13].XP;
			}
			else
			{
				mm->rt_index = 26;
				mm->exp = l->bptable[0x06].XP;
			}

			if ( ( mm->reserved[0] >> 16 ) == 0 )  
				l->mapIndex += 4; // Add 4 clones but only if there's no add value there already...
			break;
		case 131:
			// Canadine
			mm->rt_index = 28;
			mm->exp = l->bptable[0x07].XP;
			break;
		case 132:
			// Canadine Group
			mm->rt_index = 29;
			mm->exp = l->bptable[0x09].XP;
			for (ch=0;ch<8;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 28;
				mm->exp = l->bptable[0x08].XP;
			}
			break;
		case 133:
			// Dubwitch
			break;
		case 160:
			// Delsaber
			mm->rt_index = 30;
			mm->exp = l->bptable[0x52].XP;
			break;
		case 161:
			// Chaos Sorcerer + 2 Bits
			mm->rt_index = 31;
			mm->exp = l->bptable[0x0A].XP;
			l->mapIndex += 2;
			break;
		case 162:
			// Dark Gunner
			mm->rt_index = 34;
			mm->exp = l->bptable[0x1E].XP;
			break;
		case 164:
			// Chaos Bringer
			mm->rt_index = 36;
			mm->exp = l->bptable[0x0D].XP;
			break;
		case 165:
			// Dark Belra
			mm->rt_index = 37;
			mm->exp = l->bptable[0x0E].XP;
			break;
		case 166:
			// Dimenian family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 43;
				mm->exp = l->bptable[0x55].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 42;
					mm->exp = l->bptable[0x54].XP;
				}
				else
				{
					mm->rt_index = 41;
					mm->exp = l->bptable[0x53].XP;
				}
			break;
		case 167:
			// Bulclaw + 4 claws
			mm->rt_index = 40;
			mm->exp = l->bptable[0x1F].XP;
			for (ch=0;ch<4;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 38;
				mm->exp = l->bptable[0x20].XP;
			}
			break;
		case 168:
			// Claw
			mm->rt_index = 38;
			mm->exp = l->bptable[0x20].XP;
			break;
		case 192:
			// Dragon or Gal Gryphon
			if ( l->episode == 0x01 )
			{
				mm->rt_index = 44;
				mm->exp = l->bptable[0x12].XP;
			}
			else
				if ( l->episode == 0x02 )
				{
					mm->rt_index = 77;
					mm->exp = l->bptable[0x1E].XP;
				}
			break;
		case 193:
			// De Rol Le
			mm->rt_index = 45;
			mm->exp = l->bptable[0x0F].XP;
			break;
		case 194:
			// Vol Opt form 1
			break;
		case 197:
			// Vol Opt form 2
			mm->rt_index = 46;
			mm->exp = l->bptable[0x25].XP;
			break;
		case 200:
			// Dark Falz + 510 Helpers
			mm->rt_index = 47;
			if (l->difficulty)
				mm->exp = l->bptable[0x38].XP; // Form 2
			else
				mm->exp = l->bptable[0x37].XP;

			for (ch=0;ch<510;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->base = 200;
				mm->exp = l->bptable[0x35].XP;
			}
			break;
		case 202:
			// Olga Flow
			mm->rt_index = 78;
			mm->exp = l->bptable[0x2C].XP;
			l->mapIndex += 512;
			break;
		case 203:
			// Barba Ray
			mm->rt_index = 73;
			mm->exp = l->bptable[0x0F].XP;
			l->mapIndex += 47;
			break;
		case 204:
			// Gol Dragon
			mm->rt_index = 76;
			mm->exp = l->bptable[0x12].XP;
			l->mapIndex += 5;
			break;
		case 212:
			// Sinow Berill & Spigell
			/* if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) */
			if ( mm->skin >= 0x01 ) // set rare?
			{
				mm->rt_index = 63;
				mm->exp = l->bptable [0x13].XP;
			}
			else
			{
				mm->rt_index = 62;
				mm->exp = l->bptable [0x06].XP;
			}
			l->mapIndex += 4; // Add 4 clones which are never used...
			break;
		case 213:
			// Merillia & Meriltas
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 53;
				mm->exp = l->bptable [0x4C].XP;
			}
			else
			{
				mm->rt_index = 52;
				mm->exp = l->bptable [0x4B].XP;
			}
			break;
		case 214:
			if ( mm->skin & 0x02 )
			{
				// Mericus
				mm->rt_index = 58;
				mm->exp = l->bptable [0x46].XP;
			}
			else 
				if ( mm->skin & 0x01 )
				{
					// Merikle
					mm->rt_index = 57;
					mm->exp = l->bptable [0x45].XP;
				}
				else
				{
					// Mericarol
					mm->rt_index = 56;
					mm->exp = l->bptable [0x3A].XP;
				}
			break;
		case 215:
			// Ul Gibbon and Zol Gibbon
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 60;
				mm->exp = l->bptable [0x3C].XP;
			}
			else
			{
				mm->rt_index = 59;
				mm->exp = l->bptable [0x3B].XP;
			}
			break;
		case 216:
			// Gibbles
			mm->rt_index = 61;
			mm->exp = l->bptable [0x3D].XP;
			break;
		case 217:
			// Gee
			mm->rt_index = 54;
			mm->exp = l->bptable [0x07].XP;
			break;
		case 218:
			// Gi Gue
			mm->rt_index = 55;
			mm->exp = l->bptable [0x1A].XP;
			break;
		case 219:
			// Deldepth
			mm->rt_index = 71;
			mm->exp = l->bptable [0x30].XP;
			break;
		case 220:
			// Delbiter
			mm->rt_index = 72;
			mm->exp = l->bptable [0x0D].XP;
			break;
		case 221:
			// Dolmolm and Dolmdarl
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 65;
				mm->exp = l->bptable[0x50].XP;
			}
			else
			{
				mm->rt_index = 64;
				mm->exp = l->bptable[0x4F].XP;
			}
			break;
		case 222:
			// Morfos
			mm->rt_index = 66;
			mm->exp = l->bptable [0x40].XP;
			break;
		case 223:
			// Recobox & Recons
			mm->rt_index = 67;
			mm->exp = l->bptable[0x41].XP;
			num_recons = ( mm->reserved[0] >> 16 );
			for (ch=0;ch<num_recons;ch++)
			{
				if ( l->mapIndex >= 0xB50 )
					break;
				l->mapIndex++;
				mm++;
				mm->rt_index = 68;
				mm->exp = l->bptable[0x42].XP;
			}
			break;
		case 224:
			if ( ( l->episode == 0x02 ) && ( aMob ) )
			{
				// Epsilon
				mm->rt_index = 84;
				mm->exp = l->bptable[0x23].XP;
				l->mapIndex += 4;
			}
			else
			{
				// Sinow Zoa and Zele
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 70;
					mm->exp = l->bptable[0x44].XP;
				}
				else
				{
					mm->rt_index = 69;
					mm->exp = l->bptable[0x43].XP;
				}
			}
			break;
		case 225:
			// Ill Gill
			mm->rt_index = 82;
			mm->exp = l->bptable[0x26].XP;
			break;
		case 272:
			// Astark
			mm->rt_index = 1;
			mm->exp = l->bptable[0x09].XP;
			break;
		case 273:
			// Satellite Lizard and Yowie
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				if ( aMob )
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x1E].XP;
				}
				else
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x0E].XP;
				}
			}
			else
			{
				if ( aMob )
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x1D].XP;
				}
				else
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x0D].XP;
				}
			}
			break;
		case 274:
			// Merissa A/AA
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < merissa_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 5;
				mm->exp = l->bptable[0x1A].XP;
			}
			else
			{
				mm->rt_index = 4;
				mm->exp = l->bptable[0x19].XP;
			}
			break;
		case 275:
			// Girtablulu
			mm->rt_index = 6;
			mm->exp = l->bptable[0x1F].XP;
			break;
		case 276:
			// Zu and Pazuzu
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < pazuzu_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				if ( aMob )
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x1C].XP;
				}
				else
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x08].XP;
				}
			}
			else
			{
				if ( aMob )
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x1B].XP;
				}
				else
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x07].XP;
				}
			}
			break;
		case 277:
			// Boota family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 11;			
				mm->exp = l->bptable [0x03].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 10;
					mm->exp = l->bptable [0x01].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable [0x00].XP;
				}
			break;
		case 278:
			// Dorphon and Eclair
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < dorphon_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 13;
				mm->exp = l->bptable [0x10].XP;
			}
			else
			{
				mm->rt_index = 12;
				mm->exp = l->bptable [0x0F].XP;
			}
			break;
		case 279:
			// Goran family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 15;
				mm->exp = l->bptable [0x13].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 16;
					mm->exp = l->bptable [0x12].XP;
				}
				else
				{
					mm->rt_index = 14;
					mm->exp = l->bptable [0x11].XP;
				}
			break;
		case 281:
			// Saint Million, Shambertin, and Kondrieu
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x20 ) && ( mt_lrand() < kondrieu_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
				mm->rt_index = 21;
			else
			{
				if ( mm->skin & 0x01 )
					mm->rt_index = 20;
				else
					mm->rt_index = 19;
			}
			mm->exp = l->bptable [0x22].XP;
			break;
		default:
			//debug ("enemy not handled: %u", mm->base);
			break;
		}
		if ( mm->reserved[0] >> 16 ) // Have to do
			l->mapIndex += ( mm->reserved[0] >> 16 );
		l->mapIndex++;
	}
}

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
		ParseMapData ( l, (MAP_MONSTER*) &dp[0], aMob, num_records );
		//debug ("Added %u mids, total: %u", l->mapIndex - oldIndex, l->mapIndex );
	}
};
