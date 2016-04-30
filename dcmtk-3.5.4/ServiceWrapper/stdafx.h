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
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <iterator>
#include <algorithm>
#include <functional>
#include "handle_context.h"

#define NOTIFY_BASE "store_notify"
typedef std::map<HANDLE, handle_context::notify_file*> HANDLE_MAP;
typedef std::pair<HANDLE, handle_context::notify_file*> HANDLE_PAIR;
typedef std::list<handle_context::CMOVE_NOTIFY_CONTEXT*> NOTIFY_LIST;

int watch_notify(std::string &cmd, std::ostream &flog);
int cmd_instance(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog);
int cmd_patient(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog);
int cmd_study(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog);
int cmd_series(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog);

extern bool opt_verbose;
extern NOTIFY_LIST compress_queue;

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
