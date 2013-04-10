#include <time.h>
#include <errno.h>
#include <direct.h>        /* for _mkdir() */
#include <process.h>
#include <string>
#include <iostream>
#include <sstream>
#define PATH_SEPARATOR '\\'

using namespace std;

bool MkdirRecursive_dcmnet(const char *subdir)
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
	bool mkResult = MkdirRecursive_dcmnet(upperLevel.c_str());
    if(mkResult != true)
    {
      return mkResult;
    }
    // else: upper level exist, create current level
  }

  // if it is not existent create it
  if( _mkdir( subdirectoryPath.c_str() ) == -1 )
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
int GenerateLogPath_dcmnet(char *buf, size_t bufLen, const char *appName, const char pathSeparator)
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
