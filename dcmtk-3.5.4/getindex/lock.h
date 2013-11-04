#pragma once
const unsigned long key1 = 54762, key2 = 52662, key3 = 58962, key4 = 40062;
static char init_passwd[9] = "bP8K6jr0"; // abcdefgh
extern "C" unsigned long __stdcall ReadLock(int, unsigned char*, char*);
extern "C" unsigned long __stdcall Lock32_Function(unsigned long);
extern "C" int __stdcall WriteLock(int, unsigned char*, char*);
extern "C" int __stdcall Counter(char*);
extern "C" int __stdcall SetLock(int, unsigned long*, char*, char*);
extern "C" int __stdcall InOutMessageBox(char*);
extern "C" void __stdcall UnShieldLock();
inline unsigned long shieldPC(unsigned long x)
{
	unsigned long y,y1,y2,x1,x2,y11,y22;
	unsigned long outdata1,outdata2;
	outdata2 = x  & 0xffff0000;
	outdata2 = outdata2 >> 16;
	outdata1 = 0x0ffff & x;
	x1=outdata1;
	x2=outdata2;
	y1 = x1 ^ key2;
	y11 = x2 ^ key1;
	y1 = y1 + y11;
	if (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 * 16;
	while (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 % key4;
	y = y1 * key3;
	while (y > 4294967295)
		y = y - 4294967295-1;
	y11 = x1 + key1;
	if (y11 > 65535)
		y11 = y11 - 65536;
	y22 = y11 % key3;
	y11 = key4 ^ x2;
	y2 = y22 * y11;
	while (y2 > 4294967295)
		y2 = y2 - 4294967295-1; 
	y = y ^ y2;
	return(y);
}
