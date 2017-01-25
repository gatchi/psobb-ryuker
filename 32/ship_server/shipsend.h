void ShipSend04 (unsigned char command, PSO_CLIENT* client, PSO_SERVER* ship)
{
	//unsigned ch;

	ship->encryptbuf[0x00] = 0x04;
	switch (command)
	{
	case 0x00:
		// Request character data from server
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *) &ship->encryptbuf[0x02] = client->guildcard;
		*(unsigned short *) &ship->encryptbuf[0x06] = (unsigned short) client->slotnum;
		*(int *) &ship->encryptbuf[0x08] = client->plySockfd;
		*(unsigned *) &ship->encryptbuf[0x0C] = serverID;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x10 );
		break;
	case 0x02:
		// Send character data to server when not using a temporary character.
		if ((!client->mode) && (client->gotchardata == 1))
		{
			ship->encryptbuf[0x01] = 0x02;
			*(unsigned *) &ship->encryptbuf[0x02] = client->guildcard;
			*(unsigned short*) &ship->encryptbuf[0x06] = (unsigned short) client->slotnum;
			memcpy (&ship->encryptbuf[0x08], &client->character.packetSize, sizeof (CHARDATA));
			// Include character bank in packet
			memcpy (&ship->encryptbuf[0x08+0x700], &client->char_bank, sizeof (BANK));
			// Include common bank in packet
			memcpy (&ship->encryptbuf[0x08+sizeof(CHARDATA)], &client->common_bank, sizeof (BANK));
			compressShipPacket ( ship, &ship->encryptbuf[0x00], sizeof(BANK) + sizeof(CHARDATA) + 8 );
		}
		break;
	}
}

void ShipSend0E (PSO_SERVER* ship)
{
	if (logon_ready)
	{
		ship->encryptbuf[0x00] = 0x0E;
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *) &ship->encryptbuf[0x02] = serverID;
		*(unsigned *) &ship->encryptbuf[0x06] = serverNumConnections;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
	}
}

void ShipSend0D (unsigned char command, PSO_CLIENT* client, PSO_SERVER* ship)
{
	ship->encryptbuf[0x00] = 0x0D;
	switch (command)
	{
	case 0x00:
		// Requesting ship list.
		ship->encryptbuf[0x01] = 0x00;
		*(int *) &ship->encryptbuf[0x02]= client->plySockfd;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 6 );
		break;
	default:
		break;
	}
}

void ShipSend0B (PSO_CLIENT* client, PSO_SERVER* ship)
{
	ship->encryptbuf[0x00] = 0x0B;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned *) &ship->encryptbuf[0x02] = *(unsigned *) &client->decryptbuf[0x0C];
	*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &client->decryptbuf[0x18];
	*(long long *) &ship->encryptbuf[0x0A] = *(long long*) &client->decryptbuf[0x8C];
	*(long long *) &ship->encryptbuf[0x12] = *(long long*) &client->decryptbuf[0x94];
	*(long long *) &ship->encryptbuf[0x1A] = *(long long*) &client->decryptbuf[0x9C];
	*(long long *) &ship->encryptbuf[0x22] = *(long long*) &client->decryptbuf[0xA4];
	*(long long *) &ship->encryptbuf[0x2A] = *(long long*) &client->decryptbuf[0xAC];
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x32 );
}
