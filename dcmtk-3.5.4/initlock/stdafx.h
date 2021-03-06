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
#include <assert.h>
#include <direct.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <algorithm>  
#include <numeric>
#include <lock.h>
#include <openssl/rand.h>
#include <shieldpc.h>
#include <liblock.h>
#include <gencard.h>

#ifdef __cplusplus
extern "C"
{
#endif
	int genrsa(int num, char *privateKey, char *publicKey, char *pass);
	int rsaSign(char *infile, char *outfile, char *keyfile, char *pass);
	int rsaVerify(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, const char *keyfile);
	int aes256cbc_enc(void *content, size_t contentLength, char *filename, unsigned char *key, unsigned char* iv);
	int aes256cbc_dec(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, unsigned char *key, unsigned char* iv);
#ifdef __cplusplus
}
#endif
