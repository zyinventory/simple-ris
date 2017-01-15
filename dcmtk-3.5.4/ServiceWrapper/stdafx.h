// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif //DBG_NEW
#endif //_DEBUG

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
#include <functional>
#include <array>
#include <algorithm>
#include <memory>
#include <numeric>

#define FILE_BUF_SIZE 1024
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"
#include "../dcmdata/include/dcmtk/dcmdata/notify_context.h"
#include "named_pipe_listener.h"

#include "handle_context.h"
#include "handle_context2.h"

int watch_notify(std::string &cmd, std::ofstream &flog);
DWORD process_notify(const handle_context::file_notify* pt, handle_context::NOTIFY_FILE_CONTEXT *pnfc, std::ostream &flog);
int cmd_instance(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_patient(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_study(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
int cmd_series(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog);
void save_notify_context_to_ostream(const handle_context::NOTIFY_FILE_CONTEXT &nfc, bool compress_ok, std::ostream &output);
void send_all_compress_ok_notify(const std::string &association_base, std::ostream &flog);
int x_www_form_codec_encode_to_ostream(const char *str, std::ostream *output);

extern int opt_verbose;
extern bool debug_mode;
extern int store_timeout, assoc_timeout, loop_wait, lock_number, *ptr_license_count;
extern char lock_file_name[64];
extern std::string current_log_path;

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
