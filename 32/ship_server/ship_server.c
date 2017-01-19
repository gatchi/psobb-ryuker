/************************************************************************
	Tethealla Ship Server
	Copyright (C) 2008  Terry Chatman Jr.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3 as
	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/


#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>
#include	<math.h>

#include	"ship_server.h"
#include	"resource.h"
#include	"pso_crypt.h"
#include	"bbtable.h"
#include	"localgms.h"
#include	"def_map.h" // Map file name definitions
#include	"def_block.h" // Blocked packet definitions
#include	"def_packets.h" // Pre-made packet definitions
#include	"def_structs.h" // Various structure definitions
#include	"def_tables.h" // Various pre-made table definitions
#include	"network.h"
#include	"team.h"
#include	"utility.h"
#include	"fileio.h"
#include	"packet.h"
#include	"commands.h"

const unsigned char Message03[] = { "Tethealla Ship v.144" };

/* variables */

struct timeval select_timeout = { 
	0,
	5000
};

FILE* debugfile;

// Random drop rates

unsigned int WEAPON_DROP_RATE, ARMOR_DROP_RATE, MAG_DROP_RATE, TOOL_DROP_RATE, MESETA_DROP_RATE, EXPERIENCE_RATE;
unsigned int common_rates[5] = { 0 };

// Rare monster appearance rates (why is only one of them initialized to zero?

unsigned int hildebear_rate, rappy_rate, lily_rate, slime_rate, merissa_rate, pazuzu_rate, dorphon_rate, kondrieu_rate = 0;

// What the fuck is this shit?

unsigned common_counters[5] = {0};

unsigned char common_table[100000];

unsigned char PacketA0Data[0x4000] = {0};
unsigned char Packet07Data[0x4000] = {0};
unsigned short Packet07Size = 0;
unsigned char PacketData[TCP_BUFFER_SIZE];
unsigned char PacketData2[TCP_BUFFER_SIZE]; // Sometimes we need two...
unsigned char tmprcv[PACKET_BUFFER_SIZE];

/* Populated by load_config_file(): */

unsigned char serverIP[4] = {0};
int autoIP = 0;
unsigned char loginIP[4];
unsigned short serverPort;
unsigned short serverMaxConnections;
int ship_support_extnpc = 0;
unsigned serverNumConnections = 0;
unsigned blockConnections = 0;
unsigned blockTick = 0;
unsigned serverConnectionList[SHIP_COMPILED_MAX_CONNECTIONS];
unsigned short serverBlocks;
unsigned char shipEvent;
unsigned serverID = 0;
time_t servertime;
unsigned normalName = 0xFFFFFFFF;
unsigned globalName = 0xFF1D94F7;
unsigned localName = 0xFFB0C4DE;

// What's this?

unsigned short ship_banmasks[5000][4] = {0}; // IP address ban masks
BANDATA ship_bandata[5000];
unsigned num_masks = 0;
unsigned num_bans = 0;

/* Common tables */

PTDATA pt_tables_ep1[10][4];
PTDATA pt_tables_ep2[10][4];

// Episode I parsed PT data (PT?)
// Holy shit are these tables???

unsigned short weapon_drops_ep1[10][4][10][4096];
unsigned char slots_ep1[10][4][4096];
unsigned char tech_drops_ep1[10][4][10][4096];
unsigned char tool_drops_ep1[10][4][10][4096];
unsigned char power_patterns_ep1[10][4][4][4096];
char percent_patterns_ep1[10][4][6][4096];
unsigned char attachment_ep1[10][4][10][4096];

// Episode II parsed PT data

unsigned short weapon_drops_ep2[10][4][10][4096];
unsigned char slots_ep2[10][4][4096];
unsigned char tech_drops_ep2[10][4][10][4096];
unsigned char tool_drops_ep2[10][4][10][4096];
unsigned char power_patterns_ep2[10][4][4][4096];
char percent_patterns_ep2[10][4][6][4096];
unsigned char attachment_ep2[10][4][10][4096];


/* Rare tables */
// WHY ARENT THESE IN A SQL DATABASE

unsigned rt_tables_ep1[0x200 * 10 * 4] = {0}; 
unsigned rt_tables_ep2[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep4[0x200 * 10 * 4] = {0};

// What's this

unsigned char startingData[12*14];
playerLevel playerLevelData[12][200];

// ???
fd_set ReadFDs, WriteFDs, ExceptFDs;

// Is this here just like.... miscellaneous vars....

saveLobby savedlobbies[MAX_SAVED_LOBBIES];
unsigned char dp[TCP_BUFFER_SIZE*4];
unsigned ship_ignore_list[300] = {0};
unsigned ship_ignore_count = 0;
unsigned ship_gcsend_list[MAX_GCSEND*3] = {0};
unsigned ship_gcsend_count = 0;
char Ship_Name[255];
SHIPLIST shipdata[200];
BLOCK* blocks[10];
QUEST quests[512];
QUEST_MENU quest_menus[12];
unsigned* quest_allow = 0; // the "allow" list for the 0x60CA command...
unsigned quest_numallows;
unsigned numQuests = 0;
unsigned questsMemory = 0;
char* languageExts[10];
char* languageNames[10];
unsigned numLanguages = 0;
unsigned totalShips = 0;
BATTLEPARAM ep1battle[374];
BATTLEPARAM ep2battle[374];
BATTLEPARAM ep4battle[332];
BATTLEPARAM ep1battle_off[374];
BATTLEPARAM ep2battle_off[374];
BATTLEPARAM ep4battle_off[332];
unsigned battle_count;
SHOP shops[7000];
unsigned shop_checksum;
unsigned shopidx[200];
unsigned ship_index;
unsigned char ship_key[128];

// New leet parameter tables!!!!111oneoneoneeleven
// Goddammit....

unsigned char armor_equip_table[256] = {0};
unsigned char barrier_equip_table[256] = {0};
unsigned char armor_level_table[256] = {0};
unsigned char barrier_level_table[256] = {0};
unsigned char armor_dfpvar_table[256] = {0};
unsigned char barrier_dfpvar_table[256] = {0};
unsigned char armor_evpvar_table[256] = {0};
unsigned char barrier_evpvar_table[256] = {0};
unsigned char weapon_equip_table[256][256] = {0};
unsigned short weapon_atpmax_table[256][256] = {0};
unsigned char grind_table[256][256] = {0};
unsigned char special_table[256][256] = {0};
unsigned char stackable_table[256] = {0};
unsigned equip_prices[2][13][24][80] = {0};
char max_tech_level[19][12];

// And this is just by itself for some reason....
PSO_CRYPT* cipher_ptr;

// Hmm, this kinda looks like a Windows thing... maybe tray icon shit?

#define MYWM_NOTIFYICON (WM_USER+2)
int program_hidden = 1;
HWND consoleHwnd;

PSO_SERVER logon_structure;
PSO_CLIENT * connections[SHIP_COMPILED_MAX_CONNECTIONS];
PSO_SERVER * logon_connecion;
PSO_CLIENT * workConnect;
PSO_SERVER * logon;
unsigned logon_tick = 0;
unsigned logon_ready = 0;

// ...null characters?  In between the letters?  Why?

const char serverName[] = { "T\0E\0T\0H\0E\0A\0L\0L\0A\0" };
const char shipSelectString[] = {"S\0h\0i\0p\0 \0S\0e\0l\0e\0c\0t\0"};
const char blockString[] = {"B\0L\0O\0C\0K\0"};

// Send 8 what...

/*
	Im guessing by "block" it means a ship block.
*/
void ConstructBlockPacket()
{
	unsigned short Offset;
	unsigned ch;
	char tempName[255];
	char* tn;
	unsigned BlockID;

	memset (&Packet07Data[0], 0, 0x4000);

	Packet07Data[0x02] = 0x07;
	Packet07Data[0x04] = serverBlocks+1;
	_itoa (serverID, &tempName[0], 10);
	if (serverID < 10) 
	{
		tempName[0] = 0x30;
		tempName[1] = 0x30+serverID;
		tempName[2] = 0x00;
	}
	else
		_itoa (serverID, &tempName[0], 10);
	strcat (&tempName[0], ":");
	strcat (&tempName[0], &Ship_Name[0]);
	Packet07Data[0x32] = 0x08;
	Offset = 0x12;
	tn = &tempName[0];
	while (*tn != 0x00)
	{
		Packet07Data[Offset++] = *(tn++);
		Packet07Data[Offset++] = 0x00;
	}
	Offset = 0x36;
	for (ch=0;ch<serverBlocks;ch++)
	{
		Packet07Data[Offset] = 0x12;
		BlockID = 0xEFFFFFFF - ch;
		*(unsigned *) &Packet07Data[Offset+2] = BlockID;
		memcpy (&Packet07Data[Offset+0x08], &blockString[0], 10 );
		if ( ch+1 < 10 )
		{
			Packet07Data[Offset+0x12] = 0x30;
			Packet07Data[Offset+0x14] = 0x30 + (ch+1);
		}
		else
		{
			Packet07Data[Offset+0x12] = 0x31;
			Packet07Data[Offset+0x14] = 0x30;
		}
		Offset += 0x2C;
	}
	Packet07Data[Offset] = 0x12;
	BlockID = 0xFFFFFF00;
	*(unsigned *) &Packet07Data[Offset+2] = BlockID;
	memcpy (&Packet07Data[Offset+0x08], &shipSelectString[0], 22 );
	Offset += 0x2C;
	while (Offset % 8)
		Packet07Data[Offset++] = 0x00;
	*(unsigned short*) &Packet07Data[0x00] = (unsigned short) Offset;
	Packet07Size = Offset;
}

void initialize_logon()
{
	unsigned ch;

	logon_ready = 0;
	logon_tick = 0;
	logon = &logon_structure;
	if ( logon->sockfd >= 0 )
		closesocket ( logon->sockfd );
	memset (logon, 0, sizeof (PSO_SERVER));
	logon->sockfd = -1;
	for (ch=0;ch<128;ch++)
		logon->key_change[ch] = -1;
	*(unsigned *) &logon->_ip.s_addr = *(unsigned *) &loginIP[0];
}

void reconnect_logon()
{
	// Just in case this is called because of an error in communication with the logon server.
	logon->sockfd = tcp_sock_connect (  inet_ntoa (logon->_ip), 3455 );
	if (logon->sockfd >= 0)
	{
		printf ("Connection successful!\n");
		logon->last_ping = (unsigned) time(NULL);
	}
	else
	{
		printf ("Connection failed.  Retry in %u seconds...\n",  LOGIN_RECONNECT_SECONDS);
		logon_tick = 0;
	}
}

unsigned free_connection()
{
	unsigned fc;
	PSO_CLIENT* wc;

	for (fc=0;fc<serverMaxConnections;fc++)
	{
		wc = connections[fc];
		if (wc->plySockfd<0)
			return fc;
	}
	return 0xFFFF;
}

void initialize_connection (PSO_CLIENT* connect)
{
	unsigned ch, ch2;

	// Free backup character memory

	if (connect->character_backup)
	{
		if (connect->mode)
			memcpy (&connect->character, connect->character_backup, sizeof (connect->character));
		free (connect->character_backup);
		connect->character_backup = NULL;
	}

	if (connect->guildcard)
	{
		removeClientFromLobby (connect);

		if ((connect->block) && (connect->block <= serverBlocks))
			blocks[connect->block - 1]->count--;

		if (connect->gotchardata == 1)
		{
			connect->character.playTime += (unsigned) servertime - connect->connected;
			ShipSend04 (0x02, connect, logon);
		}
	}

	if (connect->plySockfd >= 0)
	{
		ch2 = 0;
		for (ch=0;ch<serverNumConnections;ch++)
		{
			if (serverConnectionList[ch] != connect->connection_index)
				serverConnectionList[ch2++] = serverConnectionList[ch];
		}
		serverNumConnections = ch2;
		closesocket (connect->plySockfd);
	}

	if (logon_ready)
	{
		printf ("Player Count: %u\n", serverNumConnections);
		ShipSend0E (logon);
	}

	memset (connect, 0, sizeof (PSO_CLIENT) );
	connect->plySockfd = -1;
	connect->block = -1;
	connect->lastTick = 0xFFFFFFFF;
	connect->slotnum = -1;
	connect->sending_quest = -1;
}

void SendToLobby (LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend )
{
	unsigned ch;

	if (!l)
		return;

	for (ch=0;ch<max_send;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->guildcard != nosend))
		{
			cipher_ptr = &l->client[ch]->server_cipher;
			encryptcopy (l->client[ch], src, size);
		}
	}
}


void removeClientFromLobby (PSO_CLIENT* client)
{
	unsigned ch, maxch, lowestID;

	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	if (client->clientID < 12)
	{
		l->slot_use[client->clientID] = 0;
		l->client[client->clientID] = 0;
	}

	if (client->lobbyNum > 0x0F)
		maxch = 4;
	else
		maxch = 12;

	l->lobbyCount = 0;

	for (ch=0;ch<maxch;ch++)
	{
		if ((l->client[ch]) && (l->slot_use[ch]))
			l->lobbyCount++;
	}

	if ( l->lobbyCount )
	{
		if ( client->lobbyNum < 0x10 )
		{
			Packet69[0x08] = client->clientID;
			SendToLobby ( client->lobby, 12, &Packet69[0], 0x0C, client->guildcard );
		}
		else
		{
			Packet66[0x08] = client->clientID;
			if (client->clientID == l->leader)
			{
				// Leader change...
				lowestID = 0xFFFFFFFF;
				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->gamePlayerID[ch] < lowestID))
					{
						// Change leader to oldest person to join game...
						lowestID = l->gamePlayerID[ch];
						l->leader = ch;
					}
				}
				Packet66[0x0A] = 0x01;
			}
			else
				Packet66[0x0A] = 0x00;
			Packet66[0x09] = l->leader;
			SendToLobby ( client->lobby, 4, &Packet66[0], 0x0C, client->guildcard );
		}
	}
	else
		memset ( l, 0, sizeof ( LOBBY ) );
	client->hasquest = 0;
	client->lobbyNum = 0;
	client->lobby = 0;
}

/*
	Does this free the game from memory or does this start
	some sort of "free" game?  What does it mean by game?
*/
unsigned free_game (PSO_CLIENT* client)
{
	unsigned ch;
	LOBBY* l;

	for (ch=16;ch<(16+SHIP_COMPILED_MAX_GAMES);ch++)
	{
		l = &blocks[client->block - 1]->lobbies[ch];
		if (l->in_use == 0)
			return ch;
	}
	return 0;
}

/*
	This function is probably waaaaaaaay bigger than it needs to be.
*/
void initialize_game (PSO_CLIENT* client)
{
	LOBBY* l;
	unsigned ch;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;
	memset (l, 0, sizeof (LOBBY));

	l->difficulty = client->decryptbuf[0x50];
	l->battle = client->decryptbuf[0x51];
	l->challenge = client->decryptbuf[0x52];
	l->episode = client->decryptbuf[0x53];
	l->oneperson = client->decryptbuf[0x54];
	l->start_time = (unsigned) servertime;
	if (l->difficulty > 0x03)
		client->todc = 1;
	else
		if ((l->battle) && (l->challenge))
			client->todc = 1;
		else
			if (l->episode > 0x03)
				client->todc = 1;
			else
				if ((l->oneperson) && ((l->challenge) || (l->battle)))
					client->todc = 1;
	if (!client->todc)
	{
		if (l->battle)
			l->battle = 1;
		if (l->challenge)
			l->challenge = 1;
		if (l->oneperson)
			l->oneperson = 1;
		memcpy (&l->gameName[0], &client->decryptbuf[0x14], 30);
		memcpy (&l->gamePassword[0], &client->decryptbuf[0x30], 32);
		l->in_use = 1;
		l->gameMonster[0] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[1] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[2] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[3] = (unsigned char) mt_lrand() % 16;
		memset (&l->gameMap[0], 0, 128);
		l->playerItemID[0] = 0x10000;
		l->playerItemID[1] = 0x210000;
		l->playerItemID[2] = 0x410000;
		l->playerItemID[3] = 0x610000;
		l->bankItemID[0] = 0x10000;
		l->bankItemID[1] = 0x210000;
		l->bankItemID[2] = 0x410000;
		l->bankItemID[3] = 0x610000;
		l->leader = 0;
		l->sectionID = client->character.sectionID;
		l->itemID = 0x810000;
		l->mapIndex = 0;
		memset (&l->mapData[0], 0, sizeof (l->mapData));
		l->rareIndex = 0;
		for (ch=0;ch<0x20;ch++)
			l->rareData[ch] = 0xFF;
		switch (l->episode)
		{
		case 0x01:
			// Episode 1
			if (!l->oneperson)
			{
				l->bptable = &ep1battle[0x60 * l->difficulty];

				load_map_data ( l, 0, "map\\map_city00_00e.dat" );
				load_object_data ( l, 0, "map\\map_city00_00o.dat" );

				l->gameMap[12]=(unsigned char) mt_lrand() % 5; // Forest 1
				load_map_data ( l, 0, Forest1_Online_Maps [l->gameMap[12]] );
				load_object_data ( l, 0, Forest1_Online_Maps [l->gameMap[12]] );

				l->gameMap[20]=(unsigned char) mt_lrand() % 5; // Forest 2
				load_map_data ( l, 0, Forest2_Online_Maps [l->gameMap[20]] );
				load_object_data ( l, 0, Forest2_Online_Maps [l->gameMap[20]] );

				l->gameMap[24]=(unsigned char) mt_lrand() % 3; // Cave 1
				l->gameMap[28]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Cave1_Online_Maps [( l->gameMap[24] * 2 ) + l->gameMap[28]] );
				load_object_data ( l, 0, Cave1_Online_Maps [( l->gameMap[24] * 2 ) + l->gameMap[28]] );

				l->gameMap[32]=(unsigned char) mt_lrand() % 3; // Cave 2
				l->gameMap[36]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Cave2_Online_Maps [( l->gameMap[32] * 2 ) + l->gameMap[36]] );
				load_object_data ( l, 0, Cave2_Online_Maps [( l->gameMap[32] * 2 ) + l->gameMap[36]] );

				l->gameMap[40]=(unsigned char) mt_lrand() % 3; // Cave 3
				l->gameMap[44]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Cave3_Online_Maps [( l->gameMap[40] * 2 ) + l->gameMap[44]] );
				load_object_data ( l, 0, Cave3_Online_Maps [( l->gameMap[40] * 2 ) + l->gameMap[44]] );

				l->gameMap[48]=(unsigned char) mt_lrand() % 3; // Mine 1
				l->gameMap[52]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );
				load_object_data ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );

				l->gameMap[56]=(unsigned char) mt_lrand() % 3; // Mine 2
				l->gameMap[60]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );
				load_object_data ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );

				l->gameMap[64]=(unsigned char) mt_lrand() % 3; // Ruins 1
				l->gameMap[68]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );
				load_object_data ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72]=(unsigned char) mt_lrand() % 3; // Ruins 2
				l->gameMap[76]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );
				load_object_data ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );

				l->gameMap[80]=(unsigned char) mt_lrand() % 3; // Ruins 3
				l->gameMap[84]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
				load_object_data ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
			}
			else
			{
				l->bptable = &ep1battle_off[0x60 * l->difficulty];

				load_map_data ( l, 0, "map\\map_city00_00e_s.dat");
				load_object_data ( l, 0, "map\\map_city00_00o_s.dat");

				l->gameMap[12]=(unsigned char) mt_lrand() % 3; // Forest 1
				load_map_data ( l, 0, Forest1_Offline_Maps [l->gameMap[12]] );
				load_object_data ( l, 0, Forest1_Offline_Objects [l->gameMap[12]] );

				l->gameMap[20]=(unsigned char) mt_lrand() % 3; // Forest 2
				load_map_data ( l, 0, Forest2_Offline_Maps [l->gameMap[20]] );
				load_object_data ( l, 0, Forest2_Offline_Objects [l->gameMap[20]] );

				l->gameMap[24]=(unsigned char) mt_lrand() % 3; // Cave 1
				load_map_data ( l, 0, Cave1_Offline_Maps [l->gameMap[24]]);
				load_object_data ( l, 0, Cave1_Offline_Objects [l->gameMap[24]]);

				l->gameMap[32]=(unsigned char) mt_lrand() % 3; // Cave 2
				load_map_data ( l, 0, Cave2_Offline_Maps [l->gameMap[32]]);
				load_object_data ( l, 0, Cave2_Offline_Objects [l->gameMap[32]]);

				l->gameMap[40]=(unsigned char) mt_lrand() % 3; // Cave 3
				load_map_data ( l, 0, Cave3_Offline_Maps [l->gameMap[40]]);
				load_object_data ( l, 0, Cave3_Offline_Objects [l->gameMap[40]]);

				l->gameMap[48]=(unsigned char) mt_lrand() % 3; // Mine 1
				l->gameMap[52]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );
				load_object_data ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );

				l->gameMap[56]=(unsigned char) mt_lrand() % 3; // Mine 2
				l->gameMap[60]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );
				load_object_data ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );

				l->gameMap[64]=(unsigned char) mt_lrand() % 3; // Ruins 1
				l->gameMap[68]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );
				load_object_data ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72]=(unsigned char) mt_lrand() % 3; // Ruins 2
				l->gameMap[76]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );
				load_object_data ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );

				l->gameMap[80]=(unsigned char) mt_lrand() % 3; // Ruins 3
				l->gameMap[84]=(unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
				load_object_data ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
			}
			load_map_data ( l, 0, "map\\map_boss01e.dat" );
			load_object_data ( l, 0, "map\\map_boss01o.dat");
			load_map_data ( l, 0, "map\\map_boss02e.dat" );
			load_object_data ( l, 0, "map\\map_boss02o.dat");
			load_map_data ( l, 0, "map\\map_boss03e.dat" );
			load_object_data ( l, 0, "map\\map_boss03o.dat");
			load_map_data ( l, 0, "map\\map_boss04e.dat" );
			if ( l->oneperson )
				load_object_data ( l, 0, "map\\map_boss04_offo.dat");
			else
				load_object_data ( l, 0, "map\\map_boss04o.dat");
			break;
		case 0x02:
			// Episode 2
			if (!l->oneperson)
			{
				l->bptable = &ep2battle[0x60 * l->difficulty];
				load_map_data ( l, 0, "map\\map_labo00_00e.dat");
				load_object_data ( l, 0, "map\\map_labo00_00o.dat");
				
				l->gameMap[8]  = (unsigned char) mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				load_map_data ( l, 0, Temple1_Online_Maps [l->gameMap[8]] );
				load_object_data ( l, 0, Temple1_Online_Maps [l->gameMap[8]] );

				l->gameMap[16] = (unsigned char) mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				load_map_data ( l, 0, Temple2_Online_Maps [l->gameMap[16]] );
				load_object_data ( l, 0, Temple2_Online_Maps [l->gameMap[16]] );

				l->gameMap[24] = (unsigned char) mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				load_map_data ( l, 0, Spaceship1_Online_Maps [l->gameMap[24]] );
				load_object_data ( l, 0, Spaceship1_Online_Maps [l->gameMap[24]] );

				l->gameMap[32] = (unsigned char) mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00; 
				load_map_data ( l, 0, Spaceship2_Online_Maps [l->gameMap[32]] );
				load_object_data ( l, 0, Spaceship2_Online_Maps [l->gameMap[32]] );

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Jungle 1
				load_map_data ( l, 0, Jungle1_Online_Maps [l->gameMap[44]] );
				load_object_data ( l, 0, Jungle1_Online_Maps [l->gameMap[44]] );

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char) mt_lrand() % 3; // Jungle 2
				load_map_data ( l, 0, Jungle2_Online_Maps [l->gameMap[52]] );
				load_object_data ( l, 0, Jungle2_Online_Maps [l->gameMap[52]] );

				l->gameMap[56] = 0x00; 
				l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Jungle 3
				load_map_data ( l, 0, Jungle3_Online_Maps [l->gameMap[60]] );
				load_object_data ( l, 0, Jungle3_Online_Maps [l->gameMap[60]] );

				l->gameMap[64] = (unsigned char) mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Jungle4_Online_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );
				load_object_data ( l, 0, Jungle4_Online_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char) mt_lrand() % 3; // Jungle 5
				load_map_data ( l, 0, Jungle5_Online_Maps [l->gameMap[76]] );
				load_object_data ( l, 0, Jungle5_Online_Maps [l->gameMap[76]] );

				l->gameMap[80] = (unsigned char) mt_lrand() % 2; // Seabed 1
				l->gameMap[84] = (unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Seabed1_Online_Maps [(l->gameMap[80] * 2 ) + l->gameMap[84]] );
				load_object_data ( l, 0, Seabed1_Online_Maps [(l->gameMap[80] * 2 ) + l->gameMap[84]] );

				l->gameMap[88] = (unsigned char) mt_lrand() % 2; // Seabed 2
				l->gameMap[92] = (unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Seabed2_Online_Maps [(l->gameMap[88] * 2 ) + l->gameMap[92]] );
				load_object_data ( l, 0, Seabed2_Online_Maps [(l->gameMap[88] * 2 ) + l->gameMap[92]] );
			}
			else
			{
				l->bptable = &ep2battle_off[0x60 * l->difficulty];
				load_map_data ( l, 0, "map\\map_labo00_00e_s.dat");
				load_object_data ( l, 0, "map\\map_labo00_00o_s.dat");
				
				l->gameMap[8]  = (unsigned char) mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				load_map_data ( l, 0, Temple1_Offline_Maps [l->gameMap[8]] );
				load_object_data ( l, 0, Temple1_Offline_Maps [l->gameMap[8]] );

				l->gameMap[16] = (unsigned char) mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				load_map_data ( l, 0, Temple2_Offline_Maps [l->gameMap[16]] );
				load_object_data ( l, 0, Temple2_Offline_Maps [l->gameMap[16]] );

				l->gameMap[24] = (unsigned char) mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				load_map_data ( l, 0, Spaceship1_Offline_Maps [l->gameMap[24]] );
				load_object_data ( l, 0, Spaceship1_Offline_Maps [l->gameMap[24]] );

				l->gameMap[32] = (unsigned char) mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00; 
				load_map_data ( l, 0, Spaceship2_Offline_Maps [l->gameMap[32]] );
				load_object_data ( l, 0, Spaceship2_Offline_Maps [l->gameMap[32]] );

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Jungle 1
				load_map_data ( l, 0, Jungle1_Offline_Maps [l->gameMap[44]] );
				load_object_data ( l, 0, Jungle1_Offline_Maps [l->gameMap[44]] );

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char) mt_lrand() % 3; // Jungle 2
				load_map_data ( l, 0, Jungle2_Offline_Maps [l->gameMap[52]] );
				load_object_data ( l, 0, Jungle2_Offline_Maps [l->gameMap[52]] );

				l->gameMap[56] = 0x00; 
				l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Jungle 3
				load_map_data ( l, 0, Jungle3_Offline_Maps [l->gameMap[60]] );
				load_object_data ( l, 0, Jungle3_Offline_Maps [l->gameMap[60]] );

				l->gameMap[64] = (unsigned char) mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char) mt_lrand() % 2;
				load_map_data ( l, 0, Jungle4_Offline_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );
				load_object_data ( l, 0, Jungle4_Offline_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char) mt_lrand() % 3; // Jungle 5
				load_map_data ( l, 0, Jungle5_Offline_Maps [l->gameMap[76]] );
				load_object_data ( l, 0, Jungle5_Offline_Maps [l->gameMap[76]] );

				l->gameMap[80] = (unsigned char) mt_lrand() % 2; // Seabed 1
				load_map_data ( l, 0, Seabed1_Offline_Maps [l->gameMap[80]] );
				load_object_data ( l, 0, Seabed1_Offline_Maps [l->gameMap[80]] );

				l->gameMap[88] = (unsigned char) mt_lrand() % 2; // Seabed 2
				load_map_data ( l, 0, Seabed2_Offline_Maps [l->gameMap[88]] );
				load_object_data ( l, 0, Seabed2_Offline_Maps [l->gameMap[88]] );
			}
			load_map_data ( l, 0, "map\\map_boss05e.dat");
			load_map_data ( l, 0, "map\\map_boss06e.dat");
			load_map_data ( l, 0, "map\\map_boss07e.dat");
			load_map_data ( l, 0, "map\\map_boss08e.dat");
			if ( l->oneperson )
			{
				load_object_data ( l, 0, "map\\map_boss05_offo.dat");
				load_object_data ( l, 0, "map\\map_boss06_offo.dat");
				load_object_data ( l, 0, "map\\map_boss07_offo.dat");
				load_object_data ( l, 0, "map\\map_boss08_offo.dat");
			}
			else
			{
				load_object_data ( l, 0, "map\\map_boss05o.dat");
				load_object_data ( l, 0, "map\\map_boss06o.dat");
				load_object_data ( l, 0, "map\\map_boss07o.dat");
				load_object_data ( l, 0, "map\\map_boss08o.dat");
			}
			break;
		case 0x03:
			// Episode 4
			if (!l->oneperson)
			{
				l->bptable = &ep4battle[0x60 * l->difficulty];
				load_map_data (l, 0, "map\\map_city02_00_00e.dat");
				load_object_data (l, 0, "map\\map_city02_00_00o.dat");
			}
			else
			{
				l->bptable = &ep4battle_off[0x60 * l->difficulty];
				load_map_data (l, 0, "map\\map_city02_00_00e_s.dat");
				load_object_data (l, 0, "map\\map_city02_00_00o_s.dat");
			}

			l->gameMap[12] = (unsigned char) mt_lrand() % 3; // Crater East
			load_map_data ( l, 0, Crater_East_Online_Maps [l->gameMap[12]] );
			load_object_data ( l, 0, Crater_East_Online_Maps [l->gameMap[12]] );

			l->gameMap[20] = (unsigned char) mt_lrand() % 3; // Crater West
			load_map_data ( l, 0, Crater_West_Online_Maps [l->gameMap[20]] );
			load_object_data ( l, 0, Crater_West_Online_Maps [l->gameMap[20]] );

			l->gameMap[28] = (unsigned char) mt_lrand() % 3; // Crater South
			load_map_data ( l, 0, Crater_South_Online_Maps [l->gameMap[28]] );
			load_object_data ( l, 0, Crater_South_Online_Maps [l->gameMap[28]] );

			l->gameMap[36] = (unsigned char) mt_lrand() % 3; // Crater North
			load_map_data ( l, 0, Crater_North_Online_Maps [l->gameMap[36]] );
			load_object_data ( l, 0, Crater_North_Online_Maps [l->gameMap[36]] );

			l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Crater Interior
			load_map_data ( l, 0, Crater_Interior_Online_Maps [l->gameMap[44]] );
			load_object_data ( l, 0, Crater_Interior_Online_Maps [l->gameMap[44]] );

			l->gameMap[48] = (unsigned char) mt_lrand() % 3; // Desert 1
			load_map_data ( l, 1, Desert1_Online_Maps [l->gameMap[48]] );
			load_object_data ( l, 1, Desert1_Online_Maps [l->gameMap[48]] );

			l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Desert 2
			load_map_data ( l, 1, Desert2_Online_Maps [l->gameMap[60]] );
			load_object_data ( l, 1, Desert2_Online_Maps [l->gameMap[60]] );

			l->gameMap[64] = (unsigned char) mt_lrand() % 3; // Desert 3
			load_map_data ( l, 1, Desert3_Online_Maps [l->gameMap[64]] );
			load_object_data ( l, 1, Desert3_Online_Maps [l->gameMap[64]] );

			load_map_data (l, 0, "map\\map_boss09_00_00e.dat");
			load_object_data (l, 0, "map\\map_boss09_00_00o.dat");
			//load_map_data (l, "map\\map_test01_00_00e.dat");
			break;
		default:
			break;
		}
	}
	else
		Send1A ("Bad game arguments supplied.", client);
}


void BroadcastToAll (unsigned short *mes, PSO_CLIENT* client)
{
	unsigned short xEE_Len;
	unsigned short *pd;
	unsigned short *cname;
	unsigned ch, connectNum;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	pd = (unsigned short*) &PacketData[xEE_Len];
	cname = (unsigned short*) &client->character.name[4];

	*(pd++) = 91;
	*(pd++) = 71;
	*(pd++) = 77;
	*(pd++) = 93;

	xEE_Len += 8;

	while (*cname != 0x0000)
	{
		*(pd++) = *(cname++);
		xEE_Len += 2;
	}

	*(pd++) = 58;
	*(pd++) = 32;

	xEE_Len += 4;

	while (*mes != 0x0000)
	{
		if (*mes == 0x0024)
		{
			*(pd++) = 0x0009;
			mes++;
		}
		else
		{
			*(pd++) = *(mes++);
		}
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	if (client->announce == 1)
	{
		// Local announcement
		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard)
			{
				cipher_ptr = &connections[connectNum]->server_cipher;
				encryptcopy (connections[connectNum], &PacketData[0], xEE_Len);
			}	
		}
	}
	else
	{
		// Global announcement
		if ((logon) && (logon->sockfd >= 0))
		{
			// Send the announcement to the logon server.
			logon->encryptbuf[0x00] = 0x12;
			logon->encryptbuf[0x01] = 0x00;
			*(unsigned *) &logon->encryptbuf[0x02] = client->guildcard;
			memcpy (&logon->encryptbuf[0x06], &PacketData[sizeof(PacketEE)], xEE_Len - sizeof (PacketEE));
			compressShipPacket (logon, &logon->encryptbuf[0x00], 6 + xEE_Len - sizeof (PacketEE));
		}
	}
	client->announce = 0;
}

/*
	Not sure yet what the difference is between this and
	"BroadcastToAll" is.
*/
void GlobalBroadcast (unsigned short *mes)
{
	unsigned short xEE_Len;
	unsigned short *pd;
	unsigned ch, connectNum;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	pd = (unsigned short*) &PacketData[xEE_Len];

	while (*mes != 0x0000)
	{
		*(pd++) = *(mes++);
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	for (ch=0;ch<serverNumConnections;ch++)
	{
		connectNum = serverConnectionList[ch];
		if (connections[connectNum]->guildcard)
		{
			cipher_ptr = &connections[connectNum]->server_cipher;
			encryptcopy (connections[connectNum], &PacketData[0], xEE_Len);
		}	
	}
}

// FFFF

INVENTORY_ITEM sort_data[30];
BANK_ITEM bank_data[200];

const char lobbyString[] = { "L\0o\0b\0b\0y\0 \0" };

void AddPB ( unsigned char* flags, unsigned char* blasts, unsigned char pb )
{
	int pb_exists = 0;
	unsigned char pbv;
	unsigned pb_slot;

	if ( ( *flags & 0x01 ) == 0x01 )
	{
		if ( ( *blasts & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags & 0x02 ) == 0x02 )
	{
		if ( ( ( *blasts / 8 ) & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags  & 0x04 ) == 0x04 )
		pb_exists = 1;

	if (!pb_exists)
	{
		if ( ( *flags & 0x01 ) == 0 )
			pb_slot = 0;
		else
		if ( ( *flags & 0x02 ) == 0 )
			pb_slot = 1;
		else
			pb_slot = 2;
		switch ( pb_slot )
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags  |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags  |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ( ( *blasts & 0x07 ) < pb )
				pbv--;
			if ( ( ( *blasts / 8 ) & 0x07 ) < pb )
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags  |= 0x04;
		}
		*blasts |= pb;
	}
}

void SkipToLevel (unsigned short target_level, PSO_CLIENT* client, int quiet)
{
	MAG* m;
	unsigned short ch, finalDFP, finalATP, finalATA, finalMST;

	if (target_level > 199)
		target_level = 199;

	if ((!client->lobby) || (client->character.level >= target_level))
		return;

	if (!quiet)
	{
		PacketBF[0x0A] = (unsigned char) client->clientID;
		*(unsigned *) &PacketBF[0x0C] = tnlxp [target_level - 1] - client->character.XP;
		SendToLobby (client->lobby, 4, &PacketBF[0], 0x10, 0);
	}

	while (client->character.level < target_level)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP  += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
	}
	
	client->character.XP = tnlxp [target_level - 1];

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

	if (!quiet)
	{
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

int ban ( unsigned gc_num, unsigned* ipaddr, long long* hwinfo, unsigned type, PSO_CLIENT* client )
{
	int banned = 1;
	unsigned ch, ch2;
	FILE* fp;

	for (ch=0;ch<num_bans;ch++)
	{
		if ( ( ship_bandata[ch].guildcard == gc_num ) && ( ship_bandata[ch].type == type ) )
		{
			banned = 0;
			ship_bandata[ch].guildcard = 0;
			ship_bandata[ch].type = 0;
			break;
		}
	}

	if (banned)
	{
		if (num_bans < 5000)
		{
			ship_bandata[num_bans].guildcard = gc_num;
			ship_bandata[num_bans].type = type;
			ship_bandata[num_bans].ipaddr = *ipaddr;
			ship_bandata[num_bans++].hwinfo = *hwinfo;
			fp = fopen ("bandata.dat", "wb");
			if (fp)
			{
				fwrite (&ship_bandata[0], 1, sizeof (BANDATA) * num_bans, fp);
				fclose (fp);
			}
			else
				write_log ("Could not open bandata.dat for writing!!");
		}
		else
		{
			banned = 0; // Can't ban with a full list...
			SendB0 ("Ship ban list is full.", client);
		}
	}
	else
	{
		ch2 = 0;
		for (ch=0;ch<num_bans;ch++)
		{
			if ((ship_bandata[ch].type != 0) && (ch != ch2))
				memcpy (&ship_bandata[ch2++], &ship_bandata[ch], sizeof (BANDATA));
		}
		num_bans = ch2;
		fp = fopen ("bandata.dat", "wb");
		if (fp)
		{
			fwrite (&ship_bandata[0], 1, sizeof (BANDATA) * num_bans, fp);
			fclose (fp);
		}
		else
			write_log ("Could not open bandata.dat for writing!!");
	}
	return banned;
}

int stfu ( unsigned gc_num )
{
	int result = 0;
	unsigned ch;

	for (ch=0;ch<ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			result = 1;
			break;
		}
	}

	return result;
}

int toggle_stfu ( unsigned gc_num, PSO_CLIENT* client )
{
	int ignored = 1;
	unsigned ch, ch2;

	for (ch=0;ch<ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			ignored = 0;
			ship_ignore_list[ch] = 0;
			break;
		}
	}

	if (ignored)
	{
		if (ship_ignore_count < 300)
			ship_ignore_list[ship_ignore_count++] = gc_num;
		else
		{
			ignored = 0; // Can't ignore with a full list...
			SendB0 ("Ship ignore list is full.", client);
		}
	}
	else
	{
		ch2 = 0;
		for (ch=0;ch<ship_ignore_count;ch++)
		{
			if ((ship_ignore_list[ch] != 0) && (ch != ch2))
				ship_ignore_list[ch2++] = ship_ignore_list[ch];
		}
		ship_ignore_count = ch2;
	}
	return ignored;
}


unsigned char cmdBuf[4000];
char* myCommand;
char* myArgs[64];

char character_file[255];

void AddGuildCard (unsigned myGC, unsigned friendGC, unsigned char* friendName, 
				   unsigned char* friendText, unsigned char friendSecID, unsigned char friendClass,
				   PSO_SERVER* ship)
{
	// Instruct the logon server to add the guild card

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;
	memcpy (&ship->encryptbuf[0x0A], friendName, 24);
	memcpy (&ship->encryptbuf[0x22], friendText, 176);
	ship->encryptbuf[0xD2] = friendSecID;
	ship->encryptbuf[0xD3] = friendClass;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0xD4 );
}

void DeleteGuildCard (unsigned myGC, unsigned friendGC, PSO_SERVER* ship)
{
	// Instruct the logon server to delete the guild card

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void ModifyGuildCardComment (unsigned myGC, unsigned friendGC, unsigned short* n, PSO_SERVER* ship)
{
	unsigned s = 1;
	unsigned short* g;

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;

	// Client writing to info board

	g = (unsigned short*) &ship->encryptbuf[0x0A];

	memset (g, 0, 0x44);

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 33))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	*(g++) = 0x0000;

	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x4E );
}

void SortGuildCard (PSO_CLIENT* client, PSO_SERVER* ship)
{
	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*) &ship->encryptbuf[0x02] = client->guildcard;
	*(unsigned*) &ship->encryptbuf[0x06] = *(unsigned*) &client->decryptbuf[0x08];
	*(unsigned*) &ship->encryptbuf[0x0A] = *(unsigned*) &client->decryptbuf[0x0C];
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x10 );
}

void ShowArrows (PSO_CLIENT* client, int to_all)
{
	LOBBY *l;
	unsigned ch, total_clients, Packet88Offset;

	total_clients = 0;
	memset (&PacketData[0x00], 0, 8);
	PacketData[0x02] = 0x88;
	PacketData[0x03] = 0x00;
	Packet88Offset = 8;

	if (!client->lobby)
		return;
	l = (LOBBY*) client->lobby;

	for (ch=0;ch<12;ch++)
	{
		if ((l->slot_use[ch] != 0) && (l->client[ch]))
		{
			total_clients++;
			PacketData[Packet88Offset+2] = 0x01;
			*(unsigned*) &PacketData[Packet88Offset+4] = l->client[ch]->character.guildCard;
			PacketData[Packet88Offset+8] = l->arrow_color[ch];
			Packet88Offset += 12;
		}
	}
	*(unsigned*) &PacketData[0x04] = total_clients;
	*(unsigned short*) &PacketData[0x00] = (unsigned short) Packet88Offset;
	if (to_all)
		SendToLobby (client->lobby, 12, &PacketData[0x00], Packet88Offset, 0);
	else
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketData[0x00], Packet88Offset);
	}
}

void BlockProcessPacket (PSO_CLIENT* client)
{
	if (client->guildcard)
	{
		switch (client->decryptbuf[0x02])
		{
		case 0x05:
			break;
		case 0x06:
			if ( ((unsigned) servertime - client->command_cooldown[0x06]) >= 1 )
			{
				client->command_cooldown[0x06] = (unsigned) servertime;
				Send06 (client);
			}
			break;
		case 0x08:
			// Get game list
			if ( ( client->lobbyNum < 0x10 ) && ( ((unsigned) servertime - client->command_cooldown[0x08]) >= 1 ) )
			{
				client->command_cooldown[0x08] = (unsigned) servertime;
				Send08 (client);
			}
			else
				if ( client->lobbyNum < 0x10 )
					Send01 ("You must wait\nawhile before\ntrying that.", client);
			break;
		case 0x09:
			Command09 (client);
			break;
		case 0x10:
			Command10 (1, client);
			break;
		case 0x1D:
			client->response = (unsigned) servertime;
			break;
		case 0x40:
			// Guild Card search
			if ( ((unsigned) servertime - client->command_cooldown[0x40]) >= 1 )
			{
				client->command_cooldown[0x40] = (unsigned) servertime;
				Command40 (client, logon);
			}
			break;
		case 0x13:
		case 0x44:
			if ( ( client->lobbyNum > 0x0F ) && ( client->sending_quest != -1 ) )
			{
				unsigned short qps;

				if ((client->character.lang < 10) && (quests[client->sending_quest].ql[client->character.lang]))
				{
					if (client->qpos < quests[client->sending_quest].ql[client->character.lang]->qsize)
					{
						qps = *(unsigned short*) &quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos];
						if ( qps % 8 )
							qps += ( 8 - ( qps % 8 ) );
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
				else
				{
					if (client->qpos < quests[client->sending_quest].ql[0]->qsize)
					{
						qps = *(unsigned short*) &quests[client->sending_quest].ql[0]->qdata[client->qpos];
						if ( qps % 8 )
							qps += ( 8 - ( qps % 8 ) );
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &quests[client->sending_quest].ql[0]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
			}
			break;
		case 0x60:
			if ((client->bursting) && (client->decryptbuf[0x08] == 0x23) && (client->lobbyNum < 0x10) && (client->lobbyNum))
			{
				// If a client has just appeared, send him the team information of everyone in the lobby
				if (client->guildcard)
				{
					unsigned ch;
					LOBBY* l;

					if (!client->lobby)
					break;

					l = client->lobby;

					for (ch=0;ch<12;ch++)
						if ( ( l->slot_use[ch] != 0 ) && ( l->client[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->client[ch]), 2152 );
						}
						ShowArrows (client, 0);
						client->bursting = 0;
				}
			}
			// Lots of fun commands here.
			Send60 (client);
			break;
		case 0x61:
			memcpy (&client->character.unknown[0], &client->decryptbuf[0x362], 10);
			Send67 (client, client->preferred_lobby);
			break;
		case 0x62:
			Send62 (client);
			break;
		case 0x6D:
			if (client->lobbyNum > 0x0F)
				Send6D (client);
			else
				initialize_connection (client);
			break;
		case 0x6F:
			if ((client->lobbyNum > 0x0F) && (client->bursting))
			{
				LOBBY* l;
				unsigned short fqs,ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				if ((l->inpquest) && (!client->hasquest))
				{
					// Send the quest
					client->bursting = 1;
					if ((client->character.lang < 10) && (quests[l->quest_loaded - 1].ql[client->character.lang]))
					{
						fqs =  *(unsigned short*) &quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0];
						if (fqs % 8)
							fqs += ( 8 - ( fqs % 8 ) );
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0], fqs);
					}
					else
					{
						fqs =  *(unsigned short*) &quests[l->quest_loaded - 1].ql[0]->qdata[0];
						if (fqs % 8)
							fqs += ( 8 - ( fqs % 8 ) );
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &quests[l->quest_loaded - 1].ql[0]->qdata[0], fqs);
					}
				}
				else
				{
					// Rare monster data go...
					memset (&client->encryptbuf[0x00], 0, 0x08);
					client->encryptbuf[0x00] = 0x28;
					client->encryptbuf[0x02] = 0xDE;
					memcpy (&client->encryptbuf[0x08], &l->rareData[0], 0x20);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x28);
					memset (&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x72;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = 0x18;
					client->encryptbuf[0x0B] = 0x08;
					SendToLobby (client->lobby, 4, &client->encryptbuf[0x00], 0x0C, 0);
					for (ch=0;ch<4;ch++)
						if ( ( l->slot_use[ch] != 0 ) && ( l->client[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->client[ch]), 2152 );
						}
					client->bursting = 0;
				}
			}
			break;
		case 0x81:
			if (client->announce)
			{
				if (client->announce == 1)
					write_gm ("GM %u made an announcement: %s", client->guildcard, unicode_to_ascii ( &client->decryptbuf[0x60]));
				BroadcastToAll ((unsigned short*) &client->decryptbuf[0x60], client);
			}
			else
			{
				if ( ((unsigned) servertime - client->command_cooldown[0x81]) >= 1 )
				{
					client->command_cooldown[0x81] = (unsigned) servertime;
					Command81 (client, logon);
				}
			}
			break;
		case 0x84:
			if (client->decryptbuf[0x0C] < 0x0F)
			{
				BLOCK* b;
				b = blocks[client->block - 1];
				if (client->lobbyNum > 0x0F)
				{
					removeClientFromLobby (client);
					client->preferred_lobby = client->decryptbuf[0x0C];
					Send95 (client);
				}
				else
				{
					if ( ((unsigned) servertime - client->command_cooldown[0x84]) >= 1 )
					{
						if (b->lobbies[client->decryptbuf[0x0C]].lobbyCount < 12)
						{
							client->command_cooldown[0x84] = (unsigned) servertime;
							removeClientFromLobby (client);
							client->preferred_lobby = client->decryptbuf[0x0C];
							Send95 (client);
						}
						else
							Send01 ("Lobby is full!", client);
					}
					else 
						Send01 ("You must wait\nawhile before\ntrying that.", client);
				}
			}
			break;
		case 0x89:
			if ((client->lobbyNum < 0x10) && (client->lobbyNum) && ( ((unsigned) servertime - client->command_cooldown[0x89]) >= 1 ) )
			{
				LOBBY* l;

				client->command_cooldown[0x89] = (unsigned) servertime;
				if (!client->lobby)
					break;
				l = client->lobby;
				l->arrow_color[client->clientID] = client->decryptbuf[0x04];
				ShowArrows(client, 1);
			}
			break;
		case 0x8A:
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;
				if (l->in_use)
				{
					memset (&PacketData[0], 0, 0x28);
					PacketData[0x00] = 0x28;
					PacketData[0x02] = 0x8A;
					memcpy (&PacketData[0x08], &l->gameName[0], 30);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &PacketData[0], 0x28);
				}
			}
			break;
		case 0xA0:
			// Ship list
			if (client->lobbyNum < 0x10)
				ShipSend0D (0x00, client, logon);
			break;
		case 0xA1:
			// Block list
			if (client->lobbyNum < 0x10)
				Send07 (client);
			break;
		case 0xA2:
			// Quest list
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;

				if ( l->floor[client->clientID] == 0 )
				{
					if ( client->decryptbuf[0x04] )
						SendA2 ( l->episode, l->oneperson, 0, 1, client );
					else
						SendA2 ( l->episode, l->oneperson, 0, 0, client );
				}
			}
			break;
		case 0xAC:
			// Quest load complete
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;
				int all_quest;
				unsigned ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				client->hasquest = 1;
				all_quest = 1;

				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->hasquest == 0))
						all_quest = 0;
				}

				if (all_quest)
				{ 
					// Send the 0xAC when all clients have the quest

					client->decryptbuf[0x00] = 0x08;
					client->decryptbuf[0x01] = 0x00;
					SendToLobby (l, 4, &client->decryptbuf[0x00], 8, 0);
				}

				client->sending_quest = -1;

				if ((l->inpquest) && (client->bursting))
				{
					// Let the leader know it's time to send the remaining state of the quest...
					cipher_ptr = &l->client[l->leader]->server_cipher;
					memset (&client->encryptbuf[0], 0, 8);
					client->encryptbuf[0] = 0x08;
					client->encryptbuf[2] = 0xDD;
					client->encryptbuf[4] = client->clientID;
					encryptcopy (l->client[l->leader], &client->encryptbuf[0], 8);
				}
			}
			break;
		case 0xC1:
			// Create game
			if (client->lobbyNum < 0x10)
			{
				if (client->decryptbuf[0x52])
					Send1A ("Challenge games are NOT supported right now.\nCheck back later.\n\n- Sodaboy", client);
				else
							{
								unsigned lNum, failed_to_create;

								failed_to_create = 0;

								if ( (!client->isgm) && (!isLocalGM(client->guildcard)))
								{
									switch (client->decryptbuf[0x53])
									{
									case 0x01:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 19))
										{
											Send01 ("Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 49))
											{
												Send01 ("Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 89))
												{
													Send01 ("Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									case 0x02:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 29))
										{
											Send01 ("Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 59))
											{
												Send01 ("Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 99))
												{
													Send01 ("Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									case 0x03:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 39))
										{
											Send01 ("Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 69))
											{
												Send01 ("Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 109))
												{
													Send01 ("Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									default:
										SendB0 ("Lol, nub.", client);
										break;
									}
								}

								if (!failed_to_create)
								{
									lNum = free_game (client);
									if (lNum)
									{
										removeClientFromLobby (client);
										client->lobbyNum = (unsigned short) lNum + 1;
										client->lobby = &blocks[client->block - 1]->lobbies[lNum];
										initialize_game (client);
										Send64 (client);
										memset (&client->encryptbuf[0x00], 0, 0x0C);
										client->encryptbuf[0x00] = 0x0C;
										client->encryptbuf[0x02] = 0x60;
										client->encryptbuf[0x08] = 0xDD;
										client->encryptbuf[0x09] = 0x03;
										client->encryptbuf[0x0A] = (unsigned char) EXPERIENCE_RATE;
										cipher_ptr = &client->server_cipher;
										encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
										UpdateGameItem (client);
									}
									else
										Send01 ("Sorry, limit of game\ncreation has been\nreached.\n\nPlease join a game\nor change ships.", client);
								}
							}
			}
			break;
		case 0xD8:
			// Show info board
			if ( ((unsigned) servertime - client->command_cooldown[0xD8]) >= 1 )
			{
				client->command_cooldown[0xD8] = (unsigned) servertime;
				CommandD8 (client);
			}
			break;
		case 0xD9:
			// Write on info board
			CommandD9 (client);
			break;
		case 0xE7:
			// Client sending character data...
			if ( client->guildcard )
			{
				if ( (client->isgm) || (isLocalGM(client->guildcard)) )
					write_gm ("GM %u (%s) has disconnected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4]) );
				else
					write_log ("User %u (%s) has disconnected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4]) );
				client->todc = 1;
			}
			break;
		case 0xE8:
			// Guild card stuff
			CommandE8 (client);
			break;
		case 0xEA:
			// Team shit
			CommandEA (client, logon);
			break;
		case 0xED:
			// Set options
			CommandED (client);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (client->decryptbuf[0x02])
		{
		case 0x05:
			printf ("Client has closed the connection.\n");
			client->todc = 1;
			break;
		case 0x93:
			{
				unsigned ch,ch2,ipaddr;
				int banned = 0, match;

				client->temp_guildcard = *(unsigned*) &client->decryptbuf[0x0C];
				client->hwinfo = *(long long*) &client->decryptbuf[0x84];
				ipaddr = *(unsigned*) &client->ipaddr[0];
				for (ch=0;ch<num_bans;ch++)
				{
					if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
					{
						banned = 1;
						break;
					}
					if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
					{
						banned = 1;
						break;
					}
					if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
					{
						banned = 1;
						break;
					}
				}

				for (ch=0;ch<num_masks;ch++)
				{
					match = 1;
					for (ch2=0;ch2<4;ch2++)
					{
						if ((ship_banmasks[ch][ch2] != 0x8000) &&
						    ((unsigned char) ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
							match = 0;
					}
					if ( match )
					{
						banned = 1;
						break;
					}
				}
				
				if (banned)
				{
					Send1A ("You are banned from this ship.", client);
					client->todc = 1;
				}
				else
					if (!client->sendCheck[RECEIVE_PACKET_93])
					{
						ShipSend0B ( client, logon );
						client->sendCheck[RECEIVE_PACKET_93] = 0x01;
					}
			}
			break;

		default:
			break;
		}
	}
}

void ShipProcessPacket (PSO_CLIENT* client)
{
	switch (client->decryptbuf[0x02])
	{
	case 0x05:
		printf ("Client has closed the connection.\n");
		client->todc = 1;
		break;
	case 0x10:
		Command10 (0, client);
		break;
	case 0x1D:
		client->response = (unsigned) servertime;
		break;
	case 0x93:
		{
			unsigned ch,ch2,ipaddr;
			int banned = 0, match;

			client->temp_guildcard = *(unsigned*) &client->decryptbuf[0x0C];
			client->hwinfo = *(long long*) &client->decryptbuf[0x84];
			ipaddr = *(unsigned*) &client->ipaddr[0];

			for (ch=0;ch<num_bans;ch++)
			{
				if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
				{
					banned = 1;
					break;
				}
			}

			for (ch=0;ch<num_masks;ch++)
			{
				match = 1;
				for (ch2=0;ch2<4;ch2++)
				{
					if ((ship_banmasks[ch][ch2] != 0x8000) &&
						((unsigned char) ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
						match = 0;
				}
				if ( match )
				{
					banned = 1;
					break;
				}
			}

			if (banned)
			{
				Send1A ("You are banned from this ship.", client);
				client->todc = 1;
			}
			else
				if (!client->sendCheck[RECEIVE_PACKET_93])
				{
					ShipSend0B ( client, logon );
					client->sendCheck[RECEIVE_PACKET_93] = 0x01;
				}
		}
		break;
	default:
		break;
	}
}

unsigned char qpd_buffer  [PRS_BUFFER];
unsigned char qpdc_buffer [PRS_BUFFER];
//LOBBY fakelobby;

unsigned csv_lines = 0;
char* csv_params[1024][64]; // 1024 lines which can carry 64 parameters each

// Release RAM from loaded CSV

void FreeCSV()
{
	unsigned ch, ch2;

	for (ch=0;ch<csv_lines;ch++)
	{
		for (ch2=0;ch2<64;ch2++)
			if (csv_params[ch][ch2] != NULL) free (csv_params[ch][ch2]);
	}
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));
}

// Load CSV into RAM

void LoadCSV(const char* filename)
{
	FILE* fp;
	char csv_data[1024];
	unsigned ch, ch2, ch3 = 0;
	//unsigned ch4;
	int open_quote = 0;
	char* csv_param;
	
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));

	//printf ("Loading CSV file %s ...\n", filename );

	if ( ( fp = fopen (filename, "r" ) ) == NULL )
	{
		printf ("The parameter file %s appears to be missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	while (fgets (&csv_data[0], 1023, fp) != NULL)
	{
		// ch2 = current parameter we're on
		// ch3 = current index into the parameter string
		ch2 = ch3 = 0;
		open_quote = 0;
		csv_param = csv_params[csv_lines][0] = malloc (256); // allocate memory for parameter
		for (ch=0;ch<strlen(&csv_data[0]);ch++)
		{
			if ((csv_data[ch] == 44) && (!open_quote)) // comma not surrounded by quotations
			{
				csv_param[ch3] = 0; // null terminate current parameter
				ch3 = 0;
				ch2++; // new parameter
				csv_param = csv_params[csv_lines][ch2] = malloc (256); // allocate memory for parameter
			}
			else
			{
				if (csv_data[ch] == 34) // quotation mark
					open_quote = !open_quote;
				else
					if (csv_data[ch] > 31) // no loading low ascii
						csv_param[ch3++] = csv_data[ch];
			}
		}
		if (ch3)
		{
			ch2++;
			csv_param[ch3] = 0;
		}
		/*
		for (ch4=0;ch4<ch2;ch4++)
			printf ("%s,", csv_params[csv_lines][ch4]);
		printf ("\n");
		*/
		csv_lines++;
		if (csv_lines > 1023)
		{
			printf ("CSV file has too many entries.\n");
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
	}
	printf ("Loaded %u lines...\r\n", csv_lines);
	fclose (fp);
}


LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if(message == MYWM_NOTIFYICON)
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			switch (wParam) 
			{
			case 100:
				if (program_hidden)
				{
					program_hidden = 0;
					reveal_window;
				}
				else
				{
					program_hidden = 1;
					ShowWindow (consoleHwnd, SW_HIDE);
				}
				return TRUE;
				break;
			}
			break;
		}
	}
	return DefWindowProc( hwnd, message, wParam, lParam );
}

/********************************************************
**
**		main  :-
**
********************************************************/

int main()
{
	unsigned ch,ch2,ch3,ch4,ch5,connectNum;
	int wep_rank;
	PTDATA ptd;
	unsigned wep_counters[24] = {0};
	unsigned tool_counters[28] = {0};
	unsigned tech_counters[19] = {0};
	struct in_addr ship_in;
	struct sockaddr_in listen_in;
	unsigned listen_length;
	int block_sockfd[10] = {-1};
	struct in_addr block_in[10];
	int ship_sockfd = -1;
	int pkt_len, pkt_c, bytes_sent;
	int wserror;
	WSADATA winsock_data;
	FILE* fp;
	unsigned char* connectionChunk;
	unsigned char* connectionPtr;
	unsigned char* blockPtr;
	unsigned char* blockChunk;
	//unsigned short this_packet;
	unsigned long logon_this_packet;
	HINSTANCE hinst;
    NOTIFYICONDATA nid = {0};
	WNDCLASS wc = {0};
	HWND hwndWindow;
	MSG msg;
		
	ch = 0;

	consoleHwnd = GetConsoleWindow();
	hinst = GetModuleHandle(NULL);

	dp[0] = 0;

	strcat (&dp[0], "Tethealla Ship Server version ");
	strcat (&dp[0], SERVER_VERSION );
	strcat (&dp[0], " coded by Sodaboy");
	SetConsoleTitle (&dp[0]);

	printf ("\nTethealla Ship Server version %s  Copyright (C) 2008  Terry Chatman Jr.\n", SERVER_VERSION);
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf ("This program comes with ABSOLUTELY NO WARRANTY; for details\n");
	printf ("see section 15 in gpl-3.0.txt\n");
    printf ("This is free software, and you are welcome to redistribute it\n");
    printf ("under certain conditions; see gpl-3.0.txt for details.\n");

	/*for (ch=0;ch<5;ch++)
	{
		printf (".");
		Sleep (1000);
	}*/
	printf ("\n\n");

	WSAStartup(MAKEWORD(1,1), &winsock_data);
	
	printf ("Loading configuration from ship.ini ... ");
#ifdef LOG_60
	debugfile = fopen ("60packets.txt", "a");
	if (!debugfile)
	{
		printf ("Could not create 60packets.txt");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit(1);
	}
#endif
	mt_bestseed();
	load_config_file();
	printf ("OK!\n\n");

	printf ("Loading language file...\n");

	load_language_file();

	printf ("OK!\n\n");

	printf ("Loading ship_key.bin... ");

	fp = fopen ("ship_key.bin", "rb" );
	if (!fp)
	{
		printf ("Could not locate ship_key.bin!\n");
		printf ("Hit [ENTER] to quit...");
		gets (&dp[0]);
		exit (1);
	}

	fread (&ship_index, 1, 4, fp );
	fread (&ship_key[0], 1, 128, fp );
	fclose (fp);

	printf ("OK!\n\nLoading weapon parameter file...\n");
	load_weapon_param();
	printf ("\n.. done!\n\n");

	printf ("Loading armor & barrier parameter file...\n");
	load_armor_param();
	printf ("\n.. done!\n\n");

	printf ("Loading technique parameter file...\n");
	load_tech_param();
	printf ("\n.. done!\n\n");

	for (ch=1;ch<200;ch++)
		tnlxp[ch] = tnlxp[ch-1] + tnlxp[ch];

	printf ("Loading battle parameter files...\n\n");
	load_battle_param (&ep1battle_off[0], "param\\BattleParamEntry.dat", 374, 0x8fef1ffe);
	load_battle_param (&ep1battle[0], "param\\BattleParamEntry_on.dat", 374, 0xb8a2d950);
	load_battle_param (&ep2battle_off[0], "param\\BattleParamEntry_lab.dat", 374, 0x3dc217f5);
	load_battle_param (&ep2battle[0], "param\\BattleParamEntry_lab_on.dat", 374, 0x4d4059cf);
	load_battle_param (&ep4battle_off[0], "param\\BattleParamEntry_ep4.dat", 332, 0x50841167);
	load_battle_param (&ep4battle[0], "param\\BattleParamEntry_ep4_on.dat", 332, 0x42bf9716);

	for (ch=0;ch<374;ch++)
		if (ep2battle_off[ch].HP)
		{
			ep2battle_off[ch].XP = ( ep2battle_off[ch].XP * 130 ) / 100; // 30% boost to EXP
			ep2battle[ch].XP     = ( ep2battle[ch].XP * 130 ) / 100;
		}

	printf ("\n.. done!\n\nBuilding common tables... \n\n");
	printf ("Weapon drop rate: %03f%%\n", (float) WEAPON_DROP_RATE / 1000);
	printf ("Armor drop rate: %03f%%\n", (float) ARMOR_DROP_RATE / 1000);
	printf ("Mag drop rate: %03f%%\n", (float) MAG_DROP_RATE / 1000);
	printf ("Tool drop rate: %03f%%\n", (float) TOOL_DROP_RATE / 1000);
	printf ("Meseta drop rate: %03f%%\n", (float) MESETA_DROP_RATE / 1000);
	printf ("Experience rate: %u%%\n\n", EXPERIENCE_RATE * 100);

	ch = 0;
	while (ch < 100000)
	{
		for (ch2=0;ch2<5;ch2++)
		{
			common_counters[ch2]++;
			if ((common_counters[ch2] >= common_rates[ch2]) && (ch<100000))
			{
				common_table[ch++] = (unsigned char) ch2;
				common_counters[ch2] = 0;
			}
		}
	}

	printf (".. done!\n\n");

	printf ("Loading param\\ItemPT.gsl...\n");
	fp = fopen ("param\\ItemPT.gsl", "rb");
	if (!fp)
	{
		printf ("Can't proceed without ItemPT.gsl\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fseek (fp, 0x3000, SEEK_SET);

	// Load up that EP1 data
	printf ("Parse Episode I data... (This may take awhile...)\n");
	for (ch2=0;ch2<4;ch2++) // For each difficulty
	{
		for (ch=0;ch<10;ch++) // For each ID
		{
			fread  (&ptd, 1, sizeof (PTDATA), fp);

			ptd.enemy_dar[44] = 100; // Dragon
			ptd.enemy_dar[45] = 100; // De Rol Le
			ptd.enemy_dar[46] = 100; // Vol Opt
			ptd.enemy_dar[47] = 100; // Falz

			for (ch3=0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian ( ptd.box_meseta[ch3][0] );
				ptd.box_meseta[ch3][1] = swapendian ( ptd.box_meseta[ch3][1] );
			}

			for (ch3=0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian ( ptd.enemy_meseta[ch3][0] );
				ptd.enemy_meseta[ch3][1] = swapendian ( ptd.enemy_meseta[ch3][1] );
			}

			ptd.enemy_meseta[47][0] = ptd.enemy_meseta[46][0] + 400 + ( 100 * ch2 ); // Give Falz some meseta
			ptd.enemy_meseta[47][1] = ptd.enemy_meseta[46][1] + 400 + ( 100 * ch2 );

			for (ch3=0;ch3<23;ch3++)
			{
				for (ch4=0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian ( ptd.percent_pattern[ch3][ch4] );
			}

			for (ch3=0;ch3<28;ch3++)
			{
				for (ch4=0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian ( ptd.tool_frequency[ch3][ch4] );
				}
			}

			memcpy (&pt_tables_ep1[ch][ch2], &ptd, sizeof (PTDATA));

			// Set up the weapon drop table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank  = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if ( wep_rank >= 0 )
							{
								weapon_drops_ep1[ch][ch2][ch5][ch3++] = ( ch4 + 1 ) + ( (unsigned char) wep_rank << 8 );
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}

			// Set up the slot table

			memset (&wep_counters[0], 0, 4 * 24 );
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4=0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep1[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5=0;ch5<4;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep1[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5=0;ch5<6;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep1[ch][ch2][ch5][ch3++] = (char) ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tool_counters[0], 0, 4 * 28 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep1[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5*2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}

	// Load up that EP2 data
	printf ("Parse Episode II data... (This may take awhile...)\n");
	for (ch2=0;ch2<4;ch2++) // For each difficulty
	{
		for (ch=0;ch<10;ch++) // For each ID
		{
			fread (&ptd, 1, sizeof (PTDATA), fp);

			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow

			for (ch3=0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian ( ptd.box_meseta[ch3][0] );
				ptd.box_meseta[ch3][1] = swapendian ( ptd.box_meseta[ch3][1] );
			}

			for (ch3=0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian ( ptd.enemy_meseta[ch3][0] );
				ptd.enemy_meseta[ch3][1] = swapendian ( ptd.enemy_meseta[ch3][1] );
			}

			ptd.enemy_meseta[78][0] = ptd.enemy_meseta[77][0] + 400 + ( 100 * ch2 ); // Give Flow some meseta
			ptd.enemy_meseta[78][1] = ptd.enemy_meseta[77][1] + 400 + ( 100 * ch2 );

			for (ch3=0;ch3<23;ch3++)
			{
				for (ch4=0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian ( ptd.percent_pattern[ch3][ch4] );
			}

			for (ch3=0;ch3<28;ch3++)
			{
				for (ch4=0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian ( ptd.tool_frequency[ch3][ch4] );
				}
			}

			memcpy ( &pt_tables_ep2[ch][ch2], &ptd, sizeof (PTDATA) );

			// Set up the weapon drop table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank  = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if ( wep_rank >= 0 )
							{
								weapon_drops_ep2[ch][ch2][ch5][ch3++] = ( ch4 + 1 ) + ( (unsigned char) wep_rank << 8 );
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}


			// Set up the slot table

			memset (&wep_counters[0], 0, 4 * 24 );
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4=0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep2[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5=0;ch5<4;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep2[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5=0;ch5<6;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep2[ch][ch2][ch5][ch3++] = (char) ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tool_counters[0], 0, 4 * 28 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep2[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5*2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}


	fclose (fp);
	printf ("\n.. done!\n\n");
	printf ("Loading param\\PlyLevelTbl.bin ... ");
	fp = fopen ( "param\\PlyLevelTbl.bin", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without PlyLevelTbl.bin!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fread ( &startingData, 1, 12*14, fp );
	fseek ( fp, 0xE4, SEEK_SET );
	fread ( &playerLevelData, 1, 28800, fp );
	fclose ( fp );

	printf ("OK!\n\n.. done!\n\nLoading quests...\n\n");
	
	memset (&quest_menus[0], 0, sizeof (quest_menus));

	// 0 = Episode 1 Team
	// 1 = Episode 2 Team
	// 2 = Episode 4 Team
	// 3 = Episode 1 Solo
	// 4 = Episode 2 Solo
	// 5 = Episode 4 Solo
	// 6 = Episode 1 Government
	// 7 = Episode 2 Government
	// 8 = Episode 4 Government
	// 9 = Battle
	// 10 = Challenge

	load_quests ("quest\\ep1team.ini", 0);
	load_quests ("quest\\ep2team.ini", 1);
	load_quests ("quest\\ep4team.ini", 2);
	load_quests ("quest\\ep1solo.ini", 3);
	load_quests ("quest\\ep2solo.ini", 4);
	load_quests ("quest\\ep4solo.ini", 5);
	load_quests ("quest\\ep1gov.ini", 6);
	load_quests ("quest\\ep2gov.ini", 7);
	load_quests ("quest\\ep4gov.ini", 8);
	load_quests ("quest\\battle.ini", 9);

	printf ("\n%u bytes of memory allocated for %u quests...\n\n", questsMemory, numQuests);

	printf ("Loading shop\\shop.dat ...");

	fp = fopen ( "shop\\shop.dat", "rb" );

	if (!fp)
	{
		printf ("Can't proceed without shop.dat!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	if ( fread ( &shops[0], 1, 7000 * sizeof (SHOP), fp )  != (7000 * sizeof (SHOP)) )
	{
		printf ("Failed to read shop data...\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	fclose ( fp );

	shop_checksum = calc_checksum ( &shops[0], 7000 * sizeof (SHOP) );

	printf ("done!\n\n");

	LoadShopData2();

	readLocalGMFile();

	// Set up shop indexes based on character levels...

	for (ch=0;ch<200;ch++)
	{
		switch (ch / 20L)
		{
		case 0:	// Levels 1-20
			shopidx[ch] = 0;
			break;
		case 1: // Levels 21-40
			shopidx[ch] = 1000;
			break;
		case 2: // Levels 41-80
		case 3:
			shopidx[ch] = 2000;
			break;
		case 4: // Levels 81-120
		case 5:
			shopidx[ch] = 3000;
			break;
		case 6: // Levels 121-160
		case 7:
			shopidx[ch] = 4000;
			break;
		case 8: // Levels 161-180
			shopidx[ch] = 5000;
			break;
		default: // Levels 180+
			shopidx[ch] = 6000;
			break;
		}
	}

	memcpy (&Packet03[0x54], &Message03[0], sizeof (Message03));
	printf ("\nShip server parameters\n");
	printf ("///////////////////////\n");
	printf ("IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3] );
	printf ("Ship Port: %u\n", serverPort );
	printf ("Number of Blocks: %u\n", serverBlocks );
	printf ("Maximum Connections: %u\n", serverMaxConnections );
	printf ("Logon server IP: %u.%u.%u.%u\n", loginIP[0], loginIP[1], loginIP[2], loginIP[3] );

	printf ("\nConnecting to the logon server...\n");
	initialize_logon();
	reconnect_logon();

	printf ("\nAllocating %u bytes of memory for blocks... ", sizeof (BLOCK) * serverBlocks );
	blockChunk = malloc ( sizeof (BLOCK) * serverBlocks );
	if (!blockChunk)
	{
		printf ("Out of memory!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	blockPtr = blockChunk;
	memset (blockChunk, 0, sizeof (BLOCK) * serverBlocks);
	for (ch=0;ch<serverBlocks;ch++)
	{
		blocks[ch] = (BLOCK*) blockPtr;
		blockPtr += sizeof (BLOCK);
	}

	printf ("OK!\n");

	printf ("\nAllocating %u bytes of memory for connections... ", sizeof (PSO_CLIENT) * serverMaxConnections );
	connectionChunk = malloc ( sizeof (PSO_CLIENT) * serverMaxConnections );
	if (!connectionChunk )
	{
		printf ("Out of memory!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	connectionPtr = connectionChunk;
	for (ch=0;ch<serverMaxConnections;ch++)
	{
		connections[ch] = (PSO_CLIENT*) connectionPtr;
		connections[ch]->guildcard = 0;
		connections[ch]->character_backup = NULL;
		connections[ch]->mode = 0;
		initialize_connection (connections[ch]);
		connectionPtr += sizeof (PSO_CLIENT);
	}

	printf ("OK!\n\n");

	printf ("Loading ban data... ");
	fp = fopen ("bandata.dat", "rb");
	if (fp)
	{
		fseek ( fp, 0, SEEK_END );
		ch = ftell ( fp );
		num_bans = ch / sizeof (BANDATA);
		if ( num_bans > 5000 )
			num_bans = 5000;
		fseek ( fp, 0, SEEK_SET );
		fread ( &ship_bandata[0], 1, num_bans * sizeof (BANDATA), fp );
		fclose ( fp );
	}
	printf ("done!\n\n%u bans loaded.\n%u IP mask bans loaded.\n\n",num_bans,num_masks);

	/* Open the ship port... */

	printf ("Opening ship port %u for connections.\n", serverPort);

#ifdef USEADDR_ANY
	ship_in.s_addr = INADDR_ANY;
#else
	memcpy (&ship_in.s_addr, &serverIP[0], 4 );
#endif
	ship_sockfd = tcp_sock_open( ship_in, serverPort );

	tcp_listen (ship_sockfd);

	for (ch=1;ch<=serverBlocks;ch++)
	{
		printf ("Opening block port %u (BLOCK%u) for connections.\n", serverPort+ch, ch);
#ifdef USEADDR_ANY
		block_in[ch-1].s_addr = INADDR_ANY;
#else
		memcpy (&block_in[ch-1].s_addr, &serverIP[0], 4 );
#endif
		block_sockfd[ch-1] = tcp_sock_open( block_in[ch-1], serverPort+ch );
		if (block_sockfd[ch-1] < 0)
		{
			printf ("Failed to open port %u for connections.\n", serverPort+ch );
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}

		tcp_listen (block_sockfd[ch-1]);

	}

	if (ship_sockfd < 0)
	{
		printf ("Failed to open ship port for connections.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	printf ("\nListening...\n");
	wc.hbrBackground =(HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hIcon = LoadIcon( hinst, IDI_APPLICATION );
	wc.hCursor = LoadCursor( hinst, IDC_ARROW );
	wc.hInstance = hinst;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "sodaboy";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (! RegisterClass( &wc ) )
	{
		printf ("RegisterClass failure.\n");
		exit (1);
	}

	hwndWindow = CreateWindow ("sodaboy","hidden window", WS_MINIMIZE, 1, 1, 1, 1, 
		NULL, 
		NULL,
		hinst,
		NULL );

	if (!hwndWindow)
	{
		printf ("Failed to create window.");
		exit (1);
	}

	ShowWindow ( hwndWindow, SW_HIDE );
	UpdateWindow ( hwndWindow );
	ShowWindow ( consoleHwnd, SW_HIDE );
	UpdateWindow ( consoleHwnd );

    nid.cbSize				= sizeof(nid);
	nid.hWnd				= hwndWindow;
	nid.uID					= 100;
	nid.uCallbackMessage	= MYWM_NOTIFYICON;
	nid.uFlags				= NIF_MESSAGE|NIF_ICON|NIF_TIP;
    nid.hIcon				= LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));
	nid.szTip[0] = 0;
	strcat (&nid.szTip[0], "Tethealla Ship ");
	strcat (&nid.szTip[0], SERVER_VERSION);
	strcat (&nid.szTip[0], " - Double click to show/hide");
    Shell_NotifyIcon(NIM_ADD, &nid);

	for (;;)
	{
		int nfds = 0;

		/* Process the system tray icon */

		if ( PeekMessage( &msg, hwndWindow, 0, 0, 1 ) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		/* Ping pong?! */

		servertime = time(NULL);

		/* Clear socket activity flags. */

		FD_ZERO (&ReadFDs);
		FD_ZERO (&WriteFDs);
		FD_ZERO (&ExceptFDs);

		// Stop blocking connections after everyone has been disconnected...

		if ((serverNumConnections == 0) && (blockConnections))
		{
			blockConnections = 0;
			printf ("No longer blocking new connections...\n");
		}

		// Process player packets

		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			workConnect = connections[connectNum];

			if (workConnect->plySockfd >= 0) 
			{
				if (blockConnections)
				{
					if (blockTick != (unsigned) servertime)
					{
						blockTick = (unsigned) servertime;
						printf ("Disconnected user %u, left to disconnect: %u\n", workConnect->guildcard, serverNumConnections - 1);
						Send1A ("You were disconnected by a GM...", workConnect);
						workConnect->todc = 1;
					}
				}

				if (workConnect->lastTick != (unsigned) servertime)
				{
					Send1D (workConnect);
					if (workConnect->lastTick > (unsigned) servertime)
						ch2 = 1;
					else
						ch2 = 1 + ((unsigned) servertime - workConnect->lastTick);
						workConnect->lastTick = (unsigned) servertime;
						workConnect->packetsSec /= ch2;
						workConnect->toBytesSec /= ch2;
						workConnect->fromBytesSec /= ch2;
				}

				FD_SET (workConnect->plySockfd, &ReadFDs);
				nfds = max (nfds, workConnect->plySockfd);
				FD_SET (workConnect->plySockfd, &ExceptFDs);
				nfds = max (nfds, workConnect->plySockfd);

				if (workConnect->snddata - workConnect->sndwritten)
				{
					FD_SET (workConnect->plySockfd, &WriteFDs);
					nfds = max (nfds, workConnect->plySockfd);
				}
			}
		}


		// Read from logon server (if connected)

		if (logon->sockfd >= 0)
		{
			if ((unsigned) servertime - logon->last_ping > 60)
			{
				printf ("Logon server ping timeout.  Attempting reconnection in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon ();
			}
			else
			{
				if (logon->packetdata)
				{
					logon_this_packet = *(unsigned *) &logon->packet[logon->packetread];
					memcpy (&logon->decryptbuf[0], &logon->packet[logon->packetread], logon_this_packet);

					LogonProcessPacket ( logon );

					logon->packetread += logon_this_packet;

					if (logon->packetread == logon->packetdata)
						logon->packetread = logon->packetdata = 0;
				}

				FD_SET (logon->sockfd, &ReadFDs);
				nfds = max (nfds, logon->sockfd);

				if (logon->snddata - logon->sndwritten)
				{
					FD_SET (logon->sockfd, &WriteFDs);
					nfds = max (nfds, logon->sockfd);
				}
			}
		}
		else
		{
			logon_tick++;
			if (logon_tick >= LOGIN_RECONNECT_SECONDS * 100)
			{
				printf ("Reconnecting to login server...\n");
				reconnect_logon();
			}
		}


		// Listen for block connections

		for (ch=0;ch<serverBlocks;ch++)
		{
			FD_SET (block_sockfd[ch], &ReadFDs);
			nfds = max (nfds, block_sockfd[ch]);
		}

		// Listen for ship connections

		FD_SET (ship_sockfd, &ReadFDs);
		nfds = max (nfds, ship_sockfd);

		/* Check sockets for activity. */

		if ( select ( nfds + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &select_timeout ) > 0 ) 
		{
			if (FD_ISSET (ship_sockfd, &ReadFDs))
			{
				// Someone's attempting to connect to the ship server.
				ch = free_connection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof (listen_in);
					workConnect = connections[ch];
					if ( ( workConnect->plySockfd = tcp_accept ( ship_sockfd, (struct sockaddr*) &listen_in, &listen_length ) ) > 0 )
					{
						if ( !blockConnections )
						{
							workConnect->connection_index = ch;
							serverConnectionList[serverNumConnections++] = ch;
							memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
							*(unsigned *) &workConnect->ipaddr = *(unsigned *) &listen_in.sin_addr;
							printf ("Accepted SHIP connection from %s:%u\n", workConnect->IP_Address, listen_in.sin_port );
							printf ("Player Count: %u\n", serverNumConnections);
							ShipSend0E (logon);
							start_encryption (workConnect);
							/* Doin' ship process... */
							workConnect->block = 0;
						}
						else
							initialize_connection ( workConnect );
					}
				}
			}

			for (ch=0;ch<serverBlocks;ch++)
			{
				if (FD_ISSET (block_sockfd[ch], &ReadFDs))
				{
					// Someone's attempting to connect to the block server.
					ch2 = free_connection();
					if (ch2 != 0xFFFF)
					{
						listen_length = sizeof (listen_in);
						workConnect = connections[ch2];
						if  ( ( workConnect->plySockfd = tcp_accept ( block_sockfd[ch], (struct sockaddr*) &listen_in, &listen_length ) ) > 0 )
						{
							if ( !blockConnections )
							{
								workConnect->connection_index = ch2;
								serverConnectionList[serverNumConnections++] = ch2;
								memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
								printf ("Accepted BLOCK connection from %s:%u\n", inet_ntoa (listen_in.sin_addr), listen_in.sin_port );
								*(unsigned *) &workConnect->ipaddr = *(unsigned *) &listen_in.sin_addr;
								printf ("Player Count: %u\n", serverNumConnections);
								ShipSend0E (logon);
								start_encryption (workConnect);
								/* Doin' block process... */
								workConnect->block = ch+1;
							}
							else
								initialize_connection ( workConnect );
						}
					}
				}
			}


			// Process client connections

			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				workConnect = connections[connectNum];

				if (workConnect->plySockfd >= 0)
				{
					if (FD_ISSET(workConnect->plySockfd, &WriteFDs))
					{
						// Write shit.

						bytes_sent = send (workConnect->plySockfd, &workConnect->sndbuf[workConnect->sndwritten],
							workConnect->snddata - workConnect->sndwritten, 0);

						if (bytes_sent == SOCKET_ERROR)
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not send data to client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection (workConnect);							
						}
						else
						{
							workConnect->toBytesSec += bytes_sent;
							workConnect->sndwritten += bytes_sent;
						}

						if (workConnect->sndwritten == workConnect->snddata)
							workConnect->sndwritten = workConnect->snddata = 0;
					}

					// Disconnect those violators of the law...

					if (workConnect->todc)
						initialize_connection (workConnect);

					if (FD_ISSET(workConnect->plySockfd, &ReadFDs))
					{
						// Read shit.
						if ( ( pkt_len = recv (workConnect->plySockfd, &tmprcv[0], TCP_BUFFER_SIZE - 1, 0) ) <= 0 )
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not read data from client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection (workConnect);
						}
						else
						{
							workConnect->fromBytesSec += (unsigned) pkt_len;
							// Work with it.

							for (pkt_c=0;pkt_c<pkt_len;pkt_c++)
							{
								workConnect->rcvbuf[workConnect->rcvread++] = tmprcv[pkt_c];

								if (workConnect->rcvread == 8)
								{
									// Decrypt the packet header after receiving 8 bytes.

									cipher_ptr = &workConnect->client_cipher;

									decryptcopy ( &workConnect->decryptbuf[0], &workConnect->rcvbuf[0], 8 );

									// Make sure we're expecting a multiple of 8 bytes.

									workConnect->expect = *(unsigned short*) &workConnect->decryptbuf[0];

									if ( workConnect->expect % 8 )
										workConnect->expect += ( 8 - ( workConnect->expect % 8 ) );

									if ( workConnect->expect > TCP_BUFFER_SIZE )
									{
										initialize_connection ( workConnect );
										break;
									}
								}

								if ( ( workConnect->rcvread == workConnect->expect ) && ( workConnect->expect != 0 ) )
								{
									// Decrypt the rest of the data if needed.

									cipher_ptr = &workConnect->client_cipher;

									if ( workConnect->rcvread > 8 )
										decryptcopy ( &workConnect->decryptbuf[8], &workConnect->rcvbuf[8], workConnect->expect - 8 );

									workConnect->packetsSec ++;

									if (
										//(workConnect->packetsSec   > 89)    ||
										(workConnect->fromBytesSec > 30000) ||
										(workConnect->toBytesSec   > 150000)
										)
									{
										printf ("%u disconnected for possible DDOS. (p/s: %u, tb/s: %u, fb/s: %u)\n", workConnect->guildcard, workConnect->packetsSec, workConnect->toBytesSec, workConnect->fromBytesSec);
										initialize_connection(workConnect);
										break;
									}
									else
									{
										switch (workConnect->block)
										{
										case 0x00:
											// Ship Server
											ShipProcessPacket (workConnect);
											break;
										default:
											// Block server
											BlockProcessPacket (workConnect);
											break;
										}
									}
									workConnect->rcvread = 0;
								}
							}
						}
					}

					if (FD_ISSET(workConnect->plySockfd, &ExceptFDs)) // Exception?
						initialize_connection (workConnect);

				}
			}


			// Process logon server connection

			if ( logon->sockfd >= 0 )
			{
				if (FD_ISSET(logon->sockfd, &WriteFDs))
				{
					// Write shit.

					bytes_sent = send (logon->sockfd, &logon->sndbuf[logon->sndwritten],
						logon->snddata - logon->sndwritten, 0);

					if (bytes_sent == SOCKET_ERROR)
					{
						wserror = WSAGetLastError();
						printf ("Could not send data to logon server...\n");
						printf ("Socket Error %u.\n", wserror );
						initialize_logon();
						printf ("Lost connection with the logon server...\n");
						printf ("Reconnect in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
						logon->sndwritten += bytes_sent;

					if (logon->sndwritten == logon->snddata)
						logon->sndwritten = logon->snddata = 0;
				}

				if (FD_ISSET(logon->sockfd, &ReadFDs))
				{
					// Read shit.
					if ( ( pkt_len = recv (logon->sockfd, &tmprcv[0], PACKET_BUFFER_SIZE - 1, 0) ) <= 0 )
					{
						wserror = WSAGetLastError();
						printf ("Could not read data from logon server...\n");
						printf ("Socket Error %u.\n", wserror );
						initialize_logon();
						printf ("Lost connection with the logon server...\n");
						printf ("Reconnect in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
					{
						// Work with it.
						for (pkt_c=0;pkt_c<pkt_len;pkt_c++)
						{
							logon->rcvbuf[logon->rcvread++] = tmprcv[pkt_c];

							if (logon->rcvread == 4)
							{
								/* Read out how much data we're expecting this packet. */
								logon->expect = *(unsigned *) &logon->rcvbuf[0];

								if ( logon->expect > TCP_BUFFER_SIZE  )
								{
									printf ("Received too much data from the logon server.\nSevering connection and will reconnect in %u seconds...\n",  LOGIN_RECONNECT_SECONDS);
									initialize_logon();
								}
							}

							if ( ( logon->rcvread == logon->expect ) && ( logon->expect != 0 ) )
							{
								decompressShipPacket ( logon, &logon->decryptbuf[0], &logon->rcvbuf[0] );

								logon->expect = *(unsigned *) &logon->decryptbuf[0];

								if (logon->packetdata + logon->expect < PACKET_BUFFER_SIZE)
								{
									memcpy ( &logon->packet[logon->packetdata], &logon->decryptbuf[0], logon->expect );
									logon->packetdata += logon->expect;
								}
								else
									initialize_logon();

								if ( logon->sockfd < 0 )
									break;

								logon->rcvread = 0;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
