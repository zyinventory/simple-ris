#include "stdafx.h"
#include <libb24.h>
#include "gencard.h"
using namespace std;

int getLeakLockNumber(const char *filter, const char *regxPattern)
{
	WIN32_FIND_DATA ffd;
	char mask[TOTAL_BUY];

	memset(mask, -1, sizeof(mask));
	HANDLE hFind = FindFirstFile(filter, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) return 0;

	regex pattern(TEXT(regxPattern));
	match_results<string::const_iterator> result;
	// List all the files in the directory with some info about them.
	do
	{
		string fileName(ffd.cFileName);
		if(regex_match(fileName, result, pattern))
		{
			char buffer[MAX_PATH];
			result[1].str().copy(buffer, result[1].length(), 0);
			buffer[result[1].length()] = '\0';
			unsigned int lockNumber = 0;
			sscanf_s(buffer, TEXT("%04d"), &lockNumber);
			if(lockNumber < TOTAL_BUY) mask[lockNumber] = 0;
		}
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);

	return find(mask, mask + TOTAL_BUY, -1) - mask;
}

int _tmain(int argc, _TCHAR* argv[])
{
	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);
	if(argc < 2 || argc > 4)
	{
		cerr << "用法: " << argv[0] << " <加密狗号> [购买盒数=2] [特定序号]" << endl;
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
	unsigned int box = argc >= 3 ? atoi(argv[2]) : 2;
	if(box == 0)
	{
		cerr << "购买盒数错误" << endl;
		return -2;
	}
	if(box > MAX_BOX)
	{
		cerr << "一次最多购买" << MAX_BOX << "盒" << endl;
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

	unsigned int fileno = 0;
	if(argc >= 4)
	{
		fileno = atoi(argv[3]);
		if(errno == EINVAL && fileno == 0)
			fileno = getLeakLockNumber("*.txt", "^(\\d{4})\\.txt$");
	}
	else
		fileno = getLeakLockNumber("*.txt", "^(\\d{4})\\.txt$");
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
		b24buf[6 - i] = base24code[buy[0] % 24];
		buy[0] /= 24;
	}
	for(int i = 0; i < 7; ++i)
	{
		b24buf[13 - i] = base24code[buy[1] % 24];
		buy[1] /= 24;
	}
	// end encrypt

	cerr << "流水号:" << fileno << ", " << box << "盒, 充值密码: " << b24buf;
		
	// begin decrypt
	int test = decodeCharge(b24buf, security.serial, privateShieldPC);
	switch(test)
	{
	case -1:
		cerr << " base24 error " << fileno << endl;
		return test;
	case -2:
		cerr << " bit check error " << fileno << endl;
		return test;
	case -3:
		cerr << " lock check error " << fileno << endl;
		return test;
	}

	DWORD box_r = ((unsigned int)test) >> 10;
	DWORD fileno_r = test & 0x3FF;
	if(box == box_r && fileno == fileno_r)
	{
		char filename[64];
		sprintf_s(filename, "%04d.txt", fileno);
		ofstream card(filename, ios_base::app);
		if(card.fail())
		{
			cerr << " 生成文件错误:" << filename << endl;
		}
		else
		{
			card << b24buf << endl;
			card.close();
			cerr << " 生成OK" << endl;
		}
	}
	else
	{
		cerr << " decode check error " << fileno << endl;
		return -4;
	}
	return 0;
}
