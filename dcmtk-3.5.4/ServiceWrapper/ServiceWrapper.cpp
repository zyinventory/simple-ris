// ServiceWrapper.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <direct.h>

#include "commonlib.h"
#include "service.h"

using namespace std;

static int argcSV;
static char **argvSV, timeBuffer[48];
static bool startXCS = false, xcsFound = false;
static HANDLE logFile = INVALID_HANDLE_VALUE;

const char *dirmakerCommand;
int commandDispatcher(const char *queueName, int processorNumber);

void mkcmd(ostringstream *cmdStream, const char *s)
{
	if(strchr(s, ' '))
		*cmdStream << '"' << s << '"' << ' ';
	else
		*cmdStream << s << ' ';

	if(!xcsFound)
	{
		if(startXCS)
		{
			dirmakerCommand = s;
			xcsFound = true;
		}
		else
			startXCS = !strcmp("-xcs", s);
	}
}

int realMain(int argc, char **argv)
{
	// Perform work until service stops.
	ostringstream cmdStream;
	for_each(argv + 2, argv + argc, bind1st(ptr_fun(mkcmd), &cmdStream)); // skip program and ServiceName
	string cmd(cmdStream.str());
	cmdStream.clear();
	//cout << cmd << endl;

	PROCESS_INFORMATION procinfo;
	STARTUPINFO sinfo;

	memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

	SECURITY_ATTRIBUTES logSA;
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE logFile = INVALID_HANDLE_VALUE;
	generateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_storescp.txt", timeBuffer, sizeof(timeBuffer));
	if(prepareFileDir(timeBuffer))
		logFile = CreateFile(timeBuffer, GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

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
	}
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return commandDispatcher(QUEUE_NAME, min(MAX_CORE, sysInfo.dwNumberOfProcessors - 1));
}

void WINAPI SvcMain(DWORD dummy_argc, LPSTR *dummy_argv)
{
	SvcInit(100);
	// Report running status when initialization is complete.
	ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

	realMain(argcSV, argvSV);

	ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
}

int _tmain(int argc, _TCHAR* argv[])
{
	locale::global(locale(CHINESE_LOCAL));
	if(argc < 3)
	{
		cout << "usage: ServiceWrapper [ServiceName] [CommandLine]" << endl;
		return -2;
	}
	else
	{
		strcpy_s(getServiceName(), 128, argv[1]); // argv[1] must be ServiceName
	}

	SERVICE_TABLE_ENTRY serviceTableEntry[] =
	{
		{ getServiceName() , SvcMain },
		{ NULL, NULL }
	};

	argcSV = argc;
	argvSV = argv;

	Capture_Ctrl_C();
	
	setEnvParentPID();

	changeWorkingDirectory(argc, argv);
/*
	char *buffer = _getcwd(NULL, 0);
	cout << "working dir: " << buffer << endl;
	free(buffer);
*/
	int stdfile = -1, oldstdout = -1, oldstderr = -1;
	generateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_service.txt", timeBuffer, sizeof(timeBuffer));
	if(prepareFileDir(timeBuffer)
		&& ! _sopen_s(&stdfile, timeBuffer, _O_APPEND | _O_CREAT | _O_TEXT | _O_WRONLY, _SH_DENYWR, _S_IREAD | _S_IWRITE))
	{
		oldstdout = _dup(_fileno(stdout));
		oldstderr = _dup(_fileno(stderr));
		_dup2(stdfile, _fileno(stdout));
		_dup2(stdfile, _fileno(stderr));
		_close(stdfile);
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	int ret = 0;
	if( StartServiceCtrlDispatcher( serviceTableEntry ) )
	{
		// This call returns when the service has stopped. 
		// The process should simply terminate when the call returns.
		ret = 0;
	}
	else if( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == GetLastError() )
	{
		// console mode
		ret = realMain(argcSV, argvSV);
	}
	else
	{
		cerr << "TestDaemon start error" << endl;
		ret = -1;
	}
	if(oldstdout != -1) _dup2(oldstdout, _fileno(stdout));
	if(oldstderr != -1) _dup2(oldstderr, _fileno(stderr));
	return ret;
}
