void Send62 (PSO_CLIENT* client)
{
	PSO_CLIENT* lClient;
	unsigned bank_size, bank_use;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t,maxt;
	unsigned itemid;
	int dont_send = 1;
	LOBBY* l;
	unsigned rt_index = 0;
	unsigned rare_lookup, rare_rate, rare_item, 
		rare_roll, box_rare, ch, itemNum;
	unsigned short mid, count;
	unsigned char* rt_table;
	unsigned char* rt_table2;
	unsigned meseta;
	unsigned DAR;
	unsigned floor_check = 0;
	SHOP* shopp;
	SHOP_ITEM* shopi;
	PTDATA* ptd;
	MAP_BOX* mb;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;
	// don't support target @ 0x02
	t = client->decryptbuf[0x04];
	if (client->lobbyNum < 0x10)
		maxt = 12;
	else
		maxt = 4;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug ("Client sent a 0x62 packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	switch (client->decryptbuf[0x08])
	{
	case 0x06:
		// Send guild card
		if ( ( size == 0x0C ) && ( t < maxt ) )
		{
			if ((l->slot_use[t]) && (l->clients[t]))
			{
				lClient = l->clients[t];
				PrepGuildCard ( client->guildcard, lClient->guildcard );
				memset (&PacketData[0], 0, 0x114);
				sprintf (&PacketData[0x00], "\x14\x01\x60");
				PacketData[0x03] = 0x00;
				PacketData[0x04] = 0x00;
				PacketData[0x08] = 0x06;
				PacketData[0x09] = 0x43;
				*(unsigned *) &PacketData[0x0C] = client->guildcard;
				memcpy (&PacketData[0x10], &client->character.name[0], 24 );
				memcpy (&PacketData[0x60], &client->character.guildcard_text[0], 176);
				PacketData[0x110] = 0x01; // ?
				PacketData[0x112] = (char)client->character.sectionID;
				PacketData[0x113] = (char)client->character._class;
				cipher_ptr = &lClient->server_cipher;
				encryptcopy (lClient, &PacketData[0], 0x114);
			}
		}
		break;
	case 0x5A:
		if ( client->lobbyNum > 0x0F )
		{			
			itemid = *(unsigned *) &client->decryptbuf[0x0C];
			if ( AddItemToClient ( itemid, client ) == 1 )
			{
				memset (&PacketData[0], 0, 16);
				PacketData[0x00] = 0x14;
				PacketData[0x02] = 0x60;
				PacketData[0x08] = 0x59;
				PacketData[0x09] = 0x03;
				PacketData[0x0A] = (unsigned char) client->clientID;
				PacketData[0x0E] = client->decryptbuf[0x10];
				PacketData[0x0C] = (unsigned char) client->clientID;
				*(unsigned *) &PacketData[0x10] = itemid;
				SendToLobby ( client->lobby, 4, &PacketData[0x00], 0x14, 0);
			}
		}
		else
			client->todc = 1;
		break;
	case 0x60:
		// Requesting a drop from a monster.
		if ( client->lobbyNum > 0x0F ) 
		{
			if ( !l->drops_disabled )
			{
				mid = *(unsigned short*) &client->decryptbuf[0x0E];
				mid &= 0xFFF;				

				if ( ( mid < 0xB50 ) && ( l->monsterData[mid].drop == 0 ) )
				{
					if (l->episode == 0x02)
						ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
					else
						ptd = &pt_tables_ep1[client->character.sectionID][l->difficulty];

					if ( ( l->episode == 0x01 ) && ( client->decryptbuf[0x0D] == 35 ) &&
						 ( l->mapData[mid].rt_index == 34 ) )
						rt_index = 35; // Save Death Gunner index...
					else
						rt_index = l->mapData[mid].rt_index; // Use map's index instead of what the client says...

					if ( rt_index < 0x64 )
					{
						if ( l->episode == 0x03 )
						{
							if ( rt_index < 0x16 )
							{
								meseta = ep4_rtremap[(rt_index * 2)+1];
								// Past a certain point is Episode II data...
								if ( meseta > 0x2F )
									ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
							}
							else
								meseta = 0;
						}
						else
							meseta = rt_index;
						if ( ( l->episode == 0x03 ) && 
							 ( rt_index >= 19 ) && 
							 ( rt_index <= 21 ) )
							DAR = 1;
						else
						{
							if ( ( ptd->enemy_dar[meseta] == 100 ) || ( l->redbox ) )
								DAR = 1;
							else
							{

								DAR = 100 - ptd->enemy_dar[meseta];
								if ( ( mt_lrand() % 100 ) >= DAR )
									DAR = 1;
								else
									DAR = 0;
							}
						}
					}
					else
						DAR = 0;

					if ( DAR )
					{
						if (rt_index < 0x64)
						{
							rt_index += ((0x1400 * l->difficulty) + ( client->character.sectionID * 0x200 ));
							switch (l->episode)
							{
							case 0x02:
								rare_lookup = rt_tables_ep2[rt_index];
								break;
							case 0x03:
								rare_lookup = rt_tables_ep4[rt_index];
								break;
							default:
								rare_lookup = rt_tables_ep1[rt_index];
								break;
							}
							rare_rate = ExpandDropRate ( rare_lookup & 0xFF );
							rare_item = rare_lookup >> 8;
							rare_roll = mt_lrand();
							//debug ("rare_roll = %u", rare_roll );
							if  ( ( ( rare_lookup & 0xFF ) != 0 ) && ( ( rare_roll < rare_rate ) || ( l->redbox ) ) )
							{
								// Drop a rare item
								itemNum = free_game_item (l);
								memset (&l->gameItem[itemNum].item.data[0], 0, 12);
								memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
								memcpy (&l->gameItem[itemNum].item.data[0], &rare_item, 3);
								GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
								l->gameItem[itemNum].item.itemid = l->itemID++;
							}
							else
							{
								// Drop a common item
								itemNum = free_game_item (l);
								if ( ( ( mt_lrand() % 100 ) < 60 ) || ( ptd->enemy_drop < 0 ) )
								{
									memset (&l->gameItem[itemNum].item.data[0], 0, 12 );
									memset (&l->gameItem[itemNum].item.data2[0], 0, 4 );
									l->gameItem[itemNum].item.data[0] = 0x04;
									rt_index = meseta;
									meseta  = ptd->enemy_meseta[rt_index][0];
									if ( ptd->enemy_meseta[rt_index][1] > ptd->enemy_meseta[rt_index][0] )
										meseta += mt_lrand() % ( ( ptd->enemy_meseta[rt_index][1] - ptd->enemy_meseta[rt_index][0] ) + 1 );
									*(unsigned *) &l->gameItem[itemNum].item.data2[0] = meseta;
									l->gameItem[itemNum].item.itemid = l->itemID++;
								}
								else
								{
									rt_index = meseta;
									GenerateCommonItem (ptd->enemy_drop[rt_index], 1, client->character.sectionID, &l->gameItem[itemNum], l, client);
								}
							}

							if ( l->gameItem[itemNum].item.itemid != 0 )
							{
								if (l->gameItemCount < MAX_SAVED_ITEMS)
									l->gameItemList[l->gameItemCount++] = itemNum;
								memset (&PacketData[0x00], 0, 16);
								PacketData[0x00] = 0x30;
								PacketData[0x01] = 0x00;
								PacketData[0x02] = 0x60;
								PacketData[0x03] = 0x00;
								PacketData[0x08] = 0x5F;
								PacketData[0x09] = 0x0D;
								*(unsigned *) &PacketData[0x0C] = *(unsigned *) &client->decryptbuf[0x0C];
								memcpy (&PacketData[0x10], &client->decryptbuf[0x10], 10);
								memcpy (&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12 );
								*(unsigned *) &PacketData[0x28] = l->gameItem[itemNum].item.itemid;
								*(unsigned *) &PacketData[0x2C] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
								SendToLobby ( client->lobby, 4, &PacketData[0], 0x30, 0);
							}
						}
					}
					l->monsterData[mid].drop = 1;
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0x6F:
	case 0x71:
		if ( ( client->lobbyNum > 0x0F ) && ( t < maxt ) )
		{
			if (l->leader == client->clientID)
			{
				if ((l->slot_use[t]) && (l->clients[t]))
				{
					if (l->clients[t]->bursting == 1)
						dont_send = 0; // More user joining game stuff...
				}
			}
		}
		break;
	case 0xA2:
		if (client->lobbyNum > 0x0F)
		{
			if (!l->drops_disabled)
			{
				// box drop
				mid = *(unsigned short*) &client->decryptbuf[0x0E];
				mid &= 0xFFF;

				if ( ( mid < 0xB50 ) && ( l->boxHit[mid] == 0 ) )
				{
					box_rare = 0;
					mb = 0;
					
					//debug ("quest loaded: %i", l->quest_loaded);

					if ( ( l->quest_loaded ) && ( (unsigned) l->quest_loaded <= numQuests ) )
					{
						QUEST* q;

						q = &quests[l->quest_loaded - 1];
						if ( mid < q->max_objects )
							mb = (MAP_BOX*) &q->objectdata[(unsigned) ( 68 * mid ) + 0x28];
					}
					else
						mb = &l->objData[mid];

					if ( mb )
					{

						if ( mb->flag1 == 0 )
						{
							if ( ( ( mb->flag2 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
								 ( ( mb->flag2 + FLOAT_PRECISION ) > (float) 1.00000 ) )
							{
								// Fixed item alert!!!

								box_rare = 1;
								itemNum = free_game_item (l);
								if ( ( ( mb->flag3 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
									 ( ( mb->flag3 + FLOAT_PRECISION ) > (float) 1.00000 ) )
								{
									// Fully fixed!

									*(unsigned *) &l->gameItem[itemNum].item.data[0] = *(unsigned *) &mb->drop[0];

									// Not used... for now.
									l->gameItem[itemNum].item.data[3] = 0;

									if (l->gameItem[itemNum].item.data[0] == 0x04)
										GenerateCommonItem (0x04, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
									else
										if ((l->gameItem[itemNum].item.data[0] == 0x00) && 
											(l->gameItem[itemNum].item.data[1] == 0x00))
											GenerateCommonItem (0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
										else
										{
											memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
											if (l->gameItem[itemNum].item.data[0] <  0x02)
												l->gameItem[itemNum].item.data[1]++; // Fix item offset
											GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
											l->gameItem[itemNum].item.itemid = l->itemID++;
										}
								}
								else
									GenerateCommonItem (mb->drop[0], 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
							}
						}
					}

					if (!box_rare)
					{
						switch (l->episode)
						{
						case 0x02:
							rt_table = (unsigned char*) &rt_tables_ep2[0];
							break;
						case 0x03:
							rt_table = (unsigned char*) &rt_tables_ep4[0];
							break;
						default:
							rt_table = (unsigned char*) &rt_tables_ep1[0];
							break;
						}
						rt_table += ( ( 0x5000 * l->difficulty ) + ( client->character.sectionID * 0x800 ) ) + 0x194;
						rt_table2 = rt_table + 0x1E;
						rare_item = 0;

						switch ( l->episode )
						{
						case 0x01:
							switch ( l->floor[client->clientID ] )
							{
							case 11:
								// dragon
								floor_check = 3;
								break;
							case 12:
								// de rol
								floor_check = 6;
								break;
							case 13:
								// vol opt
								floor_check = 8;
								break;
							case 14:
								// falz
								floor_check = 10;
								break;
							default:
								floor_check = l->floor[client->clientID ];
								break;
							}	
							break;
						case 0x02:
							switch ( l->floor[client->clientID ] )
							{
							case 14:
								// barba ray
								floor_check = 3;
								break;
							case 15:
								// gol dragon
								floor_check = 6;
								break;
							case 12:
								// gal gryphon
								floor_check = 9;
								break;
							case 13:
								// olga flow
								floor_check = 10;
								break;
							default:
								// could be tower
								if ( l->floor[client->clientID] <= 11 )
									floor_check = ep2_rtremap[(l->floor[client->clientID] * 2)+1];
								else
									floor_check = 10;
								break;
							}	
							break;
						case 0x03:
							floor_check = l->floor[client->clientID ];
							break;
						}

						for (ch=0;ch<30;ch++)
						{
							if (*rt_table == floor_check)
							{
								rare_rate = ExpandDropRate ( *rt_table2 );
								memcpy (&rare_item, &rt_table2[1], 3);
								rare_roll = mt_lrand();
								if ( ( rare_roll < rare_rate ) || ( l->redbox == 1 ) )
								{
									box_rare = 1;
									itemNum = free_game_item (l);
									memset (&l->gameItem[itemNum].item.data[0], 0, 12);
									memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
									memcpy (&l->gameItem[itemNum].item.data[0], &rare_item, 3);
									GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
									l->gameItem[itemNum].item.itemid = l->itemID++;
									break;
								}
							}
							rt_table++;
							rt_table2 += 0x04;
						}
					}

					if (!box_rare)
					{
						itemNum = free_game_item (l);
						GenerateCommonItem (0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
					}

					if (l->gameItem[itemNum].item.itemid != 0)
					{
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memset (&PacketData[0], 0, 16);
						PacketData[0x00] = 0x30;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5F;
						PacketData[0x09] = 0x0D;
						*(unsigned *) &PacketData[0x0C] = *(unsigned *) &client->decryptbuf[0x0C];
						memcpy (&PacketData[0x10], &client->decryptbuf[0x10], 10);
						memcpy (&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12 );
						*(unsigned *) &PacketData[0x28] = l->gameItem[itemNum].item.itemid;
						*(unsigned *) &PacketData[0x2C] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
						SendToLobby ( client->lobby, 4, &PacketData[0], 0x30, 0);
					}
					l->boxHit[mid] = 1;
				}
			}
		}
		break;
	case 0xA6:
		// Trade (not done yet)
		break;
	case 0xAE:
		// Chair info
		if ((size == 0x18) && (client->lobbyNum < 0x10) && (t < maxt))
			dont_send = 0;
		break;
	case 0xB5:
		// Client requesting shop
		if ( client->lobbyNum > 0x0F )
		{			 
			if ((l->floor[client->clientID] == 0) 
				&& (client->decryptbuf[0x0C] < 0x03))
			{
				client->doneshop[client->decryptbuf[0x0C]] = shopidx[client->character.level] + ( 333 * ((unsigned)client->decryptbuf[0x0C]) ) + ( mt_lrand() % 333 ) ;
				shopp = &shops[client->doneshop[client->decryptbuf[0x0C]]];
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, (unsigned char*) &shopp->packet_length, shopp->packet_length);
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB7:
		// Client buying an item
		if ( client->lobbyNum > 0x0F )
		{
			if ((l->floor[client->clientID] == 0)
				&& (client->decryptbuf[0x10] < 0x03) 
				&& (client->doneshop[client->decryptbuf[0x10]]))
			{
				if (client->decryptbuf[0x11] < shops[client->doneshop[client->decryptbuf[0x10]]].num_items)
				{
					shopi = &shops[client->doneshop[client->decryptbuf[0x10]]].item[client->decryptbuf[0x11]];
					if ((client->decryptbuf[0x12] > 1) && (shopi->data[0] != 0x03))
						client->todc = 1;
					else
						if (client->character.meseta < ((unsigned)client->decryptbuf[0x12] * shopi->price))
						{
							Send1A ("Not enough meseta for purchase.", client);
							client->todc = 1;
						}
						else
						{
							INVENTORY_ITEM i;

							memset (&i, 0, sizeof (INVENTORY_ITEM));
							memcpy (&i.item.data[0], &shopi->data[0], 12);
							// Update player item ID
							l->playerItemID[client->clientID] = *(unsigned *) &client->decryptbuf[0x0C];
							i.item.itemid = l->playerItemID[client->clientID]++;
							AddToInventory (&i, client->decryptbuf[0x12], 1, client);
							DeleteMesetaFromClient (shopi->price * (unsigned) client->decryptbuf[0x12], 0, client);
						}
				}
				else
					client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB8:
		// Client is tekking a weapon.
		if ( client->lobbyNum > 0x0F )
		{
			unsigned compare_item;

			INVENTORY_ITEM* i;

			i = NULL;

			compare_item = *(unsigned *) &client->decryptbuf[0x0C];

			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				if ((client->character.inventory[ch].item.itemid == compare_item) &&
					(client->character.inventory[ch].item.data[0] == 0x00) &&
					(client->character.inventory[ch].item.data[4] & 0x80) &&
					(client->character.meseta >= 100))
				{
					char percent_mod;
					unsigned attrib;

					i = &client->character.inventory[ch];
					attrib = i->item.data[4] & ~(0x80);
					
					client->tekked = *i;

					if ( attrib < 0x29)
					{
						client->tekked.item.data[4] = tekker_attributes [( attrib * 3) + 1];
						if ( ( mt_lrand() % 100 ) > 70 )
							client->tekked.item.data[4] += mt_lrand() % ( ( tekker_attributes [(attrib * 3) + 2] - tekker_attributes [(attrib * 3) + 1] ) + 1 );
					}
					else
						client->tekked.item.data[4] = 0;
					if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = -10;
					else
						if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = -5;
						else
							if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = 5;
							else
								if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = 10;
								else
									percent_mod = 0;
					if ((!(i->item.data[6] & 128)) && (i->item.data[7] > 0))
						(char)client->tekked.item.data[7] += percent_mod;
					if ((!(i->item.data[8] & 128)) && (i->item.data[9] > 0))
						(char)client->tekked.item.data[9] += percent_mod;
					if ((!(i->item.data[10] & 128)) && (i->item.data[11] > 0))
						(char)client->tekked.item.data[11] += percent_mod;
					DeleteMesetaFromClient (100, 0, client);
					memset (&client->encryptbuf[0x00], 0, 0x20);
					client->encryptbuf[0x00] = 0x20;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xB9;
					client->encryptbuf[0x09] = 0x08;
					client->encryptbuf[0x0A] = 0x79;
					memcpy (&client->encryptbuf[0x0C], &client->tekked.item.data[0], 16);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x20);
					break;
				}
			}

			if ( i == NULL )
			{
				Send1A ("Could not find item to Tek.", client);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBA:
		// Client accepting tekked version of weapon.
		if ( ( client->lobbyNum > 0x0F ) && ( client->tekked.item.itemid ) )
		{
			unsigned ch2;

			for (ch=0;ch<4;ch++)
			{
				if ((l->slot_use[ch]) && (l->clients[ch]))
				{
					for (ch2=0;ch2<l->clients[ch]->character.inventoryUse;ch2++)
						if (l->clients[ch]->character.inventory[ch2].item.itemid == client->tekked.item.itemid)
						{
							Send1A ("Item duplication attempt!", client);
							client->todc = 1;
							break;
						}
				}
			}

			for (ch=0;ch<l->gameItemCount;l++)
			{
				itemNum = l->gameItemList[ch];
				if (l->gameItem[itemNum].item.itemid == client->tekked.item.itemid)
				{
					// Added to the game's inventory by the client...
					// Delete it and avoid duping...
					memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
					l->gameItemList[ch] = 0xFFFFFFFF;
					break;
				}
			}

			CleanUpGameInventory (l);

			if (!client->todc)
			{
				AddToInventory (&client->tekked, 1, 1, client);
				memset (&client->tekked, 0, sizeof (INVENTORY_ITEM));
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBB:
		// Client accessing bank
		if ( client->lobbyNum < 0x10 )
			client->todc = 1;
		else
		{
			if ( ( l->floor[client->clientID] == 0) && ( ((unsigned) servertime - client->command_cooldown[0xBB]) >= 1 ))
			{
				client->command_cooldown[0xBB] = (unsigned) servertime;

				/* Which bank are we accessing? */

				client->bankAccess = client->bankType;

				if (client->bankAccess)
					memcpy (&client->character.bankUse, &client->common_bank, sizeof (BANK));
				else
					memcpy (&client->character.bankUse, &client->char_bank, sizeof (BANK));

				for (ch=0;ch<client->character.bankUse;ch++)
					client->character.bankInventory[ch].itemid = l->bankItemID[client->clientID]++;
				memset (&client->encryptbuf[0x00], 0, 0x34);
				client->encryptbuf[0x02] = 0x6C;
				client->encryptbuf[0x08] = 0xBC;
				bank_size = 0x18 * (client->character.bankUse + 1);
				*(unsigned *) &client->encryptbuf[0x0C] = bank_size;
				bank_size += 4;
				*(unsigned short *) &client->encryptbuf[0x00] = (unsigned short) bank_size;
				bank_use = mt_lrand();
				*(unsigned *) &client->encryptbuf[0x10] = bank_use;
				bank_use = client->character.bankUse;
				*(unsigned *) &client->encryptbuf[0x14] = bank_use;
				*(unsigned *) &client->encryptbuf[0x18] = client->character.bankMeseta;
				if (client->character.bankUse)
					memcpy (&client->encryptbuf[0x1C], &client->character.bankInventory[0], sizeof (BANK_ITEM) * client->character.bankUse);
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, &client->encryptbuf[0x00], bank_size);
			}
		}
		break;
	case 0xBD:
		if ( client->lobbyNum < 0x10 )
		{
			dont_send = 1;
			client->todc = 1;
		}
		else
		{
			if ( l->floor[client->clientID] == 0)
			{
				switch (client->decryptbuf[0x14])
				{
				case 0x00: 
					// Making a deposit
					itemid = *(unsigned *) &client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *) &client->decryptbuf[0x10];

						if (client->character.meseta >= meseta)
						{
							client->character.bankMeseta += meseta;
							client->character.meseta -= meseta;
							if (client->character.bankMeseta > 999999)
								client->character.bankMeseta = 999999;
						}
						else
						{
							Send1A ("Client/server data synchronization error.", client);
							client->todc = 1;
						}
					}
					else
					{
						if ( client->character.bankUse < 200 )
						{
							// Depositing something else...
							count = client->decryptbuf[0x15];
							DepositIntoBank (itemid, count, client);
							if (!client->todc)
								SortBankItems(client);
						}
						else
						{						
							Send1A ("Can't deposit.  Bank is full.", client);
							client->todc = 1;
						}
					}
					break;
				case 0x01:
					itemid = *(unsigned *) &client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *) &client->decryptbuf[0x10];
						if (client->character.bankMeseta >= meseta)
						{
							client->character.bankMeseta -= meseta;
							client->character.meseta += meseta;
						}
						else
							client->todc = 1;
					}
					else
					{
						// Withdrawing something else...
						count = client->decryptbuf[0x15];
						WithdrawFromBank (itemid, count, client);
					}
					break;
				default:
					break;
				}

				/* Update bank. */

				if (client->bankAccess)
					memcpy (&client->common_bank, &client->character.bankUse, sizeof (BANK));
				else
					memcpy (&client->char_bank, &client->character.bankUse, sizeof (BANK));

			}
		}
		break;
	case 0xC1:
	case 0xC2:
	//case 0xCD:
	//case 0xCE:
		if (t < maxt)
		{
			// Team invite for C1 & C2, Master Transfer for CD & CE.
			if (size == 0x64)
				dont_send = 0;

			if (client->decryptbuf[0x08] == 0xC2)
			{
				unsigned gcn;

				gcn = *(unsigned *) &client->decryptbuf[0x0C];
				if ((client->decryptbuf[0x10] == 0x02) &&
					(client->guildcard == gcn))
					client->teamaccept = 1;
			}

			if (client->decryptbuf[0x08] == 0xCD)
			{
				if (client->character.privilegeLevel != 0x40)
				{
					dont_send = 1;
					Send01 ("You aren't the master of your team.", client);
				}
				else
					client->masterxfer = 1;
			}
		}
		break;
	case 0xC9:
		if ( client->lobbyNum > 0x0F )
		{
			INVENTORY_ITEM add_item;
			int meseta;

			if ( l->quest_loaded )
			{
				meseta = *(int *) &client->decryptbuf[0x0C];
				if (meseta < 0)
				{
					meseta = -meseta;
					client->character.meseta -= (unsigned) meseta;
				}
				else
				{
					memset (&add_item, 0, sizeof (INVENTORY_ITEM));
					add_item.item.data[0] = 0x04;
					*(unsigned *) &add_item.item.data2[0] = *(unsigned *) &client->decryptbuf[0x0C];
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory (&add_item, 1, 0, client);
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xCA:
		if ( client->lobbyNum > 0x0F )
		{
			INVENTORY_ITEM add_item;

			if ( l->quest_loaded )
			{
				unsigned ci, compare_item = 0;

				memset (&add_item, 0, sizeof (INVENTORY_ITEM));
				memcpy ( &compare_item, &client->decryptbuf[0x0C], 3 );
				for ( ci = 0; ci < quest_numallows; ci ++)
				{
					if (compare_item == quest_allow[ci])
					{
						add_item.item.data[0] = 0x01;
						break;
					}
				}
				if (add_item.item.data[0] == 0x01)
				{
					memcpy (&add_item.item.data[0], &client->decryptbuf[0x0C], 12);
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory (&add_item, 1, 0, client);
				}
				else
				{
					SendEE ("You did not receive the quest reward.  The item requested is not on the allow list.  Your request and guild card have been logged for the server administrator.", client);
					WriteLog ("User %u attempted to claim quest reward %08x but item is not in the allow list.", client->guildcard, compare_item );
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xD0:
		// Level up player?
		// Player to level @ 0x0A
		// Levels to gain @ 0x0C
		if ( ( t < maxt ) && ( l->battle ) && ( l->quest_loaded ) )
		{
			if ( ( client->decryptbuf[0x0A] < 4 ) && ( l->clients[client->decryptbuf[0x0A]] ) )
			{
				unsigned target_lv;

				lClient = l->clients[client->decryptbuf[0x0A]];
				target_lv = lClient->character.level;
				target_lv += client->decryptbuf[0x0C];
				
				if ( target_lv > 199 )
					 target_lv = 199;

				SkipToLevel ( target_lv, lClient, 0 );
			}
		}
		break;
	case 0xD6:
		// Wrap an item
		if ( client->lobbyNum > 0x0F )
		{
			unsigned wrap_id;
			INVENTORY_ITEM backup_item;

			memset (&backup_item, 0, sizeof (INVENTORY_ITEM));
			wrap_id = *(unsigned *) &client->decryptbuf[0x18];

			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				if (client->character.inventory[ch].item.itemid == wrap_id)
				{
					memcpy (&backup_item, &client->character.inventory[ch], sizeof (INVENTORY_ITEM));
					break;
				}
			}

			if (backup_item.item.itemid)
			{
				DeleteFromInventory (&backup_item, 1, client);					
				if (!client->todc)
				{
					if (backup_item.item.data[0] == 0x02)
						backup_item.item.data2[2] |= 0x40; // Wrap a mag
					else
						backup_item.item.data[4] |= 0x40; // Wrap other
					AddToInventory (&backup_item, 1, 0, client);
				}
			}
			else
			{
				Send1A ("Could not find item to wrap.", client);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xDF:
		if ( client->lobbyNum > 0x0F )
		{
			if ( ( l->oneperson ) && ( l->quest_loaded ) && ( !l->drops_disabled ) )
			{
				INVENTORY_ITEM work_item;

				memset (&work_item, 0, sizeof (INVENTORY_ITEM));
				work_item.item.data[0] = 0x03;
				work_item.item.data[1] = 0x10;
				work_item.item.data[2] = 0x02;
				DeleteFromInventory (&work_item, 1, client);
				if (!client->todc)
					l->drops_disabled = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xE0:
		if ( client->lobbyNum > 0x0F )
		{
			if ( ( l->oneperson ) && ( l->quest_loaded ) && ( l->drops_disabled ) && ( !l->questE0 ) )
			{
				unsigned bp, bpl, new_item;

				if ( client->decryptbuf[0x0D] > 0x03 )
					bpl = 1;
				else
					bpl = l->difficulty + 1;

				for (bp=0;bp<bpl;bp++)
				{
					new_item = 0;

					switch ( client->decryptbuf[0x0D] )
					{
					case 0x00:
						// bp1 dorphon route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_dorphon_normal[mt_lrand() % (sizeof(bp_dorphon_normal)/4)];
							break;
						case 0x01:
							new_item = bp_dorphon_hard[mt_lrand() % (sizeof(bp_dorphon_hard)/4)];
							break;
						case 0x02:
							new_item = bp_dorphon_vhard[mt_lrand() % (sizeof(bp_dorphon_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_dorphon_ultimate[mt_lrand() % (sizeof(bp_dorphon_ultimate)/4)];
							break;
						}
						break;
					case 0x01:
						// bp1 rappy route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_rappy_normal[mt_lrand() % (sizeof(bp_rappy_normal)/4)];
							break;
						case 0x01:
							new_item = bp_rappy_hard[mt_lrand() % (sizeof(bp_rappy_hard)/4)];
							break;
						case 0x02:
							new_item = bp_rappy_vhard[mt_lrand() % (sizeof(bp_rappy_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_rappy_ultimate[mt_lrand() % (sizeof(bp_rappy_ultimate)/4)];
							break;
						}
						break;
					case 0x02:
						// bp1 zu route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_zu_normal[mt_lrand() % (sizeof(bp_zu_normal)/4)];
							break;
						case 0x01:
							new_item = bp_zu_hard[mt_lrand() % (sizeof(bp_zu_hard)/4)];
							break;
						case 0x02:
							new_item = bp_zu_vhard[mt_lrand() % (sizeof(bp_zu_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_zu_ultimate[mt_lrand() % (sizeof(bp_zu_ultimate)/4)];
							break;
						}
						break;
					case 0x04:
						// bp2
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp2_normal[mt_lrand() % (sizeof(bp2_normal)/4)];
							break;
						case 0x01:
							new_item = bp2_hard[mt_lrand() % (sizeof(bp2_hard)/4)];
							break;
						case 0x02:
							new_item = bp2_vhard[mt_lrand() % (sizeof(bp2_vhard)/4)];
							break;
						case 0x03:
							new_item = bp2_ultimate[mt_lrand() % (sizeof(bp2_ultimate)/4)];
							break;
						}
						break;
					}
					l->questE0 = 1;
					memset (&client->encryptbuf[0x00], 0, 0x2C);
					client->encryptbuf[0x00] = 0x2C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x5D;
					client->encryptbuf[0x09] = 0x09;
					client->encryptbuf[0x0A] = 0xFF;
					client->encryptbuf[0x0B] = 0xFB;
					memcpy (&client->encryptbuf[0x0C], &client->decryptbuf[0x0C], 12 );
					*(unsigned *) &client->encryptbuf[0x18] = new_item;
					*(unsigned *) &client->encryptbuf[0x24] = l->itemID;
					itemNum = free_game_item (l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
					*(unsigned *) &l->gameItem[itemNum].item.data[0] = new_item;
					if (new_item == 0x04)
					{
						new_item  = pt_tables_ep1[client->character.sectionID][l->difficulty].enemy_meseta[0x2E][0];
						new_item += mt_lrand() % 100;
						*(unsigned *) &client->encryptbuf[0x28] = new_item;
						*(unsigned *) &l->gameItem[itemNum].item.data2[0] = new_item;
					}
					if (l->gameItem[itemNum].item.data[0] == 0x00)
					{
						l->gameItem[itemNum].item.data[4] = 0x80; // Untekked
						client->encryptbuf[0x1C] = 0x80;
					}
					l->gameItem[itemNum].item.itemid = l->itemID++;
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
				}
			}
		}
		break;
	default:
		if (client->lobbyNum > 0x0F)
		{
			WriteLog ("62 command \"%02x\" not handled in game. (Data below)", client->decryptbuf[0x08]);
			packet_to_text ( &client->decryptbuf[0x00], size );
			WriteLog ("%s", &dp[0]);
		}
		break;
	}

	if ( ( dont_send == 0 ) && ( !client->todc ) )
	{
		if ((l->slot_use[t]) && (l->clients[t]))
		{
			lClient = l->clients[t];
			cipher_ptr = &lClient->server_cipher;
			encryptcopy (lClient, &client->decryptbuf[0], size);
		}
	}
}

void Send08(PSO_CLIENT* client)
{
	BLOCK* b;
	unsigned ch,ch2,qNum;
	unsigned char game_flags, total_games;
	LOBBY* l;
	unsigned Offset;
	QUEST* q;

	if (client->block <= serverBlocks)
	{
		total_games = 0;
		b = blocks[client->block-1];
		Offset = 0x34;
		for (ch=16;ch<(16+SHIP_COMPILED_MAX_GAMES);ch++)
		{
			l = &b->lobbies[ch];
			if (l->in_use)
			{
				memset (&PacketData[Offset], 0, 44);
				// Output game
				Offset += 2;
				PacketData[Offset] = 0x03;
				Offset += 2;
				*(unsigned *) &PacketData[Offset] = ch;
				Offset += 4;
				PacketData[Offset++] = 0x22 + l->difficulty;
				PacketData[Offset++] = l->lobbyCount;
				memcpy(&PacketData[Offset], &l->gameName[0], 30);
				Offset += 32;
				if (!l->oneperson)
					PacketData[Offset++] = 0x40 + l->episode;
				else
					PacketData[Offset++] = 0x10 + l->episode;
				if (l->inpquest)
				{
					game_flags = 0x80;
					// Grey out Government quests that the player is not qualified for...
					q = &quests[l->quest_loaded - 1];
					memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qNum = (unsigned) atoi ( &dp[0] );
					switch (l->episode)
					{
					case 0x01:
						qNum -= 401;
						qNum <<= 1;
						qNum += 0x1F3;
						for (ch2=0x1F5;ch2<=qNum;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x02:
						qNum -= 451;
						qNum <<= 1;
						qNum += 0x211;
						for (ch2=0x213;ch2<=qNum;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x03:
						qNum -= 701;
						qNum += 0x2BC;
						for (ch2=0x2BD;ch2<=qNum;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					}
				}
				else
					game_flags = 0x40;
				// Get flags for battle and one person games...
				if ((l->gamePassword[0x00] != 0x00) || 
					(l->gamePassword[0x01] != 0x00))
					game_flags |= 0x02;
				if ((l->quest_in_progress) || (l->oneperson)) // Can't join!
					game_flags |= 0x04;
				if (l->battle)
					game_flags |= 0x10;
				if (l->challenge)
					game_flags |= 0x20;
				// Wonder what flags 0x01 and 0x08 control....
				PacketData[Offset++] = game_flags;
				total_games++;
			}
		}
		*(unsigned short*) &client->encryptbuf[0x00] = (unsigned short) Offset;
		memcpy (&client->encryptbuf[0x02], &Packet08[2], 0x32);
		client->encryptbuf[0x04] = total_games;
		client->encryptbuf[0x08] = (unsigned char) client->lobbyNum;
		if ( client->block == 10 )
		{
			client->encryptbuf[0x1C] = 0x31;
			client->encryptbuf[0x1E] = 0x30;
		}
		else
			client->encryptbuf[0x1E] = 0x30 + client->block;

		if ( client->lobbyNum > 9 )
			client->encryptbuf[0x24] = 0x30 - ( 10 - client->lobbyNum );
		else
			client->encryptbuf[0x22] = 0x30 + client->lobbyNum;
		memcpy (&client->encryptbuf[0x34], &PacketData[0x34], Offset - 0x34);
		cipher_ptr = &client->server_cipher;
		encryptcopy ( client, &client->encryptbuf[0x00], Offset );
	}
}

void Send1A (const char *mes, PSO_CLIENT* client)
{
	unsigned short x1A_Len;

	memcpy (&PacketData[0], &Packet1A[0], sizeof (Packet1A));
	x1A_Len = sizeof (Packet1A);

	while (*mes != 0x00)
	{
		PacketData[x1A_Len++] = *(mes++);
		PacketData[x1A_Len++] = 0x00;
	}

	PacketData[x1A_Len++] = 0x00;
	PacketData[x1A_Len++] = 0x00;

	while (x1A_Len % 8)
		PacketData[x1A_Len++] = 0x00;

	*(unsigned short*) &PacketData[0] = x1A_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], x1A_Len);
}

void Send1D (PSO_CLIENT* client)
{
	unsigned num_minutes;

	if ((((unsigned) servertime - client->savetime) / 60L) >= 5)
	{
		// Backup character data every 5 minutes.
		client->savetime = (unsigned) servertime;
		ShipSend04 (0x02, client, logon);
	}

	num_minutes = ((unsigned) servertime - client->response) / 60L;
	if (num_minutes)
	{
		if (num_minutes > 2)
			initialize_connection (client); // If the client hasn't responded in over two minutes, drop the connection.
		else
		{
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &Packet1D[0], sizeof (Packet1D));
		}
	}
}

void Send83 (PSO_CLIENT* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &Packet83[0], sizeof (Packet83));

}

void Send64 (PSO_CLIENT* client)
{
	LOBBY* l;
	unsigned Offset;
	unsigned ch;

	if (!client->lobby)
		return;
	l = (LOBBY*) client->lobby;

	for (ch=0;ch<4;ch++)
	{
		if (!l->slot_use[ch])
		{
			l->slot_use[ch] = 1; // Slot now in use
			l->clients[ch] = client;
			// lobbyNum should be set before joining the game
			client->clientID = ch;
			l->gamePlayerCount++;
			l->gamePlayerID[ch] = l->gamePlayerCount;
			break;
		}
	}
	l->lobbyCount = 0;
	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->clients[ch]))
			l->lobbyCount++;
	}
	memset (&PacketData[0], 0, 0x1A8);
	PacketData[0x00] = 0xA8;
	PacketData[0x01] = 0x01;
	PacketData[0x02] = 0x64;
	PacketData[0x04] = (unsigned char) l->lobbyCount;
	memcpy (&PacketData[0x08], &l->gameMap[0], 128 );
	Offset = 0x88;
	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->clients[ch]))
		{
			PacketData[Offset+2] = 0x01;
			Offset += 0x04;
			*(unsigned *) &PacketData[Offset] = l->clients[ch]->guildcard;
			Offset += 0x10;
			PacketData[Offset] = l->clients[ch]->clientID;
			Offset += 0x0C;
			memcpy (&PacketData[Offset], &l->clients[ch]->character.name[0], 24);
			Offset += 0x20;
			PacketData[Offset] = 0x02;
			Offset += 0x04;
			if ((l->clients[ch]->guildcard == client->guildcard) && (l->lobbyCount > 1))
			{
				memset (&PacketData2[0], 0, 1316);
				PacketData2[0x00] = 0x34;
				PacketData2[0x01] = 0x05;
				PacketData2[0x02] = 0x65;
				PacketData2[0x04] = 0x01;
				PacketData2[0x08] = client->clientID;
				PacketData2[0x09] = l->leader;
				PacketData2[0x0A] = 0x01; // ??
				PacketData2[0x0B] = 0xFF; // ??
				PacketData2[0x0C] = 0x01; // ??
				PacketData2[0x0E] = 0x01; // ??
				PacketData2[0x16] = 0x01;
				*(unsigned *) &PacketData2[0x18] = client->guildcard;
				PacketData2[0x30] = client->clientID;
				memcpy (&PacketData2[0x34], &client->character.name[0], 24);
				PacketData2[0x54] = 0x02; // ??
				memcpy (&PacketData2[0x58], &client->character.inventoryUse, 0x4DC);
				// Prevent crashing with NPC skins...
				if (client->character.skinFlag)
					memset (&PacketData2[0x58+0x3A8], 0, 10);
				SendToLobby ( client->lobby, 4, &PacketData2[0x00], 1332, client->guildcard );
			}
		}
	}

	if (l->lobbyCount < 4)
		PacketData[0x194] = 0x02;
	if (l->lobbyCount < 3)
		PacketData[0x150] = 0x02;
	if (l->lobbyCount < 2)
		PacketData[0x10C] = 0x02;

	// Most of the 0x64 packet has been generated... now for the important stuff. =p
	// Leader ID @ 0x199
	// Difficulty @ 0x19B
	// Event @ 0x19D
	// Section ID of Leader @ 0x19E
	// Game Monster @ 0x1A0 (4 bytes)
	// Episode @ 0x1A4
	// 0x01 @ 0x1A5
	// One-person @ 0x1A6

	PacketData[0x198] = client->clientID;
	PacketData[0x199] = l->leader;
	PacketData[0x19B] = l->difficulty;
	PacketData[0x19C] = l->battle;
	if ((shipEvent < 7) && (shipEvent != 0x02))
		PacketData[0x19D] = shipEvent;
	else
		PacketData[0x19D] = 0;
	PacketData[0x19E] = l->sectionID;
	PacketData[0x19F] = l->challenge;
	*(unsigned *) &PacketData[0x1A0] = *(unsigned *) &l->gameMonster;
	PacketData[0x1A4] = l->episode;
	PacketData[0x1A5] = 0x01; // ??
	PacketData[0x1A6] = l->oneperson;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], 0x1A8);

	/* Let's send the team data... */

	SendToLobby ( client->lobby, 4, MakePacketEA15 ( client ), 2152, client->guildcard );

	client->bursting = 1;
}

void Send67 (PSO_CLIENT* client, unsigned char preferred)
{
	BLOCK* b;
	PSO_CLIENT* lClient;
	LOBBY* l;
	unsigned Offset = 0, Offset2 = 0;
	unsigned ch, ch2;

	if (!client->lobbyOK)
		return;

	client->lobbyOK = 0;

	ch = 0;
	
	b = blocks[client->block - 1];
	if ((preferred != 0xFF) && (preferred < 0x0F))
	{
		if (b->lobbies[preferred].lobbyCount >= 12)
			preferred = 0x00;
		ch = preferred;
	}

	for (ch=ch;ch<15;ch++)
	{
		l = &b->lobbies[ch];
		if (l->lobbyCount < 12)
		{
			for (ch2=0;ch2<12;ch2++)
				if (l->slot_use[ch2] == 0)
				{
					l->slot_use[ch2] = 1;
					l->clients[ch2] = client;
					l->arrow_color[ch2] = 0;
					client->lobbyNum = ch + 1;
					client->lobby = (void*) &b->lobbies[ch];
					client->clientID = ch2;
					break;
				}
				// Send68 here with joining client (use ch2 for clientid)
				l->lobbyCount = 0;
				for (ch2=0;ch2<12;ch2++)
				{
					if ((l->slot_use[ch2]) && (l->clients[ch2]))
						l->lobbyCount++;
				}

				memset (&PacketData[0x00], 0, 0x10);
				PacketData[0x04] = l->lobbyCount;
				PacketData[0x08] = client->clientID;
				PacketData[0x0B] = ch;
				PacketData[0x0C] = client->block;
				PacketData[0x0E] = shipEvent;
				Offset = 0x16;
				for (ch2=0;ch2<12;ch2++)
				{
					if ((l->slot_use[ch2]) && (l->clients[ch2]))
					{
						memset (&PacketData[Offset], 0, 1316);
						Offset2 = Offset;
						PacketData[Offset++] = 0x01;
						PacketData[Offset++] = 0x00;
						lClient = l->clients[ch2];
						*(unsigned *) &PacketData[Offset] = lClient->guildcard;
						Offset += 24;
						*(unsigned *) &PacketData[Offset] = ch2;
						Offset += 4;
						memcpy (&PacketData[Offset], &lClient->character.name[0], 24);
						Offset += 32;
						PacketData[Offset++] = 0x02;
						Offset += 3;
						memcpy (&PacketData[Offset], &lClient->character.inventoryUse, 1246);
						// Prevent crashing with NPCs
						if (lClient->character.skinFlag)
							memset (&PacketData[Offset+0x3A8], 0, 10);
						Offset += 1246;
						if (lClient->isgm == 1)
							*(unsigned *) &PacketData[Offset2 + 0x3CA] = globalName;
						else
							if (isLocalGM(lClient->guildcard))
								*(unsigned *) &PacketData[Offset2 + 0x3CA] = localName;
							else
								*(unsigned *) &PacketData[Offset2 + 0x3CA] = normalName;
						if ((lClient->guildcard == client->guildcard) && (l->lobbyCount > 1))
						{
							memcpy (&PacketData2[0x00], &PacketData[0], 0x16 );
							PacketData2[0x00] = 0x34;
							PacketData2[0x01] = 0x05;
							PacketData2[0x02] = 0x68;
							PacketData2[0x04] = 0x01;
							memcpy (&PacketData2[0x16], &PacketData[Offset2], 1316 );
							SendToLobby ( client->lobby, 12, &PacketData2[0x00], 1332, client->guildcard );
						}
					}
				}
				*(unsigned short*) &PacketData[0] = (unsigned short) Offset;
				PacketData[2] = 0x67;
				break;
		}
	}

	if (Offset > 0)
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketData[0], Offset);
	}

	// Quest data

	memset (&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0] = 0x10;
	client->encryptbuf[1] = 0x02;
	client->encryptbuf[2] = 0x60;
	client->encryptbuf[8] = 0x6F;
	client->encryptbuf[9] = 0x84;
	memcpy (&client->encryptbuf[0x0C], &client->character.quest_data1[0], 0x210 );
	memset (&client->encryptbuf[0x20C], 0, 4);
	encryptcopy (client, &client->encryptbuf[0x00], 0x210);

	/* Let's send the team data... */

	SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, client->guildcard );

	client->bursting = 1;
}

void Send95 (PSO_CLIENT* client)
{
	client->lobbyOK = 1;
	memset (&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0x00] = 0x08;
	client->encryptbuf[0x02] = 0x95;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &client->encryptbuf[0], 8);
	// Restore permanent character...
	if (client->character_backup)
	{
		if (client->mode)
		{
			memcpy (&client->character, client->character_backup, sizeof (client->character));
			client->mode = 0;
		}
		free (client->character_backup);
		client->character_backup = NULL;
	}
}

void SendA2 (unsigned char episode, unsigned char solo, unsigned char category, unsigned char gov, PSO_CLIENT* client)
{
	QUEST_MENU* qm = 0;
	QUEST* q;
	unsigned char qc = 0;
	unsigned short Offset;
	unsigned ch,ch2,ch3,show_quest,quest_flag;
	unsigned char quest_count;
	char quest_num[16];
	int qn, tier1, ep1solo;
	LOBBY* l;
	unsigned char diff;

	if (!client->lobby)
		return;

	l = (LOBBY *) client->lobby;

	memset (&PacketData[0], 0, 0x2510);

	diff = l->difficulty;

	if ( l->battle )
	{
		qm = &quest_menus[9];
		qc = 10;
	}
	else
		if ( l->challenge )
		{
		}
		else
		{
			switch ( episode )
			{
			case 0x01:
				if ( gov )
				{
					qm = &quest_menus[6];
					qc = 7;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[3];
						qc = 4;
					}
					else
					{
						qm = &quest_menus[0];
						qc = 1;
					}
				}
				break;
			case 0x02:
				if ( gov )
				{
					qm = &quest_menus[7];
					qc = 8;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[4];
						qc = 5;
					}
					else
					{
						qm = &quest_menus[1];
						qc = 2;
					}
				}
				break;
			case 0x03:
				if ( gov )
				{
					qm = &quest_menus[8];
					qc = 9;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[5];
						qc = 6;
					}
					else
					{
						qm = &quest_menus[2];
						qc = 3;
					}
				}
				break;
			}
		}

	if ((qm) && (category == 0))
	{
		PacketData[0x02] = 0xA2;
		PacketData[0x04] = qm->num_categories;
		Offset = 0x08;
		for (ch=0;ch<qm->num_categories;ch++)
		{
			PacketData[Offset+0x07]	= 0x0F;
			PacketData[Offset]		= qc;
			PacketData[Offset+2]	= gov;
			PacketData[Offset+4]	= ch + 1;
			memcpy (&PacketData[Offset+0x08], &qm->c_names[ch][0], 0x40);
			memcpy (&PacketData[Offset+0x48], &qm->c_desc[ch][0], 0xF0);
			Offset += 0x13C;
		}
	}
	else
	{
		ch2 = 0;
		PacketData[0x02] = 0xA2;
		category--;
		quest_count = 0;
		Offset = 0x08;
		ep1solo = qflag_ep1solo(&client->character.quest_data1[0], diff);
		for (ch=0;ch<qm->quest_counts[category];ch++)
		{
			q = &quests[qm->quest_indexes[category][ch]];
			show_quest = 0;
			if ((solo) && (episode == 0x01))
			{
				memcpy (&quest_num[0], &q->ql[0]->qdata[49], 3);
				quest_num[4] = 0;
				qn = atoi (&quest_num[0]);
				if ((ep1solo) || (qn > 26))
					show_quest = 1;
				if (!show_quest)
				{
					quest_flag = 0x63 + (qn << 1);
					if (qflag(&client->character.quest_data1[0], quest_flag, diff))
						show_quest = 2; // Sets a variance if they've cleared the quest...
					else
					{
						tier1 = 0;
						if ( (qflag(&client->character.quest_data1[0],0x65,diff)) && // Cleared first tier
							 (qflag(&client->character.quest_data1[0],0x67,diff)) &&
							 (qflag(&client->character.quest_data1[0],0x6B,diff)) )
							 tier1 = 1;
						if (qflag(&client->character.quest_data1[0], quest_flag, diff) == 0)
						{ // When the quest hasn't been completed...
							// Forest quests
							switch (qn)
							{
							case 4: // Battle Training
							case 2: // Claiming a Stake
							case 1: // Magnitude of Metal
								show_quest = 1;
								break;
							case 5: // Journalistic Pursuit
							case 6: // The Fake In Yellow
							case 7: // Native Research
							case 9: // Gran Squall
								if (tier1)
									show_quest = 1;
								break;
							case 8: // Forest of Sorrow
								if (qflag(&client->character.quest_data1[0],0x71,diff)) // Cleared Native Research
									show_quest = 1;
								break;
							case 26: // Central Dome Fire Swirl
								if (qflag(&client->character.quest_data1[0],0x73,diff)) // Cleared Forest of Sorrow
									show_quest = 1;
								break;
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x1F9,diff)))
							{
								// Cave quests (shown after Dragon is defeated)
								switch (qn)
								{
								case 03: // The Value of Money
								case 11: // The Lost Bride
								case 14: // Secret Delivery
								case 17: // Grave's Butler
								case 10: // Addicting Food
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 12: // Waterfall Tears
								case 15: // Soul of a Blacksmith
									if ( (qflag(&client->character.quest_data1[0],0x77,diff)) && // Cleared Addicting Food
										 (qflag(&client->character.quest_data1[0],0x79,diff)) && // Cleared The Lost Bride
										 (qflag(&client->character.quest_data1[0],0x7F,diff)) && // Cleared Secret Delivery
										 (qflag(&client->character.quest_data1[0],0x85,diff)) )  // Cleared Grave's Butler
										 show_quest = 1;
									break;
								case 13: // Black Paper
									if (qflag(&client->character.quest_data1[0],0x7B,diff)) // Cleared Waterfall Tears
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x1FF,diff)))
							{
								// Mine quests (shown after De Rol Le is defeated)
								switch (qn)
								{
								case 16: // Letter from Lionel
								case 18: // Knowing One's Heart
								case 20: // Dr. Osto's Research
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 21: // The Unsealed Door
									if ( (qflag(&client->character.quest_data1[0],0x8B,diff)) && // Cleared Dr. Osto's Research
										 (qflag(&client->character.quest_data1[0],0x7F,diff)) )  // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x207,diff)))
							{
								// Ruins quests (shown after Vol Opt is defeated)
								switch (qn)
								{
								case 19: // The Retired Hunter
								case 24: // Seek My Master
									show_quest = 1;  // Always shown if first tier was cleared
									break;
								case 25: // From the Depths
								case 22: // Soul of Steel
									if (qflag(&client->character.quest_data1[0],0x91,diff)) // Cleared Doc's Secret Plan
										show_quest = 1;
									break;
								case 23: // Doc's Secret Plan
									if (qflag(&client->character.quest_data1[0],0x7F,diff)) // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				show_quest = 1;
				if ((ch) && (gov))
				{
					// Check party's qualification for quests...
					switch (episode)
					{
					case 0x01:
						quest_flag = (0x1F3 + (ch << 1));
						for (ch2=0x1F5;ch2<=quest_flag;ch2+=2)
							for (ch3=0;ch3<4;ch3++)
								if ((l->clients[ch3]) && (!qflag(&l->clients[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					case 0x02:
						quest_flag = (0x211 + (ch << 1));
						for (ch2=0x213;ch2<=quest_flag;ch2+=2)
							for (ch3=0;ch3<4;ch3++)
								if ((l->clients[ch3]) && (!qflag(&l->clients[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					case 0x03:
						quest_flag = (0x2BC + ch);
						for (ch2=0x2BD;ch2<=quest_flag;ch2++)
							for (ch3=0;ch3<4;ch3++)
								if ((l->clients[ch3]) && (!qflag(&l->clients[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					}
				}
			}
			if (show_quest)
			{
				PacketData[Offset+0x07] = 0x0F;
				PacketData[Offset]      = qc;
				PacketData[Offset+1]	= 0x01;
				PacketData[Offset+2]    = gov;
				PacketData[Offset+3]    = ((unsigned char) qm->quest_indexes[category][ch]) + 1;
				PacketData[Offset+4]    = category;
				if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				  {
					memcpy (&PacketData[Offset+0x08], &q->ql[client->character.lang]->qname[0], 0x40);
					memcpy (&PacketData[Offset+0x48], &q->ql[client->character.lang]->qsummary[0], 0xF0);
				  }
				else
				  {
					memcpy (&PacketData[Offset+0x08], &q->ql[0]->qname[0], 0x40);
					memcpy (&PacketData[Offset+0x48], &q->ql[0]->qsummary[0], 0xF0);
				  }

				if ((solo) && (episode == 0x01))
				{
					if (qn <= 26)
					{
						*(unsigned short*) &PacketData[Offset+0x128] = ep1_unlocks[(qn-1)*2];
						*(unsigned short*) &PacketData[Offset+0x12C] = ep1_unlocks[((qn-1)*2)+1];
						*(int*) &PacketData[Offset+0x130] = qn;
						if ( show_quest == 2 )
							PacketData[Offset + 0x138] = 1;
					}
				}
				Offset += 0x13C;
				quest_count++;
			}
		}
		PacketData[0x04] = quest_count;
	}
	*(unsigned short*) &PacketData[0] = (unsigned short) Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], Offset);
}

void SendA0 (PSO_CLIENT* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketA0Data[0], *(unsigned short *) &PacketA0Data[0]);
}

void Send07 (PSO_CLIENT* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &Packet07Data[0], *(unsigned short *) &Packet07Data[0]);
}

void SendB0 (const char *mes, PSO_CLIENT* client)
{
	unsigned short xB0_Len;

	memcpy (&PacketData[0], &PacketB0[0], sizeof (PacketB0));
	xB0_Len = sizeof (PacketB0);

	while (*mes != 0x00)
	{
		PacketData[xB0_Len++] = *(mes++);
		PacketData[xB0_Len++] = 0x00;
	}

	PacketData[xB0_Len++] = 0x00;
	PacketData[xB0_Len++] = 0x00;

	while (xB0_Len % 8)
		PacketData[xB0_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xB0_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], xB0_Len);
}

void SendEE (const char *mes, PSO_CLIENT* client)
{
	unsigned short xEE_Len;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	while (*mes != 0x00)
	{
		PacketData[xEE_Len++] = *(mes++);
		PacketData[xEE_Len++] = 0x00;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], xEE_Len);
}

void Send19 (unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4, unsigned short ipp, PSO_CLIENT* client)
{
	memcpy ( &client->encryptbuf[0], &Packet19, sizeof (Packet19));
	client->encryptbuf[0x08] = ip1;
	client->encryptbuf[0x09] = ip2;
	client->encryptbuf[0x0A] = ip3;
	client->encryptbuf[0x0B] = ip4;
	*(unsigned short*) &client->encryptbuf[0x0C] = ipp;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &client->encryptbuf[0], sizeof (Packet19));
}

void SendEA (unsigned char command, PSO_CLIENT* client)
{
	switch (command)
	{
	case 0x02:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x04:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x04;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x0E:
		memset (&client->encryptbuf[0x00], 0, 0x38);
		client->encryptbuf[0x00] = 0x38;
		client->encryptbuf[0x01] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x0E;
		*(unsigned *) &client->encryptbuf[0x08] = client->guildcard;
		*(unsigned *) &client->encryptbuf[0x0C] = client->character.teamID;
		memcpy (&client->encryptbuf[0x18], &client->character.teamName[0], 28 );
		client->encryptbuf[0x34] = 0x84;
		client->encryptbuf[0x35] = 0x6C;
		memcpy (&client->encryptbuf[0x36], &client->character.teamFlag[0], 0x800);
		client->encryptbuf[0x836] = 0xFF;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x838);
		break;
	case 0x10:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x10;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x11:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x11;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x12:
		memset (&client->encryptbuf[0x00], 0, 0x40);
		client->encryptbuf[0x00] = 0x40;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x12;
		if ( client->character.teamID  )
		{
			*(unsigned *) &client->encryptbuf[0x0C] = client->guildcard;
			*(unsigned *) &client->encryptbuf[0x10] = client->character.teamID;
			client->encryptbuf[0x1C] = (unsigned char) client->character.privilegeLevel;
			memcpy (&client->encryptbuf[0x20], &client->character.teamName[0], 28);
			client->encryptbuf[0x3C] = 0x84;
			client->encryptbuf[0x3D] = 0x6C;
			client->encryptbuf[0x3E] = 0x98;
		}
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x40);
/*
		if ( client->lobbyNum < 0x10 )
			SendToLobby ( client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
		else
			SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
*/
		break;
	case 0x13:
		{
			LOBBY *l;
			PSO_CLIENT *lClient;
			unsigned ch, total_clients, EA15Offset, maxc;

			if (!client->lobby)
				break;

			l = (LOBBY*) client->lobby;

			if ( client->lobbyNum < 0x10 ) 
				maxc = 12;
			else
				maxc = 4;
			EA15Offset = 0x08;
			total_clients = 0;
			for (ch=0;ch<maxc;ch++)
			{
				if ((l->slot_use[ch]) && (l->clients[ch]))
				{
					lClient = l->clients[ch];
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.guildCard2;
					EA15Offset += 0x04;
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.teamID;
					EA15Offset += 0x04;
					memset(&client->encryptbuf[EA15Offset], 0, 8);
					EA15Offset += 0x08;
					client->encryptbuf[EA15Offset] = (unsigned char) lClient->character.privilegeLevel;
					EA15Offset += 4;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamName[0], 28);
					EA15Offset += 28;
					if ( lClient->character.teamID != 0 )
					{
						client->encryptbuf[EA15Offset++] = 0x84;
						client->encryptbuf[EA15Offset++] = 0x6C;
						client->encryptbuf[EA15Offset++] = 0x98;
						client->encryptbuf[EA15Offset++] = 0x00;
					}
					else
					{
						memset (&client->encryptbuf[EA15Offset], 0, 4);
						EA15Offset+= 4;
					}
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.guildCard;
					EA15Offset += 4;
					client->encryptbuf[EA15Offset++] = lClient->clientID;
					memset(&client->encryptbuf[EA15Offset], 0, 3);
					EA15Offset += 3;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.name[0], 24);
					EA15Offset += 24;
					memset(&client->encryptbuf[EA15Offset], 0, 8);
					EA15Offset += 8;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamFlag[0], 0x800);
					EA15Offset += 0x800;
					total_clients++;
				}
			}
			*(unsigned short*) &client->encryptbuf[0x00] = (unsigned short) EA15Offset;
			client->encryptbuf[0x02] = 0xEA;
			client->encryptbuf[0x03] = 0x13;
			*(unsigned*) &client->encryptbuf[0x04] = total_clients;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &client->encryptbuf[0], EA15Offset);
		}
		break;
	case 0x18:
		memset (&client->encryptbuf[0x00], 0, 0x4C);
		client->encryptbuf[0x00] = 0x4C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x18;
		client->encryptbuf[0x14] = 0x01;
		client->encryptbuf[0x18] = 0x01;
		client->encryptbuf[0x1C] = (unsigned char) client->character.privilegeLevel;
		*(unsigned *) &client->encryptbuf[0x20] = client->character.guildCard;
		memcpy (&client->encryptbuf[0x24], &client->character.name[0], 24);
		client->encryptbuf[0x48] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x4C);
		break;
	case 0x19:
		memset (&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x19;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1A:
		memset (&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1A;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1D:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1D;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	default:
		break;
	}
}

void Send60 (PSO_CLIENT* client)
{
	unsigned short size, size_check_index;
	unsigned short sizecheck = 0;
	int dont_send = 0;
	LOBBY* l;
	int boss_floor = 0;
	unsigned int itemid, magid, count, drop;
	unsigned short mid;
	short mHP;
	unsigned XP, ch, ch2, max_send, shop_price;
	int mid_mismatch;
	int ignored;
	int ws_ok;
	unsigned short ws_data, counter;
	PSO_CLIENT* lClient;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (!client->lobby)
		return;

#ifdef LOG_60
			packet_to_text (&client->decryptbuf[0], size);
			fprintf(debugfile, "%s\n", dp);
#endif

	if (size != sizecheck)
	{
		debug ("Client sent a 0x60 packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	l = (LOBBY*) client->lobby;

	if ( client->lobbyNum < 0x10 )
	{
		size_check_index  = client->decryptbuf[0x08];
		size_check_index *= 2;

		if (client->decryptbuf[0x08] == 0x06)
			sizecheck = 0x114; 
		else
			sizecheck = size_check_table[size_check_index+1] + 4;

		if ( ( size != sizecheck ) && ( sizecheck > 4 ) )
			dont_send = 1;

		if ( sizecheck == 4 ) // No size check packet encountered while in lobby mode...
		{
			debug ("No size check information for 0x60 lobby packet %02x", client->decryptbuf[0x08]);
			dont_send = 1;
		}
	}
	else
	{
		if (dont_send_60[(client->decryptbuf[0x08]*2) + 1] == 1)
		{
			dont_send = 1;
			write_log ("ship.log", "60 command \"%02x\" blocked in game. (Data below)", client->decryptbuf[0x08]);
			packet_to_text ( &client->decryptbuf[0x00], size );
			write_log ("ship.log", "%s", &dp[0]);
		}
	}

	if ( ( client->decryptbuf[0x0A] != client->clientID )				&&
		 ( size_check_table[(client->decryptbuf[0x08]*2)+1] != 0x00 )	&&
		 ( client->decryptbuf[0x08] != 0x07 )							&&
		 ( client->decryptbuf[0x08] != 0x79 ) )
		dont_send = 1;

	if ( ( client->decryptbuf[0x08] == 0x07 ) && 
		 ( client->decryptbuf[0x0C] != client->clientID ) )
		dont_send = 1;

	if ( client->decryptbuf[0x08] == 0x72 )
		dont_send = 1;

	if (!dont_send)
	{
		switch ( client->decryptbuf[0x08] )
		{
		case 0x07:
			// Symbol chat (throttle for spam)
			dont_send = 1;
			if ( ( ((unsigned) servertime - client->command_cooldown[0x07]) >= 1 ) && ( !stfu ( client->guildcard ) ) )
			{
				client->command_cooldown[0x07] = (unsigned) servertime;
				if ( client->lobbyNum < 0x10 )
					max_send = 12;
				else
					max_send = 4;
				for (ch=0;ch<max_send;ch++)
				{
					if ((l->slot_use[ch]) && (l->clients[ch]))
					{
						ignored = 0;
						lClient = l->clients[ch];
						for (ch2=0;ch2<lClient->ignore_count;ch2++)
						{
							if (lClient->ignore_list[ch2] == client->guildcard)
							{
								ignored = 1;
								break;
							}
						}
						if ( ( !ignored ) && ( lClient->guildcard != client->guildcard ) )
						{
							cipher_ptr = &lClient->server_cipher;
							encryptcopy ( lClient, &client->decryptbuf[0x00], size );
						}
					}
				}
			}
			break;
		case 0x0A:
			if ( client->lobbyNum > 0x0F )
			{
				// Player hit a monster
				mid = *(unsigned short*) &client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if ( ( mid < 0xB50 ) && ( l->floor[client->clientID] != 0 ) )
				{
					mHP = *(short*) &client->decryptbuf[0x0E];
					l->monsterData[mid].HP = mHP;
				}
			}
			else
				client->todc = 1;
			break;
		case 0x1F:
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			break;
		case 0x20:
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x18];
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x10];
			break;
		case 0x25:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				EquipItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x26:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				DeequipItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x27:
			// Use item
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				UseItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x28:
			// Mag feeding
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				magid = *(unsigned *) &client->decryptbuf[0x0C];
				itemid = *(unsigned *) &client->decryptbuf[0x10];
				feed_mag ( magid, itemid, client );
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x29:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				// Client dropping or destroying an item...
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				count = *(unsigned *) &client->decryptbuf[0x10];
				if (client->drop_item == itemid)
				{
					client->drop_item = 0;
					drop = 1;
				}
				else
					drop = 0;
				if (itemid != 0xFFFFFFFF)
					DeleteItemFromClient (itemid, count, drop, client); // Item
				else
					DeleteMesetaFromClient (count, drop, client); // Meseta
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x2A:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				// Client dropping complete item
				itemid = *(unsigned*) &client->decryptbuf[0x10];
				DeleteItemFromClient (itemid, 0, 1, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x3E:
		case 0x3F:
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x14];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x1C];
			break;
		case 0x40:
		case 0x42:
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x10];
			client->dead = 0;
			break;
		case 0x47:
		case 0x48:
			if (l->floor[client->clientID] == 0)
			{
				Send1A ("Using techniques on Pioneer 2 is disallowed.", client);
				dont_send = 1;
				client->todc = 1;
				break;
			}
			else
				if (client->clientID == client->decryptbuf[0x0A])
				{
					if (client->equip_flags & DROID_FLAG)
					{
						Send1A ("Androids cannot cast techniques.", client);
						dont_send = 1;
						client->todc = 1;
					}
					else
					{
						if (client->decryptbuf[0x0C] > 18)
						{
							Send1A ("Invalid technique cast.", client);
							dont_send = 1;
							client->todc = 1;
						}
						else
						{
							if (max_tech_level[client->decryptbuf[0x0C]][client->character._class] == -1)
							{
								Send1A ("You cannot cast that technique.", client);
								dont_send = 1;
								client->todc = 1;
							}
						}
					}
				}
			break;
		case 0x4D:
			// Decrease mag sync on death
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				client->dead = 1;
				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					if ((client->character.inventory[ch].item.data[0] == 0x02) &&
						(client->character.inventory[ch].flags & 0x08))
					{
						if (client->character.inventory[ch].item.data2[0] >= 5)
							client->character.inventory[ch].item.data2[0] -= 5;
						else
							client->character.inventory[ch].item.data2[0] = 0;
					}
				}
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}		
			break;
		case 0x68:
			// Telepipe check
			if ( ( client->lobbyNum < 0x10 ) || ( client->decryptbuf[0x0E] > 0x11 ) )
			{
				Send1A ("Incorrect telepipe.", client);
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0x74:
			// W/S (throttle for spam)
			dont_send = 1;
			if ( ( ((unsigned) servertime - client->command_cooldown[0x74]) >= 1 ) && ( !stfu ( client->guildcard ) ) )
			{
				client->command_cooldown[0x74] = (unsigned) servertime;
				ws_ok = 1;
				ws_data = *(unsigned short*) &client->decryptbuf[0x0C];
				if ( ( ws_data == 0 ) || ( ws_data > 3 ) )
					ws_ok = 0;
				ws_data = *(unsigned short*) &client->decryptbuf[0x0E];
				if ( ( ws_data == 0 ) || ( ws_data > 3 ) )
					ws_ok = 0;
				if ( ws_ok )
				{
					for (ch=0;ch<client->decryptbuf[0x0C];ch++)
					{
						ws_data = *(unsigned short*) &client->decryptbuf[0x10+(ch*2)];
						if ( ws_data > 0x685 )
						{
							if ( ws_data > 0x697 )
								ws_ok = 0;
							else
							{
								ws_data -= 0x68C;
								if ( ws_data >= l->lobbyCount )
									ws_ok = 0;
							}
						}
					}
					ws_data = 0xFFFF;
					for (ch=client->decryptbuf[0x0C];ch<8;ch++)
						*(unsigned short*) &client->decryptbuf[0x10+(ch*2)] = ws_data;

					if ( ws_ok ) 
					{
						if ( client->lobbyNum < 0x10 )
							max_send = 12;
						else
							max_send = 4;

						for (ch=0;ch<max_send;ch++)
						{
							if ((l->slot_use[ch]) && (l->clients[ch]))
							{
								ignored = 0;
								lClient = l->clients[ch];
								for (ch2=0;ch2<lClient->ignore_count;ch2++)
								{
									if (lClient->ignore_list[ch2] == client->guildcard)
									{
										ignored = 1;
										break;
									}
								}
								if ( ( !ignored ) && ( lClient->guildcard != client->guildcard ) )
								{
									cipher_ptr = &lClient->server_cipher;
									encryptcopy ( lClient, &client->decryptbuf[0x00], size );
								}
							}
						}
					}
				}
			}
			break;
		case 0x75:
			{
				// Set player flag

				unsigned short flag;

				if (!client->decryptbuf[0x0E])
				{
					flag = *(unsigned short*) &client->decryptbuf[0x0C];
					if ( flag < 1024 )
						client->character.quest_data1[((unsigned)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - ( flag & 0x07 ));
				}
			}
			break;
		case 0xC0:
			// Client selling item
			if ( ( client->lobbyNum > 0x0F ) && ( l->floor[client->clientID] == 0 ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					if (client->character.inventory[ch].item.itemid == itemid)
					{
						count = client->decryptbuf[0x10];
						if ((count > 1) && (client->character.inventory[ch].item.data[0] != 0x03))
							client->todc = 1;
						else
						{
							shop_price = GetShopPrice ( &client->character.inventory[ch], weapon_atpmax_table, armor_dfpvar_table, armor_evpvar_table, equip_prices, barrier_dfpvar_table, barrier_evpvar_table ) * count;
							DeleteItemFromClient ( itemid, count, 0, client );
							if (!client->todc)
							{
								client->character.meseta += shop_price;
								if ( client->character.meseta > 999999 )
									client->character.meseta = 999999;
							}
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xC3:
			// Client setting coordinates for stack drop
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				client->drop_area = *(unsigned *) &client->decryptbuf[0x0C];
				client->drop_coords = *(long long*) &client->decryptbuf[0x10];
				client->drop_item = *(unsigned *) &client->decryptbuf[0x18];
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xC4:
			// Inventory sort
			if ( client->lobbyNum > 0x0F )
			{
				dont_send = 1;
				SortClientItems ( client );
			}
			else
				client->todc = 1;
			break;
		case 0xC5:
			// Visiting hospital
			if ( client->lobbyNum > 0x0F )
				DeleteMesetaFromClient (10, 0, client);
			else
				client->todc = 1;
			break;
		case 0xC6:
			// Steal Exp
			if ( client->lobbyNum > 0x0F )
			{
				unsigned exp_percent = 0;
				unsigned exp_to_add;
				unsigned char special = 0;

				mid = *(unsigned short*) &client->decryptbuf[0x0C];
				mid &= 0xFFF;
				if (mid < 0xB50)
				{
					for (ch=0;ch<client->character.inventoryUse;ch++)
					{
						if ((client->character.inventory[ch].flags & 0x08)	&&
							(client->character.inventory[ch].item.data[0] == 0x00))
						{
							if ((client->character.inventory[ch].item.data[1] < 0x0A)	&&
								(client->character.inventory[ch].item.data[2] < 0x05))
								special = (client->character.inventory[ch].item.data[4] & 0x1F);
							else
								if ((client->character.inventory[ch].item.data[1] < 0x0D)	&&
									(client->character.inventory[ch].item.data[2] < 0x04))
									special = (client->character.inventory[ch].item.data[4] & 0x1F);
								else
									special = special_table[client->character.inventory[ch].item.data[1]]
								[client->character.inventory[ch].item.data[2]];
								switch (special)
								{
								case 0x09:
									// Master's
									exp_percent = 8;
									break;
								case 0x0A:
									// Lord's
									exp_percent = 10;
									break;
								case 0x0B:
									// King's
									exp_percent = 12;
									if (( l->difficulty	== 0x03 ) &&
										( client->equip_flags & DROID_FLAG ))
										exp_percent += 30;
									break;
								}
								break;
						}
					}

					if (exp_percent)
					{
						exp_to_add = ( l->mapData[mid].exp * exp_percent ) / 100L;
						if ( exp_to_add > 80 )  // Limit the amount of exp stolen to 80
							exp_to_add = 80;
						AddExp ( exp_to_add, client );
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC7:
			// Charge action
			if ( client->lobbyNum > 0x0F )
			{
				int meseta;

				meseta = *(int *) &client->decryptbuf[0x0C];
				if (meseta > 0)
				{
					if (client->character.meseta >= (unsigned) meseta)
						DeleteMesetaFromClient (meseta, 0, client);
					else
						DeleteMesetaFromClient (client->character.meseta, 0, client);
				}
				else
				{
					meseta = -meseta;
					client->character.meseta += (unsigned) meseta;
					if ( client->character.meseta > 999999 )
						client->character.meseta = 999999;
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC8:
			// Monster is dead
			if ( client->lobbyNum > 0x0F )
			{
				mid = *(unsigned short*) &client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if ( mid < 0xB50 )
				{
					if ( l->monsterData[mid].dead[client->clientID] == 0 )
					{
						l->monsterData[mid].dead[client->clientID] = 1;

						XP = l->mapData[mid].exp * experience_rate;

						if (!l->quest_loaded)
						{
							mid_mismatch = 0;

							switch ( l->episode )
							{
							case 0x01:
								if ( l->floor[client->clientID] > 10 )
								{
									switch ( l->floor[client->clientID] )
									{
									case 11:
										// Dragon
										if ( l->mapData[mid].base != 192 )
											mid_mismatch = 1;
										break;
									case 12:
										// De Rol Le
										if ( l->mapData[mid].base != 193 )
											mid_mismatch = 1;
										break;
									case 13:
										// Vol Opt
										if ( ( l->mapData[mid].base != 197 ) && ( l->mapData[mid].base != 194 ) )
											mid_mismatch = 1;
										break;
									case 14:
										// Dark Falz
										if ( l->mapData[mid].base != 200 )
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x02:
								if ( l->floor[client->clientID] > 10 )
								{
									switch ( l->floor[client->clientID] )
									{
									case 12:
										// Gal Gryphon
										if ( l->mapData[mid].base != 192 )
											mid_mismatch = 1;
										break;
									case 13:
										// Olga Flow
										if ( l->mapData[mid].base != 202 )
											mid_mismatch = 1;
										break;
									case 14:
										// Barba Ray
										if ( l->mapData[mid].base != 203 )
											mid_mismatch = 1;
										break;
									case 15:
										// Gol Dragon
										if ( l->mapData[mid].base != 204 )
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x03:
								if ( ( l->floor[client->clientID] == 9 ) &&
									( l->mapData[mid].base != 280 ) && 
									( l->mapData[mid].base != 281 ) && 
									( l->mapData[mid].base != 41 ) )
									mid_mismatch = 1;
								break;
							}

							if ( mid_mismatch )
							{
								SendEE ("Client/server data synchronization error.  Please reinstall your client and all patches.", client);
								client->todc = 1;
							}
						}

						//debug ("mid death: %u  base: %u, skin: %u, reserved11: %f, exp: %u", mid, l->mapData[mid].base, l->mapData[mid].skin, l->mapData[mid].reserved11, XP);

						if ( client->decryptbuf[0x10] != 1 ) // Not the last player who hit?
							XP = ( XP * 77 ) / 100L;

						if ( client->character.level < 199 )
							AddExp ( XP, client );

						// Increase kill counters for SJS, Lame d'Argent, Limiter and Swordsman Lore

						for (ch=0;ch<client->character.inventoryUse;ch++)
						{
							if (client->character.inventory[ch].flags & 0x08)
							{
								counter = 0;
								switch (client->character.inventory[ch].item.data[0])
								{
								case 0x00:
									if ((client->character.inventory[ch].item.data[1] == 0x33) ||
										(client->character.inventory[ch].item.data[1] == 0xAB))
										counter = 1;
									break;
								case 0x01:
									if ((client->character.inventory[ch].item.data[1] == 0x03) &&
										((client->character.inventory[ch].item.data[2] == 0x4D) ||
										(client->character.inventory[ch].item.data[2] == 0x4F)))
										counter = 1;
									break;
								default:
									break;
								}
								if (counter)
								{
									counter = *(unsigned short*) &client->character.inventory[ch].item.data[10];
									if (counter < 0x8000)
										counter = 0x8000;
									counter++;
									*(unsigned short*) &client->character.inventory[ch].item.data[10] = counter;
								}
							}
						}
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xCC:
			// Exchange item for team points
			{
				unsigned deleteid;

				deleteid = *(unsigned*) &client->decryptbuf[0x0C];
				DeleteItemFromClient (deleteid, 1, 0, client);
				if (!client->todc)
				{
					SendB0 ("Item donated to server.", client);
				}
			}
			break;
		case 0xCF:
			if ((l->battle) && (client->mode))
			{
				// Battle restarting...
				//
				// If rule #1 we'll copy the character backup to the character array, otherwise
				// we'll reset the character...
				//
				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->clients[ch]))
					{
						lClient = l->clients[ch];
						switch (lClient->mode)
						{
						case 0x01:
						case 0x02:
							// Copy character backup
							if (lClient->character_backup)
								memcpy (&lClient->character, lClient->character_backup, sizeof (lClient->character));
							if (lClient->mode == 0x02)
							{
								for (ch2=0;ch2<lClient->character.inventoryUse;ch2++)
								{
									if (lClient->character.inventory[ch2].item.data[0] == 0x02)
										lClient->character.inventory[ch2].in_use = 0;
								}
								CleanUpInventory (lClient);
								lClient->character.meseta = 0;
							}
							break;
						case 0x03:
							// Wipe items and reset level.
							for (ch2=0;ch2<30;ch2++)
								lClient->character.inventory[ch2].in_use = 0;
							CleanUpInventory (lClient);
							lClient->character.level = 0;
							lClient->character.XP = 0;
							lClient->character.ATP = *(unsigned short*) &startingData[(lClient->character._class*14)];
							lClient->character.MST = *(unsigned short*) &startingData[(lClient->character._class*14)+2];
							lClient->character.EVP = *(unsigned short*) &startingData[(lClient->character._class*14)+4];
							lClient->character.HP  = *(unsigned short*) &startingData[(lClient->character._class*14)+6];
							lClient->character.DFP = *(unsigned short*) &startingData[(lClient->character._class*14)+8];
							lClient->character.ATA = *(unsigned short*) &startingData[(lClient->character._class*14)+10];
							if (l->battle_level > 1)
								SkipToLevel (l->battle_level - 1, lClient, 1);
							lClient->character.meseta = 0;
							break;
						default:
							// Unknown mode?
							break;
						}
					}
				}
				// Reset boxes and monsters...
				memset (&l->boxHit, 0, 0xB50); // Reset box and monster data
				memset (&l->monsterData, 0, sizeof (l->monsterData));
			}
			break;
		case 0xD2:
			// Gallon seems to write to this area...
			dont_send = 1;
			if ( client->lobbyNum > 0x0F )
			{
				unsigned qofs;

				qofs = *(unsigned *) &client->decryptbuf[0x0C];
				if (qofs < 23)
				{
					qofs *= 4;
					*(unsigned *) &client->character.quest_data2[qofs] = *(unsigned*) &client->decryptbuf[0x10];
					memcpy ( &client->encryptbuf[0x00], &client->decryptbuf[0x00], 0x14 );
					cipher_ptr = &client->server_cipher;
					encryptcopy ( client, &client->decryptbuf[0x00], 0x14 );
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD5:
			// Exchange an item
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item;
				unsigned compare_item = 0, ci;

				memset (&work_item, 0, sizeof (INVENTORY_ITEM));
				memcpy (&work_item.item.data[0], &client->decryptbuf[0x0C], 3 );
				DeleteFromInventory (&work_item, 1, client);

				if (!client->todc)
				{
					memset (&work_item, 0, sizeof (INVENTORY_ITEM));
					memcpy (&compare_item, &client->decryptbuf[0x20], 3 );
					for ( ci = 0; ci < quest_numallows; ci ++)
					{
						if (compare_item == quest_allow[ci])
						{
							memcpy (&work_item.item.data[0], &client->decryptbuf[0x20], 3 );
							work_item.item.itemid = l->playerItemID[client->clientID];
							l->playerItemID[client->clientID]++;
							AddToInventory (&work_item, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 0x0C);
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							// BLAH :)
							*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x34];
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
							break;
						}
					}
					if ( !work_item.item.itemid )
					{
						Send1A ("Attempting to exchange for disallowed item.", client);
						client->todc = 1;
					}
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD7:
			// Trade PDs for an item from Hopkins' dad
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item;
				unsigned ci, compare_item = 0;

				memset ( &work_item, 0, sizeof (INVENTORY_ITEM) );
				memcpy ( &compare_item, &client->decryptbuf[0x0C], 3 );
				for ( ci = 0; ci < (sizeof (gallons_shop_hopkins) / 4); ci += 2)
				{
					if (compare_item == gallons_shop_hopkins[ci])
					{
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						work_item.item.data[2] = 0x00;
						break;
					}
				}
				if ( work_item.item.data[0] == 0x03 )
				{
					DeleteFromInventory (&work_item, 0xFF, client); // Delete all Photon Drops
					if (!client->todc)
					{
						memcpy (&work_item.item.data[0], &client->decryptbuf[0x0C], 12);
						*(unsigned *) &work_item.item.data2[0] = *(unsigned *) &client->decryptbuf[0x18];
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&work_item, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x0C);
						// I guess this is a sort of action confirmed by server thing...
						// Also starts an animation and sound... with the wrong values, the camera pans weirdly...
						client->encryptbuf[0x00] = 0x0C;
						client->encryptbuf[0x02] = 0xAB;
						client->encryptbuf[0x03] = 0x01;
						*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x20];
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
					}
				}
				else
				{
					Send1A ("No photon drops in user's inventory\nwhen encountering exchange command.", client);
					dont_send = 1;
					client->todc = 1;
				}
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xD8:
			// Add attribute to S-rank weapon (not implemented yet)
			break;
		case 0xD9:
			// Momoka Item Exchange
			{
				unsigned compare_item, ci;
				unsigned itemid = 0;
				INVENTORY_ITEM add_item;

				dont_send = 1;

				if ( client->lobbyNum > 0x0F )
				{
					compare_item = 0x00091203;
					for ( ci=0; ci < client->character.inventoryUse; ci++)
					{
						if ( *(unsigned *) &client->character.inventory[ci].item.data[0] == compare_item )
						{
							itemid = client->character.inventory[ci].item.itemid;
							break;
						}
					}
					if (!itemid)
					{
						memset (&client->encryptbuf[0x00], 0, 8);
						client->encryptbuf[0x00] = 0x08;
						client->encryptbuf[0x02] = 0x23;
						client->encryptbuf[0x04] = 0x01;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 8);
					}
					else
					{
						memset (&add_item, 0, sizeof (INVENTORY_ITEM));
						compare_item = *(unsigned *) &client->decryptbuf[0x20];
						for ( ci=0; ci < quest_numallows; ci++)
						{
							if ( compare_item == quest_allow[ci] )
							{
								*(unsigned *) &add_item.item.data[0] = *(unsigned *) &client->decryptbuf[0x20];
								break;
							}
						}
						if (*(unsigned *) &add_item.item.data[0] == 0)
						{
							client->todc = 1;
							Send1A ("Requested item not allowed.", client);
						}
						else
						{
							DeleteItemFromClient (itemid, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 0x18);
							client->encryptbuf[0x00] = 0x18;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0xDB;
							client->encryptbuf[0x09] = 0x06;
							client->encryptbuf[0x0C] = 0x01;
							*(unsigned *) &client->encryptbuf[0x10] = itemid;
							client->encryptbuf[0x14] = 0x01;
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x18);

							// Let everybody else know that item no longer exists...

							memset (&client->encryptbuf[0x00], 0, 0x14);
							client->encryptbuf[0x00] = 0x14;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0x29;
							client->encryptbuf[0x09] = 0x05;
							client->encryptbuf[0x0A] = client->clientID;
							*(unsigned *) &client->encryptbuf[0x0C] = itemid;
							client->encryptbuf[0x10] = 0x01;
							SendToLobby ( l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard );
							add_item.item.itemid = l->playerItemID[client->clientID];
							l->playerItemID[client->clientID]++;
							AddToInventory (&add_item, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 8);
							client->encryptbuf[0x00] = 0x08;
							client->encryptbuf[0x02] = 0x23;
							client->encryptbuf[0x04] = 0x00;
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 8);
						}
					}
				}
			}
			break;
		case 0xDA:
			// Upgrade Photon of weapon
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item, work_item2;
				unsigned ci, ai,
					compare_itemid = 0, compare_item1 = 0, compare_item2 = 0, num_attribs = 0;
				char attrib_add;

				memcpy ( &compare_item1,  &client->decryptbuf[0x0C], 3);
				compare_itemid = *(unsigned *) &client->decryptbuf[0x20];
				for ( ci=0; ci < client->character.inventoryUse; ci++)
				{
					memcpy (&compare_item2, &client->character.inventory[ci].item.data[0], 3);
					if ( ( client->character.inventory[ci].item.itemid == compare_itemid ) && 
						( compare_item1 == compare_item2 ) && ( client->character.inventory[ci].item.data[0] == 0x00 ) )
					{
						memset (&work_item, 0, sizeof (INVENTORY_ITEM));
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						if ( client->decryptbuf[0x2C] )
							work_item.item.data[2] = 0x01;
						else
							work_item.item.data[2] = 0x00;
						// Copy before shift
						memcpy ( &work_item2, &client->character.inventory[ci], sizeof (INVENTORY_ITEM) );
						DeleteFromInventory ( &work_item, client->decryptbuf[0x28], client );
						if (!client->todc)
						{
							switch (client->decryptbuf[0x28])
							{
							case 0x01:
								// 1 PS = 30%
								if ( client->decryptbuf[0x2C] )
									attrib_add = 30;
								break;
							case 0x04:
								// 4 PDs = 1%
								attrib_add = 1;
								break;
							case 0x14:
								// 20 PDs = 5%
								attrib_add = 5;
								break;
							default:
								attrib_add = 0;
								break;
							}
							ai = 0;
							if ((work_item2.item.data[6] > 0x00) && 
								(!(work_item2.item.data[6] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[6] == client->decryptbuf[0x24])
									ai = 7;
							}
							if ((work_item2.item.data[8] > 0x00) && 
								(!(work_item2.item.data[8] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[8] == client->decryptbuf[0x24])
									ai = 9;
							}
							if ((work_item2.item.data[10] > 0x00) && 
								(!(work_item2.item.data[10] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[10] == client->decryptbuf[0x24])
									ai = 11;
							}
							if (ai)
							{
								// Attribute already on weapon, increase it
								(char) work_item2.item.data[ai] += attrib_add;
								if (work_item2.item.data[ai] > 100)
									work_item2.item.data[ai] = 100;
							}
							else
							{
								// Attribute not on weapon, add it if there isn't already 3 attributes
								if (num_attribs < 3)
								{
									work_item2.item.data[6 + (num_attribs * 2)] = client->decryptbuf[0x24];
									(char) work_item2.item.data[7 + (num_attribs * 2)] = attrib_add;
								}
							}
							DeleteItemFromClient ( work_item2.item.itemid, 1, 0, client );
							memset (&client->encryptbuf[0x00], 0, 0x14);
							client->encryptbuf[0x00] = 0x14;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0x29;
							client->encryptbuf[0x09] = 0x05;
							client->encryptbuf[0x0A] = client->clientID;
							*(unsigned *) &client->encryptbuf[0x0C] = work_item2.item.itemid;
							client->encryptbuf[0x10] = 0x01;
							SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x14, 0 );
							AddToInventory ( &work_item2, 1, 0, client );
							memset (&client->encryptbuf[0x00], 0, 0x0C);
							// Don't know...
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x30];
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xDE:
			// Good Luck
			{
				unsigned compare_item, ci;
				unsigned itemid = 0;
				INVENTORY_ITEM add_item;

				dont_send = 1;

				if ( client->lobbyNum > 0x0F )
				{
					compare_item = 0x00031003;
					for ( ci=0; ci < client->character.inventoryUse; ci++)
					{
						if ( *(unsigned *) &client->character.inventory[ci].item.data[0] == compare_item )
						{
							itemid = client->character.inventory[ci].item.itemid;
							break;
						}
					}
					if (!itemid)
					{
						memset (&client->encryptbuf[0x00], 0, 0x2C);
						client->encryptbuf[0x00] = 0x2C;
						client->encryptbuf[0x02] = 0x24;
						client->encryptbuf[0x04] = 0x01;
						client->encryptbuf[0x08] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
						for (ci=0;ci<8;ci++)
							client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
					}
					else
					{
						memset (&add_item, 0, sizeof (INVENTORY_ITEM));
						*(unsigned *) &add_item.item.data[0] = good_luck[mt_lrand() % (sizeof(good_luck) >> 2)];
						DeleteItemFromClient (itemid, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x18);
						client->encryptbuf[0x00] = 0x18;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDB;
						client->encryptbuf[0x09] = 0x06;
						client->encryptbuf[0x0C] = 0x01;
						*(unsigned *) &client->encryptbuf[0x10] = itemid;
						client->encryptbuf[0x14] = 0x01;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x18);

						// Let everybody else know that item no longer exists...

						memset (&client->encryptbuf[0x00], 0, 0x14);
						client->encryptbuf[0x00] = 0x14;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0x29;
						client->encryptbuf[0x09] = 0x05;
						client->encryptbuf[0x0A] = client->clientID;
						*(unsigned *) &client->encryptbuf[0x0C] = itemid;
						client->encryptbuf[0x10] = 0x01;
						SendToLobby ( l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard );
						add_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&add_item, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x2C);
						client->encryptbuf[0x00] = 0x2C;
						client->encryptbuf[0x02] = 0x24;
						client->encryptbuf[0x04] = 0x00;
						client->encryptbuf[0x08] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
						for (ci=0;ci<8;ci++)
							client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
					}
				}
			}
			break;
		case 0xE1:
			{
				// Gallon's Plan opcode

				INVENTORY_ITEM work_item;
				unsigned ch, compare_item1, compare_item2, pt_itemid;

				compare_item2 = 0;
				compare_item1 = 0x041003;
				pt_itemid = 0;

				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
					if (compare_item1 == compare_item2)
					{
						pt_itemid = client->character.inventory[ch].item.itemid;
						break;
					}
				}

				if (!pt_itemid)
					client->todc = 1;

				if (!client->todc)
				{
					memset ( &work_item, 0, sizeof (INVENTORY_ITEM) );
					switch (client->decryptbuf[0x0E])
					{
					case 0x01:
						// Kan'ei Tsuho
						DeleteItemFromClient (pt_itemid, 10, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x00;
							work_item.item.data[1] = 0xD5;
							work_item.item.data[2] = 0x00;
						}
						break;
					case 0x02:
						// Lollipop
						DeleteItemFromClient (pt_itemid, 15, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x00;
							work_item.item.data[1] = 0x0A;
							work_item.item.data[2] = 0x07;
						}
						break;
					case 0x03:
						// Stealth Suit
						DeleteItemFromClient (pt_itemid, 20, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x01;
							work_item.item.data[1] = 0x01;
							work_item.item.data[2] = 0x57;
						}
						break;
					default:
						client->todc = 1;
						break;
					}

					if (!client->todc)
					{
						memset (&client->encryptbuf[0x00], 0, 0x18);
						client->encryptbuf[0x00] = 0x18;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDB;
						client->encryptbuf[0x09] = 0x06;
						client->encryptbuf[0x0C] = 0x01;
						*(unsigned *) &client->encryptbuf[0x10] = pt_itemid;
						client->encryptbuf[0x14] = 0x05 + ( client->decryptbuf[0x0E] * 5 );
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &client->encryptbuf[0x00], 0x18 );
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&work_item, 1, 0, client);
						// Gallon's Plan result
						memset (&client->encryptbuf[0x00], 0, 0x10);
						client->encryptbuf[0x00] = 0x10;
						client->encryptbuf[0x02] = 0x25;
						client->encryptbuf[0x08] = client->decryptbuf[0x10];
						client->encryptbuf[0x0A] = 0x3C;
						client->encryptbuf[0x0B] = 0x08;
						client->encryptbuf[0x0D] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0E] = 0x9A;
						client->encryptbuf[0x0F] = 0x08;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x10);
					}
				}
			}
			break;
		case 0x17:
		case 0x18:
			boss_floor = 0;
			switch (l->episode)
			{
			case 0x01:
				if ((l->floor[client->clientID] > 10) && (l->floor[client->clientID] < 15))
					boss_floor = 1;
				break;
			case 0x02:
				if ((l->floor[client->clientID] > 11) && (l->floor[client->clientID] < 16))
					boss_floor = 1;
				break;
			case 0x03:
				if (l->floor[client->clientID] == 9)
					boss_floor = 1;
				break;
			}
			if (!boss_floor)
				dont_send = 1;
			break;
		default:
			/* Temporary
			debug ("0x60 from %u:", client->guildcard);
			display_packet (&client->decryptbuf[0x00], size);
			write_log ("0x60 from %u\n%s\n\n:", client->guildcard, (char*) &dp[0]);
			*/
			break;
		}
		if ((!dont_send) && (!client->todc))
		{
			if ( client->lobbyNum < 0x10 )				
				SendToLobby ( client->lobby, 12, &client->decryptbuf[0], size, client->guildcard);
			else
				SendToLobby ( client->lobby, 4, &client->decryptbuf[0], size, client->guildcard);
		}
	}
}

void Send6D (PSO_CLIENT* client)
{
	PSO_CLIENT* lClient;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t;
	int dont_send = 0;
	LOBBY* l;

	if (!client->lobby)
		return;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = *(unsigned short*) &client->decryptbuf[0x0C];
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug ("Client sent a 0x6D packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck: %04x\n", client->decryptbuf[0x08],
			size, sizecheck);
		dont_send = 1;
	}

	l = (LOBBY*) client->lobby;
	t = client->decryptbuf[0x04];
	if (t >= 0x04)
		dont_send = 1;

	if (!dont_send)
	{
		switch (client->decryptbuf[0x08])
		{
		case 0x70:
			if (client->lobbyNum > 0x0F)
			{
				if ((l->slot_use[t]) && (l->clients[t]))
				{
					if (l->clients[t]->bursting == 1)
					{
						unsigned ch;

						dont_send = 0; // It's cool to send them as long as this user is bursting.

						// Let's reconstruct the 0x70 as much as possible.
						//
						// Could check guild card # here
						*(unsigned *) &client->decryptbuf[0x7C] = client->guildcard;
						// Check techniques...
						if (!(client->equip_flags & DROID_FLAG))
						{
							for (ch=0;ch<19;ch++)
							{
								if ((char) client->decryptbuf[0xC4+ch] > max_tech_level[ch][client->character._class])
								{
									(char) client->character.techniques[ch] = -1; // Unlearn broken technique.
									client->todc = 1;
								}
							}
							if (client->todc)
								Send1A ("Technique data check failed.\n\nSome techniques have been unlearned.", client);
						}
						memcpy (&client->decryptbuf[0xC4], &client->character.techniques, 20);
						// Could check character structure here
						memcpy (&client->decryptbuf[0xD8], &client->character.gcString, 104);
						// Prevent crashing with NPC skins...
						if (client->character.skinFlag)
							memset (&client->decryptbuf[0x110], 0, 10);
						// Could check stats here
						memcpy (&client->decryptbuf[0x148], &client->character.ATP, 36);
						// Could check inventory here
						client->decryptbuf[0x16C] = client->character.inventoryUse;
						memcpy (&client->decryptbuf[0x170], &client->character.inventory[0], 30 * sizeof (INVENTORY_ITEM) );
					}
				}
			}
			break;
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x72:
			if (client->lobbyNum > 0x0F)
			{
				if (l->leader == client->clientID)
				{
					if ((l->slot_use[t]) && (l->clients[t]))
					{
						if (l->clients[t]->bursting == 1)
							dont_send = 0; // It's cool to send them as long as this user is bursting and we're the leader.
					}
				}
			}
			break;
		default:
			dont_send = 1;
			break;
		}
	}

	if ( dont_send == 0 )
	{
		lClient = l->clients[t];
		cipher_ptr = &lClient->server_cipher;
		encryptcopy (lClient, &client->decryptbuf[0], size);
	}
}

void Send01 (const char *text, PSO_CLIENT* client)
{
	unsigned short mesgOfs = 0x10;
	unsigned ch;

	memset(&PacketData[0], 0, 16);
	PacketData[mesgOfs++] = 0x09;
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x45;
	PacketData[mesgOfs++] = 0x00;
	for (ch=0;ch<strlen(text);ch++)
	{
		PacketData[mesgOfs++] = text[ch];
		PacketData[mesgOfs++] = 0x00;
	}
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x00;
	while (mesgOfs % 8)
		PacketData[mesgOfs++] = 0x00;
	*(unsigned short*) &PacketData[0] = mesgOfs;
	PacketData[0x02] = 0x01;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], mesgOfs);
}

void Send06 (PSO_CLIENT* client)
{
	FILE* fp;
	unsigned short chatsize;
	unsigned pktsize;
	unsigned ch, ch2;
	unsigned char stackable, count;
	unsigned short *n;
	unsigned short target;
	unsigned myCmdArgs, itemNum, connectNum, gc_num;
	unsigned short npcID;
	unsigned max_send;
	PSO_CLIENT* lClient;
	int i, z, commandLen, ignored, found_ban, writeData;
	LOBBY* l;
	INVENTORY_ITEM ii;

	writeData = 0;
	fp = NULL;

	if (!client->lobby)
		return;

	l = client->lobby;

	pktsize = *(unsigned short*) &client->decryptbuf[0x00];

	if (pktsize > 0x100)
		return;

	memset (&chatBuf[0x00], 0, 0x0A);

	chatBuf[0x02] = 0x06;
	chatBuf[0x0A] = client->clientID;
	*(unsigned *) &chatBuf[0x0C] = client->guildcard;
	chatsize = 0x10;
	n = (unsigned short*) &client->character.name[4];
	for (ch=0;ch<10;ch++)
	{
		if (*n == 0x0000)
			break;
		*(unsigned short*) &chatBuf[chatsize] = *n;
		chatsize += 2;
		n++;
	}
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	n = (unsigned short*) &client->decryptbuf[0x12];
	if ((*(n+1) == 0x002F) && (*(n+2) != 0x002F))
	{
		commandLen = 0;

		for (ch=0;ch<(pktsize - 0x14);ch+= 2)
		{
			if (client->decryptbuf[(0x14+ch)] != 0x00)
				cmdBuf[commandLen++] = client->decryptbuf[(0x14+ch)];
			else
				break;
		}

		cmdBuf[commandLen] = 0;

		myCmdArgs = 0;
		myCommand = &cmdBuf[1];

		if ( ( i = strcspn ( &cmdBuf[1], " ," ) ) != ( strlen ( &cmdBuf[1] ) ) )
		{
			i++;
			cmdBuf[i++] = 0;
			while ( ( i < commandLen ) && ( myCmdArgs < 64 ) )
			{
				z = strcspn ( &cmdBuf[i], "," );
				myArgs[myCmdArgs++] = &cmdBuf[i];
				i += z;
				cmdBuf[i++] = 0;
			}
		}

		if ( commandLen )
		{

			if ( !strcmp ( myCommand, "debug" ) )
				writeData = 1;

			if ( !strcmp ( myCommand, "setpass" ) )
			{
				if (!client->lobbyNum < 0x10)
				{
					if ( myCmdArgs == 0 )
						SendB0 ("Need new password.", client);
					else
					{
						ch = 0;
						while (myArgs[0][ch] != 0)
						{
							l->gamePassword [ch*2] = myArgs[0][ch];
							l->gamePassword [(ch*2)+1] = 0;
							ch++;
							if (ch==31) break; // Limit amount of characters...
						}
						l->gamePassword[ch*2] = 0;
						l->gamePassword[(ch*2)+1] = 0;
						for (ch=0;ch<4;ch++)
						{
							if ((l->slot_use[ch]) && (l->clients[ch]))
								SendB0 ("Room password changed.", l->clients[ch]);
						}
					}
				}
			}

			if ( !strcmp ( myCommand, "arrow" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need arrow digit.", client);
				else
				{
					l->arrow_color[client->clientID] = atoi ( myArgs[0] );
					ShowArrows (client, 1);
				}
			}

			if ( !strcmp ( myCommand, "lang" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need language digit.", client);
				else
				{
					npcID = atoi ( myArgs[0] );

					if ( npcID > numLanguages ) npcID = 1;

					if ( npcID == 0 )
						npcID = 1;

					npcID--;

					client->character.lang = (unsigned char) npcID;

					SendB0 ("Current language:\n", client);
					SendB0 (languageNames[npcID], client);

				}
			}


			if ( !strcmp ( myCommand, "npc" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need NPC digit. (max = 11, 0 to unskin)", client);
				else
				{
					npcID = atoi ( myArgs[0] );

					if ( npcID > 7 ) 
					{
						if ( ( npcID > 11 ) || ( !ship_support_extnpc ) )
							npcID = 7;
					}

					if ( npcID == 0 )
					{
						client->character.skinFlag = 0x00;
						client->character.skinID = 0x00;
					}
					else
					{
						client->character.skinFlag = 0x02;
						client->character.skinID = npcID - 1;
					}
					SendB0 ("Skin updated, change blocks for it to take effect.", client );
				}
			}

			// Process GM commands

			if ( ( !strcmp ( myCommand, "event" ) ) && ( (client->isgm) || (playerHasRights(client->guildcard, 0))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need event digit.", client);
				else
				{

					shipEvent = atoi ( myArgs[0] );

					write_gm ("GM %u has changed ship event to %u", client->guildcard, shipEvent );

					PacketDA[0x04] = shipEvent;

					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard)
						{
							cipher_ptr = &connections[connectNum]->server_cipher;
							encryptcopy (connections[connectNum], &PacketDA[0], 8);
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "redbox") ) && ( client->isgm ) )
			{
				if (l->redbox)
				{
					l->redbox = 0;
					SendB0 ("Red box mode turned off.", client);
					write_gm ("GM %u has deactivated redbox mode", client->guildcard ); 
				}
				else
				{
					l->redbox = 1;
					SendB0 ("Red box mode turned on!", client);
					write_gm ("GM %u has activated redbox mode", client->guildcard ); 
				}
			}

			if (( !strcmp ( myCommand, "item" ) )  && (client->isgm))
			{
				// Item creation...
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot make items in the lobby!!!", client);
				else
					if ( myCmdArgs < 4 )
						SendB0 ("You must specify at least four arguments for the desired item.", client);
					else
						if ( strlen ( myArgs[0] ) < 8 ) 
							SendB0 ("Main arguments is an incorrect length.", client);
						else
						{
							if ( ( strlen ( myArgs[1] ) < 8 ) ||
								( strlen ( myArgs[2] ) < 8 ) || 
								( strlen ( myArgs[3] ) < 8 ) )
								SendB0 ("Some arguments were incorrect and replaced.", client);

							write_gm ("GM %u created an item", client->guildcard);

							itemNum = free_game_item (l);

							_strupr ( myArgs[0] );
							l->gameItem[itemNum].item.data[0]  = hex2byte (&myArgs[0][0]);
							l->gameItem[itemNum].item.data[1]  = hex2byte (&myArgs[0][2]);
							l->gameItem[itemNum].item.data[2]  = hex2byte (&myArgs[0][4]);
							l->gameItem[itemNum].item.data[3]  = hex2byte (&myArgs[0][6]);


							if ( strlen ( myArgs[1] ) >= 8 ) 
							{
								_strupr ( myArgs[1] );
								l->gameItem[itemNum].item.data[4]  = hex2byte (&myArgs[1][0]);
								l->gameItem[itemNum].item.data[5]  = hex2byte (&myArgs[1][2]);
								l->gameItem[itemNum].item.data[6]  = hex2byte (&myArgs[1][4]);
								l->gameItem[itemNum].item.data[7]  = hex2byte (&myArgs[1][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[4]  = 0;
								l->gameItem[itemNum].item.data[5]  = 0;
								l->gameItem[itemNum].item.data[6]  = 0;
								l->gameItem[itemNum].item.data[7]  = 0;
							}

							if ( strlen ( myArgs[2] ) >= 8 ) 
							{
								_strupr ( myArgs[2] );
								l->gameItem[itemNum].item.data[8]  = hex2byte (&myArgs[2][0]);
								l->gameItem[itemNum].item.data[9]  = hex2byte (&myArgs[2][2]);
								l->gameItem[itemNum].item.data[10] = hex2byte (&myArgs[2][4]);
								l->gameItem[itemNum].item.data[11] = hex2byte (&myArgs[2][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[8]  = 0;
								l->gameItem[itemNum].item.data[9]  = 0;
								l->gameItem[itemNum].item.data[10] = 0;
								l->gameItem[itemNum].item.data[11] = 0;
							}

							if ( strlen ( myArgs[3] ) >= 8 ) 
							{
								_strupr ( myArgs[3] );
								l->gameItem[itemNum].item.data2[0]  = hex2byte (&myArgs[3][0]);
								l->gameItem[itemNum].item.data2[1]  = hex2byte (&myArgs[3][2]);
								l->gameItem[itemNum].item.data2[2]  = hex2byte (&myArgs[3][4]);
								l->gameItem[itemNum].item.data2[3]  = hex2byte (&myArgs[3][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data2[0]  = 0;
								l->gameItem[itemNum].item.data2[1]  = 0;
								l->gameItem[itemNum].item.data2[2]  = 0;
								l->gameItem[itemNum].item.data2[3]  = 0;
							}

							// check stackable shit

							stackable = 0;

							if (l->gameItem[itemNum].item.data[0] == 0x03)
								stackable = stackable_table[l->gameItem[itemNum].item.data[1]];

							if ( ( stackable ) && ( l->gameItem[itemNum].item.data[5] == 0x00 ) )
								l->gameItem[itemNum].item.data[5] = 0x01; // force at least 1 of a stack to drop

							write_gm ("Item data: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								l->gameItem[itemNum].item.data[0], l->gameItem[itemNum].item.data[1], l->gameItem[itemNum].item.data[2], l->gameItem[itemNum].item.data[3],
								l->gameItem[itemNum].item.data[4], l->gameItem[itemNum].item.data[5], l->gameItem[itemNum].item.data[6], l->gameItem[itemNum].item.data[7],
								l->gameItem[itemNum].item.data[8], l->gameItem[itemNum].item.data[9], l->gameItem[itemNum].item.data[10], l->gameItem[itemNum].item.data[11],
								l->gameItem[itemNum].item.data2[0], l->gameItem[itemNum].item.data2[1], l->gameItem[itemNum].item.data2[2], l->gameItem[itemNum].item.data2[3] );

							l->gameItem[itemNum].item.itemid = l->itemID++;
							if (l->gameItemCount < MAX_SAVED_ITEMS)
								l->gameItemList[l->gameItemCount++] = itemNum;
							memset (&PacketData[0], 0, 0x2C);
							PacketData[0x00] = 0x2C;
							PacketData[0x02] = 0x60;
							PacketData[0x08] = 0x5D;
							PacketData[0x09] = 0x09;
							PacketData[0x0A] = 0xFF;
							PacketData[0x0B] = 0xFB;
							*(unsigned *) &PacketData[0x0C] = l->floor[client->clientID];
							*(unsigned *) &PacketData[0x10] = l->clientx[client->clientID];
							*(unsigned *) &PacketData[0x14] = l->clienty[client->clientID];
							memcpy (&PacketData[0x18], &l->gameItem[itemNum].item.data[0], 12 );
							*(unsigned *) &PacketData[0x24] = l->gameItem[itemNum].item.itemid;
							*(unsigned *) &PacketData[0x28] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
							SendToLobby ( client->lobby, 4, &PacketData[0], 0x2C, 0);
							SendB0 ("Item created.", client);
						}								
			}

			if (( !strcmp ( myCommand, "give" ) )  && (client->isgm))
			{
				// Insert item into inventory
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot give items in the lobby!!!", client);
				else
					if ( myCmdArgs < 4 )
						SendB0 ("You must specify at least four arguments for the desired item.", client);
					else
						if ( strlen ( myArgs[0] ) < 8 )
							SendB0 ("Main arguments is an incorrect length.", client);
						else
						{
							if ( ( strlen ( myArgs[1] ) < 8 ) ||
								 ( strlen ( myArgs[2] ) < 8 ) || 
								 ( strlen ( myArgs[3] ) < 8 ) )
								SendB0 ("Some arguments were incorrect and replaced.", client);

							write_gm ("GM %u obtained an item", client->guildcard);

							_strupr ( myArgs[0] );
							ii.item.data[0]  = hex2byte (&myArgs[0][0]);
							ii.item.data[1]  = hex2byte (&myArgs[0][2]);
							ii.item.data[2]  = hex2byte (&myArgs[0][4]);
							ii.item.data[3]  = hex2byte (&myArgs[0][6]);


							if ( strlen ( myArgs[1] ) >= 8 ) 
							{
								_strupr ( myArgs[1] );
								ii.item.data[4]  = hex2byte (&myArgs[1][0]);
								ii.item.data[5]  = hex2byte (&myArgs[1][2]);
								ii.item.data[6]  = hex2byte (&myArgs[1][4]);
								ii.item.data[7]  = hex2byte (&myArgs[1][6]);
							}
							else
							{
								ii.item.data[4]  = 0;
								ii.item.data[5]  = 0;
								ii.item.data[6]  = 0;
								ii.item.data[7]  = 0;
							}

							if ( strlen ( myArgs[2] ) >= 8 ) 
							{
								_strupr ( myArgs[2] );
								ii.item.data[8]  = hex2byte (&myArgs[2][0]);
								ii.item.data[9]  = hex2byte (&myArgs[2][2]);
								ii.item.data[10] = hex2byte (&myArgs[2][4]);
								ii.item.data[11] = hex2byte (&myArgs[2][6]);
							}
							else
							{
								ii.item.data[8]  = 0;
								ii.item.data[9]  = 0;
								ii.item.data[10] = 0;
								ii.item.data[11] = 0;
							}

							if ( strlen ( myArgs[3] ) >= 8 ) 
							{
								_strupr ( myArgs[3] );
								ii.item.data2[0]  = hex2byte (&myArgs[3][0]);
								ii.item.data2[1]  = hex2byte (&myArgs[3][2]);
								ii.item.data2[2]  = hex2byte (&myArgs[3][4]);
								ii.item.data2[3]  = hex2byte (&myArgs[3][6]);
							}
							else
							{
								ii.item.data2[0]  = 0;
								ii.item.data2[1]  = 0;
								ii.item.data2[2]  = 0;
								ii.item.data2[3]  = 0;
							}

							// check stackable shit

							stackable = 0;

							if (ii.item.data[0] == 0x03)
								stackable = stackable_table[ii.item.data[1]];

							if ( stackable )
							{
								if ( ii.item.data[5] == 0x00 )
									ii.item.data[5] = 0x01; // force at least 1 of a stack to drop
								count = ii.item.data[5];
							}
							else
								count = 1;

							write_gm ("Item data: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								ii.item.data[0], ii.item.data[1], ii.item.data[2], ii.item.data[3],
								ii.item.data[4], ii.item.data[5], ii.item.data[6], ii.item.data[7],
								ii.item.data[8], ii.item.data[9], ii.item.data[10], ii.item.data[11],
								ii.item.data2[0], ii.item.data2[1], ii.item.data2[2], ii.item.data2[3] );

							ii.item.itemid = l->itemID++;
							AddToInventory ( &ii, count, 0, client );
							SendB0 ("Item obtained.", client);
						}
			}

			if ( ( !strcmp ( myCommand, "warpme" ) )  && ((client->isgm) || (playerHasRights(client->guildcard, 3))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Can't warp in the lobby!!!", client);
				else
					if ( myCmdArgs == 0 )
						SendB0 ("Need area to warp to...", client);
					else
					{
						target = atoi ( myArgs[0] );
						if ( target > 17 )
							SendB0 ("Warping past area 17 would probably crash your client...", client);
						else
						{
							warp_packet[0x0C] = (unsigned char) atoi ( myArgs[0] );
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &warp_packet[0], sizeof (warp_packet));
						}
					}
			}

			if ( ( !strcmp ( myCommand, "dc" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 4))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to disconnect.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0 ("You may not disconnect this user.", client);
							else
							{
								write_gm ("GM %u has disconnected user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
								Send1A ("You've been disconnected by a GM.", connections[connectNum]);
								connections[connectNum]->todc = 1;
								break;
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "ban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 11))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 1))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 1, client ); // Should unban...
							write_gm ("GM %u has removed ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("Ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 1, client ) )
									{
										write_gm ("GM %u has banned user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A ("You've been banned by a GM.", connections[connectNum]);
										SendB0 ("User has been banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "ipban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to IP ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 2))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 2, client ); // Should unban...
							write_gm ("GM %u has removed IP ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("IP ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 2, client ) )
									{
										write_gm ("GM %u has IP banned user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A ("You've been banned by a GM.", connections[connectNum]);
										SendB0 ("User has been IP banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "hwban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to HW ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 3))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 3, client ); // Should unban...
							write_gm ("GM %u has removed HW ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("HW ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 3, client ) )
									{
										write_gm ("GM %u has HW banned user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A  ("You've been banned by a GM.", connections[connectNum]);
										SendB0  ("User has been HW banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}


			if ( ( !strcmp ( myCommand, "dcall" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 5))) )
			{
				printf ("Blocking connections until all users are disconnected...\n");
				write_gm ("GM %u has disconnected all users", client->guildcard);
				blockConnections = 1;
			}

			if ( ( !strcmp ( myCommand, "announce" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 6))) )
			{
				if (client->announce != 0)
				{
					SendB0 ("Announce\ncancelled.", client);
					client->announce = 0;
				}
				else
				{
					SendB0 ("Announce by\nsending a\nmail.", client);
					client->announce = 1;
				}
			}

			if ( ( !strcmp ( myCommand, "global" ) ) && (client->isgm) )
			{
				if (client->announce != 0)
				{
					SendB0 ("Announce\ncancelled.", client);
					client->announce = 0;
				}
				else
				{
					SendB0 ("Global announce\nby sending\na mail.", client);
					client->announce = 2;
				}
			}

			if ( ( !strcmp ( myCommand, "levelup" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 7))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot level up in the lobby!!!", client);
				else
					if ( l->floor[client->clientID] == 0 )
						SendB0 ("Please leave Pioneer 2 before using this command...", client);
					else
						if ( myCmdArgs == 0 )
							SendB0 ("Must specify a target level to level up to...", client);
						else
						{
							target = atoi ( myArgs[0] );
							if ( ( client->character.level + 1 ) >= target )
								SendB0 ("Target level must be higher than your current level...", client);
							else
							{
								// Do the level up!!!

								if (target > 200)
									target = 200;

								target -= 2;

								AddExp (tnlxp[target] - client->character.XP, client);
							}
						}
			}

			if ( (!strcmp ( myCommand, "updatelocalgms" )) && ((client->isgm) || (playerHasRights(client->guildcard, 8))) )
			{
				SendB0 ("Local GM file reloaded.", client);
				readLocalGMFile();
			}

			if ( (!strcmp ( myCommand, "updatemasks" )) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				SendB0 ("IP ban masks file reloaded.", client);
				load_mask_file(num_masks, ship_banmasks);
			}

			if ( !strcmp ( myCommand, "bank" ) )
			{
				if (client->bankType)
				{
					client->bankType = 0;
					SendB0 ("Bank: Character", client);
				}
				else
				{
					client->bankType = 1;
					SendB0 ("Bank: Common", client);
				}
			}

			if ( !strcmp ( myCommand, "ignore" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to ignore.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					ignored = 0;

					for (ch=0;ch<client->ignore_count;ch++)
					{
						if (client->ignore_list[ch] == gc_num)
						{
							ignored = 1;
							client->ignore_list[ch] = 0;
							SendB0 ("User no longer being ignored.", client);
							break;
						}
					}

					if (!ignored)
					{
						if (client->ignore_count < 100)
						{
							client->ignore_list[client->ignore_count++] = gc_num;
							SendB0 ("User is now ignored.", client);
						}
						else
							SendB0 ("Ignore list is full.", client);
					}
					else
					{
						ch2 = 0;
						for (ch=0;ch<client->ignore_count;ch++)
						{
							if ((client->ignore_list[ch] != 0) && (ch != ch2))
								client->ignore_list[ch2++] = client->ignore_list[ch];
						}
						client->ignore_count = ch2;
					}
				}
			}

			if ( ( !strcmp ( myCommand, "stfu" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 9))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to silence.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0 ("You may not silence this user.", client);
							else
							{							
								if (toggle_stfu(connections[connectNum]->guildcard, client))
								{
									write_gm ("GM %u has silenced user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
									SendB0  ("User has been silenced.", client);
									SendB0  ("You've been silenced.", connections[connectNum]);
								}
								else
								{
									write_gm ("GM %u has removed silence from user %u (%s)", client->guildcard, gc_num, unicode_to_ascii ((unsigned short*) &connections[connectNum]->character.name[4]));
									SendB0  ("User is now allowed to speak.", client);
									SendB0  ("You may now speak freely.", connections[connectNum]);
								}
								break;
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "warpall" ) )  && ((client->isgm) || (playerHasRights(client->guildcard, 10))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Can't warp in the lobby!!!", client);
				else
					if ( myCmdArgs == 0 )
						SendB0 ("Need area to warp to...", client);
					else
					{
						target = atoi ( myArgs[0] );
						if ( target > 17 )
							SendB0 ("Warping past area 17 would probably crash your client...", client);
						else
						{
							warp_packet[0x0C] = (unsigned char) atoi ( myArgs[0] );
							SendToLobby ( client->lobby, 4, &warp_packet[0], sizeof (warp_packet), 0 );
						}
					}
			}
		}
	}
	else
	{
		for (ch=0;ch<(pktsize - 0x12);ch+=2)
		{
			if ((*n == 0x0000) || (chatsize == 0xC0))
				break;
			if ((*n == 0x0009) || (*n == 0x000A))
				*n = 0x0020;
			*(unsigned short*) &chatBuf[chatsize] = *n;
			chatsize += 2;
			n++;
		}
		chatBuf[chatsize++] = 0x00;
		chatBuf[chatsize++] = 0x00;
		while (chatsize % 8)
			chatBuf[chatsize++] = 0x00;
		*(unsigned short*) &chatBuf[0x00] = chatsize;
		if ( !stfu ( client->guildcard ) )
		{
			if ( client->lobbyNum < 0x10 )
				max_send = 12;
			else
				max_send = 4;
			for (ch=0;ch<max_send;ch++)
			{
				if ((l->slot_use[ch]) && (l->clients[ch]))
				{
					ignored = 0;
					lClient = l->clients[ch];
					for (ch2=0;ch2<lClient->ignore_count;ch2++)
					{
						if (lClient->ignore_list[ch2] == client->guildcard)
						{
							ignored = 1;
							break;
						}
					}
					if (!ignored)
					{
						cipher_ptr = &lClient->server_cipher;
						encryptcopy ( lClient, &chatBuf[0x00], chatsize );
					}
				}
			}
		}
	}

	if ( writeData )
	{
		if (!client->debugged)
		{
			client->debugged = 1;
			_itoa (client->character.guildCard, &character_file[0], 10);
			strcat (&character_file[0], unicode_to_ascii ((unsigned short*) &client->character.name[4]));
			strcat (&character_file[0], ".bbc");
			fp = fopen (&character_file[0], "wb");
			if (fp)
			{
				fwrite (&client->character.packetSize, 1, sizeof (CHARDATA), fp);
				fclose (fp);
			}
			write_log ("ship.log", "User %u (%s) has wrote character debug data.", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4]) );
			SendB0 ("Your debug data has been saved.", client);
		}
		else
			SendB0 ("Your debug data has already been saved.", client);
	}
}
