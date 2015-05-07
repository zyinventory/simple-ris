#include "libb24.h"

extern "C" int decodeCharge(const char *b24buf, unsigned int serial, LOCK_FUNC_PTR lockfunc)
{
	unsigned int buy[2] = {0, 0};
	unsigned char cross[8];
	bool decodeOK = true;
	for(int i = 0; i < 14; ++i)
	{
		char c = b24buf[i] - 0x32;
		if(c >= 0 && c < 40 && base24decode[c] != -1)
			buy[i/7] = buy[i/7] * 24 + (unsigned int)base24decode[c];
		else
		{
			decodeOK = false;
			break;
		}
	}
	if(!decodeOK)
		return -1; // "base24 error"

	*(unsigned int*)cross = buy[0];
	*(unsigned int*)(&cross[4]) = buy[1];
	unsigned char tmp;
	tmp = cross[0]; cross[0] = cross[3]; cross[3] = tmp;
	tmp = cross[1]; cross[1] = cross[2]; cross[2] = tmp;
	tmp = cross[4]; cross[4] = cross[7]; cross[7] = tmp;
	tmp = cross[5]; cross[5] = cross[6]; cross[6] = tmp;

	buy[0] = *(unsigned int*)cross;
	buy[1] = *(unsigned int*)(&cross[4]);
	unsigned int origin = buy[0] ^ buy[1];
	if(buy[0] == lockfunc(origin))
	{
		origin ^= serial;
		unsigned int box = origin >> 24;
		unsigned int fileno = origin & 0xFFFF;

		if(((box * (fileno + 1)) & 0xFF) == ((origin >> 16) & 0xFF))
			return origin & 0xFF00FFFF;
		else
			return -2; // " bit check error "
	}
	else
		return -3; // " lock check error "
}
