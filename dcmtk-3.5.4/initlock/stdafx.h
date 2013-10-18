// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#include <atlbase.h>
#include <atlstr.h>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <strsafe.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include "lock.h"

extern "C"
{
	char *md5crypt(const char *passwd, const char *magic, const char *salt);
	int genrsa(int num, char *privateKey, char *publicKey);
	int rsaSignVerify(char *infile, char *outfile, char *keyfile, int keyType);
}

#define COUT cout			//wcout
#define CERR cerr			//wcerr
#define String string		//wstring
#define REGEX regex			//wregex
#define SCANF_S sscanf_s	//swscanf_s
#define IFSTREAM ifstream   //wifstream
