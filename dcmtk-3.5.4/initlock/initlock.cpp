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
	cerr << "����Ȩģʽ(init): " << endl
		<< '\t' << app << " init <����> <key.txt�ļ�·��> [�������� ��д����]" << endl
		<< endl
		<< "������Ȩģʽ(batch_init, reset): " << endl
		<< '\t' << app << " batch_init <����> <key.txt�ļ�·��> [�������� ��д����]" << endl
		<< '\t' << app << " reset <����> [�������� ��д����]" << endl;
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
			cerr << "���ܹ�����: shieldPC mismatch" << endl;
			return -4;
		}
	}
	MD5_digest(dictionary, DICTIONARY_SIZE * sizeof(DWORD) * 2, reinterpret_cast<unsigned char*>(&dictionary[DICTIONARY_SIZE * 2]));
	
	ofstream keyout("key.bin", ios_base::binary);
	if(keyout.fail())
	{
		cerr << "����key����" << endl;
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
		cerr << "����RSA��Կ����:" << ret << endl;
		return -8;
	}
	cerr << "����RSA��Կ:" << rsaPrivateKey << "," << rsaPublicKey << endl;

	SEED_SIV siv;
	if(loadPublicKeyContent2Pwd(rsaPublicKey, &siv, lockNumber, lock_passwd, rw_passwd))
	{
		cerr << "����RSA��Կ��ʽ����" << endl;
		return -8;
	}
	//printKeyIV(&siv, "key_iv.hex");

	size_t encnum = aes256cbc_enc(dictionary, sizeof(dictionary), licenseFileName, siv.key, siv.iv);
	ret = rsaSign(licenseFileName, licenseRSAEnc, rsaPrivateKey, passwd);
	if(ret != 0)
	{
		cerr << "RSA sign ����:" << ret << "," << licenseFileName << endl;
		return -9;
	}
	cerr << licenseFileName << " => RSA sign => " << licenseRSAEnc << endl;

	unsigned char inBuf[KEY_SIZE / 8], midBuf[KEY_SIZE / 8], outBuf[KEY_SIZE / 8];
	ifstream licenseRSAStream(licenseRSAEnc);
	licenseRSAStream.read((char*)inBuf, KEY_SIZE / 8);
	if(licenseRSAStream.fail())
	{
		cerr << "RSA verify ����, ��ȡ�ļ�����" << licenseRSAEnc << endl;
		licenseRSAStream.close();
		return -10;
	}
	licenseRSAStream.close();

	ret = rsaVerify(inBuf, KEY_SIZE / 8, midBuf, rsaPublicKey);
	if(ret <= 0)
	{
		cerr << "RSA verify ����:" << licenseRSAEnc << endl;
		return -11;
	}

	// skip magic number and salt
	ret = aes256cbc_dec(midBuf + AES_OFFSET, ret - AES_OFFSET, outBuf, siv.key, siv.iv);
	if(ret <= 0)
	{
		cerr << "AES decrypt ����:" << licenseRSAEnc << endl;
		return -12;
	}
	cerr << licenseRSAEnc << " => RSA verify => AES decrypt OK" << endl;

	DWORD digestSig[4], *originSig = reinterpret_cast<DWORD*>(&outBuf[DICTIONARY_SIZE * 8]);
	MD5_digest(outBuf, DICTIONARY_SIZE * 8, reinterpret_cast<unsigned char*>(digestSig));
	if(digestSig[0] != originSig[0] || digestSig[1] != originSig[1]
		|| digestSig[2] != originSig[2] || digestSig[3] != originSig[3])
	{
		cerr << "MD5 digest ����:" << licenseRSAEnc << endl;
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
			cerr << "�������Ȩ�ļ�" << endl;
			return -14;
		}
	}

	BOOL lockPwdOK = TRUE, rwPwdOK = TRUE;
	ofstream passwdstrm("passwd.txt", ios_base::out | ios_base::trunc);
	//��������
	if(strcmp(init_lock_passwd, lock_passwd))
	{
		DWORD tmp = 0;
		lockPwdOK = SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0);
	}
	if(lockPwdOK)
	{
		cerr << "��������:" << lock_passwd << endl;
		passwdstrm << "��������:" << lock_passwd << endl;
	}
	else
	{
		cerr << "���ܹ��޸Ĺ����������" << hex << LYFGetLastErr() << endl;
		passwdstrm << "���ܹ��޸Ĺ����������" << hex << LYFGetLastErr() << ",ԭ�������벻��:" << init_lock_passwd << endl;
		return -16;
	}
	//��д����
	if(strcmp(init_rw_passwd, rw_passwd))
	{
		DWORD tmp = 0;
		rwPwdOK = SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0);
	}
	if(rwPwdOK)
	{
		cerr << "��д����:" << rw_passwd << endl;
		passwdstrm << "��д����:" << rw_passwd << endl;
	}
	else
	{
		cerr << "���ܹ��޸Ķ�д�������" << hex << LYFGetLastErr() << endl;
		passwdstrm << "���ܹ��޸Ķ�д�������" << hex << LYFGetLastErr() << ",ԭ��д���벻��:" << init_rw_passwd << endl;
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
			cerr << "key�ļ���ʽ����" << endl;
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
		cerr << "�򿪼��ܹ�����" << endl;
		return -2;
	}
	atexit(exitHook);

	if(!SetLock(8, &lockNumber, 0, init_lock_passwd, init_lock_passwd, 0, 0))
	{
		cerr << "��ȡ���ܹ����кŴ���:" << hex << LYFGetLastErr() << endl;
		return -5;
	}
	ostringstream serialStringStream;
	serialStringStream << lockNumber;
	string lockString = serialStringStream.str();
	cerr << "���ܹ����к�:" << lockString << endl;

	if(isInit && _mkdir(lockString.c_str()))
	{
		int err = errno;
		if(errno == EEXIST)
			cerr << "Ŀ¼" << lockString << "�Ѵ��ڣ����ܸ���" << endl;
		else
			cerr << "����Ŀ¼" << lockString << "����:" << err << endl;
		return err;
	}
	if(_chdir(lockString.c_str()))
	{
		int err = errno;
		cerr << "��Ŀ¼" << lockString << "����:" << err << endl;
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
			cerr << "��ʼ������Գɹ�" << endl;
		else
		{
			cerr << "��ʼ�������ʧ��:" << hex << LYFGetLastErr() << endl;
			return -16;
		}
		int securityResult = initSecurity(lockNumber, lockString);
		if(securityResult) return securityResult;
		ofstream flagstrm("flag.txt");
		if(!flagstrm.good())
		{
			cerr << "�޷�����flag.txt" << endl;
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
			cerr << "�޷���passwd.txt" << endl;
			return -15;
		}
		passwdstrm.ignore(9, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(lock_passwd, buffer);
		else
		{
			cerr << "passwd.txt : ���������ȡ����" << endl;
			return -16;
		}
		passwdstrm.ignore(10, ':');
		passwdstrm >> buffer;
		if(8 == strlen(buffer))
			strcpy_s(rw_passwd, buffer);
		else
		{
			cerr << "passwd.txt : ��д�����ȡ����" << endl;
			return -16;
		}
		passwdstrm.close();

		if(initPasswdOK)
		{
			DWORD tmp = 0;
			// change passwd
			if(SetLock(7, &tmp, tmp, lock_passwd, init_lock_passwd, 0, 0))
			{
				cerr << "��������:" << lock_passwd << endl;
			}
			else
			{
				cerr << "���ܹ��޸Ĺ����������" << hex << LYFGetLastErr() << endl;
				return -16;
			}
			if(SetLock(7, &tmp, tmp, rw_passwd, init_rw_passwd, 0, 0))
			{
				cerr << "��д����:" << rw_passwd << endl;
			}
			else
			{
				cerr << "���ܹ��޸Ķ�д�������" << hex << LYFGetLastErr() << endl;
				return -16;
			}
		}
		else // init_lock_passwd error
		{	// test: lock_passwd(from passwd.txt) OK?
			if(!SetLock(0, &counter, 0, "", lock_passwd, 0, 0))
			{
				cerr << "��ʼ���룬passwd.txt������" << endl;
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
				cerr << "�޷��򿪻򴴽�flag.txt" << endl;
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

	cerr << "��ʼ�����ܹ��ڴ�" << endl;
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
			cerr << "���ܹ�д�����" << hex << LYFGetLastErr() << endl;
			return -17;
		}
	}
	cerr << endl;

	initNumber += DUMMY_ZERO;
	if(SetLock(1, reinterpret_cast<unsigned long*>(&initNumber), 0, lock_passwd, lock_passwd, 0, 0))
		cerr << "�������Ƴɹ�" << endl;
	else
	{
		cerr << "��������ʧ��:" << hex << LYFGetLastErr() << endl;
		return -18;
	}
	cerr << "### ���ܹ���ʼ����ȫ�ɹ� ###" << endl;
	return 0;
}
