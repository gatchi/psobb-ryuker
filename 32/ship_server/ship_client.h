/* Connected Client Structure */

typedef struct st_pso_client {
	int plySockfd;
	int block;
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned short rcvread;
	unsigned short expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE]; // Used when decrypting packets from the client...
	unsigned char sndbuf [TCP_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE]; // Used when making packets to send to the client...
	unsigned char packet [TCP_BUFFER_SIZE];
	int snddata, sndwritten;
	int crypt_on;
	PSO_CRYPT server_cipher, client_cipher;
	CHARDATA character;
	unsigned char equip_flags;
	unsigned matuse[5];
	int mode; // Usually set to 0, but changes during challenge and battle play
	void* character_backup; // regular character copied here during challenge and battle
	int gotchardata;
	unsigned int guildcard;
	unsigned int temp_guildcard;
	long long hwinfo;
	int isgm;
	int slotnum;
	unsigned response;		// Last time client responded...
	unsigned lastTick;		// The last second
	unsigned toBytesSec;	// How many bytes per second the server sends to the client
	unsigned fromBytesSec;	// How many bytes per second the server receives from the client
	unsigned packetsSec;	// How many packets per second the server receives from the client
	unsigned char sendCheck[MAX_SENDCHECK+2];
	unsigned char preferred_lobby;
	unsigned short lobbyNum;
	unsigned char clientID;
	int bursting;
	int teamaccept;
	int masterxfer;
	int todc;
	unsigned dc_time;
	unsigned char IP_Address[16]; // Text version
	unsigned char ipaddr[4]; // Binary version
	unsigned connected;
	unsigned savetime;
	unsigned connection_index;
	unsigned drop_area;
	long long drop_coords;
	unsigned drop_item;
	int released;
	unsigned char releaseIP[4];
	unsigned short releasePort;
	int sending_quest;
	unsigned qpos;
	int hasquest;
	int doneshop[3];
	int dead;
	int lobbyOK;
	unsigned ignore_list[100];
	unsigned ignore_count;
	INVENTORY_ITEM tekked;
	unsigned team_info_flag, team_info_request;
	unsigned command_cooldown[256];
	unsigned team_cooldown[32];
	int bankType;
	int bankAccess;
	BANK common_bank;
	BANK char_bank;
	void* lobby;
	int announce;
	int debugged;
} PSO_CLIENT;