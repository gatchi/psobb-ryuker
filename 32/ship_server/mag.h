/* Mag Structure */
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

/* Function Definitons */
int mag_alignment ( MAG* m );
int mag_special_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl50_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl35_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void mag_lvl10_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
void check_mag_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass );
