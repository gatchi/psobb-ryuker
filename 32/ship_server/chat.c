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
