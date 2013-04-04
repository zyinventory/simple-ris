#include "stdafx.h"
#include <signal.h>
//#include <time.h>
//#include <errno.h>
//#include <strstream>

//#ifdef _WIN32
//#include <process.h>     /* needed for declaration of getpid() */
//#endif

//#ifndef HAVE_NO_TYPEDEF_PID_T  // not include cfwin32.h
//typedef int pid_t;
//#endif

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
