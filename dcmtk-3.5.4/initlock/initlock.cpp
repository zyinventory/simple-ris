#include "stdafx.h"
#include <openssl/rand.h>
#include <shieldpc.h>
#include <liblock.h>
#include <lock.h>
#include "constant.h"
using namespace std;

static _TCHAR buffer[MAX_PATH], lock_passwd[9] = "", rw_passwd[9] = "";

class numpunct_no_gouping : public numpunct_byname<char>
{
public:
    numpunct_no_gouping(const char* name) : numpunct_byname<char>(name){ }
protected:
    virtual String do_grouping() const { return TEXT(""); } // no grouping
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
		keyivPtr = &COUT;
		writeToFile = false;
	}
	*keyivPtr << TEXT("-K ");
	ostream &formatted = *keyivPtr << uppercase << hex << setw(2);
	for_each(sivptr->key, sivptr->key + sizeof(sivptr->key), [&formatted](unsigned char c) { formatted << (int)c; });
	*keyivPtr << TEXT(" -iv ");
	for_each(sivptr->iv, sivptr->iv + sizeof(sivptr->iv), [&formatted](unsigned char c) { formatted << (int)c; });
	*keyivPtr << endl;
	if(writeToFile) keyiv.close();
}

int echoUsage(const char *app)
{
	CERR << TEXT("init  Usage: ") << app << TEXT(" init <number> <key.txt path> [init_admin_password init_rw_password]") << endl
		<< "reset Usage: " << app << TEXT(" reset <number> [init_admin_password init_rw_password]") << endl;
	return -1;
}

bool readKeyFromTxt(const char *keyPath)
{
	IFSTREAM keystrm(keyPath);
	if(!keystrm.good())
	{
		CERR << keyPath << TEXT(" open failed") << endl;
		return false;
	}
	bool keyOK = false;
	REGEX linePattern(TEXT("^key(\\d) *= *(\\d+)$"));
	while(! keystrm.getline(buffer, MAX_PATH).fail())
	{
		match_results<const _TCHAR*> result;
		if(regex_match(buffer, result, linePattern))
		{
			CERR << TEXT("key") << result[1] << TEXT(" = ") << result[2] << endl;
			int index = (int)result[1].str()[0] - (int)TEXT('1');
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
			CERR << TEXT("¼ÓÃÜ¹·´íÎó: shieldPC mismatch") << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * sizeof(DWORD) * 2, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));
	
	ofstream keyout("key.bin", ios_base::binary);
	if(keyout.fail())
	{
		CERR << TEXT("±£´ækey´íÎó") << endl;
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
	strcat_s(rsaPublicKey, sizeof(rsaPublicKey), TEXT(".key"));

	int ret = genrsa(KEY_SIZE, rsaPrivateKey, rsaPublicKey, passwd);
	if(ret != 0)
	{
		CERR << TEXT("Éú³ÉRSAÃÜÔ¿´íÎó:") << ret << endl;
		return -8;
	}
	CERR << TEXT("Éú³ÉRSAÃÜÔ¿:") << rsaPrivateKey << TEXT(",") << rsaPublicKey << endl;

	SEED_SIV siv;
	if(loadPublicKeyContent2Pwd(rsaPublicKey, &siv, lockNumber, lock_passwd, rw_passwd))
	{
		CERR << TEXT("Éú³ÉRSA¹«Ô¿¸ñÊ½´íÎó") << endl;
		return -8;
	}
	//printKeyIV(&siv, "key_iv.hex");

	size_t encnum = aes256cbc_enc(dictionary, sizeof(dictionary), licenseFileName, siv.key, siv.iv);
	ret = rsaSign(licenseFileName, licenseRSAEnc, rsaPrivateKey, passwd);
	if(ret != 0)
	{
		CERR << TEXT("RSA sign ´íÎó:") << ret << TEXT(",") << licenseFileName << endl;
		return -9;
	}
	CERR << licenseFileName << TEXT(" => RSA sign => ") << licenseRSAEnc << endl;

	unsigned char inBuf[KEY_SIZE / 8], midBuf[KEY_SIZE / 8], outBuf[KEY_SIZE / 8];
	ifstream licenseRSAStream(licenseRSAEnc);
	licenseRSAStream.read((char*)inBuf, KEY_SIZE / 8);
	if(licenseRSAStream.fail())
	{
		CERR << TEXT("RSA verify ´íÎó, ¶ÁÈ¡ÎÄ¼þ´íÎó") << licenseRSAEnc << endl;
		licenseRSAStream.close();
		return -10;
	}
	licenseRSAStream.close();

	ret = rsaVerify(inBuf, KEY_SIZE / 8, midBuf, rsaPublicKey);
	if(ret <= 0)
	{
		CERR << TEXT("RSA verify ´íÎó:") << licenseRSAEnc << endl;
		return -11;
	}

	// skip magic number and salt
	ret = aes256cbc_dec(midBuf + AES_OFFSET, ret - AES_OFFSET, outBuf, siv.key, siv.iv);
	if(ret <= 0)
	{
		CERR << TEXT("AES decrypt ´íÎó:") << licenseRSAEnc << endl;
		return -12;
	}
	CERR << licenseRSAEnc << TEXT(" => RSA verify => AES decrypt OK") << endl;

	DWORD digestSig[4], *originSig = reinterpret_cast<DWORD*>(&outBuf[DICTIONARY_SIZE * 8]);
	MD5_digest(outBuf, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(digestSig));
	if(digestSig[0] != originSig[0] || digestSig[1] != originSig[1]
		|| digestSig[2] != originSig[2] || digestSig[3] != originSig[3])
	{
		CERR << TEXT("MD5 digest ´íÎó:") << licenseRSAEnc << endl;
		char licenseRSADec[] = "license.dec";
		ofstream unrsaStream(licenseRSADec);
		unrsaStream.write((char*)outBuf, ret);
		unrsaStream.close();
		return -13;
	}
	CERR << TEXT("MD5 digest OK") << endl;
	
	DWORD *loadDict = reinterpret_cast<DWORD*>(outBuf);
	for(int i = 0; i < DICTIONARY_SIZE; ++i)
	{
		if(loadDict[DICTIONARY_SIZE + i] != (ShieldPC(loadDict[i]) ^ loadDict[i]))
		{
			CERR << TEXT("´íÎóµÄÊÚÈ¨ÎÄ¼þ") << endl;
			return -14;
		}
	}

	bool lockPwdOK = true, rwPwdOK = true;
	ofstream passwdstrm("passwd.txt", ios_base::out | ios_base::trunc);
	//¹ÜÀíÃÜÂë
	if(strcmp(init_lock_passwd, lock_passwd))
	{
		DWORD tmp = 0;
		lockPwdOK = SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0);
	}
	if(lockPwdOK)
	{
		CERR << TEXT("¹ÜÀíÃÜÂë:") << lock_passwd << endl;
		passwdstrm << TEXT("¹ÜÀíÃÜÂë:") << lock_passwd << endl;
	}
	else
	{
		CERR << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¹ÜÀíÃÜÂë´íÎó") << hex << LYFGetLastErr() << endl;
		passwdstrm << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¹ÜÀíÃÜÂë´íÎó") << hex << LYFGetLastErr() << TEXT(",Ô­¹ÜÀíÃÜÂë²»±ä:") << init_lock_passwd << endl;
		return -16;
	}
	//¶ÁÐ´ÃÜÂë
	if(strcmp(init_rw_passwd, rw_passwd))
	{
		DWORD tmp = 0;
		rwPwdOK = SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0);
	}
	if(rwPwdOK)
	{
		CERR << TEXT("¶ÁÐ´ÃÜÂë:") << rw_passwd << endl;
		passwdstrm << TEXT("¶ÁÐ´ÃÜÂë:") << rw_passwd << endl;
	}
	else
	{
		CERR << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¶ÁÐ´ÃÜÂë´íÎó") << hex << LYFGetLastErr() << endl;
		passwdstrm << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¶ÁÐ´ÃÜÂë´íÎó") << hex << LYFGetLastErr() << TEXT(",Ô­¶ÁÐ´ÃÜÂë²»±ä:") << init_rw_passwd << endl;
		return -16;
	}
	passwdstrm.close();
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD lockNumber = 0;

	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);

	// If the directory is not specified as a command-line argument, print usage.
	if(argc < 3) return echoUsage(argv[0]);

	bool isInit = (0 == strcmp(argv[1], "init"));
	int initNumber = 0;
	initNumber = atoi(argv[2]);
	if(initNumber == 0 && errno == EINVAL) initNumber = -1;
	initNumber *= 50;
	if(initNumber < 0 || initNumber > 0xFFFF) return echoUsage(argv[0]);

	if(isInit)
	{
		if(argc < 4) return echoUsage(argv[0]);
		// read key.txt
		if(!readKeyFromTxt(argv[3]))
		{
			CERR << TEXT("keyÎÄ¼þ¸ñÊ½´íÎó") << endl;
			return -7;
		}
		// init_lock_passwd and init_rw_passwd
		if(argc >= 5) StringCchCopy(init_lock_passwd, 9, argv[4]);
		if(argc >= 6) StringCchCopy(init_rw_passwd, 9, argv[5]);
	}
	else
	{
		if(argc >= 4) StringCchCopy(init_lock_passwd, 9, argv[3]);
		if(argc >= 5) StringCchCopy(init_rw_passwd, 9, argv[4]);
	}

	//open dog, get lock number
	if(!InitiateLock(0))
	{
		CERR << TEXT("´ò¿ª¼ÓÃÜ¹·´íÎó") << endl;
		return -2;
	}
	atexit(exitHook);

	if(!SetLock(8, &lockNumber, 0, init_lock_passwd, init_lock_passwd, 0, 0))
	{
		CERR << TEXT("»ñÈ¡¼ÓÃÜ¹·ÐòÁÐºÅ´íÎó:") << hex << LYFGetLastErr() << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << lockNumber;
	String lockString = serialStringStream.str();
	CERR << TEXT("¼ÓÃÜ¹·ÐòÁÐºÅ:") << lockString << endl;

	if(isInit && _mkdir(lockString.c_str()) && errno != EEXIST)
	{
		int err = errno;
		CERR << TEXT("´´½¨Ä¿Â¼") << lockString << TEXT("´íÎó:") << err << endl;
		return err;
	}
	if(_chdir(lockString.c_str()))
	{
		int err = errno;
		CERR << TEXT("´ò¿ªÄ¿Â¼") << lockString << TEXT("´íÎó:") << err << endl;
		return err;
	}
	
	unsigned long counter = 0;
	//if(Counter(init_rw_passwd, 0, 0, 0, &counter))
	bool initPasswdOK = (SetLock(0, &counter, 0, "", init_lock_passwd, 0, 0) != FALSE);
	if(isInit)
	{
		if(initPasswdOK)
			CERR << "³õÊ¼ÃÜÂë²âÊÔ³É¹¦" << endl;
		else
		{
			CERR << "³õÊ¼ÃÜÂë²âÊÔÊ§°Ü:" << hex << LYFGetLastErr() << endl;
			return -16;
		}
		int securityResult = initSecurity(lockNumber, lockString);
		if(securityResult) return securityResult;
	}
	else
	{
		IFSTREAM passwdstrm("passwd.txt");
		if(!passwdstrm.good())
		{
			CERR << TEXT("ÎÞ·¨´ò¿ªpasswd.txt") << endl;
			return -15;
		}
		passwdstrm.ignore(9, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(lock_passwd, buffer);
		else
		{
			CERR << TEXT("passwd.txt : ¹ÜÀíÃÜÂë¶ÁÈ¡´íÎó") << endl;
			return -16;
		}
		passwdstrm.ignore(9, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(rw_passwd, buffer);
		else
		{
			CERR << TEXT("passwd.txt : ¶ÁÐ´ÃÜÂë¶ÁÈ¡´íÎó") << endl;
			return -16;
		}
		passwdstrm.close();

		if(initPasswdOK)
		{
			DWORD tmp = 0;
			// change passwd
			if(SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0))
			{
				CERR << TEXT("¹ÜÀíÃÜÂë:") << lock_passwd << endl;
			}
			else
			{
				CERR << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¹ÜÀíÃÜÂë´íÎó") << hex << LYFGetLastErr() << endl;
				return -16;
			}
			if(SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0))
			{
				CERR << TEXT("¶ÁÐ´ÃÜÂë:") << rw_passwd << endl;
			}
			else
			{
				CERR << TEXT("¼ÓÃÜ¹·ÐÞ¸Ä¶ÁÐ´ÃÜÂë´íÎó") << hex << LYFGetLastErr() << endl;
				return -16;
			}
		}
		else
		{
			if(!SetLock(0, &counter, 0, "", lock_passwd, 0, 0))
			{
				CERR << TEXT("³õÊ¼ÃÜÂë£¬passwd.txt¶¼´íÎó") << endl;
				return -16;
			}
			//else passwd.txt OK, do nothing
		}
	}

	CERR << TEXT("³õÊ¼»¯¼ÓÃÜ¹·ÄÚ´æ") << endl;
	CERR << TEXT('|');
	for(int i = 0; i < 78; ++i) CERR << TEXT('-');
	CERR << TEXT('|');
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hOut, &info);
	--info.dwCursorPosition.Y;
	SetConsoleCursorPosition(hOut, info.dwCursorPosition);
	for(int i = 1; i < 1280; ++i)
	{
		DWORD data = 0;
		if(WriteLock(i, &data, lock_passwd, 0, 0))
		{
			if(i % 16 == 0) CERR << TEXT('O');
		}
		else
		{
			CERR << TEXT("¼ÓÃÜ¹·Ð´Èë´íÎó") << hex << LYFGetLastErr() << endl;
			return -17;
		}
	}
	CERR << endl;

	const int BASE_COUNTER = 10;
	initNumber += BASE_COUNTER;
	if(SetLock(1, reinterpret_cast<unsigned long*>(&initNumber), 0, lock_passwd, lock_passwd, 0, 0))
		CERR << TEXT("´ÎÊýÏÞÖÆ³É¹¦") << endl;
	else
	{
		CERR << TEXT("´ÎÊýÏÞÖÆÊ§°Ü:") << hex << LYFGetLastErr() << endl;
		return -18;
	}
	CERR << TEXT("### ¼ÓÃÜ¹·³õÊ¼»¯ÍêÈ«³É¹¦ ###") << endl;
	return 0;
}
