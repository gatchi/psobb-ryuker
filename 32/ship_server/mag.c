#include "mag.h"
#include "def_tables.h"
#include "classes.h"

int mag_alignment ( MAG* m )
{
	int v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->power;
	v2 = m->dex;
	v1 = m->mind;
	if ( v2 < v3 )
	{
		if ( v1 < v3 )
			v4 = 8;
	}
	if ( v3 < v2 )
	{
		if ( v1 < v2 )
			v4 |= 0x10u;
	}
	if ( v2 < v1 )
	{
		if ( v3 < v1 )
			v4 |= 0x20u;
	}
	v6 = 0;
	v5 = v3;
	if ( v3 <= v2 )
		v5 = v2;
	if ( v5 <= v1 )
		v5 = v1;
	if ( v5 == v3 )
		v6 = 1;
	if ( v5 == v2 )
		++v6;
	if ( v5 == v1 )
		++v6;
	if ( v6 >= 2 )
		v4 |= 0x100u;
	return v4;
}

int mag_special_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	unsigned char oldType;
	short mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->defense / 100;
		mPower = m->power / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch ( sectionID )
		{
		case ID_Viridia:
		case ID_Bluefull:
		case ID_Redria:
		case ID_Whitill:
			if ( ( mDefense + mDex ) == ( mPower + mMind ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Skyly:
		case ID_Pinkal:
		case ID_Yellowboze:
			if ( ( mDefense + mPower ) == ( mDex + mMind ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Greennill:
		case ID_Oran:
		case ID_Purplenum:
			if ( ( mDefense + mMind ) == ( mPower + mDex ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int)(oldType != m->mtype);
}

void mag_lvl50_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	int v10, v11, v12, v13;

	int Alignment = mag_alignment ( m );

	if ( EvolutionClass > 3 ) // Don't bother to check if a special mag.
		return;

	v10 = m->power / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->defense / 100;

	switch ( type )
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if ( Alignment & 0x108 )
		{
			if ( sectionID & 1 )
			{
				if ( v12 > v11 )
				{
					m->mtype = Mag_Apsaras;
					AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
			}
			else
			{
				if ( v12 > v11 )
				{
					m->mtype = Mag_Bhirava;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				}
			}
		}
		else
		{
			if ( Alignment & 0x10 )
			{
				if ( sectionID & 1 )
				{
					if ( v10 > v12 )
					{
						m->mtype = Mag_Garuda;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						AddPB ( &m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if ( v10 > v12 )
					{
						m->mtype = Mag_Ila;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					if ( sectionID & 1 )
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Soma;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
					}
					else
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Ushasu;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if ( Alignment & 0x110 )
		{
			if ( sectionID & 1 )
			{
				if ( v10 > v12 )
				{
					m->mtype = Mag_Kaitabha;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				}
			}
			else
			{
				if ( v10 > v12 )
				{
					m->mtype = Mag_Bhirava;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				if ( sectionID & 1 )
				{
					if ( v12 > v11 )
					{
						m->mtype = Mag_Kaitabha;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if ( v12 > v11 )
					{
						m->mtype = Mag_Bhirava;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					if ( sectionID & 1 )
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Durga;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Apsaras;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if ( Alignment & 0x120 )
		{
			if ( v13 > 44 )
			{
				m->mtype = Mag_Bana;
				AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
			}
			else
			{
				if ( sectionID & 1 )
				{
					if ( v11 > v10 )
					{
						m->mtype = Mag_Ila;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						AddPB ( &m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if ( v11 > v10 )
					{
						m->mtype = Mag_Kabanda;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
			}
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				if ( v13 > 44 )
				{
					m->mtype = Mag_Andhaka;
					AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					if ( sectionID & 1 )
					{
						if ( v12 > v11 )
						{
							m->mtype = Mag_Naga;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
						}
					}
					else
					{
						if ( v12 > v11 )
						{
							m->mtype = Mag_Ravana;
							AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
			else
			{
				if ( Alignment & 0x10 )
				{
					if ( v13 > 44 )
					{
						m->mtype = Mag_Bana;
						AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
					}
					else
					{
						if ( sectionID & 1 )
						{
							if ( v10 > v12 )
							{
								m->mtype = Mag_Garuda;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
						}
						else
						{
							if ( v10 > v12 )
							{
								m->mtype = Mag_Ribhava;
								AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

void mag_lvl35_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	int Alignment = mag_alignment ( m );

	if ( EvolutionClass > 3 ) // Don't bother to check if a special mag.
		return;

	switch ( type )
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if ( Alignment & 0x108 )
		{
			m->mtype = Mag_Rudra;
			AddPB ( &m->PBflags, &m->blasts, PB_Golla);
			return;
		}
		else
		{
			if ( Alignment & 0x10 )
			{
				m->mtype = Mag_Marutah;
				AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				return;
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					m->mtype = Mag_Vayu;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if ( Alignment & 0x110 )
		{
			m->mtype = Mag_Mitra;
			AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
			return;
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				m->mtype = Mag_Surya;
				AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					m->mtype = Mag_Tapas;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if ( Alignment & 0x120 )
		{
			m->mtype = Mag_Namuci;
			AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
			return;
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				m->mtype = Mag_Sumba;
				AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if ( Alignment & 0x10 )
				{
					m->mtype = Mag_Ashvinau;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

void mag_lvl10_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	switch ( type )
	{
		case CLASS_HUMAR:
		case CLASS_HUNEWEARL:
		case CLASS_HUCAST:
		case CLASS_HUCASEAL:
			m->mtype = Mag_Varuna;
			AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
			break;
		case CLASS_RAMAR:
		case CLASS_RACAST:
		case CLASS_RACASEAL:
		case CLASS_RAMARL:
			m->mtype = Mag_Kalki;
			AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
			break;
		case CLASS_FONEWM:
		case CLASS_FONEWEARL:
		case CLASS_FOMARL:
		case CLASS_FOMAR:
			m->mtype = Mag_Vritra;
			AddPB ( &m->PBflags, &m->blasts, PB_Leilla);
			break;
	}
}

void check_mag_evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	if ( ( m->level < 10 ) || ( m->level >= 35 ) )
	{
		if ( ( m->level < 35 ) || ( m->level >= 50 ) )
		{
			if ( m->level >= 50 )
			{
				if ( ! ( m->level % 5 ) ) // Divisible by 5 with no remainder.
				{
					if ( EvolutionClass <= 3 )
					{
						if ( !mag_special_evolution ( m, sectionID, type, EvolutionClass ) )
							mag_lvl50_evolution ( m, sectionID, type, EvolutionClass );
					}
				}
			}
		}
		else
		{
			if ( EvolutionClass < 2 )
				mag_lvl35_evolution ( m, sectionID, type, EvolutionClass );
		}
	}
	else
	{
		if ( EvolutionClass <= 0 )
			mag_lvl10_evolution ( m, sectionID, type, EvolutionClass );
	}
}

void AddPB ( unsigned char* flags, unsigned char* blasts, unsigned char pb )
{
	int pb_exists = 0;
	unsigned char pbv;
	unsigned pb_slot;

	if ( ( *flags & 0x01 ) == 0x01 )
	{
		if ( ( *blasts & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags & 0x02 ) == 0x02 )
	{
		if ( ( ( *blasts / 8 ) & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags  & 0x04 ) == 0x04 )
		pb_exists = 1;

	if (!pb_exists)
	{
		if ( ( *flags & 0x01 ) == 0 )
			pb_slot = 0;
		else
		if ( ( *flags & 0x02 ) == 0 )
			pb_slot = 1;
		else
			pb_slot = 2;
		switch ( pb_slot )
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags  |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags  |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ( ( *blasts & 0x07 ) < pb )
				pbv--;
			if ( ( ( *blasts / 8 ) & 0x07 ) < pb )
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags  |= 0x04;
		}
		*blasts |= pb;
	}
}
