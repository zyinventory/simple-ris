#pragma once
				   //          11111111112222
                   //012345678901234567890123
const char code[] = "9TF7VMWB8XJ2CKRQ346YPDGH";
const char decode[40] = {						   // 0x32
      11,16,17,-1,18, 3, 8, 0,-1,-1,-1,-1,-1,-1,   //    2 3 4 5 6 7 8 9 _ _ _ _ _ _
-1,-1, 7,12,21,-1, 2,22,23,-1,10,13,-1, 5,-1,-1,   //_ _ B C D _ F G H _ J K _ M _ _
20,15,14,-1, 1,-1, 4, 6, 9,19 };                   //P Q R _ T _ V W X Y

struct lock_key_st { unsigned int key[4]; unsigned int serial, lockNumber; };
typedef struct lock_key_st LOCK_KEY;
LOCK_KEY security;
unsigned long privateShieldPC(unsigned long x)
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
