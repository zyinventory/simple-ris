#include "stdafx.h"
#include "constant.h"
using namespace std;

class numpunct_no_gouping : public numpunct_byname<char>
{
public:
    numpunct_no_gouping(const char* name) : numpunct_byname<char>(name){ }
protected:
    virtual String do_grouping() const { return TEXT(""); } // no grouping
};

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
		CERR << TEXT("Can't find lock number in ") << buffer << endl;
		return -4;
	}
	FindClose(hFind);

	COUT << TEXT("Ëø±àÂë:") << lockNumber << endl;
	DWORD serial = 0;
	int retCode = SetLock(8, &serial, init_passwd, init_passwd);
	if(retCode)
	{
		CERR << TEXT("»ñÈ¡ËøÐòÁÐºÅ´íÎó:") << retCode << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << serial;
	String serialString = serialStringStream.str();
	COUT << TEXT("ËøÐòÁÐºÅ:") << serialString << endl;
	
	StringCchCopy(buffer, MAX_PATH, argv[1]);
	PathAppend(buffer, lockName.c_str());
	PathAppend(buffer, TEXT("key.txt"));
	IFSTREAM keystrm(buffer);
	if(keystrm.bad())
	{
		COUT << buffer << TEXT(" open failed") << endl;
		return -6;
	}
	DWORD key[4] = {0, 0, 0, 0};
	bool keyOK = false;
	REGEX linePattern(TEXT("^key(\\d) *= *(\\d+)$"));
	while(! keystrm.getline(buffer, MAX_PATH).fail())
	{
		match_results<const _TCHAR*> result;
		if(regex_match(buffer, result, linePattern))
		{
			COUT << TEXT("key") << result[1] << TEXT(" = ") << result[2] << endl;
			int index = (int)result[1].str()[0] - (int)TEXT('1');
			key[index] = atoi(result[2].str().c_str());
			if(keyOK = !any_of(key, key + 4, [](int value) { return value == 0; })) break;
		}
	}
	keystrm.close();
	if(!keyOK)
	{
		COUT << TEXT("keyÎÄ¼þ¸ñÊ½´íÎó") << endl;
		return -7;
	}
	sprintf_s(buffer, "%04X%04X%04X%04X", key[0], key[1], key[2], key[3]);
	String hash(md5crypt(buffer, "1", serialString.c_str()));
	COUT << TEXT("ÃÜÂë:") << hash.substr(hash.length() - 8, 8) << endl;
	// todo: set new passwd

	_TCHAR *rsaPrivateKey = "private.rsa", *rsaPublicKey = "public.rsa";
	int ret = genrsa(512, rsaPrivateKey, rsaPublicKey);
	if(ret != 0)
	{
		CERR << TEXT("Éú³ÉRSAÃÜÔ¿´íÎó:") << ret << endl;
		return -8;
	}
	COUT << TEXT("Éú³ÉRSAÃÜÔ¿:") << rsaPrivateKey << TEXT(",") << rsaPublicKey << endl;
	
	_TCHAR *srcfile = "test.txt", *encfile = "test.rsa";
	ret = rsaSignVerify(srcfile, encfile, rsaPrivateKey, KEY_PRIVKEY);
	if(ret != 0)
	{
		CERR << TEXT("RSA sign ´íÎó:") << ret << TEXT(",") << srcfile << endl;
		return -9;
	}
	COUT << srcfile << TEXT(" => RSA sign => ") << encfile << endl;

	ret = rsaSignVerify(encfile, NULL, rsaPublicKey, KEY_PUBKEY);
	if(ret != 0)
	{
		CERR << TEXT("RSA verify ´íÎó:") << ret << TEXT(",") << encfile << endl;
		return -10;
	}
	COUT << endl << encfile << TEXT(" RSA verify OK") << endl;

	// todo: ec key

	// todo: generate license file, charge 100
	return 0;
}
