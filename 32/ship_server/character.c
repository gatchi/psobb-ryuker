
int check_equip (unsigned char eq_flags, unsigned char cl_flags)
{
	int eqOK = 1;
	unsigned ch;

	for (ch=0;ch<8;ch++)
	{
		if ( ( cl_flags & (1 << ch) ) && (!( eq_flags & ( 1 << ch ) )) )
		{
			eqOK = 0;
			break;
		}
	}
	return eqOK;
}
