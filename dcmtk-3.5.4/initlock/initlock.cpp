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
	CERR << TEXT("密码:") << lock_passwd << endl;

	int read = fillSeedSIV(siv, sizeof(SEED_SIV), data, base64.size(), PUBKEY_SKIP + (lockNumber % PUBKEY_MOD));
	delete data;
	if(endTag && read == sizeof(SEED_SIV))
		return 0;
	else
		return -1;
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
		CERR << TEXT("加密狗错误: Can't find lock number in ") << buffer << endl;
		return -4;
	}
	FindClose(hFind);

	CERR << TEXT("锁编码:") << lockNumber << endl;
	DWORD serial = 0;
	int retCode = SetLock(8, &serial, NULL, init_passwd);
	if(retCode)
	{
		CERR << TEXT("获取锁序列号错误:") << retCode << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << serial;
	String serialString = serialStringStream.str();
	CERR << TEXT("锁序列号:") << serialString << endl;
	
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
		CERR << TEXT("key文件格式错误") << endl;
		return -7;
	}

	DWORD dictionary[DICTIONARY_SIZE * 2 + 4]; // (rand_request, key_response) * 2 + md5_digest
	RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(dictionary), DICTIONARY_SIZE * 4);
	for(int i = 0; i < DICTIONARY_SIZE; ++i)
	{
		DWORD hard = shieldPC(dictionary[i]), soft = privateShieldPC(dictionary[i]);
		if(hard == soft)
			dictionary[DICTIONARY_SIZE + i] = hard;
		else
		{
			CERR << TEXT("加密狗错误: Can't find lock number in ") << buffer << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));

	if(_mkdir(lockName.c_str()) && errno != EEXIST)
	{
		int err = errno;
		CERR << TEXT("创建目录") << lockName << TEXT("错误:") << err << endl;
		return err;
	}
	_chdir(lockName.c_str());

	char passwd[] = "wlt2911@^$";
	_TCHAR *rsaPrivateKey = "private.rsa", rsaPublicKey[16];
	strcpy_s(rsaPublicKey, sizeof(rsaPublicKey), lockName.c_str());
	strcat_s(rsaPublicKey, sizeof(rsaPublicKey), TEXT(".key"));
	int ret = genrsa(4096, rsaPrivateKey, rsaPublicKey, passwd);
	if(ret != 0)
	{
		CERR << TEXT("生成RSA密钥错误:") << ret << endl;
		return -8;
	}
	CERR << TEXT("生成RSA密钥:") << rsaPrivateKey << TEXT(",") << rsaPublicKey << endl;
	
	_TCHAR *srcfile = "test.txt", *encfile = "test.rsa";
	ofstream testfile(srcfile);
	testfile << "            ------------- begin test -------------            " << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "如果没有看到乱码，证明RSA密钥测试成功。# this is a test file #" << endl;
	testfile << "            -------------- end test --------------" << endl;
	testfile.close();

	ret = rsaSignVerify(srcfile, encfile, rsaPrivateKey, KEY_PRIVKEY, passwd);
	if(ret != 0)
	{
		CERR << TEXT("RSA sign 错误:") << ret << TEXT(",") << srcfile << endl;
		return -9;
	}
	CERR << srcfile << TEXT(" => RSA sign => ") << encfile << endl;

	ret = rsaSignVerify(encfile, NULL, rsaPublicKey, KEY_PUBKEY, passwd);
	if(ret != 0)
	{
		CERR << TEXT("RSA verify 错误:") << ret << TEXT(",") << encfile << endl;
		return -10;
	}
	CERR << endl << encfile << TEXT(" RSA verify OK") << endl;

	SEED_SIV siv;
	if(loadPublicKeyContent(rsaPublicKey, &siv, lockNumber))
	{
		CERR << TEXT("生成RSA公钥格式错误") << endl;
		return -8;
	}
	aes256cbc_enc("byte.aes", reinterpret_cast<unsigned char*>(passwd), strlen(passwd));
	aes256cbc_dec("byte.aes", reinterpret_cast<unsigned char*>(passwd), strlen(passwd));

	// todo: set new passwd
	// todo: generate license file, charge 100
	return 0;
}
