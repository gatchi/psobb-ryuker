#pragma once
#define MAX_SAVED_ITEMS 3000
#define NO_ALIGN


/* Item Structure (Without Flags) */
typedef struct NO_ALIGN st_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
} ITEM;

/* Item Structure (Includes Flags) */
typedef struct NO_ALIGN st_inventory_item {
	unsigned char in_use; // 0x01 = item slot in use, 0xFF00 = unused
	unsigned char reserved[3];
	unsigned flags; // 8 = equipped
	ITEM item;
} INVENTORY_ITEM;


/* Game Inventory Item Structure */
typedef struct NO_ALIGN st_game_item {
	unsigned gm_flag; // reserved
	ITEM item;
} GAME_ITEM;


/* Bank Item Structure */
typedef struct NO_ALIGN st_bank_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
	unsigned bank_count; // Why?
} BANK_ITEM;


/* Bank Structure */
typedef struct NO_ALIGN st_bank {
	unsigned bankUse;
	unsigned bankMeseta;
	BANK_ITEM bankInventory [200];
} BANK;

/* Shop Item Structure */
typedef struct NO_ALIGN st_shopitem {
	unsigned char data[12];
	unsigned reserved3;
	unsigned price;
} SHOP_ITEM;


/* Shop Structure */
typedef struct NO_ALIGN st_shop {
	unsigned short packet_length;
	unsigned short command;
	unsigned flags;
	unsigned reserved;
	unsigned char type;
	unsigned char num_items;
	unsigned short reserved2;
	SHOP_ITEM item[0x18];
	unsigned char reserved4[16];
} SHOP;


/* Function Prototypes */
void FixItem (ITEM* i, unsigned char* whatIsTHisTable, unsigned char* andThisOne,  unsigned char* andThis, unsigned char* alsoThis);
void CheckMaxGrind (INVENTORY_ITEM* i, unsigned char** grind_table);
unsigned int GetShopPrice(INVENTORY_ITEM* ci, unsigned short** weapon_atpmax_table, unsigned char* armor_dfpvar_table, unsigned char* armor_evpvar_table, unsigned int**** equip_prices, unsigned char* barrier_dfpvar_table, unsigned char* barrier_evpvar_table);
int check_equip (unsigned char eq_flags, unsigned char cl_flags);
unsigned long ExpandDropRate(unsigned char pc);