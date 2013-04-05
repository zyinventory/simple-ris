// ServiceWrapper.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <time.h>
#include <direct.h>
#include <conio.h>

#include "commonlib.h"
#include "service.h"
#define DATE_FORMAT_YEAR_TO_SECOND "%Y-%m-%d %H:%M:%S"
#define DATE_FORMAT_COMPACT "%Y%m%d%H%M%S"

using namespace std;

static int argcSV;
static char **argvSV;
static bool runInSCM = false;
static char timeBuffer[20];
int generateTime(const char *format)
{
  time_t t = time( NULL );
  struct tm tmp;
  if( localtime_s( &tmp, &t ) == 0 )
	return strftime(timeBuffer, sizeof(timeBuffer), format, &tmp);
  else
	return 0;
}

void mkcmd(ostringstream *cmdStream, char *s)
{
  if(strchr(s, ' '))
	*cmdStream << '"' << s << '"' << ' ';
  else
	*cmdStream << s << ' ';
}

int realMain(int argc, char **argv)
{
  cout << "begin real work:" << GetCurrentThreadId() << endl;
  // Perform work until service stops.
  ostringstream cmdStream;
  for_each(argv + 1, argv + argc, bind1st(ptr_fun(mkcmd), &cmdStream));
  string cmd(cmdStream.str());
  cmdStream.clear();
  cout << cmd << endl;

  PROCESS_INFORMATION procinfo;
  STARTUPINFO sinfo;
  SECURITY_ATTRIBUTES logSA;

  memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
  memset(&sinfo, 0, sizeof(STARTUPINFO));
  sinfo.cb = sizeof(STARTUPINFO);

  logSA.bInheritHandle = TRUE;
  logSA.lpSecurityDescriptor = NULL;
  logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

  generateTime(DATE_FORMAT_COMPACT);
  ostringstream filename;
  filename << "pacs_log\\" << SERVICE_NAME << '_' << timeBuffer << ".txt";
  HANDLE logFile = CreateFile(filename.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  filename.clear();
  if(logFile != INVALID_HANDLE_VALUE)
  {
	sinfo.dwFlags |= STARTF_USESTDHANDLES;
	sinfo.hStdOutput = logFile;
	sinfo.hStdError = logFile;
	sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  }

  if( CreateProcess(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &sinfo, &procinfo) )
  {
	WaitForInputIdle(procinfo.hProcess, INFINITE);
	// Close process and thread handles to avoid resource leak
	CloseHandle(procinfo.hProcess);
	CloseHandle(procinfo.hThread);
	if(logFile != INVALID_HANDLE_VALUE)	{ CloseHandle(logFile); logFile = INVALID_HANDLE_VALUE; }

	while( ! GetSignalInterruptValue() )
	{
	  if ( generateTime(DATE_FORMAT_YEAR_TO_SECOND) )
		cout << "waiting " << timeBuffer << endl;
	  else
		cerr << "get time error" << endl;
	  Sleep(1000);
	}
  }
  if(logFile != INVALID_HANDLE_VALUE) CloseHandle(logFile);
  return 0;
}

void WINAPI SvcMain(DWORD dummy_argc, LPSTR *dummy_argv)
{
  SvcInit(100);
  // Report running status when initialization is complete.
  ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

  realMain(argcSV, argvSV);

  ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
}

errno_t setEnvParentPID()
{
  char pidString[16];
  return _putenv_s("PARENT_PID", itoa(getpid(), pidString, 10));
}

int _tmain(int argc, _TCHAR* argv[])
{
  SERVICE_TABLE_ENTRY serviceTableEntry[] =
  {
	{ SERVICE_NAME , SvcMain },
	{ NULL, NULL }
  };

  argcSV = argc;
  argvSV = argv;

  Capture_Ctrl_C();

  char **endPos = argv + argc;
  char **pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "-wd")));
  if(pos == endPos) pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "--working-directory")));
  if(pos != endPos) _chdir(*++pos);

  char *buffer = _getcwd(NULL, 0);
  cout << "working dir: " << buffer << endl;
  free(buffer);

  setEnvParentPID();
  
  if( StartServiceCtrlDispatcher( serviceTableEntry ) )
  {
	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.
	runInSCM = true;
	return 0;
  }
  else if( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == GetLastError() )
  {
	// console mode
	return realMain(argcSV, argvSV);
  }
  else
  {
	cerr << "TestDaemon start error" << endl;
	return -1;
  }
}
