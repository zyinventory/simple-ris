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
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#include <functional>

#define FILE_BUF_SIZE 1024
#include "handle_context.h"

int watch_notify(std::string &cmd, std::ostream &flog);
int cmd_instance(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_patient(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_study(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_series(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
void save_notify_context_to_ostream(const handle_context::NOTIFY_FILE_CONTEXT &cnc, std::ostream &output);
void send_all_compress_ok_notify(const std::string &association_base, std::ostream &flog);

extern bool opt_verbose;
extern handle_context::NOTIFY_LIST compress_queue;
extern handle_context::STUDY_MAP map_dicomdir;

// ------- old ServiceWrapper -------

#define MAX_CORE 16

typedef struct _WorkerProcess {
	std::string *instancePath, *csvPath, *studyUid; // command level
	bool integrityStudy;  // Is study integrity?
    HANDLE hProcess, hThread, mutexIdle, mutexRec, hChildStdInWrite; // process level
	HANDLE hLogFile; std::string *logFilePath; // slot level
} WorkerProcess, *PWorkerProcess, *LPWorkerProcess;

bool RedirectMessageLabelEqualWith(const char *equalWith, const char *queueName);
int commandDispatcher(const char *queueName, int processorNumber);
void autoCleanPacsDiskByStudyDate();
bool deleteDayStudy(const char *dayxml);
bool __stdcall captureStdoutToLogStream(std::ostream &flog);
void __stdcall releaseStdout(std::ostream &flog);
