#include "stdafx.h"
#include "pso_crypt.h"
#include "bbtable.h"

/* PSO encryption and compression functions. */

/* Blue Burst encryption routines */

static void pso_crypt_init_key_bb (unsigned char *data)
{
	unsigned x;
	for (x = 0; x < 48; x += 3)
	{
		data[x] ^= 0x19;
		data[x + 1] ^= 0x16;
		data[x + 2] ^= 0x18;
	}
}

void pso_crypt_decrypt_bb (PSO_CRYPT *pcry, unsigned char *data, unsigned int length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *) &data[edx];
		ebx = ebx ^ pcry->tbl[5];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[4];
		ebp ^= *(unsigned long *) &data[edx+4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[3];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[2];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[1];
		ebp = ebp ^ pcry->tbl[0];
		ebx = ebx ^ edi;
		*(unsigned long *) &data[edx] = ebp;
		*(unsigned long *) &data[edx+4] = ebx;
		edx = edx+8;
	}
}

void pso_crypt_encrypt_bb (PSO_CRYPT *pcry, unsigned char *data, unsigned int length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *) &data[edx];
		ebx = ebx ^ pcry->tbl[0];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[1];
		ebp ^= *(unsigned long *) &data[edx+4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[2];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[3];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[4];
		ebp = ebp ^ pcry->tbl[5];
		ebx = ebx ^ edi;
		*(unsigned long *) &data[edx] = ebp;
		*(unsigned long *) &data[edx+4] = ebx;
		edx = edx+8;
	}
}

void pso_crypt_table_init_bb (PSO_CRYPT *pcry, const unsigned char *salt)
{
	unsigned long eax, ecx, edx, ebx, ebp, esi, edi, ou, x;
	unsigned char s[48];
	unsigned short* pcryp;
	unsigned short* bbtbl;
	unsigned short dx;

	pcry->cur = 0;
	pcry->mangle = NULL;
	pcry->size = 1024 + 18;

	memcpy(s, salt, sizeof(s));
	pso_crypt_init_key_bb(s);

	bbtbl = (unsigned short*) &bbtable[0];
	pcryp = (unsigned short*) &pcry->tbl[0];

	eax = 0;
	ebx = 0;

	for(ecx=0;ecx<0x12;ecx++)
	{
		dx = bbtbl[eax++];
		dx = ( ( dx & 0xFF ) << 8 ) + ( dx >> 8 );
		pcryp[ebx] = dx;
		dx = bbtbl[eax++];
		dx ^= pcryp[ebx++];
		pcryp[ebx++] = dx;
	}

/*
	pcry->tbl[0] = 0x243F6A88;
	pcry->tbl[1] = 0x85A308D3;
	pcry->tbl[2] = 0x13198A2E;
	pcry->tbl[3] = 0x03707344;
	pcry->tbl[4] = 0xA4093822;
	pcry->tbl[5] = 0x299F31D0;
	pcry->tbl[6] = 0x082EFA98;
	pcry->tbl[7] = 0xEC4E6C89;
	pcry->tbl[8] = 0x452821E6;
	pcry->tbl[9] = 0x38D01377;
	pcry->tbl[10] = 0xBE5466CF;
	pcry->tbl[11] = 0x34E90C6C;
	pcry->tbl[12] = 0xC0AC29B7;
	pcry->tbl[13] = 0xC97C50DD;
	pcry->tbl[14] = 0x3F84D5B5;
	pcry->tbl[15] = 0xB5470917;
	pcry->tbl[16] = 0x9216D5D9;
	pcry->tbl[17] = 0x8979FB1B;
	
	*/

	memcpy(&pcry->tbl[18], &bbtable[18], 4096);

	ecx=0;
	//total key[0] length is min 0x412
	ebx=0;

	while (ebx < 0x12)
	{
		//in a loop
		ebp=((unsigned long) (s[ecx])) << 0x18;
		eax=ecx+1;
		edx=eax-((eax / 48)*48);
		eax=(((unsigned long) (s[edx])) << 0x10) & 0xFF0000;
		ebp=(ebp | eax) & 0xffff00ff;
		eax=ecx+2;
		edx=eax-((eax / 48)*48);
		eax=(((unsigned long) (s[edx])) << 0x8) & 0xFF00;
		ebp=(ebp | eax) & 0xffffff00;
		eax=ecx+3;
		ecx=ecx+4;
		edx=eax-((eax / 48)*48);
		eax=(unsigned long) (s[edx]);
		ebp=ebp | eax;
		eax=ecx;
		edx=eax-((eax / 48)*48);
		pcry->tbl[ebx]=pcry->tbl[ebx] ^ ebp;
		ecx=edx;
		ebx++;
	}

	ebp=0;
	esi=0;
	ecx=0;
	edi=0;
	ebx=0;
	edx=0x48;

	while (edi < edx)
	{
		esi=esi ^ pcry->tbl[0];
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xff;
		eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
		ebx=(esi >> 8) & 0xFF;
		eax=eax ^ pcry->tbl[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->tbl[ebx+0x312];

		eax=eax ^ pcry->tbl[1];
		ecx= ecx ^ eax;
		ebx=ecx >> 0x18;
		eax=(ecx >> 0x10) & 0xFF;
		ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
		eax=(ecx >> 8) & 0xff;
		ebx=ebx ^ pcry->tbl[eax+0x212];
		eax=ecx & 0xff;
		ebx=ebx + pcry->tbl[eax+0x312];

		for (x = 0; x <= 5; x++)
		{
			ebx=ebx ^ pcry->tbl[(x*2)+2];
			esi= esi ^ ebx;
			ebx=esi >> 0x18;
			eax=(esi >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(esi >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=esi & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];

			ebx=ebx ^ pcry->tbl[(x*2)+3];
			ecx= ecx ^ ebx;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];
		}

		ebx=ebx ^ pcry->tbl[14];
		esi= esi ^ ebx;
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xFF;
		eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
		ebx=(esi >> 8) & 0xff;
		eax=eax ^ pcry->tbl[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->tbl[ebx+0x312];

		eax=eax ^ pcry->tbl[15];
		eax= ecx ^ eax;
		ecx=eax >> 0x18;
		ebx=(eax >> 0x10) & 0xFF;
		ecx=pcry->tbl[ecx+0x12]+pcry->tbl[ebx+0x112];
		ebx=(eax >> 8) & 0xff;
		ecx=ecx ^ pcry->tbl[ebx+0x212];
		ebx=eax & 0xff;
		ecx=ecx + pcry->tbl[ebx+0x312];

		ecx=ecx ^ pcry->tbl[16];
		ecx=ecx ^ esi;
		esi= pcry->tbl[17];
		esi=esi ^ eax;
		pcry->tbl[(edi / 4)]=esi;
		pcry->tbl[(edi / 4)+1]=ecx;
		edi=edi+8;
	}


	eax=0;
	edx=0;
	ou=0;
	while (ou < 0x1000)
	{
		edi=0x48;
		edx=0x448;

		while (edi < edx)
		{
			esi=esi ^ pcry->tbl[0];
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xff;
			eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
			ebx=(esi >> 8) & 0xFF;
			eax=eax ^ pcry->tbl[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->tbl[ebx+0x312];

			eax=eax ^ pcry->tbl[1];
			ecx= ecx ^ eax;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];

			for (x = 0; x <= 5; x++)
			{
				ebx=ebx ^ pcry->tbl[(x*2)+2];
				esi= esi ^ ebx;
				ebx=esi >> 0x18;
				eax=(esi >> 0x10) & 0xFF;
				ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
				eax=(esi >> 8) & 0xff;
				ebx=ebx ^ pcry->tbl[eax+0x212];
				eax=esi & 0xff;
				ebx=ebx + pcry->tbl[eax+0x312];

				ebx=ebx ^ pcry->tbl[(x*2)+3];
				ecx= ecx ^ ebx;
				ebx=ecx >> 0x18;
				eax=(ecx >> 0x10) & 0xFF;
				ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
				eax=(ecx >> 8) & 0xff;
				ebx=ebx ^ pcry->tbl[eax+0x212];
				eax=ecx & 0xff;
				ebx=ebx + pcry->tbl[eax+0x312];
			}

			ebx=ebx ^ pcry->tbl[14];
			esi= esi ^ ebx;
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xFF;
			eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
			ebx=(esi >> 8) & 0xff;
			eax=eax ^ pcry->tbl[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->tbl[ebx+0x312];

			eax=eax ^ pcry->tbl[15];
			eax= ecx ^ eax;
			ecx=eax >> 0x18;
			ebx=(eax >> 0x10) & 0xFF;
			ecx=pcry->tbl[ecx+0x12]+pcry->tbl[ebx+0x112];
			ebx=(eax >> 8) & 0xff;
			ecx=ecx ^ pcry->tbl[ebx+0x212];
			ebx=eax & 0xff;
			ecx=ecx + pcry->tbl[ebx+0x312];

			ecx=ecx ^ pcry->tbl[16];
			ecx=ecx ^ esi;
			esi= pcry->tbl[17];
			esi=esi ^ eax;
			pcry->tbl[(ou / 4)+(edi / 4)]=esi;
			pcry->tbl[(ou / 4)+(edi / 4)+1]=ecx;
			edi=edi+8;
		}
		ou=ou+0x400;
	}
}

unsigned RleEncode (unsigned char *src, unsigned char *dest, unsigned src_size)
{
	unsigned char currChar, prevChar;             /* current and previous characters */
	unsigned short count;                /* number of characters in a run */
	unsigned src_end, dest_start;

	dest_start = (unsigned)dest;
	src_end = (unsigned)src + src_size;

	prevChar  = 0xFF - *src;

	while ((unsigned) src < src_end)
	{
		currChar = *(dest++) = *(src++);

		if ( currChar == prevChar )
		{
			if ( (unsigned) src == src_end )
			{
				*(dest++) = 0;
				*(dest++) = 0;
			}
			else
			{
				count = 0;
				while (((unsigned)src < src_end) && (count < 0xFFF0))
				{
					if (*src == prevChar)
					{
						count++;
						src++;
						if ( (unsigned) src == src_end )
						{
							*(unsigned short*) dest = count;
							dest += 2;
						}
					}
					else
					{
						*(unsigned short*) dest = count;
						dest += 2;
						prevChar = 0xFF - *src;
						break;
					}
				}
			}
		}
		else
			prevChar = currChar;
	}
	return (unsigned)dest - dest_start;
}

void RleDecode (unsigned char *src, unsigned char *dest, unsigned src_size)
{
    unsigned char currChar, prevChar;             /* current and previous characters */
    unsigned short count;                /* number of characters in a run */
	unsigned src_end;

	src_end = (unsigned) src + src_size;

    /* decode */

    prevChar = 0xFF - *src;     /* force next char to be different */

    /* read input until there's nothing left */

    while ((unsigned) src < src_end)
    {
		currChar = *(src++);

		*(dest++) = currChar;

        /* check for run */
        if (currChar == prevChar)
        {
            /* we have a run.  write it out. */
			count = *(unsigned short*) src;
			src += 2;
            while (count > 0)
            {
				*(dest++) = currChar;
                count--;
            }

            prevChar = 0xFF - *src;     /* force next char to be different */
        }
        else
        {
            /* no run */
            prevChar = currChar;
        }
    }
}

/* expand a key (makes a rc4_key) */

void prepare_key (unsigned char *keydata, unsigned int len, struct rc4_key *key)
{
    unsigned index1, index2, counter;
    unsigned char *state;

    state = key->state;

    for (counter = 0; counter < 256; counter++)
        state[counter] = (unsigned char) counter;

    key->x = key->y = index1 = index2 = 0;

    for (counter = 0; counter < 256; counter++) {
        index2 = (keydata[index1] + state[counter] + index2) & 255;

        /* swap */
        state[counter] ^= state[index2];
        state[index2]  ^= state[counter];
        state[counter] ^= state[index2];

        index1 = (index1 + 1) % len;
    }
}

/* reversible encryption, will encode a buffer updating the key */

void rc4 (unsigned char *buffer, unsigned int len, struct rc4_key *key)
{
    unsigned x, y, xorIndex, counter;
    unsigned char *state;

    /* get local copies */
    x = key->x; y = key->y;
    state = key->state;

    for (counter = 0; counter < len; counter++) {
        x = (x + 1) & 255;
        y = (state[x] + y) & 255;

        /* swap */
        state[x] ^= state[y];
        state[y] ^= state[x];
        state[x] ^= state[y];

        xorIndex = (state[y] + state[x]) & 255;

        buffer[counter] ^= state[xorIndex];
    }

    key->x = x; key->y = y;
}

