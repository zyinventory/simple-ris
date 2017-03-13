#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;

COMMONLIB_API char COMMONLIB_PACS_BASE[MAX_PATH] = "", COMMONLIB_PACS_TEMP[MAX_PATH] = "";

COMMONLIB_API size_t in_process_sequence_dll(char *buff, size_t buff_size, const char *prefix)
{
    return in_process_sequence(buff, buff_size, prefix);
}

COMMONLIB_API const char* NotifyFileContextStorePath(handle_context::NOTIFY_FILE_CONTEXT_FILE_SECTION *pnfc, char sp)
{
    HashStr(pnfc->studyUID, pnfc->unique_filename, sizeof(pnfc->unique_filename));
    pnfc->unique_filename[8] = sp;
    SeriesInstancePath(pnfc->seriesUID, pnfc->instanceUID, pnfc->unique_filename + 9, sizeof(pnfc->unique_filename) - 9, sp);
    sprintf_s(pnfc->hash, "%c%c%c%c%c%c%c%c%c%c%c",
        pnfc->unique_filename[0], pnfc->unique_filename[1], sp, pnfc->unique_filename[2], pnfc->unique_filename[3], sp, 
        pnfc->unique_filename[4], pnfc->unique_filename[5], sp, pnfc->unique_filename[6], pnfc->unique_filename[7]);
    return pnfc->unique_filename;
}

COMMONLIB_API errno_t setEnvParentPID()
{
  char pidString[16];
  _itoa_s(_getpid(), pidString, 16, 10);
  return _putenv_s("PARENT_PID", pidString);
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

COMMONLIB_API size_t normalize_dicom_date(size_t buff_len, char *buff, const char *studyDate)
{
	size_t count = 0;
    size_t converted = 0;
    wchar_t wsd[65];
    mbstowcs_s(&converted, wsd, studyDate, 64);
	wcout << wsd << endl;

    wregex pattern(L"(\\d{4})[^\\d]?(\\d{1,2})[^\\d]?(\\d{1,2})");
	wcmatch mr;
    wstring result;
	if(regex_match(wsd, mr, pattern, regex_constants::match_flag_type::format_first_only | regex_constants::match_flag_type::format_no_copy))
	{
		//wcmatch::const_reference == const sub_match<const wchar_t*> &
        wcmatch::const_iterator it = ++mr.begin();
        if(it->length() < 4) result.append(4 - it->length(), L'0');
        result.append(it->first, it->length());
        ++it;
        if(it->length() < 2) result.append(2 - it->length(), L'0');
        result.append(it->first, it->length());
        ++it;
        if(it->length() < 2) result.append(2 - it->length(), L'0');
        result.append(it->first, it->length());

        if(result.length() == 8)
            wcstombs_s(&count, buff, buff_len, result.c_str(), result.length());
	}
	else
		cerr << __FUNCSIG__ << " not match study date: " << studyDate << endl;
    return count;
}

size_t WORKER_CORE_NUM = 0;
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    string ini_path;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        ini_path = GetPacsBase();
        ini_path.append("\\etc\\settings.ini");
        if(LoadSettings(ini_path.c_str(), cerr, false))
        {
            char buff[1024];
            if(GetSetting("WorkerCoreNum", buff, sizeof(buff)))
            {
                WORKER_CORE_NUM = atoi(buff);
            }
        }
        if(WORKER_CORE_NUM <= 0 || WORKER_CORE_NUM > 32)
        {
            SYSTEM_INFO sysInfo;
		    GetSystemInfo(&sysInfo);
            size_t sys_core_num = sysInfo.dwNumberOfProcessors;
            if(sys_core_num <= 4) WORKER_CORE_NUM = 2;
            else if(sys_core_num >= 8) WORKER_CORE_NUM = 4;
            else WORKER_CORE_NUM = max(2, sys_core_num - 2);
        }
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
