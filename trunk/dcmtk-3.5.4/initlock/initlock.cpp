#include "stdafx.h"
using namespace std;

//cov_2char[] and md5crypt() is copied from openssl src/apps/passwd.c
static unsigned const char cov_2char[64]={
	/* from crypto/des/fcrypt.c */
	0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
	0x36,0x37,0x38,0x39,0x41,0x42,0x43,0x44,
	0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,
	0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,
	0x55,0x56,0x57,0x58,0x59,0x5A,0x61,0x62,
	0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
	0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,
	0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A
};

//cov_2char[] and md5crypt() is copied from openssl src/apps/passwd.c
static char *md5crypt(const char *passwd, const char *magic, const char *salt)
{
	static char out_buf[6 + 9 + 24 + 2]; /* "$apr1$..salt..$.......md5hash..........\0" */
	unsigned char buf[MD5_DIGEST_LENGTH];
	char *salt_out;
	int n;
	unsigned int i;
	EVP_MD_CTX md,md2;
	size_t passwd_len, salt_len;

	passwd_len = strlen(passwd);
	out_buf[0] = '$';
	out_buf[1] = 0;
	//assert(strlen(magic) <= 4); /* "1" or "apr1" */
	strncat(out_buf, magic, 4);
	strncat(out_buf, "$", 1);
	strncat(out_buf, salt, 8);
	//assert(strlen(out_buf) <= 6 + 8); /* "$apr1$..salt.." */
	salt_out = out_buf + 2 + strlen(magic);
	salt_len = strlen(salt_out);
	//assert(salt_len <= 8);

	EVP_MD_CTX_init(&md);
	EVP_DigestInit_ex(&md,EVP_md5(), NULL);
	EVP_DigestUpdate(&md, passwd, passwd_len);
	EVP_DigestUpdate(&md, "$", 1);
	EVP_DigestUpdate(&md, magic, strlen(magic));
	EVP_DigestUpdate(&md, "$", 1);
	EVP_DigestUpdate(&md, salt_out, salt_len);

	EVP_MD_CTX_init(&md2);
	EVP_DigestInit_ex(&md2,EVP_md5(), NULL);
	EVP_DigestUpdate(&md2, passwd, passwd_len);
	EVP_DigestUpdate(&md2, salt_out, salt_len);
	EVP_DigestUpdate(&md2, passwd, passwd_len);
	EVP_DigestFinal_ex(&md2, buf, NULL);

	for (i = passwd_len; i > sizeof buf; i -= sizeof buf)
		EVP_DigestUpdate(&md, buf, sizeof buf);
	EVP_DigestUpdate(&md, buf, i);

	n = passwd_len;
	while (n)
	{
		EVP_DigestUpdate(&md, (n & 1) ? "\0" : passwd, 1);
		n >>= 1;
	}
	EVP_DigestFinal_ex(&md, buf, NULL);

	for (i = 0; i < 1000; i++)
	{
		EVP_DigestInit_ex(&md2,EVP_md5(), NULL);
		EVP_DigestUpdate(&md2, (i & 1) ? (unsigned const char *) passwd : buf,
			(i & 1) ? passwd_len : sizeof buf);
		if (i % 3)
			EVP_DigestUpdate(&md2, salt_out, salt_len);
		if (i % 7)
			EVP_DigestUpdate(&md2, passwd, passwd_len);
		EVP_DigestUpdate(&md2, (i & 1) ? buf : (unsigned const char *) passwd,
			(i & 1) ? sizeof buf : passwd_len);
		EVP_DigestFinal_ex(&md2, buf, NULL);
	}
	EVP_MD_CTX_cleanup(&md2);

	{
		/* transform buf into output string */

		unsigned char buf_perm[sizeof buf];
		int dest, source;
		char *output;

		/* silly output permutation */
		for (dest = 0, source = 0; dest < 14; dest++, source = (source + 6) % 17)
			buf_perm[dest] = buf[source];
		buf_perm[14] = buf[5];
		buf_perm[15] = buf[11];

		output = salt_out + salt_len;
		//assert(output == out_buf + strlen(out_buf));

		*output++ = '$';

		for (i = 0; i < 15; i += 3)
		{
			*output++ = cov_2char[buf_perm[i+2] & 0x3f];
			*output++ = cov_2char[((buf_perm[i+1] & 0xf) << 2) | (buf_perm[i+2] >> 6)];
			*output++ = cov_2char[((buf_perm[i] & 3) << 4) | (buf_perm[i+1] >> 4)];
			*output++ = cov_2char[buf_perm[i] >> 2];
		}
		//assert(i == 15);
		*output++ = cov_2char[buf_perm[i] & 0x3f];
		*output++ = cov_2char[buf_perm[i] >> 6];
		*output = 0;
		//assert(strlen(out_buf) < sizeof(out_buf));
	}
	EVP_MD_CTX_cleanup(&md);

	return out_buf;
}

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
	TCHAR szDir[MAX_PATH];
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
	if (length_of_arg > (MAX_PATH - 3))
	{
		CERR << TEXT("Directory path is too long.") << endl;
		return -2;
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopy(szDir, MAX_PATH, argv[1]);
	PathAppend(szDir, TEXT("*"));
  
	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		CERR << TEXT("FindFirstFile Error in ") << szDir << endl;
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
		CERR << TEXT("Can't find lock number in ") << szDir << endl;
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
	
	String hash(md5crypt("D5EACDB6E6529C7E", "1", serialString.c_str()));
	COUT << hash.substr(hash.length() - 8, 8) << endl;
	return 0;
}
