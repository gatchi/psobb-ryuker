#define NO_ALIGN

#include "items.h"

/* Battle parameter structure */
typedef struct NO_ALIGN st_battleparam {
	unsigned short ATP;
	unsigned short MST;
	unsigned short EVP;
	unsigned short HP;
	unsigned short DFP;
	unsigned short ATA;
	unsigned short LCK;
	unsigned short ESP;
	unsigned reserved1;
	unsigned reserved2;
	unsigned reserved3;
	unsigned XP;
	unsigned reserved4;
} BATTLEPARAM;

/* Function Prototypes */
void load_battle_param (BATTLEPARAM* dest, const char* filename, unsigned num_records, long expected_checksum);

void load_armor_param ();

void LoadCSV(const char* filename);
void FreeCSV();

/* Main Character Structure */
typedef struct NO_ALIGN st_chardata {
	unsigned short packetSize; // 0x00-0x01  // Always set to 0x399C
	unsigned short command; // 0x02-0x03 // // Always set to 0x00E7
	unsigned char flags[4]; // 0x04-0x07
	unsigned char inventoryUse; // 0x08
	unsigned char HPmat; // 0x09
	unsigned char TPmat; // 0x0A
	unsigned char lang; // 0x0B
	INVENTORY_ITEM inventory[30]; // 0x0C-0x353
	unsigned short ATP; // 0x354-0x355
	unsigned short MST; // 0x356-0x357
	unsigned short EVP; // 0x358-0x359
	unsigned short HP; // 0x35A-0x35B
	unsigned short DFP; // 0x35C-0x35D
	unsigned short ATA; // 0x35E-0x35F
	unsigned short LCK; // 0x360-0x361
	unsigned char unknown[10]; // 0x362-0x36B
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
	unsigned playTime; // 0x3E0 - 0x3E3;
	unsigned char unknown6[4];  // 0x3E4 - 0x3E7
	unsigned char keyConfig[232]; // 0x3E8 - 0x4CF;
	// Stored from ED 07 packet.
	unsigned char techniques[20]; // 0x4D0 - 0x4E3;
	unsigned char unknown7[16]; // 0x4E4 - 0x4F3;
	unsigned char options[4]; // 0x4F4-0x4F7;
	// Stored from ED 01 packet.
	unsigned reserved4; // not sure
	unsigned char quest_data1[512]; // 0x4FC - 0x6FB; (Quest data 1)
	unsigned reserved5;
	unsigned bankUse; // 0x700 - 0x703
	unsigned bankMeseta; // 0x704 - 0x707;
	BANK_ITEM bankInventory [200]; // 0x708 - 0x19C7
	unsigned guildCard; // 0x19C8-0x19CB;
	// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3;
	unsigned char unknown9[56]; // 0x19E4-0x1A1B;
	unsigned char guildcard_text[176]; // 0x1A1C - 0x1ACB
	unsigned char reserved1;  // 0x1ACC; // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE;
	unsigned char _class2; // 0x1ACF;
	unsigned char unknown10[4]; // 0x1AD0-0x1AD3;
	unsigned char symbol_chats[1248];	// 0x1AD4 - 0x1FB3
	// Stored from ED 02 packet.
	unsigned char shortcuts[2624];	// 0x1FB4 - 0x29F3
	// Stored from ED 03 packet.
	unsigned char autoReply[344]; // 0x29F4 - 0x2B4B;
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7;
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF;
	unsigned char challengeData[320]; // 0x2CC0 - 0X2DFF
	unsigned char techConfig[40]; // 0x2E00 - 0x2E27
	unsigned char unknown13[40]; // 0x2E28-0x2E4F
	unsigned char quest_data2[92]; // 0x2E50 - 0x2EAB (Quest data 2)
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


/* Weapon pmt structure */

typedef struct NO_ALIGN st_weappmt
{
	// Starts @ 0x4348
	unsigned index; 
	short model; 
	short skin;
	short unknown1;
	short unknown2;
	unsigned short equippable;
	short atpmin;
	short atpmax;
	short atpreq;
	short mstreq;
	short atareq;
	short mstadd;
	unsigned char grind;
	unsigned char photon_color;
	unsigned char special_type;
	unsigned char ataadd;
	unsigned char unknown4[14];
} weappmt;


/* Armor pmt structure */
typedef struct NO_ALIGN st_armorpmt
{
	// Starts @ 0x40 with barriers (Barrier and armor share the same structure...)
	// Armors start @ 0x14f0
	unsigned index;
	short model;
	short skin;
	short u1;
	short u2;
	short dfp;
	short evp;
	short u3;
	unsigned short equippable;
	unsigned char level;
	unsigned char efr;
	unsigned char eth;
	unsigned char eic;
	unsigned char edk;
	unsigned char elt;
	unsigned char dfp_var;
	unsigned char evp_var;
	short u4;
	short u5;
} armorpmt;


/* Character Data Structure */
typedef struct NO_ALIGN st_playerLevel {
	unsigned char ATP; 
	unsigned char MST; 
	unsigned char EVP; 
	unsigned char HP; 
	unsigned char DFP; 
	unsigned char ATA; 
	unsigned char LCK;
	unsigned char TP;
	unsigned XP;
} playerLevel;


/* Internal Monster Structure */

typedef struct st_monster {
	short HP;
	unsigned dead[4];
	unsigned drop;
} MONSTER;

/* What dis */
typedef struct NO_ALIGN st_ptdata
{
	unsigned char weapon_ratio[12]; // high = 0x0D
	char weapon_minrank[12];
	unsigned char weapon_reserved[12]; // ??
	unsigned char power_pattern[9][4];
	unsigned short percent_pattern[23][6];
	unsigned char area_pattern[30];
	unsigned char percent_attachment[6][10];
	unsigned char element_ranking[10];
	unsigned char element_probability[10];
	unsigned char armor_ranking[5];
	unsigned char slot_ranking[5];
	unsigned char unit_level[10];
	unsigned short tool_frequency[28][10];
	unsigned char tech_frequency[19][10];
	char tech_levels[19][20];
	unsigned char enemy_dar[100];
	unsigned short enemy_meseta[100][2];
	char enemy_drop[100];
	unsigned short box_meseta[10][2];
	unsigned char reserved[0x1000-0x8C8];
} PTDATA;


/* Map Monster Structure */

typedef struct NO_ALIGN st_mapmonster {
	unsigned base;	// 4
	unsigned reserved[11]; // 44
	float reserved11; // 4
	float reserved12; // 4
	unsigned reserved13; // 4
	unsigned exp; // 4
	unsigned skin; // 4
	unsigned rt_index;  // 4
} MAP_MONSTER;


/* Map box structure */

typedef struct st_mapbox {
	float flag1;
	float flag2;
	float flag3;
	unsigned char drop[8];
} MAP_BOX;
