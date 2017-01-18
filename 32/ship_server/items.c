void SortClientItems (PSO_CLIENT* client)
{
	unsigned ch, ch2, ch3, ch4, itemid;

	ch2 = 0x0C;

	memset (&sort_data[0], 0, sizeof (INVENTORY_ITEM) * 30);

	for (ch4=0;ch4<30;ch4++)
	{
		sort_data[ch4].item.data[1] = 0xFF;
		sort_data[ch4].item.itemid = 0xFFFFFFFF;
	}

	ch4 = 0;

	for (ch=0;ch<30;ch++)
	{
		itemid = *(unsigned *) &client->decryptbuf[ch2];
		ch2 += 4;
		if (itemid != 0xFFFFFFFF)
		{
			for (ch3=0;ch3<client->character.inventoryUse;ch3++)
			{
				if ((client->character.inventory[ch3].in_use) && (client->character.inventory[ch3].item.itemid == itemid))
				{
					sort_data[ch4++] = client->character.inventory[ch3];
					break;
				}
			}
		}
	}

	for (ch=0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];

}

void CleanUpBank (PSO_CLIENT* client)
{
	unsigned ch, ch2 = 0;

	memset (&bank_data[0], 0, sizeof (BANK_ITEM) * 200);

	for (ch2=0;ch2<200;ch2++)
		bank_data[ch2].itemid = 0xFFFFFFFF;

	ch2 = 0;

	for (ch=0;ch<200;ch++)
	{
		if (client->character.bankInventory[ch].itemid != 0xFFFFFFFF)
			bank_data[ch2++] = client->character.bankInventory[ch];
	}

	client->character.bankUse = ch2;

	for (ch=0;ch<200;ch++)
		client->character.bankInventory[ch] = bank_data[ch];

}

void CleanUpInventory (PSO_CLIENT* client)
{
	unsigned ch, ch2 = 0;

	memset (&sort_data[0], 0, sizeof (INVENTORY_ITEM) * 30);

	ch2 = 0;

	for (ch=0;ch<30;ch++)
	{
		if (client->character.inventory[ch].in_use)
			sort_data[ch2++] = client->character.inventory[ch];
	}

	client->character.inventoryUse = ch2;

	for (ch=0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];
}

void CleanUpGameInventory (LOBBY* l)
{
	unsigned ch, item_count;

	ch = item_count = 0;

	while (ch < l->gameItemCount)
	{
		// Combs the entire game inventory for items in use
		if (l->gameItemList[ch] != 0xFFFFFFFF)
		{
			if (ch > item_count)
				l->gameItemList[item_count] = l->gameItemList[ch];
			item_count++;
		}
		ch++;
	}

	if (item_count < MAX_SAVED_ITEMS)
		memset (&l->gameItemList[item_count], 0xFF, ( ( MAX_SAVED_ITEMS - item_count ) * 4 ) );

	l->gameItemCount = item_count;
}

unsigned int AddItemToClient (unsigned itemid, PSO_CLIENT* client)
{
	unsigned ch, itemNum = 0;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned count, stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	LOBBY* l;

	// Adds an item to the client's character data, but only if the item exists in the game item data
	// to begin with.

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<l->gameItemCount;ch++)
	{
		itemNum = l->gameItemList[ch]; // Lookup table for faster searching...
		if ( l->gameItem[itemNum].item.itemid == itemid )
		{
			if (l->gameItem[itemNum].item.data[0] == 0x04)
			{
				// Meseta
				count = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
				client->character.meseta += count;
				if (client->character.meseta > 999999)
					client->character.meseta = 999999;
				item_added = 1;
			}
			else
				if (l->gameItem[itemNum].item.data[0] == 0x03)
					stackable = stackable_table[l->gameItem[itemNum].item.data[1]];
			if ( ( stackable ) && ( !l->gameItem[itemNum].item.data[5] ) )
				l->gameItem[itemNum].item.data[5] = 1;
			found_item = ch;
			break;
		}
	}

	if ( found_item != -1 ) // We won't disconnect if the item isn't found because there's a possibility another
	{						// person may have nabbed it before our client due to lag...
		if ( ( item_added == 0 ) && ( stackable ) )
		{
			memcpy (&compare_item1, &l->gameItem[itemNum].item.data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					count = l->gameItem[itemNum].item.data[5];

					stack_count = client->character.inventory[ch].item.data[5];
					if ( !stack_count )
						stack_count = 1;

					if ( ( stack_count + count ) > stackable )
					{
						Send1A ("Trying to stack over the limit...", client);
						client->todc = 1;
					}
					else
					{
						// Add item to the client's current count...
						client->character.inventory[ch].item.data[5] = (unsigned char) ( stack_count + count );
						item_added = 1;
					}
					break;
				}
			}
		}

		if ( ( !client->todc ) && ( item_added == 0 ) ) // Make sure the client isn't trying to pick up more than 30 items...
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &l->gameItem[itemNum].item, sizeof (ITEM));
				client->character.inventoryUse++;
				item_added = 1;
			}
		}

		if ( item_added )
		{
			// Delete item from game's inventory
			memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
			l->gameItemList[found_item] = 0xFFFFFFFF;
			CleanUpGameInventory(l);
		}
	}
	return item_added;
}

void DeleteMesetaFromClient (unsigned count, unsigned drop, PSO_CLIENT* client)
{
	unsigned stack_count, newItemNum;
	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	stack_count = client->character.meseta;
	if (stack_count < count)
	{
		client->character.meseta = 0;
		count = stack_count;
	}
	else
		client->character.meseta -= count;
	if ( drop )
	{
		memset (&PacketData[0x00], 0, 16);
		PacketData[0x00] = 0x2C;
		PacketData[0x01] = 0x00;
		PacketData[0x02] = 0x60;
		PacketData[0x03] = 0x00;
		PacketData[0x08] = 0x5D;
		PacketData[0x09] = 0x09;
		PacketData[0x0A] = client->clientID;
		*(unsigned *) &PacketData[0x0C] = client->drop_area;
		*(long long *) &PacketData[0x10] = client->drop_coords;
		PacketData[0x18] = 0x04;
		PacketData[0x19] = 0x00;
		PacketData[0x1A] = 0x00;
		PacketData[0x1B] = 0x00;
		memset (&PacketData[0x1C], 0, 0x08);
		*(unsigned *) &PacketData[0x24] = l->playerItemID[client->clientID];
		*(unsigned *) &PacketData[0x28] = count;
		SendToLobby (client->lobby, 4, &PacketData[0], 0x2C, 0);

		// Generate new game item...

		newItemNum = free_game_item (l);
		if (l->gameItemCount < MAX_SAVED_ITEMS)
			l->gameItemList[l->gameItemCount++] = newItemNum;
		memcpy (&l->gameItem[newItemNum].item.data[0], &PacketData[0x18], 12);
		*(unsigned *) &l->gameItem[newItemNum].item.data2[0] = count;
		l->gameItem[newItemNum].item.itemid = l->playerItemID[client->clientID];
		l->playerItemID[client->clientID]++;
	}
}

void SendItemToEnd (unsigned itemid, PSO_CLIENT* client)
{
	unsigned ch;
	INVENTORY_ITEM i;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			i = client->character.inventory[ch];
			i.flags = 0x00;
			client->character.inventory[ch].in_use = 0;
			break;
		}
	}

	CleanUpInventory (client);
	
	// Add item to client.

	client->character.inventory[client->character.inventoryUse] = i;
	client->character.inventoryUse++;
}

void DeleteItemFromClient (unsigned itemid, unsigned count, unsigned drop, PSO_CLIENT* client)
{
	unsigned ch, ch2, itemNum;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// Deletes an item from the client's character data.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
			{
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];
				if ( ( stackable ) && ( !count ) && ( !drop ) )
					count = 1;
			}

			if ( ( stackable ) && ( count ) )
			{
				stack_count = client->character.inventory[ch].item.data[5];
				if (!stack_count)
					stack_count = 1;

				if ( stack_count < count )
				{
					Send1A ("Trying to delete more items than posssessed!", client);
					client->todc = 1;
				}
				else
				{
					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

					if ( !stack_count )
						delete_item = 1;

					if ( drop )
					{
						memset (&PacketData[0x00], 0, 16);
						PacketData[0x00] = 0x28;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5D;
						PacketData[0x09] = 0x08;
						PacketData[0x0A] = client->clientID;
						*(unsigned *) &PacketData[0x0C] = client->drop_area;
						*(long long *) &PacketData[0x10] = client->drop_coords;
						memcpy (&PacketData[0x18], &client->character.inventory[ch].item.data[0], 12);
						PacketData[0x1D] = (unsigned char) count;
						*(unsigned *) &PacketData[0x24] = l->playerItemID[client->clientID];

						SendToLobby (client->lobby, 4, &PacketData[0], 0x28, 0);

						// Generate new game item...

						itemNum = free_game_item (l);
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memcpy (&l->gameItem[itemNum].item.data[0], &PacketData[0x18], 12);
						l->gameItem[itemNum].item.itemid =  l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
					}
				}
			}
			else
			{
				delete_item = 1; // Not splitting a stack, item goes byebye from character's inventory.
				if ( drop ) // Client dropped the item on the floor?
				{
					// Copy to game's inventory
					itemNum = free_game_item (l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memcpy (&l->gameItem[itemNum].item, &client->character.inventory[ch].item, sizeof (ITEM));
				}
			}

			if ( delete_item )
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}

	if ( found_item == -1 )
	{
		Send1A ("Could not find item to delete.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);

}

unsigned int WithdrawFromBank (unsigned itemid, unsigned count, PSO_CLIENT* client)
{
	unsigned ch;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned delete_item = 0;
	LOBBY* l;

	// Adds an item to the client's character from it's bank, if the item is really there...

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.bankUse;ch++)
	{
		if ( client->character.bankInventory[ch].itemid == itemid )
		{
			found_item = ch;
			if ( client->character.bankInventory[ch].data[0] == 0x03 )
			{
				stackable = stackable_table[client->character.bankInventory[ch].data[1]];

				if ( stackable )
				{
					if ( !count )
						count = 1;

					stack_count = ( client->character.bankInventory[ch].bank_count & 0xFF );
					if ( !stack_count )
						stack_count = 1;				

					if ( stack_count < count ) // Naughty!
					{
						Send1A ("Trying to pull a fast one on the bank teller.", client);
						client->todc = 1;
						found_item = -1;
					}
					else
					{
						stack_count -= count;
						client->character.bankInventory[ch].bank_count = 0x10000 + stack_count;
						if ( !stack_count )
							delete_item = 1;
					}
				}
			}
			break;
		}
	}

	if ( found_item != -1 )
	{
		if ( stackable )
		{
			memcpy (&compare_item1, &client->character.bankInventory[found_item].data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ( ( stack_count + count ) > stackable )
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char) (stack_count + count);
					item_added = 1;
					break;
				}
			}
		}

		if ( (!client->todc ) && ( item_added == 0 ) ) // Make sure the client isn't trying to withdraw more than 30 items...
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &client->character.bankInventory[found_item].data[0], sizeof (ITEM));
				if ( stackable )
				{
					memset (&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
					client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char) count;
				}
				client->character.inventory[client->character.inventoryUse].item.itemid = l->itemID;
				client->character.inventoryUse++;
				item_added = 1;
				//debug ("Item added to client...");
			}
		}

		if ( item_added )
		{
			// Let people know the client has a new toy...
			memset (&client->encryptbuf[0x00], 0, 0x24);
			client->encryptbuf[0x00] = 0x24;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0xBE;
			client->encryptbuf[0x09] = 0x09;
			client->encryptbuf[0x0A] = client->clientID;
			memcpy (&client->encryptbuf[0x0C], &client->character.bankInventory[found_item].data[0], 12);
			*(unsigned *) &client->encryptbuf[0x18] = l->itemID;
			l->itemID++;
			if (!stackable)
				*(unsigned *) &client->encryptbuf[0x1C] = *(unsigned *) &client->character.bankInventory[found_item].data2[0];
			else
				client->encryptbuf[0x11] = count;
			memset (&client->encryptbuf[0x20], 0, 4);
			SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x24, 0 );
			if ( ( delete_item ) || ( !stackable ) )
				// Delete item from bank inventory
				client->character.bankInventory[found_item].itemid = 0xFFFFFFFF;
		}
		CleanUpBank (client);
	}
	else
	{
		Send1A ("Could not find bank item to withdraw.", client);
		client->todc = 1;
	}

	return item_added;
}

void FixItem (ITEM* i )
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

unsigned int free_game_item (LOBBY* l)
{
	unsigned ch, ch2, oldest_item;

	ch2 = oldest_item = 0xFFFFFFFF;

	// If the itemid at the current index is 0, just return that...

	if ((l->gameItemCount < MAX_SAVED_ITEMS) && (l->gameItem[l->gameItemCount].item.itemid == 0))
		return l->gameItemCount;

	// Scan the gameItem array for any free item slots...

	for (ch=0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if (l->gameItem[ch].item.itemid == 0)
		{
			ch2 = ch;
			break;
		}
	}

	if (ch2 != 0xFFFFFFFF)
		return ch2;

	// Out of inventory memory!  Time to delete the oldest dropped item in the game...

	for (ch=0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if ((l->gameItem[ch].item.itemid < oldest_item) && (l->gameItem[ch].item.itemid >= 0x810000))
		{
			ch2 = ch;
			oldest_item = l->gameItem[ch].item.itemid;
		}
	}

	if (ch2 != 0xFFFFFFFF)
	{
		l->gameItem[ch2].item.itemid = 0; // Item deleted.
		return ch2;
	}

	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			SendEE ("Lobby inventory problem!  It's advised you quit this game and recreate it.", l->client[ch]);
	}

	return 0;
}

void UpdateGameItem (PSO_CLIENT* client)
{
	// Updates the game item list for all of the client's items...  (Used strictly when a client joins a game...)

	LOBBY* l;
	unsigned ch;

	memset (&client->tekked, 0, sizeof (INVENTORY_ITEM)); // Reset tekking data

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++) // By default this should already be sorted at the top, so no need for an in_use check...	
		client->character.inventory[ch].item.itemid = l->playerItemID[client->clientID]++; // Keep synchronized
}

unsigned long ExpandDropRate(unsigned char pc)
{
    long shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    return ((2 << (unsigned long) shift) * ((pc & 7) + 7));
}

void GenerateRandomAttributes (unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client)
{
	unsigned ch, num_percents, max_percent, meseta, do_area, r;
	PTDATA* ptd;
	int rare;
	unsigned area;
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

	if ( area > 10 )
		area = 10;

	area--; // Our tables are zero based.

	switch (i->item.data[0])
	{
	case 0x00:
		rare = 0;
		if ( i->item.data[1] > 0x0C )
			rare = 1;
		else
			if ( ( i->item.data[1] > 0x09 ) && ( i->item.data[2] > 0x03 ) )
				rare = 1;
			else
				if ( ( i->item.data[1] < 0x0A ) && ( i->item.data[2] > 0x04 ) )
					rare = 1;
		if ( !rare )
		{

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

			// Add a grind

			if ( l->episode == 0x02 )
				ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
			else
				ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			i->item.data[3] = (unsigned char) ch;
		}
		else
			i->item.data[4] |= 0x80; // rare

		num_percents = 0;

		if ( ( i->item.data[1] == 0x33 ) || 
			 ( i->item.data[1] == 0xAB ) ) // SJS and Lame max 2 percents
			max_percent = 2;
		else
			max_percent = 3;

		for (ch=0;ch<max_percent;ch++)
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
		break;
	case 0x01:
		switch ( i->item.data[1] )
		{
		case 0x01:
			// Armor variance
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
			// Shield variance
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data [2]  = 0x05;
		i->item.data [4]  = 0xF4;
		i->item.data [5]  = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
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
		// meseta
		meseta = ptd->box_meseta[area][0];
		if ( ptd->box_meseta[area][1] > ptd->box_meseta[area][0] )
			meseta += mt_lrand() % ( ( ptd->box_meseta[area][1] - ptd->box_meseta[area][0] ) + 1 );
		*(unsigned *) &i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
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

void LoadShopData2()
{
	FILE *fp;
	fp = fopen ("shop\\shop2.dat", "rb");
	if (!fp)
	{
		printf ("shop\\shop2.dat is missing.");
		printf ("Press [ENTER] to quit...");
		gets (&dp[0]);
		exit (1);
	}
	fread (&equip_prices[0], 1, sizeof (equip_prices), fp);
	fclose (fp);
}

void UseItem (unsigned int itemid, PSO_CLIENT* client)
{
	unsigned found_item = 0, ch, ch2;
	INVENTORY_ITEM i;
	int eq_wep, eq_armor, eq_shield, eq_mag = -1;
	LOBBY* l;
	unsigned new_item, TotalMatUse, HPMatUse, max_mat;
	int mat_exceed;

	// Check item stuff here...  Like converting certain things to certain things...
	//

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;

			// Copy item before deletion (needed for consumables)
			memcpy (&i, &client->character.inventory[ch], sizeof (INVENTORY_ITEM));

			// Unwrap mag
			if ((i.item.data[0] == 0x02) && (i.item.data2[2] & 0x40))
			{
				client->character.inventory[ch].item.data2[2] &= ~(0x40);
				break;
			}

			// Unwrap item
			if ((i.item.data[0] != 0x02) && (i.item.data[4] & 0x40))
			{
				client->character.inventory[ch].item.data[4] &= ~(0x40);
				break;
			}

			if (i.item.data[0] == 0x03) // Delete consumable item right away
				DeleteItemFromClient (itemid, 1, 0, client);

			break;
		}
	}

	if (!found_item)
	{
		Send1A ("Could not find item to \"use\".", client);
		client->todc = 1;
	}
	else
	{
		// Setting the eq variables here should fix problem with ADD SLOT and such.
		eq_wep = eq_armor = eq_shield = eq_mag = -1;

		for (ch2=0;ch2<client->character.inventoryUse;ch2++)
		{
			if ( client->character.inventory[ch2].flags & 0x08 )
			{
				switch ( client->character.inventory[ch2].item.data[0] )
				{
				case 0x00:
					eq_wep = ch2;
					break;
				case 0x01:
					switch ( client->character.inventory[ch2].item.data[1] )
					{
					case 0x01:
						eq_armor = ch2;
						break;
					case 0x02:
						eq_shield = ch2;
						break;
					}
					break;
				case 0x02:
					eq_mag = ch2;
					break;
				}
			}
		}

		switch (i.item.data[0])
		{
		case 0x00:
			switch (i.item.data[1])
			{
			case 0x33:
				client->character.inventory[ch].item.data[1] = 0x32; // Sealed J-Sword -> Tsumikiri J-Sword
				SendItemToEnd (itemid, client);
				break;
			case 0x1E:
				// Heaven Punisher used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xAF) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Heaven Punisher
				break;
			case 0x42:
				// Handgun: Guld or Master Raven used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x43) && 
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Guld or Raven...
				break;
			case 0x43:
				// Handgun: Milla or Last Swan used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x42) && 
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Milla or Swan...
				break;
			case 0x8A:
				// Sange or Yasha...
				if (eq_wep != -1)
				{
					if (client->character.inventory[eq_wep].item.data[2] == !(i.item.data[2]))
					{
						client->character.inventory[eq_wep].item.data[1] = 0x89;
						client->character.inventory[eq_wep].item.data[2] = 0x03;
						client->character.inventory[eq_wep].item.data[3] = 0x00;
						client->character.inventory[eq_wep].item.data[4] = 0x00;
						SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
					}
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of the other sword...
				break;
			case 0xAB:
				client->character.inventory[ch].item.data[1] = 0xAC; // Convert Lame d'Argent into Excalibur
				SendItemToEnd (itemid, client);
				break;
			case 0xAF:
				// Ophelie Seize used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x1E) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Ophelie Seize
				break;
			case 0xB6:
				// Guren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB7) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Guren
				break;
			case 0xB7:
				// Shouren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB6) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Shouren
				break;
			}
			break;
		case 0x01:
			if (i.item.data[1] == 0x03)
			{
				if (i.item.data[2] == 0x4D) // Limiter -> Adept
				{
					client->character.inventory[ch].item.data[2] = 0x4E;
					SendItemToEnd (itemid, client);
				}

				if (i.item.data[2] == 0x4F) // Swordsman Lore -> Proof of Sword-Saint
				{
					client->character.inventory[ch].item.data[2] = 0x50;
					SendItemToEnd (itemid, client);
				}
			}
			break;
		case 0x02:
			switch (i.item.data[1])
			{
			case 0x2B:
				// Chao Mag used
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x68) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x58; // Striker of Chao
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Chao
				break;
			case 0x2C:
				// Chu Chu mag used
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[2] == 0x1C))
				{
					client->character.inventory[eq_armor].item.data[2] = 0x2C; // Chuchu Fever
					SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Chu Chu
				break;
			}
			break;
		case 0x03:
			switch (i.item.data[1])
			{
			case 0x02:
				if (i.item.data[4] < 19)
				{
					if (((char)i.item.data[2] > max_tech_level[i.item.data[4]][client->character._class]) ||
						(client->equip_flags & DROID_FLAG))
					{
						Send1A ("You can't learn that technique.", client);
						client->todc = 1;
					}
					else
						client->character.techniques[i.item.data[4]] = i.item.data[2]; // Learn technique
				}
				break;
			case 0x0A:
				if (eq_wep != -1)
				{
					client->character.inventory[eq_wep].item.data[3] += ( i.item.data[2] + 1 );
					CheckMaxGrind (&client->character.inventory[eq_wep]);
					break;
				}
				break;
			case 0x0B:
				if (!client->mode)
				{
					HPMatUse = ( client->character.HPmat + client->character.TPmat ) / 2;
					TotalMatUse = 0;
					for (ch2=0;ch2<5;ch2++)
						TotalMatUse += client->matuse[ch2];
					mat_exceed = 0;
					if ( client->equip_flags & HUMAN_FLAG )
						max_mat = 250;
					else
						max_mat = 150;
				}
				else
				{
					TotalMatUse = 0;
					HPMatUse = 0;
					max_mat = 999;
					mat_exceed = 0;
				}
				switch (i.item.data[2])  // Materials
				{
				case 0x00:
					if ( TotalMatUse < max_mat )
					{
						client->character.ATP += 2;
						if (!client->mode)
							client->matuse[0]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x01:
					if ( TotalMatUse < max_mat )
					{
						client->character.MST += 2;
						if (!client->mode)
							client->matuse[1]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x02:
					if ( TotalMatUse < max_mat )
					{
						client->character.EVP += 2;
						if (!client->mode)
							client->matuse[2]++;
					}
					else 
						mat_exceed = 1;
					break;
				case 0x03:
					if ( ( client->character.HPmat < 250 ) && ( HPMatUse < 250 ) )
						client->character.HPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x04:
					if ( ( client->character.TPmat < 250 ) && ( HPMatUse < 250 ) )
						client->character.TPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x05:
					if ( TotalMatUse < max_mat )
					{
						client->character.DFP += 2;
						if (!client->mode)
							client->matuse[3]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x06:
					if ( TotalMatUse < max_mat )
					{
						client->character.LCK += 2;
						if (!client->mode)
							client->matuse[4]++;
					}
					else
						mat_exceed = 1;
					break;
				default:
					break;
				}
				if ( mat_exceed )
				{
					Send1A ("Attempt to exceed material usage limit.", client);
					client->todc = 1;
				}
				break;
			case 0x0C:
				switch ( i.item.data[2] )
				{
				case 0x00: // Mag Cell 502
					if ( eq_mag != -1 )
					{
						if ( client->character.sectionID & 0x01 )
							client->character.inventory[eq_mag].item.data[1] = 0x1D;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x21;
					}
					break;
				case 0x01: // Mag Cell 213
					if ( eq_mag != -1 )
					{
						if ( client->character.sectionID & 0x01 )
							client->character.inventory[eq_mag].item.data[1] = 0x27;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x22;
					}
					break;
				case 0x02: // Parts of RoboChao
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x28;
					break;
				case 0x03: // Heart of Opa Opa
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x29;
					break;
				case 0x04: // Heart of Pian
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x2A;
					break;
				case 0x05: // Heart of Chao
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x2B;
					break;
				}
				break;
			case 0x0E:
				if ( ( eq_shield != -1 ) && ( i.item.data[2] > 0x15 ) && ( i.item.data[2] < 0x26 ) )
				{
					// Merges
					client->character.inventory[eq_shield].item.data[2] = 0x3A + ( i.item.data[2] - 0x16 );
					SendItemToEnd (client->character.inventory[eq_shield].item.itemid, client);
				}
				else
					switch ( i.item.data[2] )
				{
					case 0x00: 
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x8E ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x8E;
							client->character.inventory[eq_wep].item.data[2]  = 0x01; // S-Berill's Hands #1
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x01: // Parasitic Gene "Flow"
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x02:
								client->character.inventory[eq_wep].item.data[1]  = 0x9D; // Dark Flow
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x09:
								client->character.inventory[eq_wep].item.data[1]  = 0x9E; // Dark Meteor
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x0B:
								client->character.inventory[eq_wep].item.data[1]  = 0x9F; // Dark Bridge
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x02: // Magic Stone "Iritista"
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x05 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x9C; // Rainbow Baton
							client->character.inventory[eq_wep].item.data[2]  = 0x00;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x03: // Blue-Black Stone
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x2F ) && 
							( client->character.inventory[eq_wep].item.data[2] == 0x00 ) && 
							( client->character.inventory[eq_wep].item.data[3] == 0x19 ) )
						{
							client->character.inventory[eq_wep].item.data[2]  = 0x01; // Black King Bar
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x04: // Syncesta
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x1F:
								client->character.inventory[eq_wep].item.data[1]  = 0x38; // Lavis Blade
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x38:
								client->character.inventory[eq_wep].item.data[1]  = 0x30; // Double Cannon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x30:
								client->character.inventory[eq_wep].item.data[1]  = 0x1F; // Lavis Cannon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x05: // Magic Water
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x56 ) ) 								
						{
							if ( client->character.inventory[eq_wep].item.data[2] == 0x00 )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x5D; // Plantain Fan
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
							else
								if ( client->character.inventory[eq_wep].item.data[2] == 0x01 )
								{
									client->character.inventory[eq_wep].item.data[1]  = 0x63; // Plantain Huge Fan
									client->character.inventory[eq_wep].item.data[2]  = 0x00;
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
									break;
								}
						}
						break;
					case 0x06: // Parasitic Cell Type D
						if ( eq_armor != -1 )
							switch ( client->character.inventory[eq_armor].item.data[2] ) 
						{
							case 0x1D:  
								client->character.inventory[eq_armor].item.data[2] = 0x20; // Parasite Wear: De Rol
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x20: 
								client->character.inventory[eq_armor].item.data[2] = 0x21; // Parsite Wear: Nelgal
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x21: 
								client->character.inventory[eq_armor].item.data[2] = 0x22; // Parasite Wear: Vajulla
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x22: 
								client->character.inventory[eq_armor].item.data[2] = 0x2F; // Virus Armor: Lafuteria
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
						}
						break;
					case 0x07: // Magic Rock "Heart Key"
						if ( eq_armor != -1 )
						{
							if ( client->character.inventory[eq_armor].item.data[2] == 0x1C )
							{
								client->character.inventory[eq_armor].item.data[2] = 0x2D; // Love Heart
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
							}
							else
								if ( client->character.inventory[eq_armor].item.data[2] == 0x2D ) 
								{
									client->character.inventory[eq_armor].item.data[2] = 0x45; // Sweetheart
									SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								}
								else
									if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0C ) )
									{
										client->character.inventory[eq_wep].item.data[1]  = 0x24; // Magical Piece
										client->character.inventory[eq_wep].item.data[2]  = 0x00;
										client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
										client->character.inventory[eq_wep].item.data[4]  = 0x00;
										SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
									}
									else
										if ( ( eq_shield != -1 ) && ( client->character.inventory[eq_shield].item.data[2] == 0x15 ) )
										{
											client->character.inventory[eq_shield].item.data[2]  = 0x2A; // Safety Heart
											SendItemToEnd (client->character.inventory[eq_shield].item.itemid, client);
										}
						}
						break;
					case 0x08: // Magic Rock "Moola"
						if ( ( eq_armor != -1 ) && ( client->character.inventory[eq_armor].item.data[2] == 0x1C ) )
						{
							client->character.inventory[eq_armor].item.data[2] = 0x31; // Aura Field
							SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0A ) )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x4F; // Summit Moon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00; // No attribute
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							}
							break;
					case 0x09: // Star Amplifier
						if ( ( eq_armor != -1 ) && ( client->character.inventory[eq_armor].item.data[2] == 0x1C ) )
						{
							client->character.inventory[eq_armor].item.data[2] = 0x30; // Brightness Circle
							SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0C ) )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x5C; // Twinkle Star
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00; // No attribute
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							}
							break;
					case 0x0A: // Book of Hitogata
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x8C ) &&
							( client->character.inventory[eq_wep].item.data[2] == 0x02 ) && 
							( client->character.inventory[eq_wep].item.data[3] == 0x09 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x8C;
							client->character.inventory[eq_wep].item.data[2]  = 0x03;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0B: // Heart of Chu Chu
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2C;
						break;
					case 0x0C: // Parts of Egg Blaster
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x06 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x1C; // Egg Blaster
							client->character.inventory[eq_wep].item.data[2]  = 0x00;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0D: // Heart of Angel
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2E;
						break;
					case 0x0E: // Heart of Devil
						if ( eq_mag != -1 )
						{
							if ( client->character.inventory[eq_mag].item.data[1] == 0x2F )
								client->character.inventory[eq_mag].item.data[1] = 0x38;
							else
								client->character.inventory[eq_mag].item.data[1] = 0x2F;
						}
						break;
					case 0x0F: // Kit of Hamburger
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x36;
						break;
					case 0x10: // Panther's Spirit
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x37;
						break;
					case 0x11: // Kit of Mark 3
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x31;
						break;
					case 0x12: // Kit of Master System
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x32;
						break;
					case 0x13: // Kit of Genesis
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x33;
						break;
					case 0x14: // Kit of Sega Saturn
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x34;
						break;
					case 0x15: // Kit of Dreamcast
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x35;
						break;
					case 0x26: // Heart of Kapukapu
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2D;
						break;
					case 0x27: // Photon Booster
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x15:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Burning Visit
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x45:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Snow Queen
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x4E:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Iron Faust
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x6D: 
								if ( client->character.inventory[eq_wep].item.data[3] == 0x14 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Power Maser
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							}
						}
						break;

				}
				break;
			case 0x0F: // Add Slot
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[5] < 0x04))
					client->character.inventory[eq_armor].item.data[5]++;
				break;
			case 0x15:
				new_item = 0;
				switch ( i.item.data[2] )
				{
				case 0x00:
					new_item = 0x0D0E03 + ( ( mt_lrand() % 9 ) * 0x10000 );
					break;
				case 0x01:
					new_item = easter_drops[mt_lrand() % 9];
					break;
				case 0x02:
					new_item = jacko_drops[mt_lrand() % 8];
					break;
				default:
					break;
				}

				if ( new_item )
				{
					INVENTORY_ITEM add_item;

					memset (&add_item, 0, sizeof (INVENTORY_ITEM));
					*(unsigned *) &add_item.item.data[0] = new_item;
					add_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory (&add_item, 1, 0, client);
				}
				break;
			case 0x18: // Ep4 Mag Cells
				if ( eq_mag != -1 )
					client->character.inventory[eq_mag].item.data[1] = 0x42 + ( i.item.data[2] );
				break;
			}
			break;
		default:
			break;
		}
	}
}

int check_equip (unsigned char eq_flags, unsigned char cl_flags)
{
	int eqOK = 1;
	unsigned ch;

	for (ch=0;ch<8;ch++)
	{
		if ( ( cl_flags & (1 << ch) ) && (!( eq_flags & ( 1 << ch ) )) )
		{
			eqOK = 0;
			break;
		}
	}
	return eqOK;
}

void EquipItem (unsigned int itemid, PSO_CLIENT* client)
{
	unsigned ch, ch2, found_item, found_slot;
	unsigned slot[4];

	found_item = 0;
	found_slot = 0;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			//debug ("Equipped %u", itemid);
			found_item = 1;
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Check weapon equip requirements
				if (!check_equip(weapon_equip_table[client->character.inventory[ch].item.data[1]][client->character.inventory[ch].item.data[2]], client->equip_flags))
				{
					Send1A ("\"God/Equip\" is disallowed.", client);
					client->todc = 1;
				}
				if (!client->todc)
				{
					// De-equip any other weapon on character. (Prevent stacking)
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
							(client->character.inventory[ch2].flags & 0x08))
							client->character.inventory[ch2].flags &= ~(0x08);
				}
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01: // Check armor equip requirements
					if ((!check_equip (armor_equip_table[client->character.inventory[ch].item.data[2]], client->equip_flags)) || 
						(client->character.level < armor_level_table[client->character.inventory[ch].item.data[2]]))
					{
						Send1A ("\"God/Equip\" is disallowed.", client);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other armor and equipped slot items.
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
							break;
					}
					break;
				case 0x02: // Check barrier equip requirements
					if ((!check_equip (barrier_equip_table[client->character.inventory[ch].item.data[2]] & client->equip_flags, client->equip_flags)) ||
						(client->character.level < barrier_level_table[client->character.inventory[ch].item.data[2]]))
					{
						Send1A ("\"God/Equip\" is disallowed.", client);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other barrier
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
					}
					break;
				case 0x03: // Assign unit a slot
					for (ch2=0;ch2<4;ch2++)
						slot[ch2] = 0;
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					{
						// Another loop ;(
						if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
							(client->character.inventory[ch2].item.data[1] == 0x03))
						{
							if ((client->character.inventory[ch2].flags & 0x08) && 
								(client->character.inventory[ch2].item.data[4] < 0x04))
								slot [client->character.inventory[ch2].item.data[4]] = 1;
						}
					}
					for (ch2=0;ch2<4;ch2++)
						if (slot[ch2] == 0)
						{
							found_slot = ch2 + 1;
							break;
						}
						if (found_slot)
						{
							found_slot --;
							client->character.inventory[ch].item.data[4] = (unsigned char)(found_slot);
						}
						else
						{
							client->character.inventory[ch].flags &= ~(0x08);
							Send1A ("There are no free slots on your armor.  Equip unit failed.", client);
							client->todc = 1;
						}
						break;
				}
				break;
			case 0x02:
				// Remove equipped mag
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			if ( !client->todc )  // Finally, equip item
				client->character.inventory[ch].flags |= 0x08;
			break;
		}
	}
	if (!found_item)
	{
		Send1A ("Could not find item to equip.", client);
		client->todc = 1;
	}
}

void DeequipItem (unsigned int itemid, PSO_CLIENT* client)
{
	unsigned ch, ch2, found_item = 0;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;
			client->character.inventory[ch].flags &= ~(0x08);
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Remove any other weapon (in case of a glitch... or stacking)
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01:
					// Remove any other armor (stacking?) and equipped slot items.
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] != 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
						break;
				case 0x02:
					// Remove any other barrier (stacking?)
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] == 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
						break;
				case 0x03:
					// Remove unit from slot
					client->character.inventory[ch].item.data[4] = 0x00;
					break;
				}
				break;
			case 0x02:
				// Remove any other mags (stacking?)
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			break;
		}
	}
	if (!found_item)
	{
		Send1A ("Could not find item to unequip.", client);
		client->todc = 1;
	}
}

unsigned int GetShopPrice(INVENTORY_ITEM* ci)
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

void CheckMaxGrind (INVENTORY_ITEM* i)
{
	if (i->item.data[3] > grind_table[i->item.data[1]][i->item.data[2]])
		i->item.data[3] = grind_table[i->item.data[1]][i->item.data[2]];
}

void SortBankItems (PSO_CLIENT* client)
{
	unsigned ch, ch2;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned char swap_c;
	BANK_ITEM swap_item;
	BANK_ITEM b1;
	BANK_ITEM b2;

	if ( client->character.bankUse > 1 )
	{
		for ( ch=0;ch<client->character.bankUse - 1;ch++ )
		{
			memcpy (&b1, &client->character.bankInventory[ch], sizeof (BANK_ITEM));
			swap_c     = b1.data[0];
			b1.data[0] = b1.data[2];
			b1.data[2] = swap_c;
			memcpy (&compare_item1, &b1.data[0], 3);
			for ( ch2=ch+1;ch2<client->character.bankUse;ch2++ )
			{
				memcpy (&b2, &client->character.bankInventory[ch2], sizeof (BANK_ITEM));
				swap_c     = b2.data[0];
				b2.data[0] = b2.data[2];
				b2.data[2] = swap_c;
				memcpy (&compare_item2, &b2.data[0], 3);
				if (compare_item2 < compare_item1) // compare_item2 should take compare_item1's place
				{
					memcpy (&swap_item, &client->character.bankInventory[ch], sizeof (BANK_ITEM));
					memcpy (&client->character.bankInventory[ch], &client->character.bankInventory[ch2], sizeof (BANK_ITEM));
					memcpy (&client->character.bankInventory[ch2], &swap_item, sizeof (BANK_ITEM));
					memcpy (&compare_item1, &compare_item2, 3);
				}
			}
		}
	}
}

void DepositIntoBank (unsigned itemid, unsigned count, PSO_CLIENT* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned deposit_item = 0, deposit_done = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// Moves an item from the client's character to it's bank.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if ( stackable )
			{
				if (!count)
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];

				if (!stack_count)
					stack_count = 1;

				if ( stack_count < count )
				{
					Send1A ("Trying to deposit more items than in possession.", client);
					client->todc = 1; // Tried to deposit more than had?
				}
				else
				{
					deposit_item = 1;

					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

					if ( !stack_count )
						delete_item = 1;
				}
			}
			else
			{
				// Not stackable, remove from client completely.
				deposit_item = 1;
				delete_item = 1;
			}

			if ( deposit_item )
			{
				if ( stackable )
				{
					memcpy (&compare_item1, &client->character.inventory[ch].item.data[0], 3);
					for (ch2=0;ch2<client->character.bankUse;ch2++)
					{
						memcpy (&compare_item2, &client->character.bankInventory[ch2].data[0], 3);
						if (compare_item1 == compare_item2)
						{
							stack_count = ( client->character.bankInventory[ch2].bank_count & 0xFF );
							if ( ( stack_count + count ) > stackable )
							{
								count = stackable - stack_count;
								client->character.bankInventory[ch2].bank_count = 0x10000 + stackable;
							}
							else
								client->character.bankInventory[ch2].bank_count += count;
							deposit_done = 1;
							break;
						}
					}
				}

				if ( ( !client->todc ) && ( !deposit_done ) )
				{
					if (client->character.inventory[ch].item.data[0] == 0x01)
					{
						if ((client->character.inventory[ch].item.data[1] == 0x01) &&
							(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
								if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
									(client->character.inventory[ch2].item.data[1] != 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].flags &= ~(0x08);
									client->character.inventory[ch2].item.data[4] = 0x00;
								}
						}
					}

					memcpy (&client->character.bankInventory[client->character.bankUse].data[0],
						&client->character.inventory[ch].item.data[0],
						sizeof (ITEM));

					if ( stackable )
					{
						memset ( &client->character.bankInventory[client->character.bankUse].data[4], 0, 4 );
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10000 + count;
					}
					else
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10001;

					client->character.bankInventory[client->character.bankUse].itemid = client->character.inventory[ch].item.itemid; // for now
					client->character.bankUse++;
				}

				if ( delete_item )
					client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}

	if ( found_item == -1 )
	{
		Send1A ("Could not find item to deposit.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);
}

void DeleteFromInventory (INVENTORY_ITEM* i, unsigned count, PSO_CLIENT* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned compare_id;

	// Deletes an item from the client's character data.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	memcpy (&compare_item1, &i->item.data[0], 3);
	if (i->item.itemid)
		compare_id = i->item.itemid;
	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
		if (!i->item.itemid)
			compare_id = client->character.inventory[ch].item.itemid;
		if ((compare_item1 == compare_item2) && (compare_id == client->character.inventory[ch].item.itemid)) // Found the item?
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if ( stackable )
			{
				if ( !count )
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];
				if ( !stack_count )
					stack_count = 1;

				if ( stack_count < count )
					count = stack_count;

				stack_count -= count;

				client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

				if (!stack_count)
					delete_item = 1;
			}
			else
				delete_item = 1;

			memset (&client->encryptbuf[0x00], 0, 0x14);
			client->encryptbuf[0x00] = 0x14;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0x29;
			client->encryptbuf[0x09] = 0x05;
			client->encryptbuf[0x0A] = client->clientID;
			*(unsigned *) &client->encryptbuf[0x0C] =  client->character.inventory[ch].item.itemid;
			client->encryptbuf[0x10] = (unsigned char) count;

			SendToLobby (l, 4, &client->encryptbuf[0x00], 0x14, 0);

			if ( delete_item )
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}
	if ( found_item == -1 )
	{
		Send1A ("Could not find item to delete from inventory.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);

}

unsigned int AddToInventory (INVENTORY_ITEM* i, unsigned count, int shop, PSO_CLIENT* client)
{
	unsigned ch;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned notsend;
	LOBBY* l;

	// Adds an item to the client's inventory... (out of thin air)
	// The new itemid must already be set to i->item.itemid

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	if (i->item.data[0] == 0x04)
	{
		// Meseta
		count = *(unsigned *) &i->item.data2[0];
		client->character.meseta += count;
		if (client->character.meseta > 999999)
			client->character.meseta = 999999;
		item_added = 1;
	}
	else
	{
		if ( i->item.data[0] == 0x03 )
			stackable = stackable_table [i->item.data[1]];
	}

	if ( ( !client->todc ) && ( !item_added ) )
	{
		if ( stackable )
		{
			if (!count)
				count = 1;
			memcpy (&compare_item1, &i->item.data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ( ( stack_count + count ) > stackable )
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char) ( stack_count + count );
					item_added = 1;
					break;
				}
			}
		}

		if ( item_added == 0 ) // Make sure we don't go over the max inventory
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &i->item, sizeof (ITEM));
				if ( stackable )
				{
					memset (&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
					client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char) count;
				}
				client->character.inventoryUse++;
				item_added = 1;
			}
		}
	}

	if ((!client->todc) && ( item_added ) )
	{
		// Let people know the client has a new toy...
		memset (&client->encryptbuf[0x00], 0, 0x24);
		client->encryptbuf[0x00] = 0x24;
		client->encryptbuf[0x02] = 0x60;
		client->encryptbuf[0x08] = 0xBE;
		client->encryptbuf[0x09] = 0x09;
		client->encryptbuf[0x0A] = client->clientID;
		memcpy (&client->encryptbuf[0x0C], &i->item.data[0], 12);
		*(unsigned *) &client->encryptbuf[0x18] = i->item.itemid;
		if ((!stackable) || (i->item.data[0] == 0x04))
			*(unsigned *) &client->encryptbuf[0x1C] = *(unsigned *) &i->item.data2[0];
		else
			client->encryptbuf[0x11] = count;
		memset (&client->encryptbuf[0x20], 0, 4);
		if ( shop )
			notsend = client->guildcard;
		else
			notsend = 0;
		SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x24, notsend );
	}
	return item_added;
}
