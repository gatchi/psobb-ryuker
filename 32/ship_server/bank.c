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
