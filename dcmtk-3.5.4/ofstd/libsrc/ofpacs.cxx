#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofstring.h"

#include <strstream>
#include <time.h>
#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#include <direct.h>        /* for _mkdir() */
#include <process.h>     /* needed for declaration of getpid() */
#endif

OFCondition MkdirRecursive(OFString& subdirectoryPath)
{
  // check if the subdirectory is already existent
  if( OFStandard::dirExists(subdirectoryPath) )
  {
    return EC_Normal;
  }
  else
  {
    size_t position = subdirectoryPath.rfind(PATH_SEPARATOR);
    if(position != OFString_npos)
    {
      OFString upperLevel = subdirectoryPath.substr(0, position);
      OFCondition mkResult = MkdirRecursive(upperLevel);
      if(mkResult != EC_Normal)
      {
        return mkResult;
      }
      // else: upper level exist, create current level
    }

    // if it is not existent create it
    #ifdef HAVE_WINDOWS_H
    if( _mkdir( subdirectoryPath.c_str() ) == -1 )
    #else
    if( mkdir( subdirectoryPathAndName.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) == -1 )
    #endif
    {
	  CERR << "Could not create subdirectory " << subdirectoryPath.c_str() << endl;
      return EC_IllegalParameter;
    }
    else
    {
      return EC_Normal;
    }
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
	pid_t pid = _getpid();
	std::ostrstream format;
	format << "pacs_log" << pathSeparator << "%Y" << pathSeparator << "%m" << pathSeparator << "%d" << pathSeparator << "%H%M%S_" << appName << '_' << pid << ".txt" << std::ends;
	size_t pathLen = strftime(buf, bufLen, format.rdbuf()->str(), &calendar);
	format.rdbuf()->freeze(false);
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
