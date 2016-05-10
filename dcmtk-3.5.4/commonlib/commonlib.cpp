#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;

COMMONLIB_API char COMMONLIB_PACS_BASE[MAX_PATH] = "";

COMMONLIB_API size_t in_process_sequence_dll(char *buff, size_t buff_size, const char *prefix)
{
    return in_process_sequence(buff, buff_size, prefix);
}

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

COMMONLIB_API int UTF8ToGBK(unsigned char *lpUTF8Str, char *lpGBKStr, int nGBKStrLen)
{
    wchar_t * lpUnicodeStr = NULL;
    int nRetLen = 0;

    //如果UTF8字符串为NULL则出错退出
    //输出缓冲区为空则返回转换后需要的空间大小
    if(lpUTF8Str == NULL || lpGBKStr == NULL || nGBKStrLen <= 0) return 0; 

    nRetLen = MultiByteToWideChar(CP_UTF8, 0, (char*)lpUTF8Str, -1, NULL, NULL);  //获取转换到Unicode编码后所需要的字符空间长度
    lpUnicodeStr = new wchar_t[nRetLen + 1];  //为Unicode字符串空间

    nRetLen = MultiByteToWideChar(CP_UTF8, 0, (char*)lpUTF8Str, -1, lpUnicodeStr, nRetLen);  //转换到Unicode编码
    if(nRetLen == 0) goto clean_allocated_memory; //转换失败则出错退出

    nRetLen = WideCharToMultiByte(936, 0, lpUnicodeStr, -1, NULL, NULL, NULL, NULL);  //获取转换到GBK编码后所需要的字符空间长度
    if(nGBKStrLen < nRetLen) goto clean_allocated_memory; //如果输出缓冲区长度不够则退出

    nRetLen = WideCharToMultiByte(936, 0, lpUnicodeStr, -1, lpGBKStr, nRetLen, NULL, NULL);  //转换到GBK编码

clean_allocated_memory:
    if(lpUnicodeStr) delete []lpUnicodeStr;
    return nRetLen;
}

COMMONLIB_API const char *trim_const(const char *s, int maxStrLen, const char **ptail)
{
	const char *head = NULL, *tail = NULL;
	int i = 0;
	if(s == NULL) return s;
	for(i = 0; s[i] && i < maxStrLen; ++i)
	{
		bool issp = (0 != isspace(s[i]));
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
    // reverse strcspn()
    const char *p = subdir + strlen(subdir);
    while(p != subdir && *p != '/' && *p != '\\') --p;
    if(p != subdir)
    {
        string upperLevel(subdir, p - subdir);
	    bool mkResult = MkdirRecursive(upperLevel.c_str());
        if(mkResult != true)
        {
            return mkResult;
        }
        // else: upper level exist, create current level
    }

    // if it is not existent create it
    if( _mkdir(subdir) == -1 && errno != EEXIST)
    {
        cerr << __FUNCSIG__ " Could not create subdirectory " << subdir << endl;
        return false;
    }
    else
    {
        return true;
    }
}

COMMONLIB_API bool PrepareFileDir(const char *path)
{
    // reverse strcspn()
    const char *p = path + strlen(path);
    while(p != path && *p != '/' && *p != '\\') --p;
    if(p != path)
    {
        string upperLevel(path, p - path);
        return MkdirRecursive(upperLevel.c_str());
    }
    else
        return true;
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

size_t sys_core_num = 4, worker_core_num = 2;
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        GetPacsBase();
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
