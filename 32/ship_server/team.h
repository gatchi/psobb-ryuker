void TeamChat ( unsigned short* text, unsigned short chatsize, unsigned teamid, PSO_SERVER* ship )
{
	unsigned size;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x04;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	while (chatsize % 8)
		ship->encryptbuf[6 + (chatsize++)] = 0x00;
	*text = chatsize;;
	memcpy (&ship->encryptbuf[0x06], text, chatsize);
	size = chatsize + 6;;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], size );
}

void CreateTeam (unsigned short* teamname, unsigned guildcard, PSO_SERVER* ship)
{
	unsigned short *g;
	unsigned n;

	n = 0;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x00;

	g = (unsigned short*) &ship->encryptbuf[0x02];

	memset (g, 0, 24);
	while ((*teamname != 0x0000) && (n<11))
	{
		if ((*teamname != 0x0009) && (*teamname != 0x000A))
			*(g++) = *teamname;
		else
			*(g++) = 0x0020;
		teamname++;
		n++;
	}
	*(unsigned*) &ship->encryptbuf[0x1A] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x1E );
}

void UpdateTeamFlag (unsigned char* flag, unsigned teamid, PSO_SERVER* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x01;
	memcpy (&ship->encryptbuf[0x02], flag, 0x800);
	*(unsigned*) &ship->encryptbuf[0x802] = teamid;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x806 );
}

void DissolveTeam (unsigned teamid, PSO_SERVER* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x06 );
}

void RemoveTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void RequestTeamList ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x05;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void PromoteTeamMember ( unsigned teamid, unsigned guildcard, unsigned char newlevel, PSO_SERVER* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x06;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	ship->encryptbuf[0x0A] = newlevel;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0B );
}

void AddTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x07;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

