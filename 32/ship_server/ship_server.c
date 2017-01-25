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

#include	"stdafx.h"

#include	<string.h>
#include	<time.h>
#include	<math.h>

#include "network.h"
#include "pso_crypt.h"
#include "crypt.h"
#include "params.h"
#include "items.h"
#include "data.h"
#include "character.h"
#include "account.h"
#include "mag.h"
#include "quest.h"
#include "fileio.h"
#include "mtwist.h"
#include "utility.h"
#include "packet.h"
#include "def_tables.h"
#include "def_packets.h"
#include "def_map.h"
#include "def_block.h"

#define reveal_window \
	ShowWindow ( consoleHwnd, SW_NORMAL ); \
	SetForegroundWindow ( consoleHwnd ); \
	SetFocus ( consoleHwnd )

#define swapendian(x) ( ( x & 0xFF ) << 8 ) + ( x >> 8 )
#define FLOAT_PRECISION 0.00001
#define NO_ALIGN

// To do:
//
// Firewall for Team
//
// Challenge
//
// Allow quests to be reloaded while people are in them... somehow!

// These look like settings...

#define SERVER_VERSION "0.144"
#define USEADDR_ANY
//#define LOG_60
#define SHIP_COMPILED_MAX_CONNECTIONS 900
#define LOGIN_RECONNECT_SECONDS 15
#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define MAX_SAVED_LOBBIES 20
#define ALL_ARE_GM 0
#define MAX_SENDCHECK 0x0B
#define SHIP_COMPILED_MAX_GAMES 75

#define SEND_PACKET_03 0x00
#define RECEIVE_PACKET_93 0x0A

#include "classes.h"

// Class equip_flags

#define HUNTER_FLAG	1   // Bit 1
#define RANGER_FLAG	2   // Bit 2
#define FORCE_FLAG	4   // Bit 3
#define HUMAN_FLAG	8   // Bit 4
#define	DROID_FLAG	16  // Bit 5
#define	NEWMAN_FLAG	32  // Bit 6
#define	MALE_FLAG	64  // Bit 7
#define	FEMALE_FLAG	128 // Bit 8

/* Lobby Structure */

typedef struct st_lobby {
	unsigned char floor[12];
	unsigned clientx[12];
	unsigned clienty[12];
	unsigned char arrow_color[12];
	unsigned lobbyCount;
	MONSTER monsterData[0xB50];
	MAP_MONSTER mapData[0xB50]; // For figuring out which monsters go where, etc.
	MAP_BOX objData[0xB50]; // Box drop information
	unsigned mapIndex;
	unsigned objIndex;
	unsigned rareIndex;
	unsigned char rareData[0x20];
	unsigned char boxHit[0xB50];
	GAME_ITEM gameItem[MAX_SAVED_ITEMS]; // Game Item Data
	unsigned gameItemList[MAX_SAVED_ITEMS]; // Game Item Link List
	unsigned gameItemCount;
	unsigned itemID;
	unsigned playerItemID[4];
	int questE0; // Server already dropped BP reward?
	int drops_disabled; // Basically checks if someone exchanged a photon crystal
	unsigned bankItemID[4];
	unsigned leader;
	unsigned char sectionID;
	unsigned gamePlayerCount; // This number increases as people join and depart the game...
	unsigned gamePlayerID[4]; // Keep track for leader purposes...
	unsigned char gameName[30];
	unsigned char gamePassword[32];
	unsigned char gameMap[128];
	unsigned char gameMonster[0x04];
	unsigned char episode;
	unsigned char difficulty;
	unsigned char battle;
	unsigned char challenge;
	unsigned char oneperson;
	unsigned short battle_level;
	int meseta_boost;
	int quest_in_progress;
	int quest_loaded;
	int inpquest;
	unsigned start_time;
	int in_use;
	int redbox;
	int slot_use[12];
	PSO_CLIENT* clients[12];
	BATTLEPARAM* bptable;
} LOBBY;


#include "ship_client.h"


/* Connected Logon Server Structure */

typedef struct st_pso_server {
	int sockfd;
	struct in_addr _ip;
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned long rcvread;
	unsigned long expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE];
	unsigned char sndbuf [PACKET_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE];
	int snddata, sndwritten;
	unsigned char packet [PACKET_BUFFER_SIZE];
	unsigned long packetdata;
	unsigned long packetread;
	int crypt_on;
	unsigned char user_key[128];
	int key_change[128];
	struct rc4_key cs_key;
	struct rc4_key sc_key;
	unsigned last_ping;
} PSO_SERVER;



/* Ship List Structure (Assembled from Logon Packet) */

typedef struct st_shiplist {
	unsigned shipID;
	unsigned char ipaddr[4];
	unsigned short port;
} SHIPLIST;


/* Block Structure */

typedef struct st_block {
	LOBBY lobbies[16+SHIP_COMPILED_MAX_GAMES];
	unsigned count; // keep track of how many people are on this block
} BLOCK;


/* Ban Structure */

typedef struct st_bandata
{
	unsigned guildcard;
	unsigned type; // 1 = account, 2 = ipaddr, 3 = hwinfo
	unsigned ipaddr;
	long long hwinfo;
} BANDATA;


/* Saved Lobby Structure */

typedef struct st_saveLobby {
	unsigned guildcard;
	unsigned short lobby;
} saveLobby;


/* Function Defintions */

void ShowArrows (PSO_CLIENT* client, int to_all);
unsigned char* MakePacketEA15 (PSO_CLIENT* client);
void SendToLobby (LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend );
void removeClientFromLobby (PSO_CLIENT* client);

void encryptcopy (PSO_CLIENT* client, const unsigned char* src, unsigned size);
void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned size);

void compressShipPacket ( PSO_SERVER* ship, unsigned char* src, unsigned long src_size );
void decompressShipPacket ( PSO_SERVER* ship, unsigned char* dest, unsigned char* src );

void CommandED(PSO_CLIENT* client);
void Command40(PSO_CLIENT* client, PSO_SERVER* ship);
void Command09(PSO_CLIENT* client);
void Command10(unsigned blockServer, PSO_CLIENT* client);
void CommandE8 (PSO_CLIENT* client);
void CommandD8 (PSO_CLIENT* client);
void CommandD9 (PSO_CLIENT* client);
void Command81 (PSO_CLIENT* client, PSO_SERVER* ship);
void CommandEA (PSO_CLIENT* client, PSO_SERVER* ship);

void AddExp (unsigned int XP, PSO_CLIENT* client);

void CreateTeam (unsigned short* teamname, unsigned guildcard, PSO_SERVER* ship);
void UpdateTeamFlag (unsigned char* flag, unsigned teamid, PSO_SERVER* ship);
void DissolveTeam (unsigned teamid, PSO_SERVER* ship);
void RemoveTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );
void RequestTeamList ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );
void PromoteTeamMember ( unsigned teamid, unsigned guildcard, unsigned char newlevel, PSO_SERVER* ship );
void AddTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );

void TeamChat ( unsigned short* text, unsigned short chatsize, unsigned teamid, PSO_SERVER* ship );

void load_map_data (LOBBY* l, int aMob, const char* filename);
void load_object_data (LOBBY* l, int unused, const char* filename);
void parse_map_data (LOBBY* l, MAP_MONSTER* mapData, int aMob, unsigned num_records);

void feed_mag (unsigned int magid, unsigned int itemid, PSO_CLIENT* client);

void GenerateCommonItem (int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client);
unsigned int free_game_item (LOBBY* l);


const unsigned char Message03[] = { "Tethealla Ship v.144" };

/* variables */

struct timeval select_timeout = { 
	0,
	5000
};

FILE* debugfile;

// Random drop rates

unsigned int weapon_drop_rate, armor_drop_rate, mag_drop_rate, tool_drop_rate, meseta_drop_rate, experience_rate;
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

/* Ban (IP) Data */

unsigned short ship_banmasks[5000][4]; // IP address ban masks
BANDATA ship_bandata[5000];
unsigned int num_masks = 0;
unsigned int num_bans = 0;

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
unsigned char dp[TCP_BUFFER_SIZE*4];		// what is this?????
unsigned int ship_ignore_list[300] = {0};
unsigned int ship_ignore_count = 0;
char Ship_Name[255];
SHIPLIST shipdata[200];
BLOCK* blocks[10];
QUEST quests[512];
QUEST_MENU quest_menus[12];
unsigned* int quest_allow = 0; // the "allow" list for the 0x60CA command...
unsigned int quest_numallows;
unsigned int numQuests = 0;
unsigned int questsMemory = 0;
char* languageNames[10];
char* languageExts[10];
unsigned int numLanguages = 0;
unsigned int totalShips = 0;
BATTLEPARAM ep1battle[374];
BATTLEPARAM ep2battle[374];
BATTLEPARAM ep4battle[332];
BATTLEPARAM ep1battle_off[374];
BATTLEPARAM ep2battle_off[374];
BATTLEPARAM ep4battle_off[332];
unsigned int battle_count;
SHOP shops[7000];
unsigned int shop_checksum;
unsigned int shopidx[200];
unsigned int ship_index;
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

unsigned char chatBuf[4000];
unsigned char cmdBuf[4000];
char* myCommand;
char* myArgs[64];

// functions start
char* unicode_to_ascii (unsigned short* ucs)
{
	char *s,c;

	s = (char*) &chatBuf[0];
	while (*ucs != 0x0000)
	{
		c = *(ucs++) & 0xFF;
		if ((c >= 0x20) && (c <= 0x80))
			*(s++) = c;
		else
			*(s++) = 0x20;
	}
	*(s++) = 0x00;
	return (char*) &chatBuf[0];
}

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
		if ((l->slot_use[ch]) && (l->clients[ch]) && (l->clients[ch]->guildcard != nosend))
		{
			cipher_ptr = &l->clients[ch]->server_cipher;
			encryptcopy (l->clients[ch], src, size);
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
		l->clients[client->clientID] = 0;
	}

	if (client->lobbyNum > 0x0F)
		maxch = 4;
	else
		maxch = 12;

	l->lobbyCount = 0;

	for (ch=0;ch<maxch;ch++)
	{
		if ((l->clients[ch]) && (l->slot_use[ch]))
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
					if ((l->slot_use[ch]) && (l->clients[ch]) && (l->gamePlayerID[ch] < lowestID))
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

#include "sendpackets.h"

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
				write_log ("ship.log", "Could not open bandata.dat for writing!!");
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
			write_log ("ship.log", "Could not open bandata.dat for writing!!");
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

void AddGuildCard (unsigned myGC, unsigned friendGC, unsigned char* friendName, unsigned char* friendText, unsigned char friendSecID, unsigned char friendClass, PSO_SERVER* ship)
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
		if ((l->slot_use[ch] != 0) && (l->clients[ch]))
		{
			total_clients++;
			PacketData[Packet88Offset+2] = 0x01;
			*(unsigned*) &PacketData[Packet88Offset+4] = l->clients[ch]->character.guildCard;
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

#include "shipsend.h"

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
						if ( ( l->slot_use[ch] != 0 ) && ( l->clients[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->clients[ch]), 2152 );
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
						if ( ( l->slot_use[ch] != 0 ) && ( l->clients[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->clients[ch]), 2152 );
						}
					client->bursting = 0;
				}
			}
			break;
		case 0x81:
			if (client->announce)
			{
				if (client->announce == 1)
				{
					unsigned char tempbuf[4000];
					write_gm ("GM %u made an announcement: %s", client->guildcard, unicode_to_ascii ( (unsigned short*) &client->decryptbuf[0x60], tempbuf) );
				}
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
					if ((l->slot_use[ch]) && (l->clients[ch]) && (l->clients[ch]->hasquest == 0))
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
					cipher_ptr = &l->clients[l->leader]->server_cipher;
					memset (&client->encryptbuf[0], 0, 8);
					client->encryptbuf[0] = 0x08;
					client->encryptbuf[2] = 0xDD;
					client->encryptbuf[4] = client->clientID;
					encryptcopy (l->clients[l->leader], &client->encryptbuf[0], 8);
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
										client->encryptbuf[0x0A] = (unsigned char) experience_rate;
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
				unsigned char tempbuf[4000];
				if ( (client->isgm) || (isLocalGM(client->guildcard)) )
					write_gm ("GM %u (%s) has disconnected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4], tempbuf) );
				else
					write_log ("ship.log", "User %u (%s) has disconnected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4], tempbuf) );
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

//LOBBY fakelobby;

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
					CheckMaxGrind (&client->character.inventory[eq_wep], grind_table);
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

unsigned char* MakePacketEA15 (PSO_CLIENT* client)
{
	sprintf (&PacketData[0x00], "\x64\x08\xEA\x15\x01");
	memset  (&PacketData[0x05], 0, 3);
	*(unsigned *) &PacketData[0x08] = client->guildcard;
	*(unsigned *) &PacketData[0x0C] = client->character.teamID;
	PacketData [0x18] = (unsigned char) client->character.privilegeLevel;
	memcpy  (&PacketData [0x1C], &client->character.teamName[0], 28);
	sprintf (&PacketData[0x38], "\x84\x6C\x98");
	*(unsigned *) &PacketData[0x3C] = client->guildcard;
	PacketData[0x40] = client->clientID;
	memcpy  (&PacketData[0x44], &client->character.name[0], 24);
	memcpy  (&PacketData[0x64], &client->character.teamFlag[0], 0x800);
	return   &PacketData[0];
}

void LogonProcessPacket (PSO_SERVER* ship)
{
	unsigned gcn, ch, ch2, connectNum;
	unsigned char episode, part;
	unsigned mob_rate;
	long long mob_calc;

	switch (ship->decryptbuf[0x04])
	{
	case 0x00:
		// Server has sent it's welcome packet.  Start encryption and send ship info...
		memcpy (&ship->user_key[0], &RC4publicKey[0], 32);
		ch2 = 0;
		for (ch=0x1C;ch<0x5C;ch+=2)
		{
			ship->key_change [ch2+(ship->decryptbuf[ch] % 4)] = ship->decryptbuf[ch+1];
			ch2 += 4;
		}
		prepare_key(&ship->user_key[0], 32, &ship->cs_key);
		prepare_key(&ship->user_key[0], 32, &ship->sc_key);
		ship->crypt_on = 1;
		memcpy (&ship->encryptbuf[0x00], &ship->decryptbuf[0x04], 0x28);
		memcpy (&ship->encryptbuf[0x00], &ShipPacket00[0x00], 0x10); // Yep! :)
		ship->encryptbuf[0x00] = 1;
		memcpy (&ship->encryptbuf[0x28], &Ship_Name[0], 12 );
		*(unsigned *) &ship->encryptbuf[0x34] = serverNumConnections;
		*(unsigned *) &ship->encryptbuf[0x38] = *(unsigned *) &serverIP[0];
		*(unsigned short*) &ship->encryptbuf[0x3C] = (unsigned short) serverPort;
		*(unsigned *) &ship->encryptbuf[0x3E] = shop_checksum;
		*(unsigned *) &ship->encryptbuf[0x42] = ship_index;
		memcpy (&ship->encryptbuf[0x46], &ship_key[0], 32);
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x66 );
		break;
	case 0x02:
		// Server's result of our authentication packet.
		if (ship->decryptbuf[0x05] != 0x01)
		{
			switch (ship->decryptbuf[0x05])
			{
			case 0x00:
				printf ("This ship's version is incompatible with the login server.\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x02:
				printf ("This ship's IP address is already registered with the logon server.\n");
				printf ("The IP address cannot be registered twice.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x03:
				printf ("This ship did not pass the connection test the login server ran on it.\n");
				printf ("Please be sure the IP address specified in ship.ini is correct, your\n");
				printf ("firewall has ship_serv.exe on allow.  If behind a router, please be\n");
				printf ("sure your ports are forwarded.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x04:
				printf ("Please do not modify any data not instructed to when connecting to this\n");
				printf ("login server...\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x05:
				printf ("Your ship_key.bin file seems to be invalid.\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x06:
				printf ("Your ship key appears to already be in use!\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			}
			initialize_logon();
		}
		else
		{
			serverID = *(unsigned *) &ship->decryptbuf[0x06];
			if (serverIP[0] == 0x00)
			{
				*(unsigned *) &serverIP[0] = *(unsigned *) &ship->decryptbuf[0x0A];
				printf ("Updated IP address to %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
			}
			serverID++;
			if (serverID != 0xFFFFFFFF)
			{
				printf ("Ship has successfully registered with the login server!!! Ship ID: %u\n", serverID );
				printf ("Constructing Block List packet...\n\n");
				ConstructBlockPacket();
				printf ("Load quest allowance...\n");
				quest_numallows = *(unsigned *) &ship->decryptbuf[0x0E];
				if ( quest_allow )
					free ( quest_allow );
				quest_allow = malloc ( quest_numallows * 4  );
				memcpy ( quest_allow, &ship->decryptbuf[0x12], quest_numallows * 4 );
				printf ("Quest allowance item count: %u\n\n", quest_numallows );
				normalName = *(unsigned *) &ship->decryptbuf[0x12 + ( quest_numallows * 4 )];
				localName = *(unsigned *) &ship->decryptbuf[0x16 + ( quest_numallows * 4 )];
				globalName = *(unsigned *) &ship->decryptbuf[0x1A + ( quest_numallows * 4 )];
				memcpy (&ship->user_key[0], &ship_key[0], 128 ); // 1024-bit key

				// Change keys

				for (ch2=0;ch2<128;ch2++)
					if (ship->key_change[ch2] != -1)
						ship->user_key[ch2] = (unsigned char) ship->key_change[ch2]; // update the key

				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->cs_key);
				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->sc_key);
				memset ( &ship->encryptbuf[0x00], 0, 8 );
				ship->encryptbuf[0x00] = 0x0F;
				ship->encryptbuf[0x01] = 0x00;
				printf ( "Requesting drop charts from server...\n");
				compressShipPacket ( ship, &ship->encryptbuf[0x00], 4 );
			}
			else
			{
				printf ("The ship has failed authentication to the logon server.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon();
			}
		}
		break;
	case 0x03:
		// Reserved
		break;
	case 0x04:
		switch (ship->decryptbuf[0x05])
		{
		case 0x01:
			{
				// Receive and store full player data here.
				//
				PSO_CLIENT* client;
				unsigned guildcard,ch,ch2,eq_weapon,eq_armor,eq_shield,eq_mag;
				int sockfd;
				unsigned short baseATP, baseMST, baseEVP, baseHP, baseDFP, baseATA;
				unsigned char* cd;

				guildcard = *(unsigned *) &ship->decryptbuf[0x06];
				sockfd = *(int *) &ship->decryptbuf[0x0C];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->plySockfd == sockfd) && (connections[connectNum]->guildcard == guildcard))
					{
						client = connections[connectNum];
						client->gotchardata = 1;
						memcpy (&client->character.packetSize, &ship->decryptbuf[0x10], sizeof (CHARDATA));

						/* Set up copies of the banks */

						memcpy (&client->char_bank, &client->character.bankUse, sizeof (BANK));
						memcpy (&client->common_bank, &ship->decryptbuf[0x10+sizeof(CHARDATA)], sizeof (BANK));

						cipher_ptr = &client->server_cipher;
						if (client->isgm == 1)
							*(unsigned *) &client->character.nameColorBlue = globalName;
						else
							if (isLocalGM(client->guildcard))
								*(unsigned *) &client->character.nameColorBlue = localName;
							else
								*(unsigned *) &client->character.nameColorBlue = normalName;

						if (client->character.inventoryUse > 30)
							client->character.inventoryUse = 30;

						client->equip_flags = 0;
						switch (client->character._class)
						{
						case CLASS_HUMAR:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_HUNEWEARL:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_HUCAST:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_HUCASEAL:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_RAMAR:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_RACAST:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_RACASEAL:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_RAMARL:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FONEWM:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_FONEWEARL:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FOMARL:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FOMAR:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						}

						// Let's fix hacked mags and weapons

						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						{
							if (client->character.inventory[ch2].in_use)
								FixItem ( &client->character.inventory[ch2].item, armor_dfpvar_table, armor_evpvar_table, barrier_dfpvar_table, barrier_evpvar_table );
						}

						// Fix up equipped weapon, armor, shield, and mag equipment information

						eq_weapon = 0;
						eq_armor = 0;
						eq_shield = 0;
						eq_mag = 0;

						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						{
							if (client->character.inventory[ch2].flags & 0x08)
							{
								switch (client->character.inventory[ch2].item.data[0])
								{
								case 0x00:
									eq_weapon++;
									break;
								case 0x01:
									switch (client->character.inventory[ch2].item.data[1])
									{
									case 0x01:
										eq_armor++;
										break;
									case 0x02:
										eq_shield++;
										break;
									}
									break;
								case 0x02:
									eq_mag++;
									break;
								}
							}
						}

						if (eq_weapon > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all weapons when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
									(client->character.inventory[ch2].flags & 0x08))
									client->character.inventory[ch2].flags &= ~(0x08);
							}

						}

						if (eq_armor > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all armor and slot items when there is more than one armor equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
									(client->character.inventory[ch2].item.data[1] != 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].item.data[3] = 0x00;
									client->character.inventory[ch2].flags &= ~(0x08);
								}
							}
						}

						if (eq_shield > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all shields when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
									(client->character.inventory[ch2].item.data[1] == 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].item.data[3] = 0x00;
									client->character.inventory[ch2].flags &= ~(0x08);
								}
							}
						}

						if (eq_mag > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all mags when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
									client->character.inventory[ch2].flags &= ~(0x08);
							}
						}

						for (ch2=0;ch2<client->character.bankUse;ch2++)
							FixItem ( (ITEM*) &client->character.bankInventory[ch2], armor_dfpvar_table, barrier_dfpvar_table, barrier_evpvar_table );

						baseATP = *(unsigned short*) &startingData[(client->character._class*14)];
						baseMST = *(unsigned short*) &startingData[(client->character._class*14)+2];
						baseEVP = *(unsigned short*) &startingData[(client->character._class*14)+4];
						baseHP  = *(unsigned short*) &startingData[(client->character._class*14)+6];
						baseDFP = *(unsigned short*) &startingData[(client->character._class*14)+8];
						baseATA = *(unsigned short*) &startingData[(client->character._class*14)+10];

						for (ch2=0;ch2<client->character.level;ch2++)
						{
							baseATP += playerLevelData[client->character._class][ch2].ATP;
							baseMST += playerLevelData[client->character._class][ch2].MST;
							baseEVP += playerLevelData[client->character._class][ch2].EVP;
							baseHP  += playerLevelData[client->character._class][ch2].HP;
							baseDFP += playerLevelData[client->character._class][ch2].DFP;
							baseATA += playerLevelData[client->character._class][ch2].ATA;
						}

						client->matuse[0] = ( client->character.ATP - baseATP ) / 2;
						client->matuse[1] = ( client->character.MST - baseMST ) / 2;
						client->matuse[2] = ( client->character.EVP - baseEVP ) / 2;
						client->matuse[3] = ( client->character.DFP - baseDFP ) / 2;
						client->matuse[4] = ( client->character.LCK - 10 ) / 2;

						//client->character.lang = 0x00;

						cd = (unsigned char*) &client->character.packetSize;

						cd[(8*28)+0x0F]  = client->matuse[0];
						cd[(9*28)+0x0F]  = client->matuse[1];
						cd[(10*28)+0x0F] = client->matuse[2];
						cd[(11*28)+0x0F] = client->matuse[3];
						cd[(12*28)+0x0F] = client->matuse[4];

						encryptcopy (client, (unsigned char*) &client->character.packetSize, sizeof (CHARDATA) );
						client->preferred_lobby = 0xFF;

						cd[(8*28)+0x0F]  = 0x00; // Clear this stuff out to not mess up our item procedures.
						cd[(9*28)+0x0F]  = 0x00;
						cd[(10*28)+0x0F] = 0x00;
						cd[(11*28)+0x0F] = 0x00;
						cd[(12*28)+0x0F] = 0x00;

						for (ch2=0;ch2<MAX_SAVED_LOBBIES;ch2++)
						{
							if (savedlobbies[ch2].guildcard == client->guildcard)
							{
								client->preferred_lobby = savedlobbies[ch2].lobby - 1;
								savedlobbies[ch2].guildcard = 0;
								break;
							}
						}

						Send95 (client);

						if ( (client->isgm) || (isLocalGM(client->guildcard)) )
							write_gm ("GM %u (%s) has connected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4]));
						else
							write_log ("ship.log", "User %u (%s) has connected", client->guildcard, unicode_to_ascii ((unsigned short*) &client->character.name[4]));
						break;
					}
				}
			}
			break;
		case 0x03:
			{
				unsigned guildcard;
				PSO_CLIENT* client;
				
				guildcard = *(unsigned *) &ship->decryptbuf[0x06];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard == guildcard) && (connections[connectNum]->released == 1))
					{
						// Let the released client roam free...!
						client = connections[connectNum];
						Send19 (client->releaseIP[0], client->releaseIP[1], client->releaseIP[2], client->releaseIP[3], 
							client->releasePort, client);
						break;
					}
				}
			}
		}
		break;
	case 0x05:
		// Reserved
		break;
	case 0x06:
		// Reserved
		break;
	case 0x07:
		// Card database full.
		gcn = *(unsigned *) &ship->decryptbuf[0x06];

		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard == gcn)
			{
				Send1A ("Your guild card database on the server is full.\n\nYou were unable to accept the guild card.\n\nPlease delete some cards.  (40 max)", connections[connectNum]);
				break;
			}
		}
		break;
	case 0x08:
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x06];
				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						Send1A ("This account has just logged on.\n\nYou are now being disconnected.", connections[connectNum]);
						connections[connectNum]->todc = 1;
						break;
					}
				}
			}
			break;
		case 0x01:
			{
				// Someone's doing a guild card search...   Check to see if that guild card is on our ship...

				unsigned client_gcn, ch2;
				unsigned char *n;
				unsigned char *c;
				unsigned short blockPort;

				gcn = *(unsigned *) &ship->decryptbuf[0x06];
				client_gcn = *(unsigned *) &ship->decryptbuf[0x0A];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard == gcn) && (connections[connectNum]->lobbyNum))
					{
						if (connections[connectNum]->lobbyNum < 0x10)
							for (ch2=0;ch2<MAX_SAVED_LOBBIES;ch2++)
							{
								if (savedlobbies[ch2].guildcard == 0)
								{
									savedlobbies[ch2].guildcard = client_gcn;
									savedlobbies[ch2].lobby = connections[connectNum]->lobbyNum;
									break;
								}
							}
						ship->encryptbuf[0x00] = 0x08;
						ship->encryptbuf[0x01] = 0x02;
						*(unsigned *) &ship->encryptbuf[0x02] = serverID;
						*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &ship->decryptbuf[0x0E];
						// 0x10 = 41 result packet
						memset (&ship->encryptbuf[0x0A], 0, 0x136);
						ship->encryptbuf[0x10] = 0x30;
						ship->encryptbuf[0x11] = 0x01;
						ship->encryptbuf[0x12] = 0x41;
						ship->encryptbuf[0x1A] = 0x01;
						*(unsigned *) &ship->encryptbuf[0x1C] = client_gcn;
						*(unsigned *) &ship->encryptbuf[0x20] = gcn;
						ship->encryptbuf[0x24] = 0x10;
						ship->encryptbuf[0x26] = 0x19;
						*(unsigned *) &ship->encryptbuf[0x2C] = *(unsigned *) &serverIP[0];
						blockPort = serverPort + connections[connectNum]->block;
						*(unsigned short *) &ship->encryptbuf[0x30] = (unsigned short) blockPort;					
						memcpy (&ship->encryptbuf[0x34], &lobbyString[0], 12 );						
						if ( connections[connectNum]->lobbyNum < 0x10 )
						{
							if ( connections[connectNum]->lobbyNum < 10 )
							{
								ship->encryptbuf[0x40] = 0x30;
								ship->encryptbuf[0x42] = 0x30 + connections[connectNum]->lobbyNum;
							}
							else
							{
								ship->encryptbuf[0x40] = 0x31;
								ship->encryptbuf[0x42] = 0x26 + connections[connectNum]->lobbyNum;
							}
						}
						else
						{
								ship->encryptbuf[0x40] = 0x30;
								ship->encryptbuf[0x42] = 0x31;
						}
						ship->encryptbuf[0x44] = 0x2C;
						memcpy ( &ship->encryptbuf[0x46], &blockString[0], 10 );
						if ( connections[connectNum]->block < 10 )
						{
							ship->encryptbuf[0x50] = 0x30;
							ship->encryptbuf[0x52] = 0x30 + connections[connectNum]->block;
						}
						else
						{
							ship->encryptbuf[0x50] = 0x31;
							ship->encryptbuf[0x52] = 0x26 + connections[connectNum]->block;
						}

						ship->encryptbuf[0x54] = 0x2C;
						if (serverID < 10)
						{
							ship->encryptbuf[0x56] = 0x30;
							ship->encryptbuf[0x58] = 0x30 + serverID;
						}
						else
						{
							ship->encryptbuf[0x56] = 0x30 + ( serverID / 10 );
							ship->encryptbuf[0x58] = 0x30 + ( serverID % 10 );
						}
						ship->encryptbuf[0x5A] = 0x3A;
						n = (unsigned char*) &ship->encryptbuf[0x5C];
						c = (unsigned char*) &Ship_Name[0];
						while (*c != 0x00)
						{
							*(n++) = *(c++);
							n++;
						}
						if ( connections[connectNum]->lobbyNum < 0x10 )
						ship->encryptbuf[0xBC] = (unsigned char) connections[connectNum]->lobbyNum; else
						ship->encryptbuf[0xBC] = 0x01;
						ship->encryptbuf[0xBE] = 0x1A;
						memcpy (&ship->encryptbuf[0x100], &connections[connectNum]->character.name[0], 24);
						compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x140 );
						break;
					}
				}
			}
			break;
		case 0x02:
			// Send guild result to user
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x20];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						cipher_ptr = &connections[connectNum]->server_cipher;
						encryptcopy (connections[connectNum], &ship->decryptbuf[0x14], 0x130);
						break;
					}
				}
			}
			break;
		case 0x03:
			// Send mail to user
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x36];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						cipher_ptr = &connections[connectNum]->server_cipher;
						encryptcopy (connections[connectNum], &ship->decryptbuf[0x06], 0x45C);
						break;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case 0x09:
		// Reserved for team functions.
		switch (ship->decryptbuf[0x05])
		{
			PSO_CLIENT* client;
			unsigned char CreateResult;

		case 0x00:
			CreateResult = ship->decryptbuf[0x06];
			gcn = *(unsigned *) &ship->decryptbuf[0x07];
			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					client = connections[connectNum];
					switch (CreateResult)
					{
					case 0x00:
						// All good!!!
						client->character.teamID = *(unsigned *) &ship->decryptbuf[0x823];
						memcpy (&client->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						client->character.privilegeLevel = 0x40;
						client->character.unknown15 = 0x00986C84; // ??
						client->character.teamName[0] = 0x09;
						client->character.teamName[2] = 0x45;
						client->character.privilegeLevel = 0x40;
						memcpy (&client->character.teamName[4], &ship->decryptbuf[0x80B], 24);
						SendEA (0x02, client);
						SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						SendEA (0x12, client);
						SendEA (0x1D, client);
						break;
					case 0x01:
						Send1A ("The server failed to create the team due to a MySQL error.\n\nPlease contact the server administrator.", client);
						break;
					case 0x02:
						Send01 ("Cannot create team\nbecause team\n already exists!!!", client);
						break;
					case 0x03:
						Send01 ("Cannot create team\nbecause you are\nalready in a team!", client);
						break;
					}
					break;
				}
			}
			break;
		case 0x01:
			// Flag updated
			{
				unsigned teamid;
				PSO_CLIENT* tClient;

				teamid = *(unsigned *) &ship->decryptbuf[0x07];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						memcpy ( &tClient->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
					}
				}
			}
			break;
		case 0x02:
			// Team dissolved
			{
				unsigned teamid;
				PSO_CLIENT* tClient;

				teamid = *(unsigned *) &ship->decryptbuf[0x07];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						memset ( &tClient->character.guildCard2, 0, 2108 );
						SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
						SendEA ( 0x12, tClient );
					}
				}
			}
			break;
		case 0x04:
			// Team chat
			{
				unsigned teamid, size;
				PSO_CLIENT* tClient;

				size = *(unsigned *) &ship->decryptbuf[0x00];
				size -= 10;

				teamid = *(unsigned *) &ship->decryptbuf[0x06];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						cipher_ptr = &tClient->server_cipher;
						encryptcopy ( tClient, &ship->decryptbuf[0x0A], size );
					}
				}
			}
			break;
		case 0x05:
			// Request Team List
			{
				unsigned gcn;
				unsigned short size;
				PSO_CLIENT* tClient;

				gcn = *(unsigned *) &ship->decryptbuf[0x0A];
				size = *(unsigned short*) &ship->decryptbuf[0x0E];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{					
						tClient = connections[connectNum];
						cipher_ptr = &tClient->server_cipher;
						encryptcopy (tClient, &ship->decryptbuf[0x0E], size);
						break;
					}
				}
			}
			break;
		}
		break;
	case 0x0A:
		// Reserved
		break;
	case 0x0B:
		// Player authentication result from the logon server.
		gcn = *(unsigned *) &ship->decryptbuf[0x06];
		if (ship->decryptbuf[0x05] == 0)
		{
			PSO_CLIENT* client;

			// Finish up the logon process here.

			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					client = connections[connectNum];
					client->slotnum = ship->decryptbuf[0x0A];
					client->isgm = ship->decryptbuf[0x0B];
					memcpy (&client->encryptbuf[0], &PacketE6[0], sizeof (PacketE6));
					*(unsigned *) &client->encryptbuf[0x10] = gcn;
					client->guildcard = gcn;
					*(unsigned *) &client->encryptbuf[0x14] = *(unsigned*) &ship->decryptbuf[0x0C];
					*(long long *) &client->encryptbuf[0x38] = *(long long*) &ship->decryptbuf[0x10];
					if (client->decryptbuf[0x16] < 0x05)
					{
						Send1A ("Client/Server synchronization error.", client);
						client->todc = 1;
					}
					else
					{
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0], sizeof (PacketE6));
						client->lastTick = (unsigned) servertime;
						if (client->block == 0)
						{
							if (logon->sockfd >= 0)
								Send07(client);
							else
							{
								Send1A("This ship has unfortunately lost it's connection with the logon server...\nData cannot be saved.\n\nPlease reconnect later.", client);
								client->todc = 1;
							}
						}
						else
						{
							blocks[client->block - 1]->count++;
							// Request E7 information from server...
							Send83(client); // Lobby data
							ShipSend04 (0x00, client, logon);
						}
					}
					break;
				}
			}
		}
		else
		{
			// Deny connection here.
			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					Send1A ("Security violation.", connections[connectNum]);
					connections[connectNum]->todc = 1;
					break;
				}
			}
		}
		break;
	case 0x0D:
		// 00 = Request ship list
		// 01 = Ship list data (include IP addresses)
		switch (ship->decryptbuf[0x05])
		{
			case 0x01:
				{
					unsigned char ch;
					int sockfd;
					unsigned short pOffset;

					// Retrieved ship list data.  Send to client...

					sockfd = *(int *) &ship->decryptbuf[0x06];
					pOffset = *(unsigned short *) &ship->decryptbuf[0x0A];
					memcpy (&PacketA0Data[0x00], &ship->decryptbuf[0x0A], pOffset);
					pOffset += 0x0A;

					totalShips = 0;

					for (ch=0;ch<PacketA0Data[0x04];ch++)
					{
						shipdata[ch].shipID = *(unsigned *) &ship->decryptbuf[pOffset];
						pOffset +=4;
						*(unsigned *) &shipdata[ch].ipaddr[0] = *(unsigned *) &ship->decryptbuf[pOffset];
						pOffset +=4;
						shipdata[ch].port = *(unsigned short *) &ship->decryptbuf[pOffset];
						pOffset +=2;
						totalShips++;
					}

					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->plySockfd == sockfd)
						{
							SendA0 (connections[connectNum]);
							break;
						}
					}
				}
				break;
			default:
				break;
		}
		break;
	case 0x0F:
		// Receiving drop chart
		episode = ship->decryptbuf[0x05];
		part = ship->decryptbuf[0x06];
		if ( ship->decryptbuf[0x06] == 0 )
			printf ("Received drop chart from login server...\n");
		switch ( episode )
		{
		case 0x01:
			if ( part == 0 )
				printf ("Episode I ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep1[(sizeof(rt_tables_ep1) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep1) >> 1 );
			break;
		case 0x02:
			if ( part == 0 )
				printf ("Episode II ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep2[(sizeof(rt_tables_ep2) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep2) >> 1 );
			break;
		case 0x03:
			if ( part == 0 )
				printf ("Episode IV ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep4[(sizeof(rt_tables_ep4) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep4) >> 1 );
			break;
		}
		*(unsigned *) &ship->encryptbuf[0x00] = *(unsigned *) &ship->decryptbuf[0x04];
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x04 );
		break;
	case 0x10:
		// Monster appearance rates
		printf ("\nReceived rare monster appearance rates from server...\n");
		for (ch=0;ch<8;ch++)
		{
			mob_rate = *(unsigned *) &ship->decryptbuf[0x06 + (ch * 4)];
			mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
/*
			times_won = 0;
			for (ch2=0;ch2<1000000;ch2++)
			{
				if (mt_lrand() < mob_calc)
					times_won++;
			}
*/
			switch (ch)
			{
			case 0x00:
				printf ("Hildebear appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				hildebear_rate = (unsigned) mob_calc;
				break;
			case 0x01:
				printf ("Rappy appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				rappy_rate = (unsigned) mob_calc;
				break;
			case 0x02:
				printf ("Lily appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				lily_rate = (unsigned) mob_calc;
				break;
			case 0x03:
				printf ("Pouilly Slime appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				slime_rate = (unsigned) mob_calc;
				break;
			case 0x04:
				printf ("Merissa AA appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				merissa_rate = (unsigned) mob_calc;
				break;
			case 0x05:
				printf ("Pazuzu appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				pazuzu_rate = (unsigned) mob_calc;
				break;
			case 0x06:
				printf ("Dorphon Eclair appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				dorphon_rate = (unsigned) mob_calc;
				break;
			case 0x07:
				printf ("Kondrieu appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				kondrieu_rate = (unsigned) mob_calc;
				break;
			}
			//debug ("Actual rate: %3f%%\n", ((float) times_won / 1000000) * 100);
		}
		printf ("\nNow ready to serve players...\n");
		logon_ready = 1;
		break;
	case 0x11:
		// Ping received
		ship->last_ping = (unsigned) servertime;
		*(unsigned *) &ship->encryptbuf[0x00] = *(unsigned *) &ship->decryptbuf[0x04];
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x04 );
		break;
	case 0x12:
		// Global announce
		gcn = *(unsigned *) &ship->decryptbuf[0x06];
		GlobalBroadcast ((unsigned short*) &ship->decryptbuf[0x0A]);
		write_gm ("GM %u made a global announcement: %s", gcn, unicode_to_ascii ((unsigned short*) &ship->decryptbuf[0x0A]));
		break;
	default:
		// Unknown
		break;
	}
}

void compressShipPacket ( PSO_SERVER* ship, unsigned char* src, unsigned long src_size )
{
	unsigned char* dest;
	unsigned long result;

	if (ship->sockfd >= 0)
	{
		if (PACKET_BUFFER_SIZE - ship->snddata < (int) ( src_size + 100 ) )
			initialize_logon();
		else
		{
			dest = &ship->sndbuf[ship->snddata];
			// Store the original packet size before RLE compression at offset 0x04 of the new packet.
			dest += 4;
			*(unsigned *) dest = src_size;
			// Compress packet using RLE, storing at offset 0x08 of new packet.
			//
			// result = size of RLE compressed data + a DWORD for the original packet size.
			result = RleEncode (src, dest+4, src_size) + 4;
			// Encrypt with RC4
			rc4 (dest, result, &ship->sc_key);
			// Increase result by the size of a DWORD for the final ship packet size.
			result += 4;
			// Copy it to the front of the packet.
			*(unsigned *) &ship->sndbuf[ship->snddata] = result;
			ship->snddata += (int) result;
		}
	}
}

void decompressShipPacket ( PSO_SERVER* ship, unsigned char* dest, unsigned char* src )
{
	unsigned src_size, dest_size;
	unsigned char *srccpy;

	if (ship->crypt_on)
	{
		src_size = *(unsigned *) src;
		src_size -= 8;
		src += 4;
		srccpy = src;
		// Decrypt RC4
		rc4 (src, src_size+4, &ship->cs_key);
		// The first four bytes of the src should now contain the expected uncompressed data size.
		dest_size = *(unsigned *) srccpy;
		// Increase expected size by 4 before inserting into the destination buffer.  (To take account for the packet
		// size DWORD...)
		dest_size += 4;
		*(unsigned *) dest = dest_size;
		// Decompress the data...
		RleDecode (srccpy+4, dest+4, src_size);
	}
	else
	{
		src_size = *(unsigned *) src;
		memcpy (dest + 4, src + 4, src_size);
		src_size += 4;
		*(unsigned *) dest = src_size;
	}
}

void encryptcopy (PSO_CLIENT* client, const unsigned char* src, unsigned int size)
{
	unsigned char* dest;

	// Bad pointer check...
	if ( ((unsigned) client < (unsigned)connections[0]) || 
		 ((unsigned) client > (unsigned)connections[serverMaxConnections-1]) )
		return;
	if (TCP_BUFFER_SIZE - client->snddata < ( (int) size + 7 ) )
		client->todc = 1;
	else
	{
		dest = &client->sndbuf[client->snddata];
		memcpy (dest,src,size);
		while (size % 8)
			dest[size++] = 0x00;
		client->snddata += (int) size;
		pso_crypt_encrypt_bb(cipher_ptr,dest,size);
	}
}

void start_encryption (PSO_CLIENT* connect)
{
	unsigned c, c3, c4, connectNum;
	PSO_CLIENT *workConnect, *c5;

	// Limit the number of connections from an IP address to MAX_SIMULTANEOUS_CONNECTIONS.

	c3 = 0;

	for (c=0;c<serverNumConnections;c++)
	{
		connectNum = serverConnectionList[c];
		workConnect = connections[connectNum];
		//debug ("%s comparing to %s", (char*) &workConnect->IP_Address[0], (char*) &connect->IP_Address[0]);
		if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
			(workConnect->plySockfd >= 0))
			c3++;
	}

	//debug ("Matching count: %u", c3);

	if (c3 > MAX_SIMULTANEOUS_CONNECTIONS)
	{
		// More than MAX_SIMULTANEOUS_CONNECTIONS connections from a certain IP address...
		// Delete oldest connection to server.
		c4 = 0xFFFFFFFF;
		c5 = NULL;
		for (c=0;c<serverNumConnections;c++)
		{
			connectNum = serverConnectionList[c];
			workConnect = connections[connectNum];
			if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
				(workConnect->plySockfd >= 0))
			{
				if (workConnect->connected < c4)
				{
					c4 = workConnect->connected;
					c5 = workConnect;
				}
			}
		}
		if (c5)
		{
			workConnect = c5;
			initialize_connection (workConnect);
		}
	}

	memcpy (&connect->sndbuf[0], &Packet03[0], sizeof (Packet03));
	for (c=0;c<0x30;c++)
	{
		connect->sndbuf[0x68+c] = (unsigned char) mt_lrand() % 255;
		connect->sndbuf[0x98+c] = (unsigned char) mt_lrand() % 255;
	}
	connect->snddata += sizeof (Packet03);
	cipher_ptr = &connect->server_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x68]);
	cipher_ptr = &connect->client_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x98]);
	connect->crypt_on = 1;
	connect->sendCheck[SEND_PACKET_03] = 1;
	connect->connected = connect->response = connect->savetime = (unsigned) servertime;
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
		if ((l->slot_use[ch]) && (l->clients[ch]))
			SendEE ("Lobby inventory problem!  It's advised you quit this game and recreate it.", &(l->clients[ch]));
	}

	return 0;
}

void GenerateCommonItem (int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client)
{
	unsigned int ch, num_percents, item_set, meseta, do_area, r, eq_type;
	unsigned short ch2;
	PTDATA* ptd;
	unsigned int area,fl;
	unsigned int did_area[6] = {0};
	unsigned char percent;

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
				// LOOK at this later -- percent used to be signed
				i->item.data[6+(num_percents*2)+1] = percent;
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

void feed_mag (unsigned int magid, unsigned int itemid, PSO_CLIENT* client)
{
	int found_mag = -1;
	int found_item = -1;
	unsigned ch, ch2, mt_index;
	int EvolutionClass = 0;
	MAG* m;
	unsigned short* ft;
	short mIQ, mDefense, mPower, mDex, mMind;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == magid)
		{
			// Found mag
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
			{
				found_mag = ch;
				m = (MAG*) &client->character.inventory[ch].item.data[0];
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
				{
					if (client->character.inventory[ch2].item.itemid == itemid)
					{
						// Found item to feed
						if  (( client->character.inventory[ch2].item.data[0] == 0x03 ) &&
							( client->character.inventory[ch2].item.data[1]  < 0x07 ) &&
							( client->character.inventory[ch2].item.data[1] != 0x02 ) &&
							( client->character.inventory[ch2].item.data[5] >  0x00 ))
						{
							found_item = ch2;
							switch (client->character.inventory[ch2].item.data[1])
							{
							case 0x00:
								mt_index = client->character.inventory[ch2].item.data[2];
								break;
							case 0x01:
								mt_index = 3 + client->character.inventory[ch2].item.data[2];
								break;
							case 0x03:
							case 0x04:
							case 0x05:
								mt_index = 5 + client->character.inventory[ch2].item.data[1];
								break;
							case 0x06:
								mt_index = 6 + client->character.inventory[ch2].item.data[2];
								break;
							}
						}
						break;
					}
				}
			}
			break;
		}
	}

	if ( ( found_mag == -1 ) || ( found_item == -1 ) )
	{
		Send1A ("Could not find mag to feed or item to feed said mag.", client);
		client->todc = 1;
	}
	else
	{
		DeleteItemFromClient ( itemid, 1, 0, client );

		// Rescan to update Mag pointer (if changed due to clean up)
		for (ch=0;ch<client->character.inventoryUse;ch++)
		{
			if (client->character.inventory[ch].item.itemid == magid)
			{
				// Found mag (again)
				if ((client->character.inventory[ch].item.data[0] == 0x02) &&
					(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
				{
					found_mag = ch;
					m = (MAG*) &client->character.inventory[ch].item.data[0];
					break;
				}
			}
		}
			
		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (m->mtype)
		{
		case Mag_Mag:
			ft = &Feed_Table0[0];
			EvolutionClass = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			EvolutionClass = 1;
			ft = &Feed_Table1[0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &Feed_Table2[0];
			EvolutionClass = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &Feed_Table3[0];
			EvolutionClass = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			EvolutionClass = 3;
			ft = &Feed_Table4[0];
			break;
		case Mag_Ila:
		case Mag_Garuda:
		case Mag_Sita:
		case Mag_Soma:
		case Mag_Durga:
		case Mag_Nandin:
		case Mag_Yaksa:
		case Mag_Ribhava:
			EvolutionClass = 3;
			ft = &Feed_Table5[0];
			break;
		case Mag_Andhaka:
		case Mag_Kabanda:
		case Mag_Naga:
		case Mag_Naraka:
		case Mag_Bana:
		case Mag_Marica:
		case Mag_Madhu:
		case Mag_Ravana:
			EvolutionClass = 3;
			ft = &Feed_Table6[0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &Feed_Table5[0];
			EvolutionClass = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &Feed_Table6[0];
			EvolutionClass = 4;
			break;
		default:
			ft = &Feed_Table7[0];
			EvolutionClass = 4;
			break;
		}
		mt_index *= 6;
		m->synchro += ft[mt_index];
		if (m->synchro < 0)
			m->synchro = 0;
		if (m->synchro > 120)
			m->synchro = 120;
		mIQ = m->IQ;
		mIQ += ft[mt_index+1];
		if (mIQ < 0)
			mIQ = 0;
		if (mIQ > 200)
			mIQ = 200;
		m->IQ = (unsigned char) mIQ;

		// Add Defense

		mDefense  = m->defense % 100;
		mDefense += ft[mt_index+2];

		if ( mDefense < 0 )
			mDefense = 0;

		if ( mDefense >= 100 )
		{
			if ( m->level == 200 )
				mDefense = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->defense  = ( ( m->defense / 100 ) * 100 ) + mDefense;
			check_mag_evolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->defense  = ( ( m->defense / 100 ) * 100 ) + mDefense;

		// Add Power

		mPower  = m->power % 100;
		mPower += ft[mt_index+3];

		if ( mPower < 0 )
			mPower = 0;

		if ( mPower >= 100 )
		{
			if ( m->level == 200 )
				mPower = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->power  = ( ( m->power / 100 ) * 100 ) + mPower;
			check_mag_evolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->power  = ( ( m->power / 100 ) * 100 ) + mPower;

		// Add Dex

		mDex  = m->dex % 100;
		mDex += ft[mt_index+4];

		if ( mDex < 0 )
			mDex = 0;

		if ( mDex >= 100 )
		{
			if ( m->level == 200 )
				mDex = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->dex  = ( ( m->dex / 100 ) * 100 ) + mDex;
			check_mag_evolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->dex  = ( ( m->dex / 100 ) * 100 ) + mDex;

		// Add Mind

		mMind  = m->mind % 100;
		mMind += ft[mt_index+5];

		if ( mMind < 0 )
			mMind = 0;

		if ( mMind >= 100 )
		{
			if ( m->level == 200 )
				mMind = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->mind  = ( ( m->mind / 100 ) * 100 ) + mMind;
			check_mag_evolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->mind  = ( ( m->mind / 100 ) * 100 ) + mMind;
	}
}

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

void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned int size)
{
	memcpy (dest,src,size);
	pso_crypt_decrypt_bb(cipher_ptr,dest,size);
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

#include "team.h"
#include "load.h"
#include "commands.h"
// end of functions

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

	dp[0] = 0;	// clears dp, so the following can be put into it

	strcat (dp, "Ryuker Ship Server version ");
	strcat (dp, SERVER_VERSION );
	strcat (dp, " coded by gatchipatchi");
	SetConsoleTitle (dp);	// after this, dp goes back to being a general buffer

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
		gets (dp);	// see, whaddaye say
		exit(1);
	}
#endif
	mt_bestseed();
	load_config_file();
	printf ("OK!\n\n");

	printf ("Loading language file...\n");

	load_language_file(languageNames, languageExts, numLanguages);

	printf ("OK!\n\n");

	printf ("Loading ship_key.bin... ");

	fp = fopen ("ship_key.bin", "rb" );
	if (!fp)
	{
		printf ("Could not locate ship_key.bin!\n");
		printf ("Hit [ENTER] to quit...");
		gets (dp);
		exit (1);
	}

	fread (&ship_index, 1, 4, fp );
	fread (&ship_key[0], 1, 128, fp );
	fclose (fp);

	printf ("OK!\n\nLoading weapon parameter file...\n");
	load_weapon_param(weapon_equip_table, grind_table, special_table, weapon_atpmax_table);
	printf ("\n.. done!\n\n");

	printf ("Loading armor & barrier parameter file...\n");
	load_armor_param ();
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
	printf ("Weapon drop rate: %03f%%\n", (float) weapon_drop_rate / 1000);
	printf ("Armor drop rate: %03f%%\n", (float) armor_drop_rate / 1000);
	printf ("Mag drop rate: %03f%%\n", (float) mag_drop_rate / 1000);
	printf ("Tool drop rate: %03f%%\n", (float) tool_drop_rate / 1000);
	printf ("Meseta drop rate: %03f%%\n", (float) meseta_drop_rate / 1000);
	printf ("Experience rate: %u%%\n\n", experience_rate * 100);

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
		gets (dp);
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
		gets (dp);
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
		gets (dp);
		exit (1);
	}

	if ( fread ( &shops[0], 1, 7000 * sizeof (SHOP), fp )  != (7000 * sizeof (SHOP)) )
	{
		printf ("Failed to read shop data...\n");
		printf ("Press [ENTER] to quit...");
		gets (dp);
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

