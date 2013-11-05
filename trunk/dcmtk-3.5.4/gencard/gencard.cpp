#include "stdafx.h"
#include "gencard.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);
	if(argc != 2 && argc != 3)
	{
		cerr << "用法: " << argv[0] << " <加密狗号> [购买盒数=2]" << endl;
		return -1;
	}

	if(_chdir(argv[1]))
	{
		if(errno == ENOENT)
			cerr << "目录不存在" << argv[1] << endl;
		else
			cerr << "未知错误" << endl;
		return -2;
	}
	unsigned int box = argc == 3 ? atoi(argv[2]) : 2;
	if(box == 0)
	{
		cerr << "购买盒数错误" << endl;
		return -2;
	}
	if(box > 20)
	{
		cerr << "一次最多购买20盒" << endl;
		return -2;
	}

	ifstream keystrm("key.bin", ios_base::binary);
	if(keystrm.fail())
	{
		cerr << "没有文件 key.bin" << endl;
		return -3;
	}
	keystrm.read(reinterpret_cast<char*>(&security), sizeof(LOCK_KEY));
	if(keystrm.fail())
	{
		cerr << "key.bin格式错误" << endl;
		keystrm.close();
		return -3;
	}
	keystrm.close();

	// scan *.txt, fileno = max(fileno) + 1 
	unsigned int fileno = 0;

	for(fileno = 0; fileno < 100; ++fileno)
	{
		unsigned char cross[8];
		DWORD buy[2], realbuy;
		realbuy = ((box << 10) + fileno) & 0xFFFF;
		buy[1] = realbuy ^ (security.serial & 0xFFFF);
		buy[1] |= (realbuy << 16) ^ (security.serial & 0xFFFF0000);
		buy[0] = privateShieldPC(buy[1]);
		buy[1] ^= buy[0];
		*reinterpret_cast<DWORD*>(cross) = buy[0];
		*reinterpret_cast<DWORD*>(&cross[4]) = buy[1];
		swap(cross[0], cross[3]);
		swap(cross[1], cross[2]);
		swap(cross[4], cross[7]);
		swap(cross[5], cross[6]);

		buy[0] = *reinterpret_cast<DWORD*>(cross);
		buy[1] = *reinterpret_cast<DWORD*>(&cross[4]);
		char b24buf[15] = "              ";
		for(int i = 0; i < 7; ++i)
		{
			b24buf[6 - i] = code[buy[0] % 24];
			buy[0] /= 24;
		}
		for(int i = 0; i < 7; ++i)
		{
			b24buf[13 - i] = code[buy[1] % 24];
			buy[1] /= 24;
		}
		// end encrypt

		cerr << b24buf << ' ';
		
		// begin decrypt
		buy[0] = 0;
		buy[1] = 0;
		bool decodeOK = true;
		for(int i = 0; i < 14; ++i)
		{
			char c = b24buf[i] - 0x32;
			if(c >= 0 && c < 40 && decode[c] != -1)
				buy[i/7] = buy[i/7] * 24 + (DWORD)decode[c];
			else
			{
				decodeOK = false;
				break;
			}
		}
		if(!decodeOK)
		{
			cerr << "base24 error" << endl;
			continue;
		}

		*reinterpret_cast<DWORD*>(cross) = buy[0];
		*reinterpret_cast<DWORD*>(&cross[4]) = buy[1];
		swap(cross[0], cross[3]);
		swap(cross[1], cross[2]);
		swap(cross[4], cross[7]);
		swap(cross[5], cross[6]);
		buy[0] = *reinterpret_cast<DWORD*>(cross);
		buy[1] = *reinterpret_cast<DWORD*>(&cross[4]);
		DWORD origin = buy[0] ^ buy[1];
		if(buy[0] == privateShieldPC(origin))
		{
			DWORD test1 = (origin & 0xFFFF) ^ (security.serial & 0xFFFF);
			DWORD test2 = (origin & 0xFFFF0000) ^ (security.serial & 0xFFFF0000);
			test2 >>= 16;
			if(test1 == test2)
			{
				DWORD box_r = test1 >> 10;
				DWORD fileno_r = test1 & 0x3FF;
				if(box == box_r && fileno == fileno_r)
					cerr << fileno_r << ',' << box_r << endl;
				else
					cerr << "decode check error " << fileno << endl;
			}
			else
				cerr << "bit check error " << fileno << endl;
		}
		else
			cerr << "lock check error " << fileno << endl;
	}

	return 0;
}

