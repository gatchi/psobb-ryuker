int mag_alignment ( MAG* m );
int mag_special_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl50_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl35_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl10_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void check_mag_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void feed_mag (unsigned int magid, unsigned int itemid, PSO_CLIENT* client);
