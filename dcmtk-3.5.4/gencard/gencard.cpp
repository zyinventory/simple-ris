#include "stdafx.h"
#include <libb24.h>
#include <shieldpc.h>
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
	if(box > MAX_BOX)
	{
		cerr << "一次最多购买" << MAX_BOX << "盒" << endl;
		return -2;
	}

	DWORD flag = 0;
	ifstream flagstrm("flag.txt");
	if(!flagstrm.good())
	{
		ofstream flagostrm("flag.txt");
		if(!flagostrm.good())
		{
			cerr << "无法打开或创建flag.txt" << endl;
			return -15;
		}
		flagostrm << flag << endl;
		flagostrm.close();
	}
	else
	{
		flagstrm >> flag;
		flagstrm.close();
	}

	if(flag & BATCH_INIT_FLAG)
	{
		cerr << "批量授权不能充值" << endl;
		return -10;
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
	for(int i = 0; i < 4; ++i) key[i] = security.key[i];

	unsigned int fileno = 0;
	if(argc >= 4)
	{
		fileno = atoi(argv[3]);
		if(errno == EINVAL && fileno == 0)
			fileno = getLeakLockNumber("*.txt", "^(\\d{4})\\.txt$");
	}
	else
		fileno = getLeakLockNumber("*.txt", "^(\\d{4})\\.txt$");

	long salt = ShieldPC(security.lockNumber);
	for(unsigned int i = 0; i < (fileno % 13); ++i)
		salt = ShieldPC(salt);

	char b24buf[15] = "              ";
	if(box == 0) goto DecodeFromFile;

	unsigned char cross[8];
	DWORD buy[2], realbuy;
	realbuy = (((fileno + 1) * box) << 16) & 0xFF0000;
	realbuy |= (box << 24) + fileno;
	buy[1] = realbuy ^ salt;
	buy[0] = ShieldPC(buy[1]);
	buy[1] ^= buy[0];

	*reinterpret_cast<DWORD*>(cross) = buy[0];
	*reinterpret_cast<DWORD*>(&cross[4]) = buy[1];
	swap(cross[0], cross[3]);
	swap(cross[1], cross[2]);
	swap(cross[4], cross[7]);
	swap(cross[5], cross[6]);

	buy[0] = *reinterpret_cast<DWORD*>(cross);
	buy[1] = *reinterpret_cast<DWORD*>(&cross[4]);

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

DecodeFromFile:
	if(box == 0)
	{
		char filename[64];
		sprintf_s(filename, "%04d.txt", fileno);
		ifstream card(filename);
		if(!card.good())
		{
			cerr << "打开文件" << filename << "错误" << endl;
			return -5;
		}
		card >> b24buf;
		card.close();
	}

	// begin decrypt
	unsigned int test = decodeCharge(b24buf, salt, ShieldPC);
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
	
	DWORD box_r = test >> 24;
	DWORD fileno_r = test & 0xFFFF;
	if(box == 0 && fileno == fileno_r) // query
	{
		cerr << "流水号:" << fileno_r << ", " << box_r << "盒, 充值密码: " << b24buf;
	}
	else if(box == box_r && fileno == fileno_r) //release
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
