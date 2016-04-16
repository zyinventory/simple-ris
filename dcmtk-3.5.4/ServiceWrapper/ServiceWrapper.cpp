// ServiceWrapper.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "commonlib.h"
#include "service.h"

using namespace std;

static int argcSV;
static char **argvSV, timeBuffer[48];
static bool startXCS = false, xcsFound = false;
static ofstream flog;

const char *dirmakerCommand;
bool opt_verbose = false;

std::ostream& time_header_out(ostream &os)
{
	char timeBuffer[32];
	if(GenerateTime(DATE_FORMAT_YEAR_TO_SECOND, timeBuffer, sizeof(timeBuffer))) os << timeBuffer << ' ';
	return os;
}

static size_t checkDiskFreeSpaceInMB(const char * path)
{
	DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	size_t freeGB = 0;
	if(GetDiskFreeSpace(path, &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters))
	{
		long long freespace = dwFreeClusters;
		freespace *= dwSectPerClust * dwBytesPerSect;
		freeGB = static_cast<size_t>(freespace / (1024 * 1024));
	}
	return freeGB;
}

static void collectDayXmlFile(const char *filepath, list<WIN32_FIND_DATA> &findDataList)
{
	// if(opt_verbose) time_header_out(flog) << "start searching " << filepath << endl;
	WIN32_FIND_DATA wfd;
	char fileFilter[MAX_PATH];
	strcpy_s(fileFilter, filepath);
	PathAppend(fileFilter, "*.*");
	HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
	if (hDiskSearch == INVALID_HANDLE_VALUE)  // 如果没有找到或查找失败
	{
		DWORD winerr = GetLastError();
		if(ERROR_FILE_NOT_FOUND == winerr)
			time_header_out(flog) << fileFilter << " not found" << endl;
		else
			time_header_out(flog) << fileFilter << " unknown error: " << winerr << endl;
		return;
	}
	do
	{
		if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0) 
			continue; // skip . ..
		fileFilter[strlen(filepath)] = '\0';
		PathAppend(fileFilter, wfd.cFileName);
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			collectDayXmlFile(fileFilter, findDataList);
		}
		else if(strstr(wfd.cFileName, ".xml"))
		{
			strcpy_s(wfd.cFileName, MAX_PATH, fileFilter);
			findDataList.push_back(wfd);
		}
		else if(opt_verbose)
			time_header_out(flog) << filepath << '\\' << wfd.cFileName << " name is not '.xml', skip it." << endl;
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // 关闭查找句柄
}

static bool noMoreFree = false;
static bool findAndDeleteUnarchived(list<WIN32_FIND_DATA> &findDataList, size_t lower)
{
	findDataList.sort([](WIN32_FIND_DATA &wfd1, WIN32_FIND_DATA &wfd2)
		{ return *reinterpret_cast<__int64*>(&wfd1.ftLastAccessTime) > *reinterpret_cast<__int64*>(&wfd2.ftLastAccessTime); });
	do{
		WIN32_FIND_DATA &backItem = findDataList.back();
		//if(opt_verbose) time_header_out(flog) << "pop_back " << backItem.cFileName << ", last access time: "
		//	<< *reinterpret_cast<__int64*>(&backItem.ftLastAccessTime) << endl;
		if(deleteDayStudy(backItem.cFileName))
		{
			HANDLE hFile = CreateFile(backItem.cFileName, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(INVALID_HANDLE_VALUE == hFile)
			{
				DWORD winerr = GetLastError();
				time_header_out(flog) << backItem.cFileName << " open file error:" << winerr << endl;
			}
			else
			{
				FILETIME ft;
				SYSTEMTIME st;
				GetSystemTime(&st);              // Gets the current system time
				SystemTimeToFileTime(&st, &ft);  // Converts the current system time to file time format
				if(!SetFileTime(hFile, (LPFILETIME) NULL, &ft, (LPFILETIME) NULL))
				{
					DWORD winerr = GetLastError();
					time_header_out(flog) << backItem.cFileName << " set last access time error:" << winerr << endl;
				}
			}
		}
		size_t freeMB = checkDiskFreeSpaceInMB("archdir\\");
		if(freeMB > lower) return true;
		findDataList.pop_back();
	}while(!findDataList.empty());

	size_t lastFreeMB = checkDiskFreeSpaceInMB("archdir\\");
	time_header_out(flog) << "All day xml is free, free " << lastFreeMB << " M, need " << lower << " M" << endl;
	noMoreFree = true;
	return false;
}

void autoCleanPacsDiskByStudyDate()
{
	if(noMoreFree) return;
	size_t freeMB = checkDiskFreeSpaceInMB("archdir\\");
	size_t lower = 20 * 1024; //20G
	if(freeMB > lower) return;
	list<WIN32_FIND_DATA> findDataList;
	collectDayXmlFile("indexdir\\00080020", findDataList);
	if(findDataList.empty())
	{
		if(opt_verbose) time_header_out(flog) << "No day xml files to free, free " << freeMB << " M, need " << lower << " M" << endl;
	}
	else
		findAndDeleteUnarchived(findDataList, lower);
	if(opt_verbose) time_header_out(flog) << "auto clean done" << endl;	
}

static void mkcmd(ostringstream *cmdStream, const char *s)
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

static int realMain(int argc, char **argv)
{
	//deleteSubTree("storedir");
	//resetStatus(QUEUE_NAME);

	// Perform work until service stops.
	ostringstream cmdStream;
#ifdef _DEBUG
    char *p = strrchr(argv[0], '\\');
    if(p)
    {
        ++p;
        cmdStream.write(argv[0], p - argv[0]);
        cmdStream << "dcmqrscp.exe ";
    }
    else
        cmdStream << "..\\bin\\dcmqrscp.exe ";
#else
    cmdStream << "..\\bin\\dcmqrscp.exe ";
#endif
	for_each(argv + 2, argv + argc, bind1st(ptr_fun(mkcmd), &cmdStream)); // skip program and ServiceName
	string cmd(cmdStream.str());
	cmdStream.clear();
	opt_verbose = (string::npos != cmd.find(" -v "));

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
	GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_dcmqrscp.txt", timeBuffer, sizeof(timeBuffer));
	if(PrepareFileDir(timeBuffer))
		logFile = CreateFile(timeBuffer, GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if(logFile != INVALID_HANDLE_VALUE)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdOutput = logFile;
		sinfo.hStdError = logFile;
		sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	}

    char *qrcmd = new char[cmd.length() + 1];
    strcpy_s(qrcmd, cmd.length() + 1, cmd.c_str());
	if( CreateProcess(NULL, qrcmd, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &sinfo, &procinfo) )
	{
        delete[] qrcmd;
		//WaitForInputIdle(procinfo.hProcess, INFINITE);
		CloseHandle(procinfo.hProcess);
		CloseHandle(procinfo.hThread);
        CloseHandle(logFile);
        
        if(opt_verbose) time_header_out(flog) << "create dcmqrscp process OK: " << cmd << endl;

		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		//return commandDispatcher(QUEUE_NAME, min(MAX_CORE, sysInfo.dwNumberOfProcessors - 1));
        while(GetSignalInterruptValue() == 0)
        {
            // todo: listen notify dir
            Sleep(1000);
        }
	}
	else
	{
        delete[] qrcmd;
		time_header_out(flog) << "create dcmqrscp process error: " << cmd << endl;
        CloseHandle(logFile);
		return -2;
	}
}

static void WINAPI SvcMain(DWORD dummy_argc, LPSTR *dummy_argv)
{
	SvcInit(100);
	// Report running status when initialization is complete.
	ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );
	CoInitialize(NULL);
	realMain(argcSV, argvSV);
	CoUninitialize();
	ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
}

int _tmain(int argc, _TCHAR* argv[])
{
	//locale::global(locale(CHINESE_LOCAL));
	if(argc < 2)
	{
		cerr << "使用说明: ServiceWrapper [ServiceName] [params]" << endl;
		return -2;
	}
	else
	{
		if(strcpy_s(service_name, argv[1])) // argv[1] must be ServiceName
		    cerr << "ServiceName too long: " << service_name << endl;
	}

	SERVICE_TABLE_ENTRY serviceTableEntry[] =
	{
		{ service_name , SvcMain },
		{ NULL, NULL }
	};

	argcSV = argc;
	argvSV = argv;

	Capture_Ctrl_C();

	if(ChangeToPacsWebSub(COMMONLIB_PACS_BASE, sizeof(COMMONLIB_PACS_BASE))) return -3;

	GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_service.txt", timeBuffer, sizeof(timeBuffer));
	if(PrepareFileDir(timeBuffer))
    {
        flog.open(timeBuffer);
        if(flog.fail())
        {
            cerr << "ServiceWrapper open " << timeBuffer << " failed" << endl;
            return -4;
        }
    }
	else
		return -4;
    /*
	if( ! captureStdoutToLogStream(flog))
	{
		flog << "captureStdoutToLogStream(flog) failed" << endl;
		return -5;
	}
    */
	int ret = 0;
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\dcmtk_ServiceWrapper");

    if(hMutex == NULL)
    {
        displayErrorToCerr("ServiceWrapper CreateMutex()", GetLastError(), &flog);
		ret = -1;
        goto exit_service_wrapper;
    }
    
	if( StartServiceCtrlDispatcher( serviceTableEntry ) )
	{
		// This call returns when the service has stopped. 
		// The process should simply terminate when the call returns.
		ret = 0;
	}
	else if( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == GetLastError() )
	{
		// console mode
		CoInitialize(NULL);
		ret = realMain(argcSV, argvSV);
		CoUninitialize();
	}
	else
	{
		flog << "ServiceWrapper start error" << endl;
		ret = -1;
	}
	
    if(hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
exit_service_wrapper:
	//releaseStdout(flog);
    if(flog.is_open()) flog.close();
	return ret;
}
