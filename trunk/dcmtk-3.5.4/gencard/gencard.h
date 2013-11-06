#pragma once

struct lock_key_st { unsigned int key[4]; unsigned int serial, lockNumber; };
typedef struct lock_key_st LOCK_KEY;
LOCK_KEY security;
unsigned int privateShieldPC(unsigned int x)
{
	unsigned long y,y1,y2,x1,x2,y11,y22;
	unsigned long outdata1,outdata2;
	outdata2 = x  & 0xffff0000;
	outdata2 = outdata2 >> 16;
	outdata1 = 0x0ffff & x;
	x1=outdata1;
	x2=outdata2;
	y1 = x1 ^ security.key[1];
	y11 = x2 ^ security.key[0];
	y1 = y1 + y11;
	if (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 * 16;
	while (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 % security.key[3];
	y = y1 * security.key[2];
	while (y > 4294967295)
		y = y - 4294967295-1;
	y11 = x1 + security.key[0];
	if (y11 > 65535)
		y11 = y11 - 65536;
	y22 = y11 % security.key[2];
	y11 = security.key[3] ^ x2;
	y2 = y22 * y11;
	while (y2 > 4294967295)
		y2 = y2 - 4294967295-1; 
	y = y ^ y2;
	return(y);
}
