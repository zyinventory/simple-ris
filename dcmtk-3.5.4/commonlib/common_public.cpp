#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <share.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <errno.h>
#include <windows.h>

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API size_t GenerateTime(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#else
size_t GenerateTime_internal(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#endif
{
	time_t t, *pt = time_now ? time_now : &t;
	*pt = time( NULL );
	struct tm tmp;
	if( localtime_s( &tmp, pt ) == 0 )
		return strftime(timeBuffer, bufferSize, format, &tmp);
	else
		return 0;
}

#define ENV_PACS_BASE "PACS_BASE"
static char PACS_BASE_CACHE[MAX_PATH] = "";

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API char* GetPacsBase()
#else
static char* GetPacsBase_internal()
#endif
{
	size_t requiredSize = strlen(PACS_BASE_CACHE) + 1;
    if(requiredSize == 1)
    {
	    getenv_s( &requiredSize, NULL, 0, ENV_PACS_BASE);
	    if(requiredSize > 0)
            getenv_s(&requiredSize, PACS_BASE_CACHE, requiredSize, ENV_PACS_BASE);
	    else
            strcpy_s(PACS_BASE_CACHE, sizeof(PACS_BASE_CACHE), "C:\\usr\\local\\dicom");
    }
    return PACS_BASE_CACHE;
}

#define SEQ_MUTEX_NAME "Global\\DCM_GetNextUniqueNo"
static FILE *fpseq = NULL;
static HANDLE mutex_seq = NULL;
static struct _timeb storeTimeHistory = {0, 0, 0, 0};

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size)
#else
int GetNextUniqueNo_internal(const char *prefix, char *pbuf, const size_t buf_size)
#endif
{
    struct _timeb storeTimeThis;
    __time64_t diff = 0;
    char temp_path[MAX_PATH], *basedir = NULL;
    if(buf_size < 40) return -1;
    basedir = GetPacsBase();
    strcpy_s(temp_path, basedir);
    strcat_s(temp_path, "\\temp\\sequence.dat");

    bool owner_mutex = false;
    if(mutex_seq == NULL)
    {
        mutex_seq = CreateMutex(NULL, FALSE, SEQ_MUTEX_NAME);
        if(mutex_seq == NULL && GetLastError() == ERROR_ALREADY_EXISTS)
            mutex_seq = OpenMutex(SYNCHRONIZE, FALSE, SEQ_MUTEX_NAME);
    }
    if(mutex_seq && WAIT_FAILED != WaitForSingleObject(mutex_seq, INFINITE))
    {
        owner_mutex = true;
        if(fpseq == NULL)
        {
            fpseq = _fsopen(temp_path, "r+b", _SH_DENYNO);
            if(fpseq == NULL && errno == ENOENT)
            {
                fpseq = _fsopen(temp_path, "w+b", _SH_DENYNO);
            }
        }
    }
    if(fpseq == NULL)
    {
        perror(temp_path);
    }
    else if(owner_mutex)
    {
        fseek(fpseq, 0, SEEK_SET);
        if(sizeof(_timeb) != fread(&storeTimeHistory, 1, sizeof(_timeb), fpseq))
        {
            memset(&storeTimeHistory, 0, sizeof(_timeb));
            fputs("time history read error", stderr);
        }
    }

    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < storeTimeHistory.time || (storeTimeThis.time == storeTimeHistory.time && storeTimeThis.millitm <= storeTimeHistory.millitm))
    {
        if(storeTimeHistory.millitm == 999)
        {
            ++storeTimeHistory.time;
            storeTimeHistory.millitm = 0;
        }
        else
            ++storeTimeHistory.millitm;

        diff = (storeTimeHistory.time - storeTimeThis.time) * 1000 + storeTimeHistory.millitm - storeTimeThis.millitm;
        storeTimeThis = storeTimeHistory;
    }
    else
        storeTimeHistory = storeTimeThis;

    if(fpseq)
    {
        fseek(fpseq, 0, SEEK_SET);
        fwrite(&storeTimeThis, sizeof(_timeb), 1, fpseq);
        fflush(fpseq);
    }
    if(owner_mutex) ReleaseMutex(mutex_seq);
    return sprintf_s(pbuf, buf_size, "%s%lld%03hd-%lld_", prefix, storeTimeThis.time, storeTimeThis.millitm, diff);
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API void ReleaseUniqueNoResource()
#else
void ReleaseUniqueNoResource_internal()
#endif
{
    if(fpseq)
    {
        fclose(fpseq);
        fpseq = NULL;
    }
    if(mutex_seq)
    {
        ReleaseMutex(mutex_seq);
        CloseHandle(mutex_seq);
        mutex_seq = NULL;
    }
}
