void SortClientItems (PSO_CLIENT* client);
void CleanUpBank (PSO_CLIENT* client);
void CleanUpInventory (PSO_CLIENT* client);
void CleanUpGameInventory (LOBBY* l);
unsigned int AddItemToClient (unsigned itemid, PSO_CLIENT* client);
void DeleteMesetaFromClient (unsigned count, unsigned drop, PSO_CLIENT* client);
void SendItemToEnd (unsigned itemid, PSO_CLIENT* client);
void DeleteItemFromClient (unsigned itemid, unsigned count, unsigned drop, PSO_CLIENT* client);
unsigned int WithdrawFromBank (unsigned itemid, unsigned count, PSO_CLIENT* client);
unsigned int AddToInventory (INVENTORY_ITEM* i, unsigned count, int shop, PSO_CLIENT* client);
void DeleteFromInventory (INVENTORY_ITEM* i, unsigned count, PSO_CLIENT* client);
void DepositIntoBank (unsigned itemid, unsigned count, PSO_CLIENT* client);
void SortBankItems (PSO_CLIENT* client);
void FixItem (ITEM* i );
unsigned int free_game_item (LOBBY* l);
void UpdateGameItem (PSO_CLIENT* client);
void CheckMaxGrind (INVENTORY_ITEM* i);
unsigned int GetShopPrice(INVENTORY_ITEM* ci);
void DeequipItem (unsigned int itemid, PSO_CLIENT* client);
void EquipItem (unsigned int itemid, PSO_CLIENT* client);
int check_equip (unsigned char eq_flags, unsigned char cl_flags);
void UseItem (unsigned int itemid, PSO_CLIENT* client);
void GenerateCommonItem (int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client);
void GenerateRandomAttributes (unsigned char sid, GAME_ITEM* i, LOBBY* l, PSO_CLIENT* client);
unsigned long ExpandDropRate(unsigned char pc);
void LoadShopData2();