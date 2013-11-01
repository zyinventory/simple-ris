// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <atlbase.h>
#include <atlstr.h>

// TODO: 在此处引用程序需要的其他头文件
#include <direct.h>
#include <strsafe.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <algorithm>  
#include <numeric>
#include "lock.h"

extern "C"
{
	char *md5crypt(const char *passwd, const char *magic, const char *salt);
	int genrsa(int num, char *privateKey, char *publicKey, char *pass);
	int rsaSign(char *infile, char *outfile, char *keyfile, char *pass);
	int rsaVerify(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, const char *keyfile);
	int aes256cbc_enc(void *content, size_t contentLength, char *filename, unsigned char *key, unsigned char* iv);
	int aes256cbc_dec(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, unsigned char *key, unsigned char* iv);
	int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start);
	void MD5_digest(void *data, size_t dataLength, unsigned char *md);
	void base64test();
}

#define COUT cout			//wcout
#define CERR cerr			//wcerr
#define String string		//wstring
#define REGEX regex			//wregex
#define SCANF_S sscanf_s	//swscanf_s
#define IFSTREAM ifstream   //wifstream
