#include "stdafx.h"
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <direct.h>        /* for _mkdir() */
#include <process.h>

#include <string>
#include <iostream>
#include <sstream>

#define PATH_SEPARATOR '\\'

using namespace std;

bool IsASCII(const char *str)
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

/* param:
  s:	  input string
  maxLen: max length of string s
 */
char *rtrim(char *s, int maxLen)
{
  if(s == NULL) return 0;
  if(maxLen == -1)
	maxLen = strlen(s) - 1;
  else
	maxLen = min(maxLen, static_cast<int>(strlen(s))) - 1;
  int i;
  for(i = maxLen; i >= 0; --i)
  {
	if(s[i]==' ' || s[i]=='\n' || s[i] == '\r' || s[i]=='\t')
	  s[i]='\0';
	else
	  break;
  }
  // new size = i + 1
  return s;
}

static int signalInterruptFlag = 0;

void SignalInterruptHandler(int signal)
{
  signalInterruptFlag = signal;
}

void Capture_Ctrl_C()
{
  signalInterruptFlag = 0;
  signal(SIGINT, SignalInterruptHandler);
}

int GetSignalInterruptValue()
{
  return signalInterruptFlag;
}

LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime)
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

bool MkdirRecursive(const char *subdir)
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

//return 0 if successful, otherwise errno
int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator)
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

BOOL DeleteEmptyFile(const char *filePath)
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

time_t dcmdate2tm(int dcmdate)
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
