/************************************************************************
	Tethealla Login Server
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


// Notes:
//
// - Limit to 40 guild cards for now.
//

#define NO_SQL
#define NO_CONNECT_TEST

#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define LOGIN_COMPILED_MAX_CONNECTIONS 300
#define SHIP_COMPILED_MAX_CONNECTIONS 50
#define MAX_EB02 800000
#define SERVER_VERSION "0.048"
#define MAX_ACCOUNTS 2000
#define MAX_DRESS_FLAGS 500
#define DRESS_FLAG_EXPIRY 7200

const char *PSO_CLIENT_VER_STRING = "TethVer12510";
#define PSO_CLIENT_VER 0x41

//#define USEADDR_ANY
#define DEBUG_OUTPUT
#define TCP_BUFFER_SIZE 64000
#define PACKET_BUFFER_SIZE ( TCP_BUFFER_SIZE * 16 )

#define SEND_PACKET_03 0x00 // Done
#define SEND_PACKET_E6 0x01 // Done
#define SEND_PACKET_E2 0x02 // Done
#define SEND_PACKET_E5 0x03 // Done
#define SEND_PACKET_E8 0x04 // Done
#define SEND_PACKET_DC 0x05 // Done
#define SEND_PACKET_EB 0x06 // Done
#define SEND_PACKET_E4 0x07 // Done
#define SEND_PACKET_B1 0x08
#define SEND_PACKET_A0 0x09
#define RECEIVE_PACKET_93 0x0A
#define MAX_SENDCHECK 0x0B

#define CLASS_HUMAR 0x00
#define CLASS_HUNEWEARL 0x01
#define CLASS_HUCAST 0x02
#define CLASS_RAMAR 0x03
#define CLASS_RACAST 0x04
#define CLASS_RACASEAL 0x05
#define CLASS_FOMARL 0x06
#define CLASS_FONEWM 0x07
#define CLASS_FONEWEARL 0x08
#define CLASS_HUCASEAL 0x09
#define CLASS_FOMAR 0x0A
#define CLASS_RAMARL 0x0B
#define CLASS_MAX 0x0C

struct timeval select_timeout = { 
	0, 
	5000
};

/* functions */

void send_to_server(int sock, char* packet);
int receive_from_server(int sock, char* packet);
void debug(char *fmt, ...);
void debug_perror(char * msg);
void tcp_listen (int sockfd);
int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len );
int tcp_sock_connect(char* dest_addr, int port);
int tcp_sock_open(struct in_addr ip, int port);

/* Ship Packets */

unsigned char ShipPacket00[] = {
	0x00, 0x00, 0x3F, 0x01, 0x03, 0x04, 0x19, 0x55, 0x54, 0x65, 0x74, 0x68, 0x65, 0x61, 0x6C, 0x6C,
	0x61, 0x20, 0x4C, 0x6F, 0x67, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Start Encryption Packet */

unsigned char Packet03[] = {
	0xC8, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x68, 0x61, 0x6E, 0x74, 0x61, 0x73, 0x79,
	0x20, 0x53, 0x74, 0x61, 0x72, 0x20, 0x4F, 0x6E, 0x6C, 0x69, 0x6E, 0x65, 0x20, 0x42, 0x6C, 0x75,
	0x65, 0x20, 0x42, 0x75, 0x72, 0x73, 0x74, 0x20, 0x47, 0x61, 0x6D, 0x65, 0x20, 0x53, 0x65, 0x72,
	0x76, 0x65, 0x72, 0x2E, 0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x31,
	0x39, 0x39, 0x39, 0x2D, 0x32, 0x30, 0x30, 0x34, 0x20, 0x53, 0x4F, 0x4E, 0x49, 0x43, 0x54, 0x45,
	0x41, 0x4D, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30
}; 

const unsigned char Message03[] = { "Tethealla Gate v.047" };


/* Server Redirect */

const unsigned char Packet19[] = {
	0x10, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


/* Login message */

const unsigned char Packet1A[] = {
	0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x45, 0x00 
};

/* Ping pong */

const unsigned char Packet1D[] = {
	0x08, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Current time */

const unsigned char PacketB1[] = {
	0x24, 0x00, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Guild Card Data */

unsigned char PacketDC01[0x14] = {0};
unsigned char PacketDC02[0x10] = {0};
unsigned char PacketDC_Check[54672] = {0};

/* No Character Header */

unsigned char PacketE4[0x10] = { 0 };


/* Character Header */

unsigned char PacketE5[0x88] = { 0 };

/* Security Packet */

const unsigned char PacketE6[] = {
0x44, 0x00, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x3F, 0x71, 0x8D, 0x34, 0x37, 0x7A, 0xBD,
0x67, 0x39, 0x65, 0x6B, 0x2C, 0xB1, 0xA5, 0x7C, 0x17, 0x93, 0x93, 0x29, 0x4A, 0x90, 0xE9, 0x11,
0xB8, 0xB5, 0x0E, 0x77, 0x41, 0x30, 0x9B, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x01, 0x00, 0x00
};

/* Acknowledging client's expected checksum */

const unsigned char PacketE8[] = {
	0x0C, 0x00, 0xE8, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

/* The "Top Broadcast" Packet */

const unsigned char PacketEE[] = { 
		0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

unsigned char E2_Base[2808] = { 0 };
unsigned char PacketE2Data[2808] = { 0 };

/* String sent to server to retrieve IP address. */

char* HTTP_REQ = "GET http://www.pioneer2.net/remote.php HTTP/1.0\r\n\r\n\r\n";

/* Populated by load_config_file(): */

char mySQL_Host[255] = {0};
char mySQL_Username[255] = {0};
char mySQL_Password[255] = {0};
char mySQL_Database[255] = {0};
unsigned int mySQL_Port;
unsigned char serverIP[4];
unsigned short serverPort;
int override_on = 0;
unsigned char overrideIP[4];
unsigned short serverMaxConnections;
unsigned short serverMaxShips;
unsigned serverNumConnections = 0;
unsigned serverConnectionList[LOGIN_COMPILED_MAX_CONNECTIONS];
unsigned serverNumShips = 0;
unsigned serverShipList[SHIP_COMPILED_MAX_CONNECTIONS];
unsigned quest_numallows;
unsigned* quest_allow;
unsigned max_ship_keys = 0;

/* Rare table structure */

unsigned rt_tables_ep1[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep2[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep4[0x200 * 10 * 4] = {0};

unsigned mob_rate[8]; // rare appearance rate

char Welcome_Message[255] = {0};
time_t servertime;

#ifndef NO_SQL

MYSQL * myData;
char myQuery[0x10000] = {0};
MYSQL_ROW myRow ;
MYSQL_RES * myResult;

#endif

#define NO_ALIGN

/* Mag data structure */

typedef struct NO_ALIGN st_mag
{
	unsigned char two; // "02" =P
	unsigned char mtype;
	unsigned char level;
	unsigned char blasts;
	short defense;
	short power;
	short dex;
	short mind;
	unsigned itemid;
	char synchro;
	unsigned char IQ;
	unsigned char PBflags;
	unsigned char color;
} MAG;


/* Character Data Structure */

typedef struct NO_ALIGN st_minichar {
	unsigned short packetSize; // 0x00 - 0x01
	unsigned short command; // 0x02 - 0x03
	unsigned char flags[4]; // 0x04 - 0x07
	unsigned char unknown[8]; // 0x08 - 0x0F
	unsigned short level; // 0x10 - 0x11
	unsigned short reserved; // 0x12 - 0x13
	char gcString[10]; // 0x14 - 0x1D
	unsigned char unknown2[14]; // 0x1E - 0x2B
	unsigned char nameColorBlue; // 0x2C
	unsigned char nameColorGreen; // 0x2D
	unsigned char nameColorRed; // 0x2E
	unsigned char nameColorTransparency; // 0x2F
	unsigned short skinID; // 0x30 - 0x31
	unsigned char unknown3[18]; // 0x32 - 0x43
	unsigned char sectionID; // 0x44
	unsigned char _class; // 0x45
	unsigned char skinFlag; // 0x46
	unsigned char unknown4[5]; // 0x47 - 0x4B (same as unknown5 in E7)
	unsigned short costume; // 0x4C - 0x4D
	unsigned short skin; // 0x4E - 0x4F
	unsigned short face; // 0x50 - 0x51
	unsigned short head; // 0x52 - 0x53
	unsigned short hair; // 0x54 - 0x55
	unsigned short hairColorRed; // 0x56 - 0x57
	unsigned short hairColorBlue; // 0x58 - 0x59
	unsigned short hairColorGreen; // 0x5A - 0x5B
	unsigned proportionX; // 0x5C - 0x5F
	unsigned proportionY; // 0x60 - 0x63
	unsigned char name[24]; // 0x64 - 0x7B
	unsigned char unknown5[8] ; // 0x7C - 0x83
	unsigned playTime;
} MINICHAR;

//packetSize = 0x399C;
//command = 0x00E7;

typedef struct NO_ALIGN st_bank_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
	unsigned bank_count; // Why?
} BANK_ITEM;

typedef struct NO_ALIGN st_bank {
	unsigned bankUse;
	unsigned bankMeseta;
	BANK_ITEM bankInventory [200];
} BANK;

typedef struct NO_ALIGN st_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
} ITEM;

typedef struct NO_ALIGN st_inventory {
	unsigned in_use; // 0x01 = item slot in use, 0xFF00 = unused
	unsigned flags; // 8 = equipped
	ITEM item;
} INVENTORY;

typedef struct NO_ALIGN st_chardata {
	unsigned short packetSize; // 0x00-0x01  // Always set to 0x399C
	unsigned short command; // 0x02-0x03 // // Always set to 0x00E7
	unsigned char flags[4]; // 0x04-0x07
	unsigned char inventoryUse; // 0x08
	unsigned char HPuse; // 0x09
	unsigned char TPuse; // 0x0A
	unsigned char lang; // 0x0B
	INVENTORY inventory[30]; // 0x0C-0x353
	unsigned short ATP; // 0x354-0x355
	unsigned short MST; // 0x356-0x357
	unsigned short EVP; // 0x358-0x359
	unsigned short HP; // 0x35A-0x35B
	unsigned short DFP; // 0x35C-0x35D
	unsigned short TP; // 0x35E-0x35F
	unsigned short LCK; // 0x360-0x361
	unsigned short ATA; // 0x362-0x363
	unsigned char unknown[8]; // 0x364-0x36B  (Offset 0x360 has 0x0A value on Schthack's...)
	unsigned short level; // 0x36C-0x36D;
	unsigned short unknown2; // 0x36E-0x36F;
	unsigned XP; // 0x370-0x373
	unsigned meseta; // 0x374-0x377;
	char gcString[10]; // 0x378-0x381;
	unsigned char unknown3[14]; // 0x382-0x38F;  // Same as E5 unknown2
	unsigned char nameColorBlue; // 0x390;
	unsigned char nameColorGreen; // 0x391;
	unsigned char nameColorRed; // 0x392;
	unsigned char nameColorTransparency; // 0x393;
	unsigned short skinID; // 0x394-0x395;
	unsigned char unknown4[18]; // 0x396-0x3A7
	unsigned char sectionID; // 0x3A8;
	unsigned char _class; // 0x3A9;
	unsigned char skinFlag; // 0x3AA;
	unsigned char unknown5[5]; // 0x3AB-0x3AF;  // Same as E5 unknown4.
	unsigned short costume; // 0x3B0 - 0x3B1;
	unsigned short skin; // 0x3B2 - 0x3B3;
	unsigned short face; // 0x3B4 - 0x3B5;
	unsigned short head; // 0x3B6 - 0x3B7;
	unsigned short hair; // 0x3B8 - 0x3B9;
	unsigned short hairColorRed; // 0x3BA-0x3BB;
	unsigned short hairColorBlue; // 0x3BC-0x3BD;
	unsigned short hairColorGreen; // 0x3BE-0x3BF;
	unsigned proportionX; // 0x3C0-0x3C3;
	unsigned proportionY; // 0x3C4-0x3C7;
	unsigned char name[24]; // 0x3C8-0x3DF;
	unsigned playTime; // 0x3E0 - 0x3E3
	unsigned char unknown6[4]; // 0x3E4 - 0x3E7;
	unsigned char keyConfig[232]; // 0x3E8 - 0x4CF;
								// Stored from ED 07 packet.
	unsigned char techniques[20]; // 0x4D0 - 0x4E3;
	unsigned char unknown7[16]; // 0x4E4 - 0x4F3;
	unsigned char options[4]; // 0x4F4-0x4F7;
								// Stored from ED 01 packet.
	unsigned char unknown8[520]; // 0x4F8 - 0x6FF;
	unsigned bankUse; // 0x700 - 0x703
	unsigned bankMeseta; // 0x704 - 0x707;
	BANK_ITEM bankInventory [200]; // 0x708 - 0x19C7
	unsigned guildCard; // 0x19C8-0x19CB;
						// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3;
	unsigned char unknown9[232]; // 0x19E4-0x1ACB;
	unsigned char reserved1;  // 0x1ACC; // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE;
	unsigned char _class2; // 0x1ACF;
	unsigned char unknown10[4]; // 0x1AD0-0x1AD3;
	unsigned char symbol_chats[1248];	// 0x1AD4 - 0x1FB3
										// Stored from ED 02 packet.
	unsigned char shortcuts[2624];	// 0x1FB4 - 0x29F3
									// Stored from ED 03 packet.
	unsigned char unknown11[344]; // 0x29F4 - 0x2B4B;
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7;
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF;
	unsigned char challengeData[320]; // 0x2CC0 - 0X2DFF
	unsigned char unknown13[172]; // 0x2E00 - 0x2EAB;
	unsigned char unknown14[276]; // 0x2EAC - 0x2FBF; // I don't know what this is, but split from unknown13 because this chunk is
													  // actually copied into the 0xE2 packet during login @ 0x08
	unsigned char keyConfigGlobal[364]; // 0x2FC0 - 0x312B  // Copied into 0xE2 login packet @ 0x11C
										// Stored from ED 04 packet.
	unsigned char joyConfigGlobal[56]; // 0x312C - 0x3163 // Copied into 0xE2 login packet @ 0x288
										// Stored from ED 05 packet.
	unsigned guildCard2; // 0x3164 - 0x3167 (From here on copied into 0xE2 login packet @ 0x2C0...)
	unsigned teamID; // 0x3168 - 0x316B
	unsigned char teamInformation[8]; // 0x316C - 0x3173 (usually blank...)
	unsigned short privilegeLevel; // 0x3174 - 0x3175
	unsigned short reserved3; // 0x3176 - 0x3177
	unsigned char teamName[28]; // 0x3178 - 0x3193
	unsigned unknown15; // 0x3194 - 0x3197
	unsigned char teamFlag[2048]; // 0x3198 - 0x3997
	unsigned char teamRewards[8]; // 0x3998 - 0x39A0
} CHARDATA;


/* Player Structure */

typedef struct st_pso_client {
	int plySockfd;
	int login;
	unsigned char peekbuf[8];
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned short rcvread;
	unsigned short expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE];
	unsigned char sndbuf [TCP_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE];
	int snddata, 
		sndwritten;
	unsigned char packet [TCP_BUFFER_SIZE];
	unsigned short packetdata;
	unsigned short packetread;
	int crypt_on;
	PSO_CRYPT server_cipher, client_cipher;
	unsigned guildcard;
	char guildcard_string[12];
	unsigned char guildcard_data[20000];
	int sendingchars;
	short slotnum;
	unsigned lastTick;		// The last second
	unsigned toBytesSec;	// How many bytes per second the server sends to the client
	unsigned fromBytesSec;	// How many bytes per second the server receives from the client
	unsigned packetsSec;	// How many packets per second the server receives from the client
	unsigned connected;
	unsigned char sendCheck[MAX_SENDCHECK+2];
	int todc;
	unsigned char IP_Address[16];
	char hwinfo[18];
	int isgm;
	int dress_flag;
	unsigned connection_index;
} PSO_CLIENT;

typedef struct st_dressflag {
	unsigned guildcard;
	unsigned flagtime;
} DRESSFLAG;

/* a RC4 expanded key session */
struct rc4_key {
    unsigned char state[256];
    unsigned x, y;
};

/* Ship Structure */

typedef struct st_pso_server {
	int shipSockfd;
	unsigned char name[13];
	unsigned playerCount;
	unsigned char shipAddr[5];
	unsigned char listenedAddr[4];
	unsigned short shipPort;
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned long rcvread;
	unsigned long expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE];
	unsigned char sndbuf [PACKET_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE];
	unsigned char packet [PACKET_BUFFER_SIZE];
	unsigned long packetread;
	unsigned long packetdata;
	int snddata, 
		sndwritten;
	unsigned shipID;
	int authed;
	int todc;
	int crypt_on;
	unsigned char user_key[128];
	int key_change[128];
	unsigned key_index;
	struct rc4_key cs_key; // Encryption keys
	struct rc4_key sc_key; // Encryption keys
	unsigned connection_index;
	unsigned connected;
	unsigned last_ping;
	int sent_ping;
} PSO_SERVER;

fd_set ReadFDs, WriteFDs, ExceptFDs;

DRESSFLAG dress_flags[MAX_DRESS_FLAGS];
unsigned char dp[TCP_BUFFER_SIZE*4];
unsigned char tmprcv[PACKET_BUFFER_SIZE];
char Packet1AData[TCP_BUFFER_SIZE];
char PacketEEData[TCP_BUFFER_SIZE];
unsigned char PacketEB01[0x4C8*2] = {0};
unsigned char PacketEB02[MAX_EB02] = {0};
unsigned PacketEB01_Total;
unsigned PacketEB02_Total;

unsigned keys_in_use [SHIP_COMPILED_MAX_CONNECTIONS+1] = { 0 };

#ifdef NO_SQL

typedef struct st_bank_file {
	unsigned guildcard;
	BANK common_bank;
} L_BANK_DATA;

typedef struct st_account {
	char username[18];
	char password[33];
	char email[255];
	unsigned regtime;
	char lastip[16];
	long long lasthwinfo;
	unsigned guildcard;
	int isgm;
	int isbanned;
	int islogged;
	int isactive;
	int teamid;
	int privlevel;
	unsigned char lastchar[24];
} L_ACCOUNT_DATA;


typedef struct st_character {
	unsigned guildcard;
	int slot;
	CHARDATA data;
	MINICHAR header;
} L_CHARACTER_DATA;


typedef struct st_guild {
	unsigned accountid;
	unsigned friendid;
	char friendname[24];
	char friendtext[176];
	unsigned short reserved;
	unsigned short sectionid;
	unsigned short pclass;
	unsigned short comment[45];
	int priority;	
} L_GUILD_DATA;


typedef struct st_hwbans {
	unsigned guildcard;
	long long hwinfo;
} L_HW_BANS;


typedef struct st_ipbans {
	unsigned char ipinfo;
} L_IP_BANS;


typedef struct st_key_data {
	unsigned guildcard;
	unsigned char controls[420];
} L_KEY_DATA;


typedef struct st_security_data {
	unsigned guildcard;
	unsigned thirtytwo;
	long long sixtyfour;
	int slotnum;
	int isgm;
} L_SECURITY_DATA;


typedef struct st_ship_data {
	unsigned char rc4key[128];
	unsigned idx;
} L_SHIP_DATA;


typedef struct st_team_data {
	unsigned short name[12];
	unsigned owner;
	unsigned char flag[2048];
	unsigned teamid;
} L_TEAM_DATA;

// Oh brother :D

L_BANK_DATA *bank_data[MAX_ACCOUNTS];
L_ACCOUNT_DATA *account_data[MAX_ACCOUNTS];
L_CHARACTER_DATA *character_data[MAX_ACCOUNTS*4];
L_GUILD_DATA *guild_data[MAX_ACCOUNTS*40];
L_HW_BANS *hw_bans[MAX_ACCOUNTS];
L_IP_BANS *ip_bans[MAX_ACCOUNTS];
L_KEY_DATA *key_data[MAX_ACCOUNTS];
L_SECURITY_DATA *security_data[MAX_ACCOUNTS];
L_SHIP_DATA *ship_data[SHIP_COMPILED_MAX_CONNECTIONS];
L_TEAM_DATA *team_data[MAX_ACCOUNTS];

unsigned num_accounts = 0;
unsigned num_characters = 0;
unsigned num_guilds = 0;
unsigned num_hwbans = 0;
unsigned num_ipbans = 0;
unsigned num_keydata = 0;
unsigned num_bankdata = 0;
unsigned num_security = 0;
unsigned num_shipkeys = 0;
unsigned num_teams = 0;
unsigned ds,ds2;
int ds_found, new_record, free_record;

#endif

BANK empty_bank;

/* encryption stuff */

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key);

PSO_CRYPT* cipher_ptr;

void encryptcopy (PSO_CLIENT* client, const unsigned char* src, unsigned size);
void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned size);
void compressShipPacket ( PSO_SERVER* ship, unsigned char* src, unsigned long src_size );
void decompressShipPacket ( PSO_SERVER* ship, unsigned char* dest, unsigned char* src );

#ifdef NO_SQL

void UpdateDataFile ( const char* filename, unsigned count, void* data, unsigned record_size, int new_record );
void DumpDataFile ( const char* filename, unsigned* count, void** data, unsigned record_size );

unsigned lastdump = 0;

#endif

#define MYWM_NOTIFYICON (WM_USER+2)
int program_hidden = 1;
HWND consoleHwnd;
HWND backupHwnd;

/* Ship start authentication */

const unsigned char RC4publicKey[32] = {
	103, 196, 247, 176, 71, 167, 89, 233, 200, 100, 044, 209, 190, 231, 83, 42,
	6, 95, 151, 28, 140, 243, 130, 61, 107, 234, 243, 172, 77, 24, 229, 156
};
