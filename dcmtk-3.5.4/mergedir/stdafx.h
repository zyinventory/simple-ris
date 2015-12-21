// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif // DEBUG_NEW
#endif // _DEBUG

// TODO: 在此处引用程序需要的其他头文件
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#define TYPE_ASSOC      'T'
#define TYPE_FILE       'F'
#define TYPE_PATIENT    'P'
#define TYPE_STUDY      'S'
#define TYPE_SERIES     'E'
#define TYPE_INSTANCE   'I'

#define ASSOC_ESTA  0x00010010
#define FILE_START  0x00011000
#define ASSOC_TERM  0xFFFFFFFF
#define ASSOC_ABORT 0xFFFFFFFD

typedef struct {
    std::string callingAE, callingAddr, calledAE, calledAddr;
    unsigned short port;
} ASSOC_CONTEXT;
