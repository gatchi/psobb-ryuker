#include "stdafx.h"
#define PRS_BUFFER 262144

/* Assembled Quest Menu Structure */

typedef struct st_questmenu {
	unsigned num_categories;
	unsigned char c_names[10][256];
	unsigned char c_desc[10][256];
	unsigned quest_counts[10];
	unsigned quest_indexes[10][32];
} QUEST_MENU;


/* Quest Details Structure */

typedef struct st_qdetails {
	unsigned short qname[32];
	unsigned short qsummary[128];
	unsigned short qdetails[256];
	unsigned char* qdata;
	unsigned qsize;
} QDETAILS;	

/* Loaded Quest Structure */

typedef struct st_quest {
	QDETAILS* ql[10];  // Supporting 10 languages
	unsigned char* mapdata;
	unsigned max_objects;
	unsigned char* objectdata;
} QUEST;

/* Function Definitions */
void load_quests (const char* filename, unsigned int category, QUEST_MENU* quest_menus, unsigned int numLanguages, QUEST* quests, unsigned int numQuests, unsigned int questsMemory);
