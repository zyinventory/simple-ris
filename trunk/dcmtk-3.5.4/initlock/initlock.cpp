#include "stdafx.h"
#include <openssl/rand.h>
#include <shieldpc.h>
#include <liblock.h>
#include <lock.h>
#include "constant.h"
using namespace std;

static char lock_passwd[9] = "";

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

int _tmain(int argc, _TCHAR* argv[])
{
	_TCHAR buffer[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD lockNumber = 0;

	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);

	// If the directory is not specified as a command-line argument, print usage.
	if(argc < 2)
	{
		CERR << TEXT("Usage: ") << argv[0] << TEXT(" <key.txt path> [init_password]") << endl;
		return -1;
	}

	// read key.txt
	IFSTREAM keystrm(argv[1]);
	if(keystrm.bad())
	{
		CERR << buffer << TEXT(" open failed") << endl;
		return -6;
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
	if(!keyOK)
	{
		CERR << TEXT("keyÎÄ¼þ¸ñÊ½´íÎó") << endl;
		return -7;
	}

	//open dog, get lock number
	if(!InitiateLock(0))
	{
		CERR << TEXT("´ò¿ª¼ÓÃÜ¹·´íÎó") << endl;
		return -2;
	}
	atexit(exitHook);

	if(!SetLock(8, &lockNumber, 0, init_passwd, init_passwd, 0, 0))
	{
		CERR << TEXT("»ñÈ¡¼ÓÃÜ¹·ÐòÁÐºÅ´íÎó:") << hex << LYFGetLastErr() << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << lockNumber;
	String lockString = serialStringStream.str();
	CERR << TEXT("¼ÓÃÜ¹·ÐòÁÐºÅ:") << lockString << endl;


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

	if(_mkdir(lockString.c_str()) && errno != EEXIST)
	{
		int err = errno;
		CERR << TEXT("´´½¨Ä¿Â¼") << lockString << TEXT("´íÎó:") << err << endl;
		return err;
	}
	_chdir(lockString.c_str());
	
	char plainFileName[] = "license.plain", licenseFileName[] = "license.aes", licenseRSAEnc[] = "license.key";

	ofstream keyout("key.bin", ios_base::binary);
	if(keyout.fail())
	{
		CERR << TEXT("±£´ækey´íÎó") << endl;
		return -7;
	}
	keyout.write(reinterpret_cast<char*>(key), sizeof(key));
	keyout.write(reinterpret_cast<char*>(&lockNumber), sizeof(lockNumber));
	keyout.close();

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
	if(loadPublicKeyContent(rsaPublicKey, &siv, lockNumber, lock_passwd))
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

	DWORD data = 0;
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
		if(WriteLock(i, &data, init_passwd, 0, 0))
		{
			if(i % 16 == 0) CERR << TEXT('O');
		}
		else
		{
			CERR << TEXT("¼ÓÃÜ¹·Ð´Èë´íÎó") << hex << LYFGetLastErr() << endl;
			return -15;
		}
	}
	CERR << endl;

	if(strcmp(init_passwd, lock_passwd))
	{
		DWORD tmp = 0;
		if(SetLock(7, &tmp, tmp, lock_passwd, init_passwd, 0, 0))
		{
			CERR << TEXT("ÐÂÃÜÂë:") << lock_passwd << endl;
			ofstream passwdstrm("passwd.txt", ios_base::out | ios_base::trunc);
			passwdstrm << lock_passwd << endl;
			passwdstrm.close();
		}
		else
		{
			CERR << TEXT("¼ÓÃÜ¹·ÐÞ¸ÄÃÜÂë´íÎó") << hex << LYFGetLastErr() << endl;
			return -16;
		}
	}

	return 0;
}
