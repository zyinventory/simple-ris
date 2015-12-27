#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <share.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <errno.h>

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
static char PACS_BASE_CACHE[260] = "";

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

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size)
#else
int GetNextUniqueNo_internal(const char *prefix, char *pbuf, const size_t buf_size)
#endif
{
    struct _timeb storeTimeThis, storeTimeHistory = {0, 0, 0, 0};
    __time64_t diff = 0;
    char temp_path[260], *basedir = NULL;
    FILE *fpseq = NULL;
    if(buf_size < 40) return -1;
#ifdef COMMONLIB_EXPORTS
    basedir = GetPacsBase();
#else
    basedir = GetPacsBase_internal();
#endif
    strcpy_s(temp_path, basedir);
    strcat_s(temp_path, "\\temp\\sequence.dat");

    int try_open = 100;
    while(try_open)
    {
        errno_t fe = 0;
        if(fpseq == NULL) fe = _access_s(temp_path, 06);
        switch(fe)
        {
        case 0:
            if(fpseq == NULL) fpseq = _fsopen(temp_path, "r+b", _SH_DENYRW);
            if(fpseq && sizeof(_timeb) == fread(&storeTimeHistory, 1, sizeof(_timeb), fpseq))
            {
                fseek(fpseq, 0, SEEK_SET);
                try_open = 0;
            }
            else
                _sleep(1);
            --try_open;
            break;
        case ENOENT:
            fpseq = _fsopen(temp_path, "w+b", _SH_DENYRW);
            if(fpseq) try_open = 0;
            break;
        default: // EACCES EINVAL
            try_open = 0;
            break;
        }
    }
    if(fpseq == NULL) perror(temp_path);

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
        fwrite(&storeTimeThis, sizeof(_timeb), 1, fpseq);
        fclose(fpseq);
    }
    return sprintf_s(pbuf, buf_size, "%s%lld%03hd-%lld", prefix, storeTimeThis.time, storeTimeThis.millitm, diff);
}
