#include "stdafx.h"
#include "commonlib.h"
#define PATH_SEPARATOR '\\'

using namespace std;

COMMONLIB_API errno_t setEnvParentPID()
{
  char pidString[16];
  _itoa_s(_getpid(), pidString, 16, 10);
  return _putenv_s("PARENT_PID", pidString);
}

COMMONLIB_API bool IsASCII(const char *str)
{
  bool isAscii = true;
  if(str == NULL) return isAscii;
  size_t length = strlen(str);
  for(unsigned int i = 0; i < length; i++)
  {
    if(str[i] > '~' || str[i] < 0)
    {
      isAscii = false;
      break;
    }
  }
  return isAscii;
}

COMMONLIB_API const char *trim_const(const char *s, int maxStrLen, const char **ptail)
{
	const char *head = NULL, *tail = NULL;
	int i = 0;
	if(s == NULL) return s;
	for(i = 0; s[i] && i < maxStrLen; ++i)
	{
		bool issp = isspace(s[i]);
		if(head == NULL)
		{
			if(!issp)
			{
				head = &s[i];
				if(ptail == NULL) break;
			}
		}
		else
		{
			if(issp && tail == NULL) tail = &s[i];
			else if(!issp && tail != NULL) tail = NULL;
		}
	}
	if(head == NULL)
	{
		head = &s[i];
		// s[i] == '\0' ::=> i <= maxStrLen
		if(ptail && s[i] == '\0') *ptail = head;
		// else do nothing, keep *ptail 's value unmodified
	}
	else if(ptail && s[i] == '\0') // s[i] == '\0' ::=> i <= maxStrLen
	{
		if(tail) *ptail = tail;
		else *ptail = &s[i];
	}
	// else do nothing, keep *ptail 's value unmodified
	return head;
}

COMMONLIB_API char *rtrim(char *s, int maxStrLen)
{
	const char *tail = NULL;
	trim_const(s, maxStrLen, &tail);
	if(tail) *const_cast<char*>(tail) = '\0';
	return s;
}

COMMONLIB_API char *trim(char *s, int maxStrLen)
{
	const char *tail = NULL;
	char *head = const_cast<char*>(trim_const(s, maxStrLen, &tail));
	if(tail) *const_cast<char*>(tail) = '\0';
	return head;
}

static int signalInterruptFlag = 0;

COMMONLIB_API void SignalInterruptHandler(int signal)
{
  signalInterruptFlag = signal;
}

COMMONLIB_API void Capture_Ctrl_C()
{
  signalInterruptFlag = 0;
  signal(SIGINT, SignalInterruptHandler);
}

COMMONLIB_API int GetSignalInterruptValue()
{
  return signalInterruptFlag;
}

COMMONLIB_API LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime)
{
  FILETIME fileTime;
  SYSTEMTIME utcTime;
  LARGE_INTEGER fileSize;

  HANDLE handle = ::CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if( handle != INVALID_HANDLE_VALUE) 
  {
    if(localTime != NULL && ::GetFileTime(handle, NULL, NULL, &fileTime))
    {
      FileTimeToSystemTime(&fileTime, &utcTime);
      SystemTimeToTzSpecificLocalTime(NULL, &utcTime, localTime);
    }
    ::GetFileSizeEx(handle, &fileSize);
    ::CloseHandle(handle);
	return fileSize.QuadPart;
  }
  else
	return -1LL;
}

COMMONLIB_API bool MkdirRecursive(const char *subdir)
{
  // check if the subdirectory is already existent
  if( _mkdir(subdir) )
  {
	if(errno == EEXIST)
	  return true;
  }
  else
  {
	return true;
  }

  string subdirectoryPath = subdir;
  size_t position = subdirectoryPath.rfind(PATH_SEPARATOR);
  if(position != string::npos)
  {
    string upperLevel = subdirectoryPath.substr(0, position);
	bool mkResult = MkdirRecursive(upperLevel.c_str());
    if(mkResult != true)
    {
      return mkResult;
    }
    // else: upper level exist, create current level
  }

  // if it is not existent create it
  if( _mkdir( subdirectoryPath.c_str() ) == -1 && errno != EEXIST)
  {
	cerr << "Could not create subdirectory " << subdirectoryPath.c_str() << endl;
    return false;
  }
  else
  {
    return true;
  }
}

COMMONLIB_API bool prepareFileDir(const char *path)
{
  string filePath = path;
  string::size_type p = filePath.rfind('\\');
  if(p == string::npos || p == 0) return true;
  return MkdirRecursive(filePath.substr(0, p).c_str());
}

//return 0 if successful, otherwise errno
COMMONLIB_API int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator)
{
  time_t now = time(NULL);
  struct tm calendar;
  errno_t err = localtime_s(&calendar, &now);
  if(!err)
  {
	int pid = _getpid();
	ostringstream format;
	format << "pacs_log" << pathSeparator << "%Y" << pathSeparator << "%m" << pathSeparator << "%d" << pathSeparator << "%H%M%S_" << appName << '_' << pid << ".txt" << std::ends;
	size_t pathLen = strftime(buf, bufLen, format.rdbuf()->str().c_str(), &calendar);
	if( ! pathLen ) err = EINVAL;
  }
  return err;
}

COMMONLIB_API BOOL DeleteEmptyFile(const char *filePath)
{
  LARGE_INTEGER fileSize;
  HANDLE handle = ::CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if( handle != INVALID_HANDLE_VALUE) 
  {
    ::GetFileSizeEx(handle, &fileSize);
    ::CloseHandle(handle);
	if(fileSize.QuadPart == 0LL)
	  return  ::DeleteFile(filePath);
	else
	  return TRUE;
  }
  else
	return FALSE;
}

COMMONLIB_API time_t dcmdate2tm(int dcmdate)
{
  struct tm timeBirth;
  timeBirth.tm_year = dcmdate / 10000 - 1900;
  timeBirth.tm_mon = dcmdate % 10000 / 100 - 1;
  timeBirth.tm_mday = dcmdate % 100;
  timeBirth.tm_hour = 0;
  timeBirth.tm_min = 0;
  timeBirth.tm_sec = 0;
  return mktime(&timeBirth);
}

COMMONLIB_API int ChangeToPacsWebSub(char *pPacsBase, size_t buff_size, const char *subdir)
{
    char *static_buf = GetPacsBase();
    size_t requiredSize = strlen(static_buf) + 1;
    strcat_s(static_buf, MAX_PATH, "\\pacs"); // PACS_BASE + <web_dir>
    if(subdir) strcat_s(static_buf, MAX_PATH, subdir); // PACS_BASE + <web_dir> + <sub_dir>
    errno_t en = _chdir(static_buf);
    static_buf[requiredSize - 1] = '\0';
    if(en) return en;
    return strcpy_s(pPacsBase, buff_size, static_buf);
}

COMMONLIB_API int changeWorkingDirectory(int argc, char **argv, char *pPacsBase)
{
	if(argc > 0)
	{
		char **endPos = argv + argc;
		char **pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "-wd")));
		if(pos == endPos) pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "--working-directory")));
		if(pos != endPos)
		{
			if(! _chdir(*++pos))
			{
				if(pPacsBase) strcpy_s(pPacsBase, MAX_PATH, *++pos);
				return -2;
			}
			else
				return 0;
		}
		// else get working dir from PACS_BASE in env
	}
    return ChangeToPacsWebSub(pPacsBase, MAX_PATH);
}

int sys_core_num = 4, worker_core_num = 2;
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
        sys_core_num = sysInfo.dwNumberOfProcessors;
        worker_core_num = max(2, sys_core_num - 2);
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
