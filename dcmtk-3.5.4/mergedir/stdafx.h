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
    char callingAE[65], callingAddr[40], calledAE[65], calledAddr[40];
    unsigned short port;
} ASSOC_SECTION;

typedef struct {
    unsigned int tag;
    char filename[MAX_PATH], patientID[65], studyUID[65], seriesUID[65], instanceUID[65], xfer[16];
    bool inFile;
} FILE_SECTION;

typedef struct {
    char patientID[65], patientsName[65], birthday[9], height[10], weight[10], sex;
} PATIENT_SECTION;

typedef struct {
    char studyUID[65], studyDate[9], studyTime[15], accessionNumber[65];
} STUDY_SECTION;

typedef struct {
    char seriesUID[65], modality[17];
} SERIES_SECTION;
