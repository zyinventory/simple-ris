#include "stdafx.h"
#include "constant.h"
using namespace std;

#define MEM_CAPACITY 1280

static _TCHAR buffer[MAX_PATH], lock_passwd[9] = "", rw_passwd[9] = "";

class numpunct_no_gouping : public numpunct_byname<char>
{
public:
    numpunct_no_gouping(const char* name) : numpunct_byname<char>(name){ }
protected:
    virtual string do_grouping() const { return ""; } // no grouping
};

void exitHook()
{
	TerminateLock(0);
}

static void printKeyIV(SEED_SIV *sivptr, const char *filename)
{
	bool writeToFile = true;
	ofstream keyiv(filename);
	ostream *keyivPtr = &keyiv;
	if(keyiv.fail())
	{
		keyivPtr = &cout;
		writeToFile = false;
	}
	*keyivPtr << "-K ";
	ostream &formatted = *keyivPtr << uppercase << hex << setw(2);
	for_each(sivptr->key, sivptr->key + sizeof(sivptr->key), [&formatted](unsigned char c) { formatted << (int)c; });
	*keyivPtr << " -iv ";
	for_each(sivptr->iv, sivptr->iv + sizeof(sivptr->iv), [&formatted](unsigned char c) { formatted << (int)c; });
	*keyivPtr << endl;
	if(writeToFile) keyiv.close();
}

int echoUsage(const _TCHAR *app)
{
	cerr << "单授权模式(init): " << endl
		<< '\t' << app << " init <盒数> <key.txt文件路径> [管理密码 读写密码]" << endl
		<< endl
		<< "批量授权模式(batch_init, reset): " << endl
		<< '\t' << app << " batch_init <盒数> <key.txt文件路径> [管理密码 读写密码]" << endl
		<< '\t' << app << " reset <盒数> [管理密码 读写密码]" << endl;
	return -1;
}

bool readKeyFromTxt(const char *keyPath)
{
	ifstream keystrm(keyPath);
	if(!keystrm.good())
	{
		cerr << keyPath << " open failed" << endl;
		return false;
	}
	bool keyOK = false;
	regex linePattern("^key(\\d) *= *(\\d+)$");
	while(! keystrm.getline(buffer, MAX_PATH).fail())
	{
		match_results<const _TCHAR*> result;
		if(regex_match(buffer, result, linePattern))
		{
			cerr << "key" << result[1] << " = " << result[2] << endl;
			int index = (int)result[1].str()[0] - (int)'1';
			key[index] = atoi(result[2].str().c_str());
			if(keyOK = !any_of(key, key + 4, [](int value) { return value == 0; })) break;
		}
	}
	keystrm.close();
	return keyOK;
}

int initSecurity(int lockNumber, string &lockString)
{
	long dictionary[DICTIONARY_SIZE * 2 + 4]; // (rand_request, key_response) * 2 + md5_digest
	for(int i = 0; i < DICTIONARY_SIZE; ++i)
	{
		do{
			RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(&dictionary[i]), sizeof(DWORD));
		}while(&dictionary[i] != find(dictionary, &dictionary[i], dictionary[i]));
		long hard = 0, soft = ShieldPC(dictionary[i]);
		Lock32_Function(dictionary[i], &hard, 0);
		if(hard == soft)
		{
			dictionary[DICTIONARY_SIZE + i] = hard ^ dictionary[i];
			assert(hard == (dictionary[DICTIONARY_SIZE + i] ^ dictionary[i]));
		}
		else
		{
			cerr << "加密狗错误: shieldPC mismatch" << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * sizeof(DWORD) * 2, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));
	
	ofstream keyout("key.bin", ios_base::binary);
	if(keyout.fail())
	{
		cerr << "保存key错误" << endl;
		return -7;
	}
	keyout.write(reinterpret_cast<char*>(key), sizeof(key));
	keyout.write(reinterpret_cast<char*>(&lockNumber), sizeof(lockNumber));
	keyout.close();

	char plainFileName[] = "license.plain", licenseFileName[] = "license.aes", licenseRSAEnc[] = "license.key";
	ofstream licenseFile(plainFileName, ios_base::binary);
	size_t dictLength = sizeof(dictionary);
	licenseFile.write(reinterpret_cast<const char*>(dictionary), dictLength);
	licenseFile.close();

	
	_TCHAR *rsaPrivateKey = "private.rsa", rsaPublicKey[16];
	strcpy_s(rsaPublicKey, sizeof(rsaPublicKey), lockString.c_str());
	strcat_s(rsaPublicKey, sizeof(rsaPublicKey), ".key");

	int ret = genrsa(KEY_SIZE, rsaPrivateKey, rsaPublicKey, passwd);
	if(ret != 0)
	{
		cerr << "生成RSA密钥错误:" << ret << endl;
		return -8;
	}
	cerr << "生成RSA密钥:" << rsaPrivateKey << "," << rsaPublicKey << endl;

	SEED_SIV siv;
	if(loadPublicKeyContent2Pwd(rsaPublicKey, &siv, lockNumber, lock_passwd, rw_passwd))
	{
		cerr << "生成RSA公钥格式错误" << endl;
		return -8;
	}
	//printKeyIV(&siv, "key_iv.hex");

	size_t encnum = aes256cbc_enc(dictionary, sizeof(dictionary), licenseFileName, siv.key, siv.iv);
	ret = rsaSign(licenseFileName, licenseRSAEnc, rsaPrivateKey, passwd);
	if(ret != 0)
	{
		cerr << "RSA sign 错误:" << ret << "," << licenseFileName << endl;
		return -9;
	}
	cerr << licenseFileName << " => RSA sign => " << licenseRSAEnc << endl;

	unsigned char inBuf[KEY_SIZE / 8], midBuf[KEY_SIZE / 8], outBuf[KEY_SIZE / 8];
	ifstream licenseRSAStream(licenseRSAEnc);
	licenseRSAStream.read((char*)inBuf, KEY_SIZE / 8);
	if(licenseRSAStream.fail())
	{
		cerr << "RSA verify 错误, 读取文件错误" << licenseRSAEnc << endl;
		licenseRSAStream.close();
		return -10;
	}
	licenseRSAStream.close();

	ret = rsaVerify(inBuf, KEY_SIZE / 8, midBuf, rsaPublicKey);
	if(ret <= 0)
	{
		cerr << "RSA verify 错误:" << licenseRSAEnc << endl;
		return -11;
	}

	// skip magic number and salt
	ret = aes256cbc_dec(midBuf + AES_OFFSET, ret - AES_OFFSET, outBuf, siv.key, siv.iv);
	if(ret <= 0)
	{
		cerr << "AES decrypt 错误:" << licenseRSAEnc << endl;
		return -12;
	}
	cerr << licenseRSAEnc << " => RSA verify => AES decrypt OK" << endl;

	DWORD digestSig[4], *originSig = reinterpret_cast<DWORD*>(&outBuf[DICTIONARY_SIZE * 8]);
	MD5_digest(outBuf, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(digestSig));
	if(digestSig[0] != originSig[0] || digestSig[1] != originSig[1]
		|| digestSig[2] != originSig[2] || digestSig[3] != originSig[3])
	{
		cerr << "MD5 digest 错误:" << licenseRSAEnc << endl;
		char licenseRSADec[] = "license.dec";
		ofstream unrsaStream(licenseRSADec);
		unrsaStream.write((char*)outBuf, ret);
		unrsaStream.close();
		return -13;
	}
	cerr << "MD5 digest OK" << endl;
	
	DWORD *loadDict = reinterpret_cast<DWORD*>(outBuf);
	for(int i = 0; i < DICTIONARY_SIZE; ++i)
	{
		if(loadDict[DICTIONARY_SIZE + i] != (ShieldPC(loadDict[i]) ^ loadDict[i]))
		{
			cerr << "错误的授权文件" << endl;
			return -14;
		}
	}

	BOOL lockPwdOK = TRUE, rwPwdOK = TRUE;
	ofstream passwdstrm("passwd.txt", ios_base::out | ios_base::trunc);
	//管理密码
	if(strcmp(init_lock_passwd, lock_passwd))
	{
		DWORD tmp = 0;
		lockPwdOK = SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0);
	}
	if(lockPwdOK)
	{
		cerr << "管理密码:" << lock_passwd << endl;
		passwdstrm << "管理密码:" << lock_passwd << endl;
	}
	else
	{
		cerr << "加密狗修改管理密码错误" << hex << LYFGetLastErr() << endl;
		passwdstrm << "加密狗修改管理密码错误" << hex << LYFGetLastErr() << ",原管理密码不变:" << init_lock_passwd << endl;
		return -16;
	}
	//读写密码
	if(strcmp(init_rw_passwd, rw_passwd))
	{
		DWORD tmp = 0;
		rwPwdOK = SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0);
	}
	if(rwPwdOK)
	{
		cerr << "读写密码:" << rw_passwd << endl;
		passwdstrm << "读写密码:" << rw_passwd << endl;
	}
	else
	{
		cerr << "加密狗修改读写密码错误" << hex << LYFGetLastErr() << endl;
		passwdstrm << "加密狗修改读写密码错误" << hex << LYFGetLastErr() << ",原读写密码不变:" << init_rw_passwd << endl;
		return -16;
	}
	passwdstrm.close();
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD lockNumber = 0, flag = 0;

	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);

	// If the directory is not specified as a command-line argument, print usage.
	if(argc < 3) return echoUsage(argv[0]);

	int initNumber = 0;
	initNumber = atoi(argv[2]);
	if(initNumber == 0 && errno == EINVAL) initNumber = -1;
	initNumber *= 50;
	if(initNumber < 0 || initNumber > 0xFFFF) return echoUsage(argv[0]);
	bool isInit = false;
	if(0 == strcmp(argv[1], "batch_init"))
	{
		isInit = true;
		flag |= BATCH_INIT_FLAG;
	}
	else if(0 == strcmp(argv[1], "init"))
	{
		isInit = true;
	}

	if(isInit)
	{
		if(argc < 4) return echoUsage(argv[0]);
		// read key.txt
		if(!readKeyFromTxt(argv[3]))
		{
			cerr << "key文件格式错误" << endl;
			return -7;
		}
		// init_lock_passwd and init_rw_passwd
		if(argc >= 5) strcpy_s(init_lock_passwd, 9, argv[4]);
		if(argc >= 6) strcpy_s(init_rw_passwd, 9, argv[5]);
	}
	else
	{
		if(argc >= 4) strcpy_s(init_lock_passwd, 9, argv[3]);
		if(argc >= 5) strcpy_s(init_rw_passwd, 9, argv[4]);
	}

	//open dog, get lock number
	if(!InitiateLock(0))
	{
		cerr << "打开加密狗错误" << endl;
		return -2;
	}
	atexit(exitHook);

	if(!SetLock(8, &lockNumber, 0, init_lock_passwd, init_lock_passwd, 0, 0))
	{
		cerr << "获取加密狗序列号错误:" << hex << LYFGetLastErr() << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << lockNumber;
	string lockString = serialStringStream.str();
	cerr << "加密狗序列号:" << lockString << endl;

	if(isInit && _mkdir(lockString.c_str()))
	{
		int err = errno;
		if(errno == EEXIST)
			cerr << "目录" << lockString << "已存在，不能覆盖" << endl;
		else
			cerr << "创建目录" << lockString << "错误:" << err << endl;
		return err;
	}
	if(_chdir(lockString.c_str()))
	{
		int err = errno;
		cerr << "打开目录" << lockString << "错误:" << err << endl;
		return err;
	}
	
	long counter_signed = currentCount(init_lock_passwd);
	//if(Counter(init_rw_passwd, 0, 0, 0, &counter))
	bool initPasswdOK = (counter_signed == -1 || counter_signed <= MAX_MEDIA_COUNT);  // test: init_lock_passwd OK?
	unsigned long counter = 0;
	if(initPasswdOK) counter = counter_signed < 0 ? 0 : counter_signed;
	if(isInit)
	{
		if(initPasswdOK)
			cerr << "初始密码测试成功" << endl;
		else
		{
			cerr << "初始密码测试失败:" << hex << LYFGetLastErr() << endl;
			return -16;
		}
		int securityResult = initSecurity(lockNumber, lockString);
		if(securityResult) return securityResult;
		ofstream flagstrm("flag.txt");
		if(!flagstrm.good())
		{
			cerr << "无法创建flag.txt" << endl;
			return -15;
		}
		flagstrm << flag << endl;
		flagstrm.close();
	}
	else // reset
	{
		ifstream passwdstrm("passwd.txt");
		if(!passwdstrm.good())
		{
			cerr << "无法打开passwd.txt" << endl;
			return -15;
		}
		passwdstrm.ignore(9, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(lock_passwd, buffer);
		else
		{
			cerr << "passwd.txt : 管理密码读取错误" << endl;
			return -16;
		}
		passwdstrm.ignore(10, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(rw_passwd, buffer);
		else
		{
			cerr << "passwd.txt : 读写密码读取错误" << endl;
			return -16;
		}
		passwdstrm.close();

		if(initPasswdOK)
		{
			DWORD tmp = 0;
			// change passwd
			if(SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0))
			{
				cerr << "管理密码:" << lock_passwd << endl;
			}
			else
			{
				cerr << "加密狗修改管理密码错误" << hex << LYFGetLastErr() << endl;
				return -16;
			}
			if(SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0))
			{
				cerr << "读写密码:" << rw_passwd << endl;
			}
			else
			{
				cerr << "加密狗修改读写密码错误" << hex << LYFGetLastErr() << endl;
				return -16;
			}
		}
		else // init_lock_passwd error
		{	// test: lock_passwd(from passwd.txt) OK?
			if(!SetLock(0, &counter, 0, "", lock_passwd, 0, 0))
			{
				cerr << "初始密码，passwd.txt都错误" << endl;
				return -16;
			}
			//else passwd.txt OK, don't change passwd
		}
		
		ifstream flagstrm("flag.txt");
		if(!flagstrm.good())
		{	// in reset mode: if flag.txt doesn't exists, create it with content BATCH_INIT_FLAG.
			ofstream flagostrm("flag.txt");
			if(!flagostrm.good())
			{
				cerr << "无法打开或创建flag.txt" << endl;
				return -15;
			}
			flag = BATCH_INIT_FLAG;
			flagostrm << flag << endl;
			flagostrm.close();
		}
		else
		{
			flagstrm >> flag;
			flagstrm.close();
		}
	}

	cerr << "初始化加密狗内存" << endl;
	cerr << '|';
	for(int i = 0; i < 78; ++i) cerr << '-';
	cerr << '|';
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hOut, &info);
	--info.dwCursorPosition.Y;
	SetConsoleCursorPosition(hOut, info.dwCursorPosition);
	for(int i = 1; i < MEM_CAPACITY; ++i)
	{
		DWORD data = 0;
		if(i == MODE_FLAG_POS) data = flag;  // batch mode flag
		if(WriteLock(i, &data, lock_passwd, 0, 0))
		{
			if(i % 16 == 0) cerr << 'O';
		}
		else
		{
			cerr << "加密狗写入错误" << hex << LYFGetLastErr() << endl;
			return -17;
		}
	}
	cerr << endl;

	initNumber += DUMMY_ZERO;
	if(SetLock(1, reinterpret_cast<unsigned long*>(&initNumber), 0, lock_passwd, lock_passwd, 0, 0))
		cerr << "次数限制成功" << endl;
	else
	{
		cerr << "次数限制失败:" << hex << LYFGetLastErr() << endl;
		return -18;
	}
	cerr << "### 加密狗初始化完全成功 ###" << endl;
	return 0;
}
