void AddExp (unsigned int XP, PSO_CLIENT* client)
{
	MAG* m;
	unsigned short levelup, ch, finalDFP, finalATP, finalATA, finalMST;

	if (!client->lobby)
		return;

	client->character.XP += XP;

	PacketBF[0x0A] = (unsigned char) client->clientID;
	*(unsigned *) &PacketBF[0x0C] = XP;
	SendToLobby (client->lobby, 4, &PacketBF[0], 0x10, 0);
	if (client->character.XP >= tnlxp[client->character.level])
		levelup = 1;
	else
		levelup = 0;

	while (levelup)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP  += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
		if ((client->character.level == 199) || (client->character.XP < tnlxp[client->character.level]))
			break;
	}

	if (levelup)
	{
		finalDFP = client->character.DFP;
		finalATP = client->character.ATP;
		finalATA = client->character.ATA;
		finalMST = client->character.MST;

		// Add the mag bonus to the 0x30 packet
		for (ch=0;ch<client->character.inventoryUse;ch++)
		{
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].flags & 0x08))
			{
				m = (MAG*) &client->character.inventory[ch].item.data[0];
				finalDFP += ( m->defense / 100 );
				finalATP += ( m->power / 100 ) * 2;
				finalATA += ( m->dex / 100 ) / 2;
				finalMST += ( m->mind / 100 ) * 2;
				break;
			}
		}

		*(unsigned short*) &Packet30[0x0C] = finalATP;
		*(unsigned short*) &Packet30[0x0E] = finalMST;
		*(unsigned short*) &Packet30[0x10] = client->character.EVP;
		*(unsigned short*) &Packet30[0x12] = client->character.HP;
		*(unsigned short*) &Packet30[0x14] = finalDFP;
		*(unsigned short*) &Packet30[0x16] = finalATA;
		*(unsigned short*) &Packet30[0x18] = client->character.level;
		Packet30[0x0A] = (unsigned char) client->clientID;
		SendToLobby ( client->lobby, 4, &Packet30[0x00], 0x1C, 0);
	}
}
