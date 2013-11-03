#include "stdafx.h"
#include <openssl/rand.h>
#include "constant.h"
using namespace std;

class numpunct_no_gouping : public numpunct_byname<char>
{
public:
    numpunct_no_gouping(const char* name) : numpunct_byname<char>(name){ }
protected:
    virtual String do_grouping() const { return TEXT(""); } // no grouping
};

static int loadPublicKeyContent(const char* publicKey, SEED_SIV *siv, DWORD lockNumber)
{
	ifstream keystrm(publicKey);
	if(keystrm.fail()) return -2;
	ostringstream contentBase64;
	bool startTag = false, endTag = false;
	char buffer[82];
	while(!endTag)
	{
		keystrm.getline(buffer, sizeof(buffer));
		if(keystrm.fail()) break;
		if(buffer[0] == '-' && buffer[1] == '-')
		{
			if(startTag)
				endTag = true;
			else
				startTag = true;
		}
		else if(startTag && !endTag)
			contentBase64 << buffer << endl;
	}
	string base64(contentBase64.str());
	char *data = new char[base64.size()];
	base64.copy(data, base64.size());

	ostringstream saltBase64;
	saltBase64 << hex << setw(8) << setfill('0') << privateShieldPC(lockNumber);
	String hash(md5crypt(base64.c_str(), "1", saltBase64.str().c_str()));
	hash.copy(lock_passwd, 8, hash.length() - 8);
	lock_passwd[8] = '\0';
	CERR << TEXT("����:") << lock_passwd << endl;

	int read = fillSeedSIV(siv, sizeof(SEED_SIV), data, base64.size(), PUBKEY_SKIP + (lockNumber % PUBKEY_MOD));
	delete data;
	if(endTag && read == sizeof(SEED_SIV))
		return 0;
	else
		return -1;
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
	WIN32_FIND_DATA ffd;
	_TCHAR buffer[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD lockNumber = 0;
	String lockName;

	locale locChina(locale("chinese"), new numpunct_no_gouping("chinese"));
	locale::global(locChina);
	// If the directory is not specified as a command-line argument, print usage.
	if(argc != 2)
	{
		CERR << TEXT("Usage: ") << argv[0] << TEXT(" <directory name>") << endl;
		return -1;
	}
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
  
	// Find the first file in the directory.
	hFind = FindFirstFile(buffer, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		CERR << TEXT("FindFirstFile Error in ") << buffer << endl;
		return -3;
	} 
	// List all the files in the directory with some info about them.
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			REGEX pattern(TEXT("^\\d{8}$"));
			lockName = ffd.cFileName;
			if(regex_match(lockName, pattern))
			{
				SCANF_S(ffd.cFileName, TEXT("%d"), &lockNumber);
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if(! lockNumber)
	{
		DWORD dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) CERR << TEXT("FindNextFile error ") << dwError << endl;
		FindClose(hFind);
		CERR << TEXT("���ܹ�����: Can't find lock number in ") << buffer << endl;
		return -4;
	}
	FindClose(hFind);

	CERR << TEXT("������:") << lockNumber << endl;
	DWORD serial = 0;
	int retCode = SetLock(8, &serial, NULL, init_passwd);
	if(retCode)
	{
		CERR << TEXT("��ȡ�����кŴ���:") << retCode << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << serial;
	String serialString = serialStringStream.str();
	CERR << TEXT("�����к�:") << serialString << endl;
	
	StringCchCopy(buffer, MAX_PATH, argv[1]);
	PathAppend(buffer, lockName.c_str());
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
		CERR << TEXT("key�ļ���ʽ����") << endl;
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
			assert(hard == dictionary[DICTIONARY_SIZE + i] ^ dictionary[i]);
		}
		else
		{
			CERR << TEXT("���ܹ�����: shieldPC mismatch") << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));

	if(_mkdir(lockName.c_str()) && errno != EEXIST)
	{
		int err = errno;
		CERR << TEXT("����Ŀ¼") << lockName << TEXT("����:") << err << endl;
		return err;
	}
	_chdir(lockName.c_str());
	
	char plainFileName[] = "license.plain", licenseFileName[] = "license.aes", licenseRSAEnc[] = "license.key";
	ofstream licenseFile(plainFileName, ios_base::binary);
	size_t dictLength = sizeof(dictionary);
	licenseFile.write(reinterpret_cast<const char*>(dictionary), dictLength);
	licenseFile.close();

	char passwd[] = "wlt2911@^$";
	_TCHAR *rsaPrivateKey = "private.rsa", rsaPublicKey[16];
	strcpy_s(rsaPublicKey, sizeof(rsaPublicKey), lockName.c_str());
	strcat_s(rsaPublicKey, sizeof(rsaPublicKey), TEXT(".key"));
	int ret = genrsa(KEY_SIZE, rsaPrivateKey, rsaPublicKey, passwd);
	if(ret != 0)
	{
		CERR << TEXT("����RSA��Կ����:") << ret << endl;
		return -8;
	}
	CERR << TEXT("����RSA��Կ:") << rsaPrivateKey << TEXT(",") << rsaPublicKey << endl;

	SEED_SIV siv;
	if(loadPublicKeyContent(rsaPublicKey, &siv, lockNumber))
	{
		CERR << TEXT("����RSA��Կ��ʽ����") << endl;
		return -8;
	}
	printKeyIV(&siv, "key_iv.hex");

	size_t encnum = aes256cbc_enc(dictionary, sizeof(dictionary), licenseFileName, siv.key, siv.iv);
	ret = rsaSign(licenseFileName, licenseRSAEnc, rsaPrivateKey, passwd);
	if(ret != 0)
	{
		CERR << TEXT("RSA sign ����:") << ret << TEXT(",") << licenseFileName << endl;
		return -9;
	}
	CERR << licenseFileName << TEXT(" => RSA sign => ") << licenseRSAEnc << endl;

	unsigned char inBuf[KEY_SIZE / 8], midBuf[KEY_SIZE / 8], outBuf[KEY_SIZE / 8];
	ifstream licenseRSAStream(licenseRSAEnc);
	licenseRSAStream.read((char*)inBuf, KEY_SIZE / 8);
	if(licenseRSAStream.fail())
	{
		CERR << TEXT("RSA verify ����, ��ȡ�ļ�����") << licenseRSAEnc << endl;
		licenseRSAStream.close();
		return -10;
	}
	licenseRSAStream.close();

	ret = rsaVerify(inBuf, KEY_SIZE / 8, midBuf, rsaPublicKey);
	if(ret <= 0)
	{
		CERR << TEXT("RSA verify ����:") << licenseRSAEnc << endl;
		return -11;
	}

	// skip magic number and salt
	ret = aes256cbc_dec(midBuf + AES_OFFSET, ret - AES_OFFSET, outBuf, siv.key, siv.iv);
	if(ret <= 0)
	{
		CERR << TEXT("AES decrypt ����:") << licenseRSAEnc << endl;
		return -12;
	}
	CERR << licenseRSAEnc << TEXT(" => RSA verify => AES decrypt OK") << endl;

	DWORD digestSig[4], *originSig = reinterpret_cast<DWORD*>(&outBuf[DICTIONARY_SIZE * 8]);
	MD5_digest(outBuf, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(digestSig));
	if(digestSig[0] != originSig[0] || digestSig[1] != originSig[1]
		|| digestSig[2] != originSig[2] || digestSig[3] != originSig[3])
	{
		CERR << TEXT("MD5 digest ����:") << licenseRSAEnc << endl;
		char licenseRSADec[] = "license.dec";
		ofstream unrsaStream(licenseRSADec);
		unrsaStream.write((char*)outBuf, ret);
		unrsaStream.close();
		return -13;
	}
	CERR << TEXT("MD5 digest OK") << endl;

	// todo: set new passwd
	// todo: generate license file, charge 100
	return 0;
}
