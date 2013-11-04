#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <lock.h>
#include "liblock.h"

using namespace std;

extern "C" unsigned int getLockNumber(const char *filter, const char *regxPattern, int isDirectory, char *filenamebuf)
{
	WIN32_FIND_DATA ffd;
	unsigned int lockNumber = 0;

	HANDLE hFind = FindFirstFile(filter, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		cerr << TEXT("FindFirstFile Error in ") << filter << endl;
		return -3;
	}
	
	regex pattern(TEXT(regxPattern));
	match_results<string::const_iterator> result;
	// List all the files in the directory with some info about them.
	do
	{
		if (isDirectory ? ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY : true)
		{
			string fileName(ffd.cFileName);
			if(regex_match(fileName, result, pattern))
			{
				char buffer[MAX_PATH];
				result[1].str().copy(buffer, result[1].length(), 0);
				buffer[result[1].length()] = '\0';
				sscanf_s(buffer, TEXT("%d"), &lockNumber);
				if(filenamebuf)
				{
					fileName.copy(filenamebuf, fileName.length(), 0);
					filenamebuf[fileName.length()] = '\0';
				}
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
	return lockNumber;
}

void mkpasswd(const char *base64, unsigned int salt, char *lock_passwd)
{
	ostringstream saltBase64;
	saltBase64 << hex << setw(8) << setfill('0') << Lock32_Function(salt);
	string hash(md5crypt(base64, "1", saltBase64.str().c_str()));
	hash.copy(lock_passwd, 8, hash.length() - 8);
	lock_passwd[8] = '\0';
}

int loadPublicKeyContent(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char *gen_passwd)
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
	char *data = new char[base64.size() + 1];
	base64.copy(data, base64.size());
	data[base64.size()] = '\0';
	mkpasswd(data, lockNumber, gen_passwd);

	int read = fillSeedSIV(siv, sizeof(SEED_SIV), data, base64.size(), PUBKEY_SKIP + (lockNumber % PUBKEY_MOD));
	delete data;
	if(endTag && read == sizeof(SEED_SIV))
		return 0;
	else
		return -1;
}
