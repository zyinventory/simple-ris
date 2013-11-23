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
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <iterator>
#include <algorithm>
#include <functional>

#define MAX_CORE 16

typedef struct _WorkerProcess {
	std::string *instancePath, *csvPath, *studyUid; // command level
    HANDLE hProcess, hThread, mutexIdle, mutexRec, hChildStdInWrite; // process level
	HANDLE hLogFile; std::string *logFilePath; // slot level
} WorkerProcess, *PWorkerProcess, *LPWorkerProcess;

bool RedirectMessageLabelEqualWith(const char *equalWith, const char *queueName);
bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName);
HRESULT QLetEveryoneFullControl(LPCWSTR wszFormatNameBuffer);
int commandDispatcher(const char *queueName, int processorNumber);
void autoCleanPacsDiskByStudyDate();
bool deleteDayStudy(const char *dayxml);
void resetStatus(const char *queueName);
