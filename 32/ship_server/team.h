void CreateTeam (unsigned short* teamname, unsigned guildcard, PSO_SERVER* ship);
void UpdateTeamFlag (unsigned char* flag, unsigned teamid, PSO_SERVER* ship);
void DissolveTeam (unsigned teamid, PSO_SERVER* ship);
void RemoveTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );
void RequestTeamList ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );
void PromoteTeamMember ( unsigned teamid, unsigned guildcard, unsigned char newlevel, PSO_SERVER* ship );
void AddTeamMember ( unsigned teamid, unsigned guildcard, PSO_SERVER* ship );