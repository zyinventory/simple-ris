#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <libb24.h>
#include "lock.h"
#include "liblock.h"

using namespace std;

const char regxPattern[] = "^(\\d{8,12})\\.key$";
extern "C" int getLockNumber(const char *filter, int isDirectory, char *filenamebuf, size_t filenamebuf_size)
{
	WIN32_FIND_DATA ffd;
	unsigned int lockNumber = -1;

	HANDLE hFind = FindFirstFile(filter, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		cerr << TEXT("FindFirstFile Error in ") << filter << endl;
		return -1;
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
				/*
				char buffer[MAX_PATH];
				size_t copied = result[1].str()._Copy_s(buffer, MAX_PATH, result[1].length(), 0);
				buffer[min(copied, MAX_PATH - 1)] = '\0';
				*/
				sscanf_s(result[1].str().c_str(), TEXT("%d"), &lockNumber);
				if(filenamebuf)
				{
					size_t copied = fileName._Copy_s(filenamebuf, filenamebuf_size, fileName.length(), 0);
					filenamebuf[min(copied, filenamebuf_size - 1)] = '\0';
				}
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
	return lockNumber;
}

extern "C" void mkpasswd(const char *base64, unsigned int salt, char *lock_passwd)
{
	ostringstream saltBase64;
	long salt_tr = 0;
	Lock32_Function(salt, &salt_tr, 0);
	saltBase64 << hex << setw(8) << setfill('0') << salt_tr;
	string hash(md5crypt(base64, "1", saltBase64.str().c_str()));
	size_t copied = hash._Copy_s(lock_passwd, 9, 8, hash.length() - 8);
	lock_passwd[min(copied, 8)] = '\0';
}

extern "C" int loadPublicKeyContentImpl(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char **ppdata, char *gen_rw_passwd)
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
	*ppdata = new char[base64.size() + 1];
	size_t copied = base64._Copy_s(*ppdata, base64.size() + 1, base64.size());
	(*ppdata)[min(copied, base64.size())] = '\0';
	//if(gen_lock_passwd) mkpasswd(*ppdata, lockNumber, gen_lock_passwd);
	if(gen_rw_passwd) mkpasswd((*ppdata) + 8, lockNumber, gen_rw_passwd);

	int read = fillSeedSIV(siv, sizeof(SEED_SIV), *ppdata, base64.size(), PUBKEY_SKIP + (lockNumber % PUBKEY_MOD));
	//delete data;
	if(endTag && read == sizeof(SEED_SIV))
		return 0;
	else
		return -1;
}

extern "C" int loadPublicKeyContentRW(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char *gen_rw_passwd)
{
	char *data = NULL;
	int result = loadPublicKeyContentImpl(publicKey, siv, lockNumber, &data, gen_rw_passwd);
	if(data) delete data;
	return result;
}

extern "C" int invalidLock(const char *licenseRSAEnc, const char *rsaPublicKey, SEED_SIV *sivptr)
{
	unsigned char inBuf[KEY_SIZE / 8], midBuf[KEY_SIZE / 8], outBuf[KEY_SIZE / 8];
	ifstream licenseRSAStream(licenseRSAEnc, ios_base::binary);
	licenseRSAStream.read((char*)inBuf, KEY_SIZE / 8);
	if(licenseRSAStream.fail())
	{
		licenseRSAStream.close();
		return -10;
	}
	licenseRSAStream.close();

	int ret = rsaVerify(inBuf, KEY_SIZE / 8, midBuf, rsaPublicKey);
	if(ret <= 0) return -11;

	// skip magic number and salt
	ret = aes256cbc_dec(midBuf + AES_OFFSET, ret - AES_OFFSET, outBuf, sivptr->key, sivptr->iv);
	if(ret <= 0) return -12;

	long digestSig[4], *originSig = reinterpret_cast<long*>(&outBuf[DICTIONARY_SIZE * 8]);
	MD5_digest(outBuf, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(digestSig));
	if(digestSig[0] != originSig[0] || digestSig[1] != originSig[1]
		|| digestSig[2] != originSig[2] || digestSig[3] != originSig[3])
		return -13;

	time_t t = time( NULL );
	struct tm tmp;
	localtime_s( &tmp, &t );
	int i = tmp.tm_yday % DICTIONARY_SIZE;
	long *dict = reinterpret_cast<long*>(outBuf);
	long dict_tr = 0;
	Lock32_Function(dict[i], &dict_tr, 0);
	if(dict[i] == (dict[DICTIONARY_SIZE + i] ^ dict_tr))
		return 0;
	else
		return -14;
}

extern "C" int currentCount(char *passwd)
{
	long licenseCount;
	if(SetLock(0, reinterpret_cast<unsigned long*>(&licenseCount), 0, "s.Wa2pUc", passwd, 0, 0))
	{
		if(licenseCount > 0 && licenseCount < DUMMY_ZERO)
			licenseCount = 0;
		else
			licenseCount -= DUMMY_ZERO;
		return licenseCount;
	}
	else
		return -15;
}

extern "C" int decreaseCount(char *passwd)
{
	if(currentCount(passwd) > 0)
	{
		long licenseCount = 0;
		if(Counter(passwd, 0, 0, 0, reinterpret_cast<unsigned long*>(&licenseCount)))
			return licenseCount - DUMMY_ZERO;
	}
	return -15;
}

extern "C" int increaseCount(char *passwd, int charge)
{
	unsigned long licenseCount = currentCount(passwd);
	if(licenseCount >= 0)
	{
		licenseCount += charge + DUMMY_ZERO;
		if(licenseCount < DUMMY_ZERO) licenseCount = DUMMY_ZERO;
		if(SetLock(1, &licenseCount, 0, "bOl=y8Nm", passwd, 0, 0))
			return licenseCount - DUMMY_ZERO;
	}
	return -15;
}

extern "C" int loadPublicKeyContent2Pwd(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char *gen_lock_passwd, char *gen_rw_passwd)
{
	char *data = NULL;
	int result = loadPublicKeyContentImpl(publicKey, siv, lockNumber, &data, gen_rw_passwd);
	if(data && gen_lock_passwd) mkpasswd(data, lockNumber, gen_lock_passwd);
	if(data) delete data;
	return result;
}

extern "C" int licenseCounter()
{
	int licenseCount = -1;
	char passwd[9] = "", filename[64] = "..\\etc\\*.key";
	int lockNumber = getLockNumber(filename, FALSE, filename + 7, 64 - 7);
	if(lockNumber <= 0) return -1;
	SEED_SIV siv;
	if(!InitiateLock(0)) return -1;
	if(0 == loadPublicKeyContentRW(filename, &siv, lockNumber, passwd))
	{
		if(!invalidLock("..\\etc\\license.key", filename, &siv))
		{
			licenseCount = currentCount(passwd);
			if(licenseCount < 0 || licenseCount > 0xFFFF) licenseCount = -1;
		}
	}
	TerminateLock(0);
	return licenseCount;
}