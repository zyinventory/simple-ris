#include "stdafx.h"
#include <openssl/rand.h>
#include <liblock.h>
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
	WIN32_FIND_DATA ffd;
	_TCHAR buffer[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD lockNumber = 0;
	_TCHAR lockName[32];

	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);

	atoi(buffer);
	// If the directory is not specified as a command-line argument, print usage.
	if(argc <= 2)
	{
		CERR << TEXT("Usage: ") << argv[0] << TEXT(" <directory name> [init_password]") << endl;
		return -1;
	}

	size_t length_of_arg;
	// Check that the input path plus 3 is not longer than MAX_PATH.
  	// Three characters are for the "\*" plus NULL appended below.
	StringCchLength(argv[1], MAX_PATH, &length_of_arg);
	if (length_of_arg > (MAX_PATH - 18))  // 18 = strlen("\\12345678\\key.txt") + 1
	{
		CERR << TEXT("Directory path is too long.") << endl;
		return -2;
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopy(buffer, MAX_PATH, argv[1]);
	PathAppend(buffer, TEXT("*"));
	
	if(argc == 3)
		StringCchCopy(init_passwd, 9, argv[2]);
	else
		StringCchCopy(init_passwd, 9, "abcdefgh");

	lockNumber = getLockNumber(buffer, "^(\\d{8})$", TRUE, lockName);
	if(! lockNumber)
	{
		DWORD dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) CERR << TEXT("FindNextFile error ") << dwError << endl;
		FindClose(hFind);
		CERR << TEXT("¼ÓÃÜ¹·´íÎó: Can't find lock number in ") << buffer << endl;
		return -4;
	}

	CERR << TEXT("Ëø±àÂë:") << lockNumber << endl;
	DWORD serial = 0;
	int retCode = SetLock(8, &serial, NULL, init_passwd);
	if(retCode)
	{
		CERR << TEXT("»ñÈ¡ËøÐòÁÐºÅ´íÎó:") << retCode << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << serial;
	String serialString = serialStringStream.str();
	CERR << TEXT("ËøÐòÁÐºÅ:") << serialString << endl;
	
	StringCchCopy(buffer, MAX_PATH, argv[1]);
	PathAppend(buffer, lockName);
	PathAppend(buffer, TEXT("key.txt"));
	IFSTREAM keystrm(buffer);
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

	DWORD dictionary[DICTIONARY_SIZE * 2 + 4]; // (rand_request, key_response) * 2 + md5_digest
	RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(dictionary), DICTIONARY_SIZE * 4);
	for(int i = 0; i < DICTIONARY_SIZE; ++i)
	{
		DWORD hard = shieldPC(dictionary[i]), soft = privateShieldPC(dictionary[i]);
		if(hard == soft)
		{
			dictionary[DICTIONARY_SIZE + i] = hard ^dictionary[i];
			assert(hard == (dictionary[DICTIONARY_SIZE + i] ^ dictionary[i]));
		}
		else
		{
			CERR << TEXT("¼ÓÃÜ¹·´íÎó: shieldPC mismatch") << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));

	if(_mkdir(lockName) && errno != EEXIST)
	{
		int err = errno;
		CERR << TEXT("´´½¨Ä¿Â¼") << lockName << TEXT("´íÎó:") << err << endl;
		return err;
	}
	_chdir(lockName);
	
	char plainFileName[] = "license.plain", licenseFileName[] = "license.aes", licenseRSAEnc[] = "license.key";
	ofstream licenseFile(plainFileName, ios_base::binary);
	size_t dictLength = sizeof(dictionary);
	licenseFile.write(reinterpret_cast<const char*>(dictionary), dictLength);
	licenseFile.close();

	char passwd[] = "wlt2911@^$";
	_TCHAR *rsaPrivateKey = "private.rsa", rsaPublicKey[16];
	strcpy_s(rsaPublicKey, sizeof(rsaPublicKey), lockName);
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
	printKeyIV(&siv, "key_iv.hex");

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
		if(loadDict[DICTIONARY_SIZE + i] != (privateShieldPC(loadDict[i]) ^ loadDict[i]))
		{
			CERR << TEXT("´íÎóµÄÊÚÈ¨ÎÄ¼þ") << endl;
			return -14;
		}
	}

	WORD data[4] = { 0, 0, 0, 0 };
	for(int i = 0; i < 16; ++i)
	{
		if(i == 15) data[3] = 0xffff;
		ret = WriteLock(i, reinterpret_cast<unsigned char*>(&data), init_passwd);
	}
	if(strcmp(init_passwd, lock_passwd))
		ret = SetLock(7, 0, lock_passwd, init_passwd);
	return 0;
}
